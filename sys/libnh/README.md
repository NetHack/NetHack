# About
This creates a library for NetHack that can be incorporated into other programs. There are two different libraries that are currently available:
* libnethack.a - a binary Unix library
* nethack.js / nethack.wasm - a [WebAssembly / WASM](https://webassembly.org/) library for use in JavaScript programs (both nodejs and browser)

## Build
This library has only been built on MacOS, but should work on Linux and other unix-ish platforms. If you have problems, start by stealing hints files from the `sys/unix/hints` for your platform. Contributions for other platforms are happily accepted.

Building the WASM module requires that you have the [emscripten toolchain / sdk installed](https://emscripten.org/docs/getting_started/downloads.html).

Generally the build is the same as the unix build:

[Edit Oct 4, 2020: Use the existing Makefile and hints, hints/include system for cross-compiles]
1. `cd sys/unix`
2. `./setup.sh hints/macOS.370`
3. `cd ../..`
4. For `libnethack.a`: `make WANT_LIBNH=1 all`
5. For `nethack.js`: `make CROSS_TO_WASM=1 all`

[Original text was:]
1. `cd sys/lib`
2. For `libnethack.a`: `./setup.sh hints/macOS.370`; for `nethack.js`: `./setup.sh hints/wasm`
3. `cd ../..`
4. `make`


[Edit Oct 4, 2020:]
Resulting libaries will be in the `targets/wasm` directory for `CROSS_TO_WASM=1`.
Resulting libaries will be in the `src` directory for `WANT_LIBNH=1`.

[Original text:]
Resulting libaries will be in the `src` directory.

WASM also has a npm module that can be published out of `sys/lib/npm-library`. After building the `nethack.js` it can be published by:
1. `cd sys/lib/npm-library`
2. `npm publish`

## API: libnethack.a
The API is two functions:
* `nhmain(int argc, char *argv[])` - The main function for NetHack that configures the program and runs the `moveloop()` until the game is over. The arguments to this function are the [command line arguments](https://nethackwiki.com/wiki/Options) to NetHack.
* `shim_graphics_set_callback(shim_callback_t cb)` - A single function that sets a callback to gather graphics events: write a string to screen, get user input, etc. Your job is to pass in a callback and handle all the requested rendering events to show NetHack on the scrren. The callback is `void shim_callback_t(const char *name, void *ret_ptr, const char *fmt,  ...)`
  * `name` is the name of the [window function](https://github.com/NetHack/NetHack/blob/NetHack-3.7/doc/window.txt) that needs to be handled
  * `ret_ptr` is a pointer to a memory space for the return value. The type expected to be returned in this pointer is described by the first character of the `fmt` string.
  * `fmt` is a string that describes the signature of the callback. The first character in the string is the return type and any additional characters describe the variable arguments: `i` for integer, `s` for string, `p` for pointer, `c` for character, `v` for void. For example, if format is "vis" the callback will have no return (void), the first argument will be an integer, and the second argument will be a string. If format is "iii" the callback must return an integer, and both the arguments passed in will be integers.
  * [Variadic arguments](https://www.gnu.org/software/libc/manual/html_node/Variadic-Example.html): a variable number and type of arguments depending on the `window function` that is being called. The arguments associated with each `name` are described in the [NetHack window.txt](https://github.com/NetHack/NetHack/blob/NetHack-3.7/doc/window.txt).

Where is the header file for the API you ask? There isn't one. It's three functions, just drop the forward declarations at the top of your file (or create your own header). It's more work figuring out how to install and copy around header files than it's worth for such a small API. If you disagree, feel free to sumbit a PR to fix it. :)

## API: nethack.js
The WebAssembly API has a similar signature to `libnethack.a` with minor syntactic differences:
* `main(int argc, char argv[])` - The main function for NetHack
* `shim_graphics_set_callback(char *cbName)` - A `String` representing a name of a callback function. The callback function be registered as `globalThis[cbName] = function yourCallback(name, ... args) { /* your stuff */ }`. Note that [globalThis](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/globalThis) points to `window` in browsers and `global` in node.js.
  * `name` is the name of the [window function](https://github.com/NetHack/NetHack/blob/NetHack-3.7/doc/window.txt) that needs to be handled
  * `... args` is a variable number and type of arguments depending on the `window function` that is being called. The arguments associated with each `name` are described in the [NetHack window.txt](https://github.com/NetHack/NetHack/blob/NetHack-3.7/doc/window.txt)
  * The function must return the value expected for the specified `name`


## API Stability
The "shim graphics" API should generally be stable. I aspire to replace the command line arguments (argc / argv) with a structure of options, so the `nhmain()` and `main()` functions may change at some point.

## libnethack.a example
``` c
#include <stdio.h>

int nhmain(int argc, char *argv[]);
typedef void(*shim_callback_t)(const char *name, void *ret_ptr, const char *fmt, ...);
void shim_graphics_set_callback(shim_callback_t cb);

void window_cb(const char *name, void *ret_ptr, const char *fmt, ...) {
    /* TODO */
}

int main(int argc, char *argv[]) {
    shim_graphics_set_callback(window_cb);
    nhmain(argc, argv);
}
```

## nethack.js example
``` js
const path = require("path");

// starts nethack
function nethackStart(cb, inputModule = {}) {
    // set callback
    let cbName = cb.name;
    if (cbName === "") cbName = "__anonymousNetHackCallback";
    let userCallback = globalThis[cbName] = cb;

    // Emscripten Module config
    let Module = inputModule;
    savedOnRuntimeInitialized = Module.onRuntimeInitialized;
    Module.onRuntimeInitialized = function (... args) {
        // after the WASM is loaded, add the shim graphics callback function
        Module.ccall(
            "shim_graphics_set_callback", // C function name
            null, // return type
            ["string"], // arg types
            [cbName], // arg values
            {async: true} // options
        );
    };

    // load and run the module
    var factory = require(path.join(__dirname, "../build/nethack.js"));
    factory(Module);
}

nethackStart(yourCallbackFunction);
```
