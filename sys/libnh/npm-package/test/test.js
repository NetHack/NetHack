let nethackStart = require("../src/nethackShim.js");
Error.stackTraceLimit = 20;

// debugging to make sure the JavaScript event loop isn't blocked
// const {performance} = require("perf_hooks");
// let currentTime = 0;
// let lastTime = 0;
// setInterval(() => {
//     lastTime = currentTime;
//     currentTime = performance.now();
//     console.log("Time since last JavaScript loop:", currentTime-lastTime);
// }, 10);

let Module = {};
let winCount = 0;

/* global globalThis */
nethackStart(async function (name, ... args) {
    switch(name) {
    case "shim_init_nhwindows":
        console.log("globalThis.nethackGlobal", globalThis.nethackGlobal);
        break;
    case "shim_create_nhwindow":
        winCount++;
        console.log("creating window", args, "returning", winCount);
        return winCount;
    case "shim_print_glyph":
        var x = args[1];
        var y = args[2];
        var glyph = args[3];

        var ret = globalThis.nethackGlobal.helpers.mapglyphHelper(glyph, x, y, 0);
        console.log(`GLYPH (${x},${y}): ${String.fromCharCode(ret.ch)}`);
        return;
    // case "shim_update_inventory":
    //     globalThis.nethackGlobal.helpers.displayInventory();
    //     return;
    case "shim_select_menu":
        return await selectMenu(...args);
    case "shim_yn_function":
    case "shim_message_menu":
        return 121; // 'y'
    case "shim_getmsghistory":
        return "";
    case "shim_nhgetch":
    case "shim_nh_poskey":
        return 0;
    default:
        console.log(`called doGraphics: ${name} [${args}]`);
        return 0;
    }
}, Module);

async function selectMenu(window, how, selected) {
    Module.setValue(selected, 0, "*");
    return -1;
}