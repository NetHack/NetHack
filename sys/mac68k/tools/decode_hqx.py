#!/usr/bin/env python3
"""Decode BinHex 4.0 (.hqx) files into data fork + resource fork."""

import sys
import binascii
import struct
import os

# BinHex 4.0 character set
CHARS = '!"#$%&\'()*+,-012345689@ABCDEFGHIJKLMNPQRSTUVXYZ[`abcdefhijklmpqr'
TABLE = {c: i for i, c in enumerate(CHARS)}

def decode_hqx_stream(data):
    """Decode BinHex 4.0 encoded data to raw bytes."""
    # Strip header line and find content between colons
    lines = data.split('\n')
    content = ''
    in_data = False
    for line in lines:
        line = line.strip()
        if not in_data:
            # Note: some encoders put the opening ':' at the end of the
            # "(This file must be converted with BinHex 4.0)" intro line
            # instead of on its own line.  That case still decodes
            # correctly because non-alphabet characters are filtered
            # below, and the section CRCs catch any genuine corruption.
            if line.startswith(':'):
                in_data = True
                content += line[1:]  # skip leading colon
            continue
        if line.endswith(':'):
            content += line[:-1]  # skip trailing colon
            break
        content += line

    # Decode 6-bit characters to bytes
    bits = 0
    nbits = 0
    raw = bytearray()
    for c in content:
        if c in TABLE:
            bits = (bits << 6) | TABLE[c]
            nbits += 6
            while nbits >= 8:
                nbits -= 8
                raw.append((bits >> nbits) & 0xFF)

    # RLE decode: 0x90 is escape, 0x90 0x00 = literal 0x90
    decoded = bytearray()
    i = 0
    while i < len(raw):
        if raw[i] == 0x90:
            i += 1
            if i >= len(raw):
                break
            if raw[i] == 0x00:
                decoded.append(0x90)
            else:
                count = raw[i] - 1
                if decoded:
                    last = decoded[-1]
                    decoded.extend([last] * count)
                else:
                    print("Warning: RLE run marker with no preceding byte;"
                          " ignored", file=sys.stderr)
            i += 1
        else:
            decoded.append(raw[i])
            i += 1

    return bytes(decoded)

def parse_hqx_header(data):
    """Parse BinHex header: name, type, creator, flags, data len, rsrc len."""
    pos = 0
    namelen = data[pos]
    pos += 1
    name = data[pos:pos+namelen].decode('mac_roman', errors='replace')
    pos += namelen
    pos += 1  # version byte

    ftype = data[pos:pos+4].decode('ascii', errors='replace')
    pos += 4
    creator = data[pos:pos+4].decode('ascii', errors='replace')
    pos += 4
    flags = struct.unpack('>H', data[pos:pos+2])[0]
    pos += 2
    datalen = struct.unpack('>I', data[pos:pos+4])[0]
    pos += 4
    rsrclen = struct.unpack('>I', data[pos:pos+4])[0]
    pos += 4
    pos += 2  # header CRC

    return name, ftype, creator, flags, datalen, rsrclen, pos

def crc16_binhex(data):
    """BinHex 4.0 section CRC: CRC-16/XMODEM (poly 0x1021, init 0) over the
    section bytes, exactly binascii's crc_hqx (verified against the stored
    CRCs in NHrsrc.hqx)."""
    return binascii.crc_hqx(bytes(data), 0)


def apply_creator_fixup(fork, old_creator, new_creator):
    """Rename the signature resource type and patch the BNDL creator field.

    The historic NHrsrc.hqx carries the NetHack 3.1-era creator code; the
    application is built with a newer one, and the Finder only shows the
    app's icons when the signature resource and BNDL agree with it."""
    old_b, new_b = old_creator.encode(), new_creator.encode()
    if len(old_b) != 4 or len(new_b) != 4:
        raise ValueError("creator codes must be exactly 4 bytes")
    data = bytearray(fork)
    doff = struct.unpack('>I', data[0:4])[0]
    moff = struct.unpack('>I', data[4:8])[0]
    tl_off = struct.unpack('>H', data[moff+24:moff+26])[0]
    tl_start = moff + tl_off
    ntypes = struct.unpack('>H', data[tl_start:tl_start+2])[0] + 1
    pos = tl_start + 2
    found_sig = found_bndl = False
    for _ in range(ntypes):
        rtype = bytes(data[pos:pos+4])
        count = struct.unpack('>H', data[pos+4:pos+6])[0] + 1
        roff = struct.unpack('>H', data[pos+6:pos+8])[0]
        if rtype == old_b:
            data[pos:pos+4] = new_b
            found_sig = True
            print(f"Fixup:   signature resource type {old_creator} -> {new_creator}")
        if rtype == b'BNDL':
            ref_start = tl_start + roff
            for j in range(count):
                rp = ref_start + j * 12
                ao = struct.unpack('>I', data[rp+4:rp+8])[0]
                rdoff = ao & 0x00FFFFFF
                abs_off = doff + rdoff
                if bytes(data[abs_off+4:abs_off+8]) == old_b:
                    data[abs_off+4:abs_off+8] = new_b
                    found_bndl = True
                    print(f"Fixup:   BNDL creator {old_creator} -> {new_creator}")
        pos += 8
    if not (found_sig and found_bndl):
        print(f"Error: --creator-fixup found no"
              f"{'' if found_sig else ' signature'}"
              f"{'' if found_bndl else ' BNDL'}"
              f" resource for creator {old_creator!r}", file=sys.stderr)
        sys.exit(1)
    return bytes(data)


def main():
    global creator_fixup
    creator_fixup = None
    args = sys.argv[1:]
    if '--creator-fixup' in args:
        i = args.index('--creator-fixup')
        creator_fixup = (args[i+1], args[i+2])
        del args[i:i+3]
    sys.argv[1:] = args
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <file.hqx> [output_dir]"
              " [--creator-fixup OLD NEW]")
        sys.exit(1)

    hqx_file = sys.argv[1]
    outdir = sys.argv[2] if len(sys.argv) > 2 else '.'

    with open(hqx_file, 'r') as f:
        raw_data = f.read()

    decoded = decode_hqx_stream(raw_data)
    name, ftype, creator, flags, datalen, rsrclen, hdr_end = parse_hqx_header(decoded)

    print(f"Name:    {name}")
    print(f"Type:    {ftype}")
    print(f"Creator: {creator}")
    print(f"Flags:   0x{flags:04x}")
    print(f"Data:    {datalen} bytes")
    print(f"Rsrc:    {rsrclen} bytes")

    data_start = hdr_end
    data_end = data_start + datalen
    rsrc_start = data_end + 2  # skip data CRC
    rsrc_end = rsrc_start + rsrclen

    # Verify all three section CRCs; a bad .hqx must not silently produce
    # a corrupt resource fork.
    hdr_crc = struct.unpack('>H', decoded[hdr_end-2:hdr_end])[0]
    data_crc = struct.unpack('>H', decoded[data_end:data_end+2])[0]
    rsrc_crc = struct.unpack('>H', decoded[rsrc_end:rsrc_end+2])[0]
    for label, blob, want in (("header", decoded[0:hdr_end-2], hdr_crc),
                              ("data fork", decoded[data_start:data_end], data_crc),
                              ("resource fork", decoded[rsrc_start:rsrc_end], rsrc_crc)):
        got = crc16_binhex(blob)
        if got != want:
            print(f"Error: {label} CRC mismatch"
                  f" (stored {want:#06x}, computed {got:#06x})", file=sys.stderr)
            sys.exit(1)
    print("CRC:     header/data/rsrc verified")

    data_fork = decoded[data_start:data_end]
    rsrc_fork = decoded[rsrc_start:rsrc_end]

    os.makedirs(outdir, exist_ok=True)

    safe_name = name.replace('/', '_').replace(':', '_')
    if datalen > 0:
        data_path = os.path.join(outdir, safe_name + '.data')
        with open(data_path, 'wb') as f:
            f.write(data_fork)
        print(f"Wrote:   {data_path} ({len(data_fork)} bytes)")

    if rsrclen > 0:
        if creator_fixup:
            rsrc_fork = apply_creator_fixup(rsrc_fork, *creator_fixup)
        rsrc_path = os.path.join(outdir, safe_name + '.rsrc')
        with open(rsrc_path, 'wb') as f:
            f.write(rsrc_fork)
        print(f"Wrote:   {rsrc_path} ({len(rsrc_fork)} bytes)")

if __name__ == '__main__':
    main()
