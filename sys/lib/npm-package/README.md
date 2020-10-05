# NetHack
This is the ACTUAL NetHack game, originally written in 1982 and one of the longest-standing open source collaborations. This isn't a wrapper around the binary NetHack, but the game itself compiled into [WebAssembly](https://webassembly.org/) (WASM) using [emscripten](https://emscripten.org/). This module should run [anywhere WebAssembly is supported](https://developer.mozilla.org/en-US/docs/WebAssembly#Browser_compatibility) including node.js and modern browsers.

Since NetHack typically uses the [TTY](https://en.wikipedia.org/wiki/Computer_terminal#Text_terminals) to show depictions of the game and that won't work for WebAssembly, you are required to implement the graphics portion of NetHack to make this work. This allows a wide variety of UIs to be created, both text and graphics based as well as using keyboard and mouse to control the game. The API for implementing graphics is described below.

## Install

``` sh
npm install nethack
```

## API
The main module returns a setup function: `startNethack(uiCallback, moduleOptions)`.
* `uiCallback(name, ... args)` - Your callback function that will handle rendering NetHack on the screen of your choice. The `name` argument is one of the UI functions of the [NetHack Window Interface](https://github.com/NetHack/NetHack/blob/NetHack-3.7/doc/window.doc) and the `args` are corresponding to the window interface function that is being called. You are required to return the correct type of data for the function that is implemented. The `uiCallback` may be an `async` function.
* `moduleOptions` - An optional [emscripten Module object](https://emscripten.org/docs/api_reference/module.html) for configuring the WASM that will be run.
  * `Module.arguments` - Of note is the [arguments property](https://emscripten.org/docs/api_reference/module.html#Module.arguments) which gets passed to NetHack as its [command line parameters](https://nethackwiki.com/wiki/Options).

There are a number of auxilary functions and variables that may help with building your applications. All of these are under `globalThis.nethackOptions`. Use `console.log(globalThis.nethackOptions)` for a full list of options. Some worth mentioning are:
* `globalThis.nethackOptions.helpers` - Helper functions that are useful for NetHack windowing ports
  * `globalThis.nethackOptions.mapglyphHelper` - Converts an integer glyph into a character to be displayed. Useful if you are using ASCII characters for representing NetHack (as opposed to tiles). Interface is `mapglyphHelper(glyph, x, y, mgflags)` and will typically be called as part of the `shim_print_glyph` function.
* `globalThis.nethackOptions.constants` - A Object full of constants that are `#define`'d in NetHack's C code. Useful for translating to / from numbers in the APIs and return values.

## Example
``` js
let nethackStart = require("nethack");

nethackStart(doGraphics);

let winCount = 0;
async function doGraphics(name, ... args) {
    console.log(`shim graphics: ${name} [${args}]`);

    switch(name) {
    case "shim_create_nhwindow":
        winCount++;
        console.log("creating window", args, "returning", winCount);
        return winCount;
    case "shim_yn_function":
    case "shim_message_menu":
        return 121; // return 'y' to all questions
    case "shim_nhgetch":
    case "shim_nh_poskey":
        return 0; // simulates a mouse click on "exit up the stairs"
    default:
        return 0;
    }
}
```

## Other Notes
* This module isn't small -- the WASM code is about 10MB. It may be slow to load over non-broadband connections. There are some emscripten build optimizations that may help with browser builds (such as dynamic loading), but those aren't currently part of this package. Pull requests are always welcome. :)
* This hasn't been tested on browsers. If you get this to work on a browser, please let me know and I will add notes. Or if anyone wants to help setup automated browser testing, that would be supremely appreciated.