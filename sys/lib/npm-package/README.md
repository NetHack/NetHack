# NetHack
This is the ACTUAL NetHack game, originally written in 1982 and one of the longest-standing open source collaborations. This isn't a wrapper around the binary NetHack, but the game itself compiled into [WebAssembly](https://webassembly.org/) (WASM) using [emscripten](https://emscripten.org/). This module should run [anywhere WebAssembly is supported](https://developer.mozilla.org/en-US/docs/WebAssembly#Browser_compatibility) including node.js and modern browsers.

Since NetHack typically uses the [TTY](https://en.wikipedia.org/wiki/Computer_terminal#Text_terminals) to show depictions of the game and that won't work for WebAssembly, you are required to implement the graphics portion of NetHack to make this work. This allows a wide variety of UIs to be created, both text and graphics based as well as using keyboard and mouse to control the game. The API for implementing graphics is described below.

## Install

``` sh
npm install nethack
```

## API
The main module returns a setup function: `startNethack(myCallback, moduleOptions)`.

## Example
``` js
let nethackStart = require("nethack");

nethackStart(doGraphics);

let winCount = 0;
function doGraphics(name, ... args) {
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