# Awsome NNG pure C N-API module

## System requirements

Your distribution should have the following packages. I will provide examples of package names for Debian/Ubuntu,
but if you are using other distributions, you probably know well how to find the corresponding packages or
build the necessary ones:

  * node headers (automatic setup)
  * cmake >= 3.15
  * libnng-dev
  * gcc, pkg-config and etc.

I didn't have any desire, and I faced difficulties with builds for MacOS and MS-Windows,
so I only tested it under Linux.

If you wish, you can contribute to improving the package, but officially,
I don't strongly support these systems.

## Install package

    $ npm install awe-nng --save

## Usage example

```javascript
const req = new addon.Socket('req');
await req.Dial('tcp://127.0.0.1:5555');
await req.Send(JSON.stringify({
    ...
}));
const msg = await req.Recv();
console.log(`msg = ${msg}`);
req.Close();
```
