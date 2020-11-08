const path = require("path");

let Module;
let userCallback;
let savedOnRuntimeInitialized;

// starts nethack
function nethackStart(cb, inputModule = {}) {
    if (typeof cb !== "string" && typeof cb !== "function") {
        throw new TypeError("expected first argument to be 'Function' or 'String' representing global callback function name");
    }

    if (typeof inputModule !== "object") {
        throw new TypeError("expected second argument to be object");
    }

    let cbName;
    if (typeof cb === "function") {
        cbName = cb.name;
        if (cbName === "") {
            cbName = "__anonymousNetHackCallback";
        }

        if (globalThis[cbName] === undefined) {
            globalThis[cbName] = cb;
        } else if (globalThis[cbName] !== cb) {
            throw new Error(`'globalThis["${cbName}"]' is not the same as specified callback`);
        }
    }

    /* global globalThis */
    userCallback = globalThis[cbName];
    if (typeof userCallback !== "function") {
        throw new TypeError(`expected 'globalThis["${cbName}"]' to be a function`);
    }
    // if(userCallback.constructor.name !== "AsyncFunction") throw new TypeError(`expected 'globalThis["${cbName}"]' to be an async function`);

    // Emscripten Module config
    Module = inputModule;
    savedOnRuntimeInitialized = Module.onRuntimeInitialized;
    Module.onRuntimeInitialized = function(... args) {
        // after the WASM is loaded, add the shim graphics callback function
        Module.ccall(
            "shim_graphics_set_callback", // C function name
            null, // return type
            ["string"], // arg types
            [cbName], // arg values
            {async: true}, // options
        );

        // if the user had their own onRuntimeInitialized(), call it now
        if (savedOnRuntimeInitialized) {
            savedOnRuntimeInitialized(... args);
        }
    };

    // load and run the module
    let factory = require(path.join(__dirname, "../build/nethack.js"));
    factory(Module);
}

// TODO: ES6 'import' style module
module.exports = nethackStart;

