#!/usr/bin/env python3
"""Dump resource types and IDs from a Mac resource fork."""
import struct
import sys

def dump_rsrc(path):
    with open(path, 'rb') as f:
        data = f.read()

    if len(data) < 16:
        print("File too small for resource fork")
        return

    # Resource fork header
    data_offset = struct.unpack('>I', data[0:4])[0]
    map_offset = struct.unpack('>I', data[4:8])[0]
    data_length = struct.unpack('>I', data[8:12])[0]
    map_length = struct.unpack('>I', data[12:16])[0]

    print(f"Data offset: {data_offset}, length: {data_length}")
    print(f"Map offset:  {map_offset}, length: {map_length}")

    if map_offset + 28 > len(data):
        print("Map extends beyond file")
        return

    # Resource map
    map_start = map_offset
    type_list_offset = struct.unpack('>H', data[map_start+24:map_start+26])[0]
    name_list_offset = struct.unpack('>H', data[map_start+26:map_start+28])[0]

    type_list_start = map_start + type_list_offset
    num_types = struct.unpack('>H', data[type_list_start:type_list_start+2])[0] + 1

    print(f"\n{num_types} resource types:")
    print(f"{'Type':>6}  {'Count':>5}  {'IDs'}")
    print(f"{'----':>6}  {'-----':>5}  {'---'}")

    pos = type_list_start + 2
    for i in range(num_types):
        rtype = data[pos:pos+4].decode('mac_roman', errors='replace')
        count = struct.unpack('>H', data[pos+4:pos+6])[0] + 1
        ref_offset = struct.unpack('>H', data[pos+6:pos+8])[0]

        # Read resource IDs
        ref_start = type_list_start + ref_offset
        ids = []
        for j in range(count):
            rid = struct.unpack('>h', data[ref_start + j*12:ref_start + j*12 + 2])[0]
            ids.append(rid)

        ids_str = ', '.join(str(x) for x in sorted(ids))
        if len(ids_str) > 60:
            ids_str = ids_str[:57] + '...'
        print(f"  {rtype!r:>6}  {count:5d}  {ids_str}")
        pos += 8

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <resource-fork-file>", file=sys.stderr)
        sys.exit(1)
    try:
        dump_rsrc(sys.argv[1])
    except (struct.error, IndexError):
        print("Error: truncated or corrupt resource fork", file=sys.stderr)
        sys.exit(1)
