# About
This creates a library for NetHack that can be incorporated into other programs. There are two different libraries that are currently available:
* libnethack.a - a binary Unix library
* nethack.js / nethack.wasm - a [WebAssembly / WASM](https://webassembly.org/) library for use in JavaScript programs (both nodejs and browser)

## API: libnethack.a
This libarary has only been built on MacOS, but should work on Linux and other unix-ish platforms. If you have problems, start by stealing hints files from the `sys/unix/hints` for your platform. Contributions for other platforms are happily accepted.

The API is two functions:
* `nhmain(int argc, char *argv[])` - The main function for NetHack that configures the program and runs the `moveloop()` until the game is over. The arguments to this function are the [command line arguments](https://nethackwiki.com/wiki/Options) to NetHack.
* `stub_graphics_set_callback(stub_callback_t cb)` - A single function that sets a callback to gather graphics events: write a string to screen, get user input, etc. Your job is to pass in a callback and handle all the requested rendering events to show NetHack on the scrren. The callback is `void stub_callback_t(const char *name, void *ret_ptr, const char *fmt,  ...)`
  * `name` is the name of the [window function](https://github.com/NetHack/NetHack/blob/NetHack-3.7/doc/window.doc) that needs to be handled
  * `ret_ptr` is a pointer to a memory space for the return value. The type expected to be returned in this pointer is described by the first character of the `fmt` string.
  * `fmt` is a string that describes the signature of the callback. The first character in the string is the return type and any additional characters describe the variable arguments: `i` for integer, `s` for string, `p` for pointer, `c` for character, `v` for void. For example, if format is "vis" the callback will have no return (void), the first argument will be an integer, and the second argument will be a string. If format is "iii" the callback must return an integer, and both the arguments passed in will be integers.
  * [Variadic arguments](https://www.gnu.org/software/libc/manual/html_node/Variadic-Example.html): a variable number and type of arguments depending on the `window function` that is being called. The arguments associated with each `name` are described in the [NetHack window.doc](https://github.com/NetHack/NetHack/blob/NetHack-3.7/doc/window.doc).

Where is the header file for the API you ask? There isn't one. It's three functions, just drop the forward declarations at the top of your file (or create your own header). It's more work figuring out how to install and copy around header files than it's worth for such a small API. If you disagree, feel free to sumbit a PR to fix it. :)

## API: nethack.js
Building the WASM module requires that you have the [emscripten toolchain / sdk installed](https://emscripten.org/docs/getting_started/downloads.html).

The WebAssembly API has a similar signature to `libnethack.a` with minor syntactic differences:
* `main(int argc, char argv[])` - The main function for NetHack
* `stub_graphics_set_callback(stub_callback_t cb)` - The same as above, but the signature of the callback is slightly different because WASM can't handle variadic callbacks. The callback is: `void stub_callback_t(const char *name, void *ret_ptr, const char *fmt,  void *args[])`
  * `name` - same as above
  * `ret_ptr` - same as above
  * `fmt` - same as above
  * `args` - an array of pointers to the arguments, where each pointer can be de-referenced to a value as specified in the `fmt` string.



## API Stability
The "stub graphics" API should generally be stable. I aspire to replace the command line arguments (argc / argv) with a structure of options, so the `nhmain()` and `main()` functions may change at some point.

## libnethack.a example
``` c
#include <stdio.h>

int nhmain(int argc, char *argv[]);
typedef void(*stub_callback_t)(const char *name, void *ret_ptr, const char *fmt, ...);
void stub_graphics_set_callback(stub_callback_t cb);

void window_cb(const char *name, void *ret_ptr, const char *fmt, ...) {
    /* TODO -- see windowCallback below for hints */
}

int main(int argc, char *argv[]) {
    stub_graphics_set_callback(window_cb);
    nhmain(argc, argv);
}
```

## nethack.js example
``` js
// Module is defined by emscripten
// https://emscripten.org/docs/api_reference/module.html
let Module = {
    // if this is true, main() won't be called automatically
    // noInitialRun: true,

    // after loading the library, set the callback function
    onRuntimeInitialized: function (... args) {
        setGraphicsCallback();
    }
};

var factory = require("./src/nethack.js");

// run NetHack!
factory(Module);

// register the callback with the WASM library
function setGraphicsCallback() {
    console.log("creating WASM callback function");
    let cb = Module.addFunction(windowCallback, "viiii");

    console.log("setting callback function with library");
    Module.ccall(
        "stub_graphics_set_callback", // C function name
        null, // return type
        ["number"], // arg types
        [cb], // arg values
        {async: true} // options
    );

    /* TODO: removeFunction */
}

// this is the "shim graphics" callback function
// it gets called every time something needs to be displayed
// or input needs to be gathered from the user
function windowCallback(name, retPtr, fmt, args) {
    name = Module.UTF8ToString(name);
    fmt = Module.UTF8ToString(fmt);
    let argTypes = fmt.split("");
    let retType = argTypes.shift();

    // convert arguments to JavaScript types
    let jsArgs = [];
    for (let i = 0; i < argTypes.length; i++) {
        let ptr = args + (4*i);
        let val = typeLookup(argTypes[i], ptr);
        jsArgs.push(val);
    }
    console.log(`graphics callback: ${name} [${jsArgs}]`);
    /**********
     * YOU HAVE TO IMPLEMENT THIS FUNCTION to render things
     **********/
    let ret = yourFunctionToRenderGraphics(name, jsArgs);
    setReturn(retPtr, retType, ret);
}

// takes a character `type` and a WASM pointer and returns a JavaScript value
function typeLookup(type, ptr) {
    switch(type) {
    case "s": // string
        return Module.UTF8ToString(Module.getValue(ptr, "*"));
    case "p": // pointer
        return Module.getValue(Module.getValue(ptr, "*"), "*");
    case "c": // char
        return String.fromCharCode(Module.getValue(Module.getValue(ptr, "*"), "i8"));
    case "0": /* 2^0 = 1 byte */
        return Module.getValue(Module.getValue(ptr, "*"), "i8");
    case "1": /* 2^1 = 2 bytes */
        return Module.getValue(Module.getValue(ptr, "*"), "i16");
    case "2": /* 2^2 = 4 bytes */
    case "i": // integer
    case "n": // number
        return Module.getValue(Module.getValue(ptr, "*"), "i32");
    case "f": // float
        return Module.getValue(Module.getValue(ptr, "*"), "float");
    case "d": // double
        return Module.getValue(Module.getValue(ptr, "*"), "double");
    default:
        throw new TypeError ("unknown type:" + type);
    }
}

// takes a a WASM pointer, a charater `type` and a value and sets the value at pointer
function setReturn(ptr, type, value = 0) {
    switch (type) {
    case "p":
        throw new Error("not implemented");
    case "s":
        value=value?value:"(no value)";
        var strPtr = Module.getValue(ptr, "i32");
        Module.stringToUTF8(value, strPtr, 1024);
        break;
    case "i":
        Module.setValue(ptr, value, "i32");
        break;
    case "c":
        Module.setValue(ptr, value, "i8");
        break;
    case "f":
        // XXX: I'm not sure why 'double' works and 'float' doesn't
        Module.setValue(ptr, value, "double");
        break;
    case "d":
        Module.setValue(ptr, value, "double");
        break;
    case "v":
        break;
    default:
        throw new Error("unknown type");
    }
}
```