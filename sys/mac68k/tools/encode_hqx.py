#!/usr/bin/env python3
"""Encode a file into BinHex 4.0 (.hqx), inverse of decode_hqx.py.

The input file becomes the resource fork (data fork empty), matching
how the NetHack UI resource files are shipped.  Python's binascii lost
its hqx codecs in 3.13, so the RLE90 + 6-bit stages live here; the
section CRCs are CRC-16/XMODEM exactly as decode_hqx.py verifies them."""

import binascii
import os
import struct
import sys

# BinHex 4.0 character set (same table as decode_hqx.py)
CHARS = '!"#$%&\'()*+,-012345689@ABCDEFGHIJKLMNPQRSTUVXYZ[`abcdefhijklmpqr'

BANNER = '(This file must be converted with BinHex 4.0)'


def rle90_encode(data):
    """RLE-compress: runs of 4..255 become <byte> 0x90 <count>; a literal
    0x90 is escaped as 0x90 0x00 (the run marker then still applies to
    it, so runs of 0x90 encode as 0x90 0x00 0x90 <count>)."""
    out = bytearray()
    i = 0
    n = len(data)
    while i < n:
        b = data[i]
        run = 1
        while run < 255 and i + run < n and data[i + run] == b:
            run += 1
        if b == 0x90:
            out += b'\x90\x00'
            if run > 1:
                out += bytes((0x90, run))
        elif run >= 4:
            out += bytes((b, 0x90, run))
        else:
            out += bytes((b,)) * run
        i += run
    return bytes(out)


def encode_6bit(data):
    """Pack bytes into the 6-bit BinHex alphabet (zero-padded tail)."""
    out = []
    bits = 0
    nbits = 0
    for b in data:
        bits = (bits << 8) | b
        nbits += 8
        while nbits >= 6:
            nbits -= 6
            out.append(CHARS[(bits >> nbits) & 0x3F])
    if nbits:
        out.append(CHARS[(bits << (6 - nbits)) & 0x3F])
    return ''.join(out)


def crc16_binhex(data):
    """BinHex 4.0 section CRC: CRC-16/XMODEM (poly 0x1021, init 0)."""
    return binascii.crc_hqx(bytes(data), 0)


def build_payload(name, ftype, creator, flags, data_fork, rsrc_fork):
    """Assemble header + forks with the three section CRCs."""
    name_b = name.encode('mac_roman')
    if len(name_b) > 63:
        raise ValueError("file name too long for BinHex header")
    header = bytes([len(name_b)]) + name_b + b'\0'
    header += ftype.encode('mac_roman') + creator.encode('mac_roman')
    header += struct.pack('>HII', flags, len(data_fork), len(rsrc_fork))

    payload = bytearray()
    payload += header + struct.pack('>H', crc16_binhex(header))
    payload += data_fork + struct.pack('>H', crc16_binhex(data_fork))
    payload += rsrc_fork + struct.pack('>H', crc16_binhex(rsrc_fork))
    return bytes(payload)


def main():
    opts = {'--type': 'RSRC', '--creator': 'RSED',
            '--name': None, '--flags': '0x0100'}
    args = sys.argv[1:]
    for opt in list(opts):
        if opt in args:
            i = args.index(opt)
            opts[opt] = args[i + 1]
            del args[i:i+2]
    if len(args) != 2:
        print(f"Usage: {sys.argv[0]} <file> <out.hqx>"
              " [--type RSRC] [--creator RSED] [--name NAME] [--flags 0xNNNN]",
              file=sys.stderr)
        sys.exit(1)

    infile, outfile = args
    name = opts['--name'] if opts['--name'] is not None \
        else os.path.basename(infile)
    flags = int(opts['--flags'], 0)

    with open(infile, 'rb') as f:
        rsrc_fork = f.read()

    payload = build_payload(name, opts['--type'], opts['--creator'],
                            flags, b'', rsrc_fork)
    stream = ':' + encode_6bit(rle90_encode(payload)) + ':'
    lines = [BANNER]
    lines += [stream[i:i+64] for i in range(0, len(stream), 64)]

    with open(outfile, 'w') as f:
        f.write('\n'.join(lines) + '\n')

    print(f"Name:    {name}")
    print(f"Type:    {opts['--type']}")
    print(f"Creator: {opts['--creator']}")
    print(f"Flags:   0x{flags:04x}")
    print(f"Rsrc:    {len(rsrc_fork)} bytes")
    print(f"Wrote:   {outfile}")


if __name__ == '__main__':
    main()
