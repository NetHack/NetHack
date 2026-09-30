#!/usr/bin/env python3
"""Remove all resources of the given type(s) from a Mac resource fork.

The output file is rebuilt from scratch (data area, map, type list,
reference lists, name list), preserving the order, IDs, names and
attribute bytes of every surviving resource."""

import struct
import sys


def parse_rsrc(data):
    """Parse a resource fork; return (fork_attrs, ordered list of
    (type, [(id, name|None, attr, bytes), ...]))."""
    data_offset, map_offset = struct.unpack('>II', data[0:8])
    fork_attrs = struct.unpack('>H', data[map_offset+22:map_offset+24])[0]
    type_list_offset, name_list_offset = \
        struct.unpack('>HH', data[map_offset+24:map_offset+28])
    tl = map_offset + type_list_offset
    nl = map_offset + name_list_offset
    num_types = struct.unpack('>H', data[tl:tl+2])[0] + 1

    types = []
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
        types.append((rtype, entries))
        pos += 8
    return fork_attrs, types


def build_rsrc(fork_attrs, types):
    """Serialize (fork_attrs, [(type, entries)]) back into a resource fork."""
    # Data area: 4-byte length + payload per resource, in map order.
    data_area = bytearray()
    data_offsets = {}
    for rtype, entries in types:
        for rid, name, attr, blob in entries:
            data_offsets[(rtype, rid)] = len(data_area)
            data_area += struct.pack('>I', len(blob)) + blob

    # Name list / reference lists / type list.
    name_list = bytearray()
    type_list = bytearray(struct.pack('>H', len(types) - 1))
    ref_lists = bytearray()
    ref_lists_base = 2 + 8 * len(types)   # from start of type list
    for rtype, entries in types:
        type_list += rtype
        type_list += struct.pack('>HH', len(entries) - 1,
                                 ref_lists_base + len(ref_lists))
        for rid, name, attr, blob in entries:
            if name is None:
                name_off = -1
            else:
                name_off = len(name_list)
                name_list += bytes([len(name)]) + name
            doff = data_offsets[(rtype, rid)]
            ref_lists += struct.pack('>hh', rid, name_off)
            ref_lists += bytes([attr]) + doff.to_bytes(4, 'big')[1:]
            ref_lists += b'\0\0\0\0'

    # Map: 16-byte header copy (zero) + handle/file-ref (zero) + attrs
    # + type-list / name-list offsets, then the lists themselves.
    type_list_offset = 28
    name_list_offset = type_list_offset + len(type_list) + len(ref_lists)
    rmap = bytearray(22)
    rmap += struct.pack('>HHH', fork_attrs, type_list_offset,
                        name_list_offset)
    rmap += type_list + ref_lists + name_list

    data_offset = 256
    map_offset = data_offset + len(data_area)
    header = struct.pack('>IIII', data_offset, map_offset,
                         len(data_area), len(rmap))
    return header + bytes(240) + bytes(data_area) + bytes(rmap)


def main():
    if len(sys.argv) < 4:
        print(f"Usage: {sys.argv[0]} <in.rsrc> <out.rsrc> <TYPE> [TYPE...]",
              file=sys.stderr)
        sys.exit(1)

    in_path, out_path = sys.argv[1], sys.argv[2]
    for t in sys.argv[3:]:
        if len(t.encode('mac_roman')) != 4:
            print(f"Error: resource type {t!r} is not 4 characters",
                  file=sys.stderr)
            sys.exit(1)
    strip = {t.encode('mac_roman') for t in sys.argv[3:]}

    with open(in_path, 'rb') as f:
        data = f.read()
    fork_attrs, types = parse_rsrc(data)

    present = {rtype for rtype, _ in types}
    missing = strip - present
    if missing:
        print("Error: type(s) not in file: "
              + ', '.join(sorted(t.decode('mac_roman') for t in missing)),
              file=sys.stderr)
        sys.exit(1)

    kept = [(rtype, entries) for rtype, entries in types
            if rtype not in strip]
    removed = sum(len(entries) for rtype, entries in types
                  if rtype in strip)

    out = build_rsrc(fork_attrs, kept)
    with open(out_path, 'wb') as f:
        f.write(out)
    print(f"Removed: {removed} resources"
          f" ({', '.join(sorted(t.decode('mac_roman') for t in strip))})")
    print(f"Wrote:   {out_path} ({len(out)} bytes,"
          f" {sum(len(e) for _, e in kept)} resources,"
          f" {len(kept)} types)")


if __name__ == '__main__':
    main()
