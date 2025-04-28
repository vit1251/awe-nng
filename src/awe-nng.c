
#include <node_api.h>

#include "awe-nng.h"

unsigned socket_index = 0;

void socket_finalize(napi_env env, void* data, void* hint);
napi_value Socket(napi_env env, napi_callback_info info);

void SocketDestructor(napi_env env, void* data, void* hint) {
    struct SocketWrapper* wrapper = (struct SocketWrapper*)data;
    if (!wrapper->closed) {
#ifdef _DEBUG
        fprintf(stderr, "debug: close socket\n");
#endif
        nng_close(wrapper->socket);
    }
    free(wrapper);
}

napi_value SocketConstructor(napi_env env, napi_callback_info info) {
    napi_status status;
    size_t argc = 1;
    napi_value argv[1];
    napi_value jsthis;

    status = napi_get_cb_info(env, info, &argc, argv, &jsthis, NULL);
    if ((status != napi_ok) || (argc < 1)) {
        napi_throw_error(env, NULL, "Expected 1 arguments: kind: 'pub' | 'sub'");
        return NULL;
    }

    char type[4];
    size_t type_len;
    status = napi_get_value_string_utf8(env, argv[0], type, sizeof(type), &type_len);
    if (status != napi_ok) {
        napi_throw_error(env, NULL, "Invalid socket type");
        return NULL;
    }

    struct SocketWrapper* wrapper = (struct SocketWrapper*)malloc(sizeof(struct SocketWrapper));
    wrapper->index = socket_index++;
    wrapper->closed = false;

    int result = -1;
    if (strcmp(type, "pub") == 0) {
        result = nng_pub0_open(&wrapper->socket);
    } else if (strcmp(type, "sub") == 0) {
        result = nng_sub0_open(&wrapper->socket);
    } else if (strcmp(type, "req") == 0) {
#ifdef _DEBUG
        fprintf(stderr, "debug: create request socket %u\n", wrapper->index);
#endif
        result = nng_req0_open(&wrapper->socket);
    } else if (strcmp(type, "rep") == 0) {
        result = nng_rep0_open(&wrapper->socket);
    }
    if (result != 0) {
        free(wrapper);
        napi_throw_error(env, NULL, "Failed to create socket");
        return NULL;
    }

    status = napi_wrap(env, jsthis, wrapper, SocketDestructor, NULL, &wrapper->ref);
    if (status != napi_ok) {
        free(wrapper);
        return NULL;
    }

    return jsthis;
}

napi_value Close(napi_env env, napi_callback_info info) {
    struct SocketWrapper* wrapper;
    size_t argc = 1;
    napi_value argv[1];
    napi_status status;
    napi_value jsthis;
    napi_value resp;
    char *buf = NULL;
    int result;
    size_t sz = 0;
    int rv;
    char addr[512];
    size_t addr_len;

    status = napi_get_cb_info(env, info, &argc, argv, &jsthis, NULL);
    if (status != napi_ok) {
        return NULL;
    }

    status = napi_unwrap(env, jsthis, (void **)&wrapper);
    if (status != napi_ok) {
        return NULL;
    }

    rv = nng_close(wrapper->socket);
    if (rv != 0) {
        return NULL;
    }

    wrapper->closed = true;

    return NULL;
}

napi_value Listen(napi_env env, napi_callback_info info) {
    struct SocketWrapper* wrapper;
    size_t argc = 1;
    napi_value argv[1];
    napi_status status;
    napi_value jsthis;
    napi_value resp;
    char *buf = NULL;
    int result;
    size_t sz = 0;
    int rv;
    char addr[512];
    size_t addr_len;

    status = napi_get_cb_info(env, info, &argc, argv, &jsthis, NULL);
    if (status != napi_ok) {
        return NULL;
    }

    status = napi_unwrap(env, jsthis, (void **)&wrapper);
    if (status != napi_ok) {
        return NULL;
    }

    status = napi_get_value_string_utf8(env, argv[0], addr, sizeof(addr), &addr_len);
    if (status != napi_ok) {
        napi_throw_error(env, NULL, "Invalid addr");
        return NULL;
    }

    rv = nng_listen(wrapper->socket, addr, NULL, 0);
    if (rv != 0) {
      goto on_error;
    }

    return NULL;
on_error:

    return NULL;
}

napi_value Dial(napi_env env, napi_callback_info info) {
    struct SocketWrapper* wrapper;
    size_t argc = 1;
    napi_value argv[1];
    napi_status status;
    napi_value jsthis;
    napi_value resp;
    int rv;
    char addr[512];
    size_t addr_len;

    status = napi_get_cb_info(env, info, &argc, argv, &jsthis, NULL);
    if (status != napi_ok) {
        return NULL;
    }

    status = napi_unwrap(env, jsthis, (void **)&wrapper);
    if (status != napi_ok) {
        return NULL;
    }

    status = napi_get_value_string_utf8(env, argv[0], addr, sizeof(addr), &addr_len);
    if (status != napi_ok) {
        napi_throw_error(env, NULL, "Invalid addr");
        return NULL;
    }

#ifdef _DEBUG
    fprintf(stderr, "debug: Dial socket %u on %s\n", wrapper->index, addr);
#endif

    rv = nng_dial(wrapper->socket, addr, NULL, 0);
    if (rv != 0) {
        const char *err = nng_strerror(rv);
        napi_throw_error(env, NULL, err);
        return NULL;
    }

    return NULL;
}

struct AsyncRead {
    nng_socket      socket;   /* NNG socket     */
    napi_deferred   deferred; /* Deferred       */
    napi_async_work work;     /* Async work     */
    char*           msg;      /* Message        */
    int             err;      /* Error code     */
};

static void ReadExecute(napi_env env, void* data) {
    struct AsyncRead* arg = (struct AsyncRead*)data;
    char *buf = NULL;
    size_t sz = 0;
    int rv;

    rv = nng_recv(arg->socket, &buf, &sz, NNG_FLAG_ALLOC);

#ifdef _DEBUG
    fprintf(stderr, "debug: RX err = %u\n", rv);
#endif

    if (rv == 0) {
#ifdef _DEBUG
        fprintf(stderr, "debug: RX size = %u buf = %.*s\n", sz, sz, buf);
#endif
        arg->msg = buf != NULL ? strndup(buf, sz) : NULL;
        nng_free(buf, sz);
    } else {
    }

    arg->err = rv;
}

static void ReadComplete(napi_env env, napi_status status, void* data) {
    struct AsyncRead* arg = (struct AsyncRead*)data;
    napi_status status2;

    if (arg->err == 0) {
        napi_value result;
#ifdef _DEBUG
        fprintf(stderr, "info: RX err = %u\n", arg->err);
#endif
        status2 = napi_create_string_utf8(env, arg->msg, NAPI_AUTO_LENGTH, &result);
        status2 = napi_resolve_deferred(env, arg->deferred, result);

    } else {
        napi_value error;
        napi_value code;
        napi_value msg;

        const char *err = nng_strerror(arg->err);

#ifdef _DEBUG
        fprintf(stderr, "error: RX err = %u\n", arg->err);
#endif

        status2 = napi_get_value_int32(env, code, &arg->err);
        status2 = napi_create_string_utf8(env, err, NAPI_AUTO_LENGTH, &msg);
        status2 = napi_create_error(env, code, msg, &error);
        status2 = napi_reject_deferred(env, arg->deferred, error);
    }

    status2 = napi_delete_async_work(env, arg->work);

    free(arg->msg);
    free(arg);
}

napi_value Recv(napi_env env, napi_callback_info info) {
    struct SocketWrapper* wrapper;
    size_t argc = 1;
    napi_value argv[1];
    napi_status status;
    napi_value jsthis;
    napi_value resp;

    int rv;
    napi_value promise;
    napi_value resource_name;
    struct AsyncRead* ptr = NULL;

    status = napi_get_cb_info(env, info, &argc, argv, &jsthis, NULL);
    if (status != napi_ok) {
        return NULL;
    }

    status = napi_unwrap(env, jsthis, (void **)&wrapper);
    if (status != napi_ok) {
        return NULL;
    }

#ifdef _DEBUG
    fprintf(stderr, "debug: Receive on socket %u\n", wrapper->index);
#endif

    ptr = (struct AsyncRead *)malloc(sizeof(struct AsyncRead));
    if (ptr == NULL) {
        return NULL;
    }
    ptr->socket = wrapper->socket;

    status = napi_create_promise(env, &ptr->deferred, &promise);
    if (status != napi_ok) {
        return NULL;
    }

    status = napi_create_string_utf8(env, "awe-nng-read", NAPI_AUTO_LENGTH, &resource_name);
    if (status != napi_ok) {
        return NULL;
    }

    status = napi_create_async_work(env, NULL, resource_name, ReadExecute, ReadComplete, (void *)ptr, &ptr->work);
    if (status != napi_ok) {
        return NULL;
    }

    status = napi_queue_async_work(env, ptr->work);
    if (status != napi_ok) {
        return NULL;
    }

    return promise;

on_error:


    return NULL;
}


napi_value Send(napi_env env, napi_callback_info info) {
    struct SocketWrapper* wrapper;
    size_t argc = 1;
    napi_value argv[1];
    napi_status status;
    napi_value jsthis;
    char *message = NULL;
    size_t message_len;
    int rv;
    napi_value resp;

    status = napi_get_cb_info(env, info, &argc, argv, &jsthis, NULL);
    if (status != napi_ok) {
        napi_throw_error(env, NULL, "Expected 1 arguments");
        goto on_error;
    }

    if (argc != 1) {
        napi_throw_error(env, NULL, "Expected 1 arguments");
        goto on_error;
    }

    status = napi_unwrap(env, jsthis, (void **)&wrapper);
    if (status != napi_ok) {
        napi_throw_error(env, NULL, "Expected 1 arguments");
        goto on_error;
    }

    status = napi_get_value_string_utf8(env, argv[0], NULL, 0, &message_len);
    if (status != napi_ok) {
        napi_throw_error(env, NULL, "Expected 1 arguments");
        goto on_error;
    }

    message = (char *)malloc(message_len + 1);
    if (message == NULL) {
        goto on_error;
    }

    status = napi_get_value_string_utf8(env, argv[0], message, message_len + 1, &message_len);
    if (status != napi_ok) {
        napi_throw_error(env, NULL, "Expected 1 arguments");
        goto on_error;
    }

#ifdef _DEBUG
    fprintf(stderr, "debug: TX on socket %u message '%.*s'\n", wrapper->index, message_len, message);
#endif

    rv = nng_send(wrapper->socket, message, message_len, 0);
    if (rv != 0) {
        napi_throw_error(env, NULL, "Failed to send message");
        goto on_error;
    }

    status = napi_get_value_int32(env, resp, &rv);
    if (status != napi_ok) {
        goto on_error;
    }

    free(message);
    return resp;
on_error:

    free(message);
    return NULL;
}

napi_value Init(napi_env env, napi_value exports) {
    napi_status status;

    napi_property_descriptor properties[] = {
      { "Listen", NULL, Listen, NULL, NULL, NULL, napi_default, NULL },
      { "Dial", NULL, Dial, NULL, NULL, NULL, napi_default, NULL },
      { "Recv", NULL, Recv, NULL, NULL, NULL, napi_default, NULL },
      { "Send", NULL, Send, NULL, NULL, NULL, napi_default, NULL },
      { "Close", NULL, Close, NULL, NULL, NULL, napi_default, NULL },
    };

    napi_value socketClass;
    status = napi_define_class(env, "Socket", NAPI_AUTO_LENGTH, SocketConstructor, NULL, sizeof(properties) / sizeof(properties[0]),
        properties, &socketClass);
    if (status != napi_ok) {
        return NULL;
    }

    status = napi_set_named_property(env, exports, "Socket", socketClass);
    if (status != napi_ok) {
        return NULL;
    }

    return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init);
