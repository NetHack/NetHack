#!/usr/bin/env python3
"""Transcribe MENU, STR# and MNU# resources from a Mac resource fork
into Rez source.

MENU and STR# are emitted as structured Rez resources against the
Retro68 Multiverse.r templates so the result is hand-editable; 'MNU#'
is a NetHack-private type (menu-bar / submenu lists, see macmenu.c)
and is emitted as a raw data block.  Every byte of the originals is
preserved: if a MENU resource contains anything the structured
template cannot express (non-zero width/height/filler words, trailing
bytes), the tool falls back to a raw data block for that resource and
says so on stderr."""

import struct
import sys

EMITTED_TYPES = (b'MENU', b'STR#', b'MNU#')

# Resource attribute bits -> Rez attribute keywords (resChanged has no
# on-disk meaning and is rejected below).
ATTR_KEYWORDS = (
    (0x40, 'sysheap'),
    (0x20, 'purgeable'),
    (0x10, 'locked'),
    (0x08, 'protected'),
    (0x04, 'preload'),
)


def parse_rsrc(data):
    """Parse a resource fork; return {type: [(id, name|None, attr, bytes)]}
    preserving type-list and reference-list order."""
    data_offset, map_offset = struct.unpack('>II', data[0:8])
    type_list_offset, name_list_offset = \
        struct.unpack('>HH', data[map_offset+24:map_offset+28])
    tl = map_offset + type_list_offset
    nl = map_offset + name_list_offset
    num_types = struct.unpack('>H', data[tl:tl+2])[0] + 1

    out = {}
    pos = tl + 2
    for _ in range(num_types):
        rtype = data[pos:pos+4]
        count = struct.unpack('>H', data[pos+4:pos+6])[0] + 1
        ref_offset = struct.unpack('>H', data[pos+6:pos+8])[0]
        entries = []
        for j in range(count):
            rp = tl + ref_offset + j * 12
            rid, name_off = struct.unpack('>hh', data[rp:rp+4])
            attr = data[rp+4]
            doff = struct.unpack('>I', b'\0' + data[rp+5:rp+8])[0]
            name = None
            if name_off != -1:
                nlen = data[nl + name_off]
                name = data[nl+name_off+1:nl+name_off+1+nlen]
            dlen = struct.unpack('>I',
                                 data[data_offset+doff:data_offset+doff+4])[0]
            blob = data[data_offset+doff+4:data_offset+doff+4+dlen]
            entries.append((rid, name, attr, blob))
        out[rtype] = entries
        pos += 8
    return out


def rez_string(raw):
    """Render raw bytes as a Rez string literal (mac_roman text; anything
    outside printable ASCII becomes a \\0xNN escape)."""
    parts = ['"']
    for b in raw:
        if b == 0x22:
            parts.append('\\"')
        elif b == 0x5C:
            # Retro68 Rez compiles the "\\" escape to zero characters;
            # the hex escape round-trips correctly.
            parts.append('\\0x5C')
        elif 0x20 <= b <= 0x7E:
            parts.append(chr(b))
        else:
            parts.append('\\0x%02X' % b)
    parts.append('"')
    return ''.join(parts)


def rez_spec(rid, name, attr):
    """Render the (id, "name", attributes) resource specification."""
    spec = [str(rid)]
    if name is not None:
        spec.append(rez_string(name))
    rest = attr
    for bit, keyword in ATTR_KEYWORDS:
        if attr & bit:
            spec.append(keyword)
            rest &= ~bit
    if rest:
        raise ValueError(f"unrepresentable attribute bits 0x{rest:02x}")
    return ', '.join(spec)


def emit_data(out, rtype, rid, name, attr, blob, comment=None):
    """Emit a raw data block: data 'TYPE' (spec) { $"..." };"""
    out.append(f"data '{rtype.decode('mac_roman')}' ({rez_spec(rid, name, attr)}) {{"
               + (f"  /* {comment} */" if comment else ''))
    hexstr = blob.hex().upper()
    for i in range(0, len(hexstr), 32):
        chunk = hexstr[i:i+32]
        grouped = ' '.join(chunk[k:k+4] for k in range(0, len(chunk), 4))
        out.append(f'    $"{grouped}"')
    out.append('};')


def menu_key(b):
    """Render a menu item icon/key/mark/style byte as Rez source."""
    if b == 0x00:
        return 'noKey'
    if b == 0x1B:
        # Not the symbolic hierarchicalMenu: Multiverse.r defines it as
        # the malformed escape "\0x$1B", which Rez compiles to 0x01.
        return '"\\0x1B" /* hierarchicalMenu */'
    return rez_string(bytes([b]))


def menu_mark(b):
    if b == 0x00:
        return 'noMark'
    if b == 0x12:
        return 'check'
    return rez_string(bytes([b]))


def emit_menu(out, rid, name, attr, blob):
    """Emit one MENU resource against the Multiverse.r 'MENU' template:
    menuID, procID, enable flags, enabled, title, { items }.  Returns
    False (caller falls back to a data block) if the blob has content
    the template cannot reproduce byte-for-byte."""
    if len(blob) < 15:
        return False
    menu_id, width, height, proc_id, filler = struct.unpack('>5H', blob[0:10])
    flags = struct.unpack('>I', blob[10:14])[0]
    if width or height or filler:
        return False        # template hardwires these words to zero
    pos = 14
    tlen = blob[pos]
    title = blob[pos+1:pos+1+tlen]
    pos += 1 + tlen
    items = []
    while pos < len(blob) and blob[pos] != 0:
        ilen = blob[pos]
        text = blob[pos+1:pos+1+ilen]
        pos += 1 + ilen
        if pos + 4 > len(blob):
            return False
        icon, key, mark, style = blob[pos:pos+4]
        pos += 4
        items.append((text, icon, key, mark, style))
    if pos != len(blob) - 1:
        return False        # expect exactly one terminating zero byte

    enable31 = flags >> 1
    out.append(f"resource 'MENU' ({rez_spec(rid, name, attr)}) {{")
    out.append(f'    {menu_id},')
    out.append('    textMenuProc,' if proc_id == 0 else f'    {proc_id},')
    out.append('    allEnabled,' if enable31 == 0x7FFFFFFF
               else f'    0x{enable31:X},')
    out.append('    enabled,' if flags & 1 else '    disabled,')
    out.append('    apple,' if title == b'\x14' else f'    {rez_string(title)},')
    out.append('    {')
    for i, (text, icon, key, mark, style) in enumerate(items):
        fields = [rez_string(text),
                  'noIcon' if icon == 0 else str(icon),
                  menu_key(key),
                  menu_mark(mark),
                  'plain' if style == 0 else str(style)]
        sep = '' if i == len(items) - 1 else ','
        out.append('        ' + ', '.join(fields) + sep)
    out.append('    }')
    out.append('};')
    return True


def emit_strlist(out, rid, name, attr, blob):
    """Emit one STR# resource: { "s1", "s2", ... }.  Returns False if the
    blob is not a clean pstring list."""
    if len(blob) < 2:
        return False
    count = struct.unpack('>H', blob[0:2])[0]
    pos = 2
    strings = []
    for _ in range(count):
        if pos >= len(blob):
            return False
        slen = blob[pos]
        if pos + 1 + slen > len(blob):
            return False
        strings.append(blob[pos+1:pos+1+slen])
        pos += 1 + slen
    if pos != len(blob):
        return False
    out.append(f"resource 'STR#' ({rez_spec(rid, name, attr)}) {{")
    out.append('    {')
    for i, s in enumerate(strings):
        sep = '' if i == len(strings) - 1 else ','
        out.append('        ' + rez_string(s) + sep)
    out.append('    }')
    out.append('};')
    return True


def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <in.rsrc> <out.r>", file=sys.stderr)
        sys.exit(1)

    with open(sys.argv[1], 'rb') as f:
        resources = parse_rsrc(f.read())

    out = []
    out.append('/* nhmenus.r -- menu resources (MENU, STR#, MNU#) for the')
    out.append(' * Mac 68k port.  Transcribed from the historic NHrsrc.hqx by')
    out.append(' * tools/rsrc_to_rez.py; see macmenu.c for the MNU# format and')
    out.append(" * the STR# keystroke-dispatch convention ('\\0xA5' = comment/")
    out.append(' * separator marker, key 0x1B + mark = hierarchical submenu).')
    out.append(' */')
    out.append('')
    out.append('#include "Multiverse.r"')

    for rtype in sorted(t for t in resources if t in EMITTED_TYPES):
        for rid, name, attr, blob in sorted(resources[rtype]):
            out.append('')
            if rtype == b'MENU':
                if not emit_menu(out, rid, name, attr, blob):
                    print(f"Note: MENU {rid} does not fit the Rez template;"
                          " emitted as raw data", file=sys.stderr)
                    emit_data(out, rtype, rid, name, attr, blob)
            elif rtype == b'STR#':
                if not emit_strlist(out, rid, name, attr, blob):
                    print(f"Note: STR# {rid} is not a clean string list;"
                          " emitted as raw data", file=sys.stderr)
                    emit_data(out, rtype, rid, name, attr, blob)
            else:
                emit_data(out, rtype, rid, name, attr, blob,
                          comment='short firstMenuID; short count;'
                                  ' { short mresID; short 0; } [count]')

    with open(sys.argv[2], 'w') as f:
        f.write('\n'.join(out) + '\n')
    counts = ', '.join(f"{len(resources[t])} {t.decode()}"
                       for t in EMITTED_TYPES if t in resources)
    print(f"Wrote:   {sys.argv[2]} ({counts})")


if __name__ == '__main__':
    main()
