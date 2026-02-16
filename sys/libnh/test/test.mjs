#!/usr/bin/env node

// Integration test for the NetHack 3.6 WASM build.
// Verifies that the module loads, callbacks fire correctly, and the game
// produces valid rendering data (map glyphs, status fields, text output).
//
// Usage:
//   node sys/libnh/test/test.mjs              # from the NetHack-3.6 directory
//   node NetHack-3.6/sys/libnh/test/test.mjs  # from the repository root

import { fileURLToPath } from "url";
import { dirname, join, resolve } from "path";
import { existsSync } from "fs";

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

// Find nethack.js — try relative to this file first, then cwd
const candidates = [
    join(__dirname, "..", "..", "..", "src", "nethack.js"),   // from NetHack-3.6/sys/libnh/test/
    join(process.cwd(), "src", "nethack.js"),                // from NetHack-3.6/
    join(process.cwd(), "NetHack-3.6", "src", "nethack.js"), // from repository root
];
const nethackPath = candidates.find(p => existsSync(p));
if (!nethackPath) {
    console.error("ERROR: Cannot find src/nethack.js. Run build-wasm.sh first.");
    console.error("Searched:", candidates.map(p => resolve(p)).join("\n  "));
    process.exit(1);
}

// ── Glyph offset constants (NUMMONS=381, NUM_OBJECTS=453, MAXPCHARS=96) ──
const NUMMONS = 381;
const NUM_OBJECTS = 453;
const GLYPH_MON_OFF = 0;
const GLYPH_PET_OFF = NUMMONS;
const GLYPH_INVIS_OFF = NUMMONS + GLYPH_PET_OFF;
const GLYPH_DETECT_OFF = 1 + GLYPH_INVIS_OFF;
const GLYPH_BODY_OFF = NUMMONS + GLYPH_DETECT_OFF;
const GLYPH_RIDDEN_OFF = NUMMONS + GLYPH_BODY_OFF;
const GLYPH_OBJ_OFF = NUMMONS + GLYPH_RIDDEN_OFF;
const GLYPH_CMAP_OFF = NUM_OBJECTS + GLYPH_OBJ_OFF;
// NO_GLYPH = MAX_GLYPH = 5976 for this build
const NO_GLYPH = 5976;

// S_ cmap symbols -> ASCII (from include/rm.h + src/drawing.c)
const CMAP_CHARS = {
    0: " ", 1: "|", 2: "-", 3: "-", 4: "-", 5: "-", 6: "-", 7: "-",
    8: "-", 9: "-", 10: "|", 11: "|", 12: ".", 13: "|", 14: "-",
    15: "+", 16: "+", 17: "#", 18: "#", 19: ".", 20: ".", 21: "#",
    22: "#", 23: "<", 24: ">", 25: "<", 26: ">", 27: "_", 28: "|",
    29: "\\", 30: "{", 31: "{", 32: "}", 33: ".", 34: "}", 35: ".",
    36: ".", 37: "#", 38: "#", 39: " ", 40: "#", 41: "}",
};

function glyphToChar(g) {
    if (g >= GLYPH_CMAP_OFF) return CMAP_CHARS[g - GLYPH_CMAP_OFF] || "?";
    if (g >= GLYPH_OBJ_OFF) return ")";
    if (g >= GLYPH_RIDDEN_OFF) return "u";
    if (g >= GLYPH_BODY_OFF) return "%";
    if (g >= GLYPH_DETECT_OFF) return "M";
    if (g === GLYPH_INVIS_OFF) return "I";
    if (g >= GLYPH_PET_OFF) return "d";
    if (g >= GLYPH_MON_OFF) return "@";
    return "?";
}

function glyphCategory(g) {
    if (g === NO_GLYPH) return "NO_GLYPH";
    if (g >= GLYPH_CMAP_OFF) return `CMAP(${g - GLYPH_CMAP_OFF})`;
    if (g >= GLYPH_OBJ_OFF) return `OBJ(${g - GLYPH_OBJ_OFF})`;
    if (g >= GLYPH_RIDDEN_OFF) return `RIDDEN(${g - GLYPH_RIDDEN_OFF})`;
    if (g >= GLYPH_BODY_OFF) return `BODY(${g - GLYPH_BODY_OFF})`;
    if (g >= GLYPH_DETECT_OFF) return `DETECT(${g - GLYPH_DETECT_OFF})`;
    if (g === GLYPH_INVIS_OFF) return "INVISIBLE";
    if (g >= GLYPH_PET_OFF) return `PET(${g - GLYPH_PET_OFF})`;
    return `MON(${g})`;
}

// ── Test harness ──

let passed = 0;
let failed = 0;

function assert(condition, label) {
    if (condition) {
        passed++;
    } else {
        failed++;
        console.log(`  FAIL: ${label}`);
    }
}

// ── Data collectors ──

const windows = {};
const glyphs = [];
const statusFields = {};
const putstrCalls = [];
const rawPrints = [];
const callbackNames = new Set();
let cmdCount = 0;
let nextWinId = 1;
let cursorX = -1, cursorY = -1;
let Module;

// ── Main ──

async function main() {
    console.log("NetHack 3.6 WASM — Integration Test\n");

    // ── Load module ──
    console.log("Loading module...");
    const createModule = (await import(nethackPath)).default;
    Module = await createModule({
        noInitialRun: true,
        print: () => {},
        printErr: () => {},
    });

    // ── Install helpers ──
    globalThis.nethackGlobal = globalThis.nethackGlobal || {};
    globalThis.nethackGlobal.helpers = {
        getPointerValue: (name, ptr, type) => {
            switch (type) {
                case "i": case "2": return Module.getValue(ptr, "i32");
                case "0": return Module.getValue(ptr, "i8");
                case "1": return Module.getValue(ptr, "i16");
                case "s": return Module.UTF8ToString(ptr);
                case "p": return ptr;
                case "b": return Module.getValue(ptr, "i8") !== 0;
                case "c": return String.fromCharCode(Module.getValue(ptr, "i8"));
                default:  return ptr;
            }
        },
        setPointerValue: (name, ptr, type, val) => {
            if (!ptr) return;
            switch (type) {
                case "i": case "2": Module.setValue(ptr, val | 0, "i32"); break;
                case "0": Module.setValue(ptr, val | 0, "i8"); break;
                case "1": Module.setValue(ptr, val | 0, "i16"); break;
                case "b": Module.setValue(ptr, val ? 1 : 0, "i8"); break;
                case "c": Module.setValue(ptr, typeof val === "string"
                              ? val.charCodeAt(0) : (val | 0), "i8"); break;
                case "v": break;
                default:  Module.setValue(ptr, val | 0, "i32"); break;
            }
        },
    };

    // ── Install callback ──
    globalThis.nethackCallback = async (name, ...args) => {
        callbackNames.add(name);
        switch (name) {
            case "shim_init_nhwindows": return;
            case "shim_askname": return;
            case "shim_player_selection": return;
            case "shim_create_nhwindow": {
                const type = args[0];
                const winid = nextWinId++;
                const names = { 1: "MESSAGE", 2: "STATUS", 3: "MAP", 4: "MENU", 5: "TEXT" };
                windows[winid] = { type: names[type] || `TYPE${type}`, typeNum: type };
                return winid;
            }
            case "shim_display_nhwindow": return;
            case "shim_clear_nhwindow": return;
            case "shim_putstr":
                putstrCalls.push({ winid: args[0], attr: args[1], text: args[2] });
                return;
            case "shim_print_glyph":
                glyphs.push({ winid: args[0], x: args[1], y: args[2],
                              glyph: args[3], bkglyph: args[4] });
                return;
            case "shim_curs":
                cursorX = args[1]; cursorY = args[2];
                return;
            case "shim_status_init": return;
            case "shim_status_enablefield": return;
            case "shim_status_update": {
                const fldidx = args[0];
                const ptr = args[1];
                let strVal = "";
                if (ptr && typeof ptr === "number" && ptr > 0) {
                    try { strVal = Module.UTF8ToString(ptr); } catch (e) { strVal = ""; }
                }
                statusFields[fldidx] = { strVal, chg: args[2], percent: args[3] };
                return;
            }
            case "shim_start_menu": return;
            case "shim_add_menu": return;
            case "shim_end_menu": return;
            case "shim_select_menu": return 0;
            case "shim_nhgetch":
            case "shim_nh_poskey":
                cmdCount++;
                if (cmdCount > 5) { runTests(); process.exit(failed > 0 ? 1 : 0); }
                return 32;
            case "shim_yn_function": return 121; // 'y'
            case "shim_raw_print":
            case "shim_raw_print_bold":
                rawPrints.push(String(args[0] || ""));
                return;
            case "shim_getmsghistory": return "";
            case "shim_putmsghistory": return;
            case "shim_getlin": return;
            case "shim_message_menu": return 0;
            case "shim_nhbell": return;
            case "shim_delay_output": return;
            case "shim_mark_synch": return;
            case "shim_wait_synch": return;
            case "shim_start_screen": return;
            case "shim_end_screen": return;
            case "shim_get_nh_event": return;
            case "shim_number_pad": return;
            case "shim_exit_nhwindows": return;
            case "shim_cliparound": return;
            case "shim_preference_update": return;
            case "shim_destroy_nhwindow": return;
            case "shim_doprev_message": return 0;
            case "shim_update_positionbar": return;
            case "shim_outrip": return;
            case "shim_display_file": return;
            case "shim_get_ext_cmd": return -1;
            case "shim_suspend_nhwindows": return;
            case "shim_resume_nhwindows": return;
            default: return 0;
        }
    };

    const setCallback = Module.cwrap("shim_graphics_set_callback", null, ["string"]);
    setCallback("nethackCallback");
    try { Module._main(0, 0); } catch (e) {}
}

// ── Tests ──

function runTests() {
    console.log(""); // blank line after any raw_print output

    // 1. Module exports
    testSection("Module Exports");
    assert(typeof Module._main === "function", "_main is exported");
    assert(typeof Module._malloc === "function", "_malloc is exported");
    assert(typeof Module.cwrap === "function", "cwrap is available");
    assert(typeof Module.getValue === "function", "getValue is available");
    assert(typeof Module.setValue === "function", "setValue is available");
    assert(typeof Module.UTF8ToString === "function", "UTF8ToString is available");
    assert(typeof Module.FS === "object", "FS is available");

    // 2. Embedded filesystem
    testSection("Filesystem");
    const rootFiles = Module.FS.readdir("/").filter(f => f !== "." && f !== "..");
    assert(rootFiles.includes("nhdat"), "nhdat exists in virtual FS");
    assert(rootFiles.includes("sysconf"), "sysconf exists in virtual FS");
    const nhdat = Module.FS.stat("/nhdat");
    assert(nhdat.size > 1000000, `nhdat is large enough (${nhdat.size} bytes)`);

    // 3. Callbacks fired
    testSection("Callbacks");
    assert(callbackNames.has("shim_init_nhwindows"), "shim_init_nhwindows was called");
    assert(callbackNames.has("shim_player_selection"), "shim_player_selection was called");
    assert(callbackNames.has("shim_create_nhwindow"), "shim_create_nhwindow was called");
    assert(callbackNames.has("shim_print_glyph"), "shim_print_glyph was called");
    assert(callbackNames.has("shim_status_update"), "shim_status_update was called");
    assert(callbackNames.has("shim_nh_poskey") || callbackNames.has("shim_nhgetch"),
        "input callback (nh_poskey or nhgetch) was called");
    assert(callbackNames.size >= 10, `at least 10 unique callbacks (got ${callbackNames.size})`);

    // 4. Windows
    testSection("Windows");
    const winTypes = Object.values(windows).map(w => w.type);
    assert(winTypes.includes("MESSAGE"), "MESSAGE window was created");
    assert(winTypes.includes("MAP"), "MAP window was created");
    assert(Object.keys(windows).length >= 2, `at least 2 windows created (got ${Object.keys(windows).length})`);

    // 5. Map / glyph data
    testSection("Map");
    assert(glyphs.length > 20, `enough glyphs received (got ${glyphs.length})`);

    const mapWinId = Object.entries(windows).find(([, w]) => w.type === "MAP")?.[0];
    const mapGlyphs = mapWinId ? glyphs.filter(g => g.winid === parseInt(mapWinId)) : [];
    assert(mapGlyphs.length > 20, `glyphs sent to MAP window (got ${mapGlyphs.length})`);

    // Check coordinate ranges are plausible (NetHack map is 80x21)
    const xs = mapGlyphs.map(g => g.x);
    const ys = mapGlyphs.map(g => g.y);
    assert(Math.min(...xs) >= 0 && Math.max(...xs) < 80, `x range valid (${Math.min(...xs)}-${Math.max(...xs)})`);
    assert(Math.min(...ys) >= 0 && Math.max(...ys) < 21, `y range valid (${Math.min(...ys)}-${Math.max(...ys)})`);

    // Check glyph values are in valid range
    const allGlyphVals = mapGlyphs.map(g => g.glyph);
    assert(allGlyphVals.every(g => g >= 0 && g <= NO_GLYPH), "all glyphs in valid range");

    // Check background glyphs
    const bkgs = [...new Set(mapGlyphs.map(g => g.bkglyph))];
    assert(bkgs.every(g => g >= 0 && g <= NO_GLYPH), "background glyphs in valid range");

    // Should have a mix of cmap (walls/floors) and at least one monster/object
    const hasCmap = allGlyphVals.some(g => g >= GLYPH_CMAP_OFF && g < GLYPH_CMAP_OFF + 96);
    const hasNonCmap = allGlyphVals.some(g => g < GLYPH_CMAP_OFF);
    assert(hasCmap, "map contains cmap glyphs (walls, floors)");
    assert(hasNonCmap, "map contains non-cmap glyphs (monsters, objects, pets)");

    // Check that room floor (S_room=19) is the most common glyph
    const roomGlyph = GLYPH_CMAP_OFF + 19;
    const roomCount = allGlyphVals.filter(g => g === roomGlyph).length;
    assert(roomCount >= 5, `room floor glyph is common (${roomCount} cells)`);

    // Cursor position should be on the map
    assert(cursorX >= 0 && cursorX < 80, `cursor X on map (${cursorX})`);
    assert(cursorY >= 0 && cursorY < 21, `cursor Y on map (${cursorY})`);

    // 6. Status
    testSection("Status");
    assert(Object.keys(statusFields).length >= 10, `enough status fields (got ${Object.keys(statusFields).length})`);

    // TITLE (field 0) should be a non-empty string
    const title = statusFields[0];
    assert(title && title.strVal.length > 0, `BL_TITLE has text ("${title?.strVal}")`);

    // HP (field 18) should be a positive number
    const hp = statusFields[18];
    assert(hp && parseInt(hp.strVal) > 0, `BL_HP is positive (${hp?.strVal})`);

    // HPMAX (field 19) should match or exceed HP
    const hpmax = statusFields[19];
    assert(hpmax && parseInt(hpmax.strVal) >= parseInt(hp?.strVal || "0"),
        `BL_HPMAX >= BL_HP (${hpmax?.strVal} >= ${hp?.strVal})`);

    // LEVELDESC (field 20) should say Dlvl:1
    const level = statusFields[20];
    assert(level && level.strVal.includes("Dlvl:1"), `BL_LEVELDESC is Dlvl:1 ("${level?.strVal.trim()}")`);

    // AC (field 14) should be a number
    const ac = statusFields[14];
    assert(ac && !isNaN(parseInt(ac.strVal)), `BL_AC is numeric (${ac?.strVal})`);

    // 7. Text output
    testSection("Text Output");
    assert(putstrCalls.length > 0, `putstr was called (${putstrCalls.length} times)`);

    // Quest intro text should be present
    const introLine = putstrCalls.find(p => p.text && p.text.includes("It is written"));
    assert(introLine, "quest intro text was displayed");

    const molochLine = putstrCalls.find(p => p.text && p.text.includes("Moloch"));
    assert(molochLine, "creation story mentions Moloch");

    // 8. Welcome message
    testSection("Welcome");
    const welcomeMsg = rawPrints.find(m => m.includes("welcome to NetHack"));
    assert(welcomeMsg, "welcome message received");
    assert(welcomeMsg && !welcomeMsg.includes("(null)"), "welcome message has no (null) role");

    // 9. No errors
    testSection("No Errors");
    const errors = rawPrints.filter(m =>
        m.includes("Program in disorder") ||
        m.includes("init-prob error") ||
        m.includes("Configuration incompatibility") ||
        m.includes("Dungeon description not valid") ||
        m.includes("CANNOT OPEN")
    );
    assert(errors.length === 0, `no fatal errors in raw_print (${errors.length > 0 ? errors[0] : "ok"})`);

    const qtError = rawPrints.find(m => m.includes("load_qtlist"));
    assert(!qtError, "no quest text loading errors");

    // ── Render the ASCII map ──
    console.log("\n── ASCII Map ──\n");
    if (mapGlyphs.length > 0) {
        const grid = {};
        for (const g of mapGlyphs) grid[`${g.y},${g.x}`] = glyphToChar(g.glyph);
        const minY = Math.min(...ys), maxY = Math.max(...ys);
        const minX = Math.min(...xs), maxX = Math.max(...xs);
        for (let y = minY; y <= maxY; y++) {
            let row = "";
            for (let x = minX; x <= maxX; x++) {
                if (y === cursorY && x === cursorX) row += "@";
                else row += grid[`${y},${x}`] || " ";
            }
            console.log(`  ${row}`);
        }

        console.log(`\n  Cursor: (${cursorX}, ${cursorY})`);
        const glyphFreq = {};
        for (const g of mapGlyphs) {
            const cat = glyphCategory(g.glyph);
            glyphFreq[cat] = (glyphFreq[cat] || 0) + 1;
        }
        console.log("  Glyphs:", Object.entries(glyphFreq)
            .sort((a, b) => b[1] - a[1])
            .map(([cat, n]) => `${cat}:${n}`)
            .join("  "));
    }

    // ── Status line ──
    console.log("\n── Status ──\n");
    const fieldNames = {
        0: "Title", 1: "Str", 2: "Dx", 3: "Co", 4: "In", 5: "Wi", 6: "Ch",
        7: "Align", 9: "Cap", 10: "Gold", 11: "Ene", 12: "EneMax",
        13: "XP", 14: "AC", 17: "Hunger", 18: "HP", 19: "HPmax",
        20: "Dlvl", 22: "Cond",
    };
    const statusLine = [];
    for (const [idx, name] of Object.entries(fieldNames)) {
        const f = statusFields[idx];
        if (f && f.strVal) statusLine.push(`${name}:${f.strVal.trim()}`);
    }
    console.log(`  ${statusLine.join("  ")}`);

    // ── Summary ──
    console.log(`\n── Results: ${passed} passed, ${failed} failed ──\n`);
    if (failed > 0) console.log("FAIL");
    else console.log("OK");
}

function testSection(name) {
    console.log(`  ${name}`);
}

main().catch(e => {
    console.error("FATAL:", e);
    process.exit(1);
});
