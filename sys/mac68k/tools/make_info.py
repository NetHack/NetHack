#!/usr/bin/env python3
# NetHack 5.0	make_info.py	*/
# Copyright (c) Ingo Paschke, 2026.
# NetHack may be freely redistributed.  See license for details.
"""Write a macutils-style .info sidecar for the StuffIt staging.

The sit packer takes a file's Mac metadata from <name>.info: Pascal
name at offset 2, type/creator at 66/70, and creation/modification
dates (Mac epoch seconds) at 92/96 -- which is why this is generated
at package time rather than checked in: zeroed dates display as 1904.

Usage: make_info.py <mac-name> <TYPE> <CREATOR> <output.info>
"""
import struct
import sys
import time

MAC_EPOCH_DELTA = 2082844800  # 1904-01-01 -> 1970-01-01


def main():
    if len(sys.argv) != 5:
        print(f"Usage: {sys.argv[0]} <mac-name> <TYPE> <CREATOR>"
              " <output.info>", file=sys.stderr)
        sys.exit(1)
    name, ftype, creator, out = sys.argv[1:5]
    name_b = name.encode('mac_roman')
    if len(name_b) > 31 or len(ftype) != 4 or len(creator) != 4:
        print("Error: name must be <= 31 chars; type/creator exactly 4",
              file=sys.stderr)
        sys.exit(1)
    now = int(time.time()) + MAC_EPOCH_DELTA
    info = (b'\x00\x00'
            + bytes([len(name_b)]) + name_b.ljust(63, b'\x00')
            + ftype.encode('mac_roman') + creator.encode('mac_roman')
            + b'\x00' * 18                      # flags + reserved + lengths
            + struct.pack('>II', now, now))     # creation, modification
    with open(out, 'wb') as f:
        f.write(info)


if __name__ == '__main__':
    main()
