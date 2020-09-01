const path = require("path");

let Module;
let userCallback;
let savedOnRuntimeInitialized;
function nethackStart(cb, inputModule = {}) {
    if(typeof cb !== "function") throw new TypeError("expected first argument to be callback function");
    if(typeof inputModule !== "object") throw new TypeError("expected second argument to be object");

    // Emscripten Module config
    Module = inputModule;
    userCallback = cb;
    savedOnRuntimeInitialized = Module.onRuntimeInitialized;
    Module.onRuntimeInitialized = function (... args) {
        // after the WASM is loaded, add the shim graphics callback function
        let cb = Module.addFunction(windowCallback, "viiii");
        Module.ccall(
            "shim_graphics_set_callback", // C function name
            null, // return type
            ["number"], // arg types
            [cb], // arg values
            {async: true} // options
        );

        /* TODO: Module.removeFunction() */

        // if the user had their own onRuntimeInitialized(), call it now
        if (savedOnRuntimeInitialized) savedOnRuntimeInitialized(... args);
    };

    // load and run the module
    var factory = require(path.join(__dirname, "../build/nethack.js"));
    factory(Module);
}

function windowCallback(name, retPtr, fmt, args) {
    name = Module.UTF8ToString(name);
    fmt = Module.UTF8ToString(fmt);
    let argTypes = fmt.split("");
    let retType = argTypes.shift();

    // build array of JavaScript args from WASM parameters
    let jsArgs = [];
    for (let i = 0; i < argTypes.length; i++) {
        let ptr = args + (4*i);
        let val = typeLookup(argTypes[i], ptr);
        jsArgs.push(val);
    }
    let retVal = userCallback(name, ... jsArgs);
    setReturn(name, retPtr, retType, retVal);
}

function typeLookup(type, ptr) {
    switch(type) {
    case "s": // string
        return Module.UTF8ToString(Module.getValue(ptr, "*"));
    case "p": // pointer
        ptr = Module.getValue(ptr, "*");
        if(!ptr) return 0; // null pointer
        return Module.getValue(ptr, "*");
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

function setReturn(name, ptr, type, value = 0) {

    switch (type) {
    case "p":
        throw new Error("not implemented");
    case "s":
        if(typeof value !== "string")
            throw new TypeError(`expected ${name} return type to be string`);
        value=value?value:"(no value)";
        var strPtr = Module.getValue(ptr, "i32");
        Module.stringToUTF8(value, strPtr, 1024);
        break;
    case "i":
        if(typeof value !== "number" || !Number.isInteger(value))
            throw new TypeError(`expected ${name} return type to be integer`);
        Module.setValue(ptr, value, "i32");
        break;
    case "c":
        if(typeof value !== "number" || value < 0 || value > 128)
            throw new TypeError(`expected ${name} return type to be integer representing an ASCII character`);
        Module.setValue(ptr, value, "i8");
        break;
    case "f":
        if(typeof value !== "number" || isFloat(value))
            throw new TypeError(`expected ${name} return type to be float`);
        // XXX: I'm not sure why 'double' works and 'float' doesn't
        Module.setValue(ptr, value, "double");
        break;
    case "d":
        if(typeof value !== "number" || isFloat(value))
            throw new TypeError(`expected ${name} return type to be float`);
        Module.setValue(ptr, value, "double");
        break;
    case "v":
        break;
    default:
        throw new Error("unknown type");
    }

    function isFloat(n){
        return n === +n && n !== (n|0) && !Number.isInteger(n);
    }
}

// TODO: ES6 'import' style module
module.exports = nethackStart;

