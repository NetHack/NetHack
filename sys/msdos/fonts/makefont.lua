#!/usr/bin/env lua
--   Copyright (c) 2016, 2022 Ray Chason
--   NetHack may be freely redistributed.  See license for details.

-- IBM437 order as NetHack expects
ibm437 = {
    0x0000, 0x263A, 0x263B, 0x2665, 0x2666, 0x2663, 0x2660, 0x2022,
    0x25D8, 0x25CB, 0x25D9, 0x2642, 0x2640, 0x266A, 0x266B, 0x263C,
    0x25BA, 0x25C4, 0x2195, 0x203C, 0x00B6, 0x00A7, 0x25AC, 0x21A8,
    0x2191, 0x2193, 0x2192, 0x2190, 0x221F, 0x2194, 0x25B2, 0x25BC,
    0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
    0x0028, 0x0029, 0x002A, 0x002B, 0x002C, 0x002D, 0x002E, 0x002F,
    0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
    0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D, 0x003E, 0x003F,
    0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
    0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
    0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
    0x0058, 0x0059, 0x005A, 0x005B, 0x005C, 0x005D, 0x005E, 0x005F,
    0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
    0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,
    0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
    0x0078, 0x0079, 0x007A, 0x007B, 0x007C, 0x007D, 0x007E, 0x2302,
    0x00C7, 0x00FC, 0x00E9, 0x00E2, 0x00E4, 0x00E0, 0x00E5, 0x00E7,
    0x00EA, 0x00EB, 0x00E8, 0x00EF, 0x00EE, 0x00EC, 0x00C4, 0x00C5,
    0x00C9, 0x00E6, 0x00C6, 0x00F4, 0x00F6, 0x00F2, 0x00FB, 0x00F9,
    0x00FF, 0x00D6, 0x00DC, 0x00A2, 0x00A3, 0x00A5, 0x20A7, 0x0192,
    0x00E1, 0x00ED, 0x00F3, 0x00FA, 0x00F1, 0x00D1, 0x00AA, 0x00BA,
    0x00BF, 0x2310, 0x00AC, 0x00BD, 0x00BC, 0x00A1, 0x00AB, 0x00BB,
    0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556,
    0x2555, 0x2563, 0x2551, 0x2557, 0x255D, 0x255C, 0x255B, 0x2510,
    0x2514, 0x2534, 0x252C, 0x251C, 0x2500, 0x253C, 0x255E, 0x255F,
    0x255A, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256C, 0x2567,
    0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256B,
    0x256A, 0x2518, 0x250C, 0x2588, 0x2584, 0x258C, 0x2590, 0x2580,
    0x03B1, 0x00DF, 0x0393, 0x03C0, 0x03A3, 0x03C3, 0x00B5, 0x03C4,
    0x03A6, 0x0398, 0x03A9, 0x03B4, 0x221E, 0x03C6, 0x03B5, 0x2229,
    0x2261, 0x00B1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00F7, 0x2248,
    0x00B0, 0x2219, 0x00B7, 0x221A, 0x207F, 0x00B2, 0x25A0, 0x00A0
}

-- As ibm437, but mapping Unicode back to the IBM437 position
ibm437_rev = {}
for i = 1, #ibm437 do
    ibm437_rev[ibm437[i]] = i
end

-- Information about a single glyph in the font
function new_glyph(width, height)
    local g = {}
    g.width = width
    g.height = height
    g.bytes = ""
    g.code = {}
    return g
end

-- This converts an unsigned integer to four little endian bytes
function write_u32(x)
    return string.pack("<I4", x)
end

-- Return true if str starts with prefix
function start_with(str, prefix)
    return string.sub(str, 1, string.len(prefix)) == prefix
end

-- Split a string at its spaces; return an array containing the pieces
function split(str)
    local pieces = {}
    local index = 1
    local len = string.len(str)
    while index <= len do
        local end_index, end_match
        end_index, end_match = string.find(str, " ", index)
        if end_index == fail then
            end_index = len
            end_match = len
        end
        substr = string.sub(str, index, end_index)
        pieces[#pieces+1] = substr
        index = end_match + 1
    end
    return pieces
end

-- Variables to be used in the main loop
font = {}          -- Font position to glyph
next_pos = 257     -- Next position for non-IBM437 code points
font_by_bytes = {} -- Bitmap string to font position
font_by_code = {}  -- Unicode code point to glyph
width = nil
height = nil
bwidth = nil
glyph = nil
bitmap = false

if #arg < 2 then
    io.stderr:write("Usage: " .. arg[0] .. " <input-bdf> [<input-bdf>...] <output-psf>\n")
    os.exit(1)
end

for inp = 1, #arg-1 do

    fp = io.open(arg[inp], "r")
    if fp == nil then
        io.stderr:write("Could not open: " .. arg[inp] .. "\n")
        os.exit(1)
    end

    while true do
        line = fp:read()
        if (line == fail) then
            break
        end
        if start_with(line, 'FONTBOUNDINGBOX ') then
            -- Width and height of a glyph
            rec = split(line)
            if inp == 1 then
                -- First input file sets the dimensions
                width = tonumber(rec[2])
                height = tonumber(rec[3])
                bwidth = math.floor((width + 7) / 8)
            else
                -- Any others must match, or an error results
                if width ~= tonumber(rec[2]) or height ~= tonumber(rec[3]) then
                    io.stderr:write(arg[inp] .. " bounding box does not match that of " .. arg[1] .. "\n")
                    os.exit(1)
                end
            end
        elseif start_with(line, 'STARTCHAR ') then
            -- A glyph begins here
            glyph = new_glyph(width, height)
        elseif start_with(line, 'ENCODING ') then
            -- This line provides the Unicode code point
            rec = split(line)
            glyph.code[#glyph.code+1] = tonumber(rec[2])
        elseif start_with(line, 'BITMAP') then
            -- Bitmap data appears on following lines
            bitmap = true
        elseif start_with(line, 'ENDCHAR') then
            -- End of bitmap data
            -- Position will be according to IBM437 if the code point is in
            -- IBM437, else matching any prior occurrence if the same glyph has
            -- appeared before, else as a new glyph
            pos = ibm437_rev[glyph.code[1]] or font_by_bytes[glyph.bytes]
            if pos == nil then
                pos = next_pos
                next_pos = next_pos + 1
            end
            if font[pos] == nil then
                font[pos] = glyph
                font_by_bytes[glyph.bytes] = pos
            else
                for i = 1, #glyph.code do
                    font[pos].code[#font[pos].code+1] = glyph.code[i]
                end
            end
            font_by_code[glyph.code[1]] = pos
            glyph = nil
            bitmap = false
        elseif bitmap then
            -- Hex data after BITMAP and before ENDCHAR
            for i = 1, bwidth do
                byte = string.sub(line, i*2-1, i*2)
                glyph.bytes = glyph.bytes .. string.char(tonumber(byte, 16))
            end
        end
    end

    fp:close()
end

-- The provided BDFs code positions 16 and 17 differently from what NetHack
-- expects
if font[ibm437_rev[0x25BA]] == nil and font_by_code[0x25B6] ~= nil then
    pos1 = font_by_code[0x25B6]
    pos2 = ibm437_rev[0x25BA]
    font[pos2] = font[pos1]
    font[pos2].code[#font[pos2].code+1] = 0x25BA
    font[pos1] = nil
end
if font[ibm437_rev[0x25C4]] == nil and font_by_code[0x25C0] ~= nil then
    pos1 = font_by_code[0x25C0]
    pos2 = ibm437_rev[0x25C4]
    font[pos2] = font[pos1]
    font[pos2].code[#font[pos2].code+1] = 0x25C4
    font[pos1] = nil
end

-- Fill any empty slots and warn
for i = 1, 256 do
    if font[i] == nil then
        if next_pos > 257 then
            next_pos = next_pos - 1
            font[i] = font[next_pos]
            font[next_pos] = nil
        end
    end
end

outfile = io.open(arg[#arg], "wb")

-- Write the PSF header
outfile:write("\x72\xB5\x4A\x86")           -- magic
outfile:write(write_u32(0))                 -- version
outfile:write(write_u32(32))                -- headersize
outfile:write(write_u32(0x01))              -- flags (Unicode table present)
outfile:write(write_u32(next_pos-1))        -- length
outfile:write(write_u32(bwidth * height))   -- charsize
outfile:write(write_u32(height))            -- height
outfile:write(write_u32(width))             -- width

-- Write the glyphs
for i = 1, next_pos-1 do
    glyph = font[i]
    outfile:write(glyph.bytes)
end

-- Write the Unicode mappings
for i = 1, next_pos-1 do
    glyph = font[i]
    for j = 1, #glyph.code do
        outfile:write(utf8.char(glyph.code[j]))
    end
    outfile:write("\xFF")
end

outfile:close()
