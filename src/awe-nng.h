
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include <nng/nng.h>
#include <nng/protocol/pubsub0/pub.h>
#include <nng/protocol/pubsub0/sub.h>
#include <nng/protocol/reqrep0/req.h>
#include <nng/protocol/reqrep0/rep.h>

struct SocketWrapper {
    unsigned   index;  /* Socket number  */
    nng_socket socket; /* NNG socket     */
    bool       closed; /* Socket close   */
    napi_ref   ref;    /* Node reference */
};
