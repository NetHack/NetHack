#include <stdio.h>
#include <stdarg.h>

/* external functions */
int nhmain(int argc, char *argv[]);
typedef void(*stub_callback_t)(const char *name, void *ret_ptr, const char *fmt, ...);
void shim_graphics_set_callback(stub_callback_t cb);

/* forward declarations */
void window_cb(const char *name, void *ret_ptr, const char *fmt, ...);
void *yourFunctionToRenderGraphics(const char *name, va_list args);

int main(int argc, char *argv[]) {
    shim_graphics_set_callback(window_cb);
    nhmain(argc, argv);
}

void *yourFunctionToRenderGraphics(const char *name, va_list args) {
    printf("yourFunctionToRenderGraphics name %s\n", name);
    /* DO SOMETHING HERE */
    return NULL;
}

void window_cb(const char *name, void *ret_ptr, const char *fmt, ...) {
    void *ret;
    va_list args;
    /* TODO -- see windowCallback below for hints */
    va_start(args, fmt);

    ret = yourFunctionToRenderGraphics(name, args);
    // *((int *)ret_ptr = *((int *)ret); // e.g. yourFunctionToRenderGraphics returns an int

    va_end(args);
}

#if 0
function variadicCallback(name, retPtr, fmt, args) {
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
    setReturn(retPtr, retType);
}

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
        Module.setValue(ptr, value, "i8"); // 'Z'
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
#endif /* 0 */