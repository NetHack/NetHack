const util = require("util");

// Emscripten Module config
let Module = {
    // noInitialRun: true,
    onRuntimeInitialized: function () {
        setGraphicsCallback();
    },
};

var factory = require("./src/nethack.js");

// load and run the module
factory(Module);

function setGraphicsCallback() {
    console.log("setting callback function");
    let cb = Module.addFunction(windowCallback, "viiii");

    console.log("doing call");
    Module.ccall(
        "stub_graphics_set_callback", // C function name
        null, // return type
        ["number"], // arg types
        [cb], // arg values
        {async: true} // options
    );

    /* TODO: removeFunction */
}

function windowCallback(name, retPtr, fmt, args) {
    // console.log ("variadicCallback called...");
    // console.log("typeof name", typeof name);
    // console.log("typeof fmt", typeof fmt);
    // console.log("typeof args", typeof args);
    name = Module.UTF8ToString(name);
    fmt = Module.UTF8ToString(fmt);
    // console.log ("name:", name);
    // console.log ("fmt:", fmt);
    let argTypes = fmt.split("");
    let retType = argTypes.shift();
    // console.log ("arg count:", argTypes.length);
    // console.log ("arg types:", argTypes);
    // console.log ("ret type:", retType);

    let jsArgs = [];
    for (let i = 0; i < argTypes.length; i++) {
        let ptr = args + (4*i);
        let val = typeLookup(argTypes[i], ptr);
        jsArgs.push(val);
    }
    console.log(`graphics callback: ${name} [${jsArgs}]`);
    let retVal = doGraphics(name, ... jsArgs);
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

let winCount = 0;
function doGraphics(name, ... args) {
    switch(name) {
    case "shim_create_nhwindow":
        winCount++;
        console.log("creating window", args, "returning", winCount);
        return winCount;
    case "shim_yn_function":
    case "shim_message_menu":
        return 121; // 'y'
    case "shim_nhgetch":
    case "shim_nh_poskey":
        return 46;
    default:
        return 0;
    }
}

// var Module = null;
// factory(Module).then((ret) => {
//     if(ret !== Module) {
//         console.log("ret and Module are not the same");
//     } else {
//         console.log("ret and Module are the same");
//     }
//     // console.log("Module", util.inspect(Module, {depth: null, showHidden: true}));
//     // TODO:
//     // stub_graphics_set_callback();

//     // options:
//     // https://emscripten.org/docs/api_reference/module.html
//     // logReadFiles
//     // printWithColors
//     // noInitialRun
//     // onRuntimeInitialized
//     console.log("doing run");
//     // Module.run();

//     // // main loop
//     // console.log("creating function");
//     // let fn = Module.addFunction(wasmCallback, "vii");

//     // console.log("doing call");
//     // Module.ccall(
//     //     "mainloop", // C function name
//     //     null, // return type
//     //     ["number"], // arg types
//     //     [fn], // arg values
//     //     {async: true} // options
//     // );
//     // // TODO: removeFunction(fn)

//     // var i = 0;
//     // setInterval(function() {
//     //     console.log("interval", i++);
//     // },1000);
// });

