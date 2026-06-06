#!/usr/bin/env python3
"""Wrap a bare HFS volume in an Apple Partition Map SCSI disk image.

Default layout (no driver):
- Block 0: DDM with signature 0x4552, driver count=0
- Block 1: APM entry for HFS partition (first entry!)
- Block 2: APM entry for partition map itself
- Block 3: APM entry for free space (Apple_Free)
- Block 96+: HFS volume

This is enough for BlueSCSI and real hardware whose disk already has a
driver, but classic Mac OS will NOT auto-mount such an image as a plain
emulated SCSI disk (e.g. in QEMU): the ROM loads the mounting driver
from the disk itself.

--driver-from <donor.img> fixes that by cloning the donor's entire
header region (Driver Descriptor Record, partition map, and the
Apple_Driver43 partition) VERBATIM, then patching only the sizes.
Reconstructing the structures field-by-field is not enough -- the
driver checks details like the automount bits in pmPartStatus -- so the
donor's bytes are taken as-is.  Any formatted Mac disk image (e.g. a
QEMU boot disk) works as a donor.  The driver is Apple/LaCie-licensed
code: point at a local image, never check one in.

Usage: make_scsi_image2.py <hfs_image> <output.img> [--driver-from <donor>]
"""
import struct
import sys
import os

BLOCK = 512


def read_apm_entries(f):
    """Return [(index, type, start, count)] from an APM image."""
    entries = []
    blk = 1
    while True:
        f.seek(blk * BLOCK)
        e = f.read(BLOCK)
        if len(e) < BLOCK or e[0:2] != b'PM':
            break
        map_cnt, = struct.unpack('>I', e[4:8])
        start, cnt = struct.unpack('>II', e[8:16])
        ptype = e[48:80].rstrip(b'\0').decode('ascii', 'replace')
        entries.append((blk, ptype, start, cnt))
        if blk >= map_cnt:
            break
        blk += 1
    return entries


def build_from_donor(hfs_data, out_path, donor_path):
    hfs_blocks = len(hfs_data) // BLOCK

    with open(donor_path, 'rb') as f:
        entries = read_apm_entries(f)
        hfs_entries = [e for e in entries if e[1] == 'Apple_HFS']
        if not hfs_entries:
            print(f"Error: no Apple_HFS partition in donor {donor_path}",
                  file=sys.stderr)
            sys.exit(1)
        blk, _, hfs_start, _ = hfs_entries[0]
        if not any(e[1].startswith('Apple_Driver') for e in entries):
            print(f"Warning: donor {donor_path} has no driver partition;"
                  " the image will not auto-mount", file=sys.stderr)
        f.seek(0)
        header = bytearray(f.read(hfs_start * BLOCK))

    free_cnt = 32
    free_start = hfs_start + hfs_blocks
    total_blocks = free_start + free_cnt

    # DDR: total block count
    struct.pack_into('>I', header, 4, total_blocks)
    # HFS entry: partition and data block counts
    struct.pack_into('>I', header, blk * BLOCK + 12, hfs_blocks)
    struct.pack_into('>I', header, blk * BLOCK + 84, hfs_blocks)
    # Free entry (if the donor has one): new start and counts
    for fblk, ptype, _, _ in entries:
        if ptype == 'Apple_Free':
            struct.pack_into('>I', header, fblk * BLOCK + 8, free_start)
            struct.pack_into('>I', header, fblk * BLOCK + 12, free_cnt)
            struct.pack_into('>I', header, fblk * BLOCK + 84, free_cnt)
            break

    with open(out_path, 'wb') as f:
        f.write(header)
        f.write(hfs_data)
        f.write(b'\0' * (free_cnt * BLOCK))
    print(f"Driver header cloned from {donor_path} "
          f"({hfs_start} blocks incl. driver)")
    return hfs_start, total_blocks


def build_driverless(hfs_data, out_path):
    hfs_blocks = len(hfs_data) // BLOCK
    hfs_start = 96  # match working image
    total_blocks = hfs_start + hfs_blocks
    # Round up to at least 8MB
    min_blocks = 8 * 1024 * 1024 // BLOCK  # SCSI emulators reject tiny images
    if total_blocks < min_blocks:
        total_blocks = min_blocks

    num_apm_entries = 3

    # === Block 0: DDM ===
    ddm = bytearray(BLOCK)
    struct.pack_into('>H', ddm, 0, 0x4552)        # sbSig
    struct.pack_into('>H', ddm, 2, BLOCK)         # sbBlkSize
    struct.pack_into('>I', ddm, 4, total_blocks)  # sbBlkCount
    struct.pack_into('>H', ddm, 8, 0x0001)        # sbDevType
    struct.pack_into('>H', ddm, 10, 0x0001)       # sbDevId
    struct.pack_into('>I', ddm, 12, 0)            # sbData
    struct.pack_into('>H', ddm, 16, 0)            # sbDrvrCount = 0

    # === Block 1: APM entry - HFS partition ===
    apm1 = bytearray(BLOCK)
    struct.pack_into('>H', apm1, 0, 0x504D)                 # pmSig
    struct.pack_into('>I', apm1, 4, num_apm_entries)        # pmMapEntries
    struct.pack_into('>I', apm1, 8, hfs_start)              # pmPyPartStart
    struct.pack_into('>I', apm1, 12, hfs_blocks)            # pmPartBlkCnt
    apm1[16:16+len(b'MacOS')] = b'MacOS'                    # pmPartName
    apm1[48:48+len(b'Apple_HFS')] = b'Apple_HFS'            # pmParType
    struct.pack_into('>I', apm1, 80, 0)                     # pmLgDataStart
    struct.pack_into('>I', apm1, 84, hfs_blocks)            # pmDataCnt
    struct.pack_into('>I', apm1, 88, 0x40000033)            # pmPartStatus

    # === Block 2: APM entry - partition map itself ===
    apm2 = bytearray(BLOCK)
    struct.pack_into('>H', apm2, 0, 0x504D)
    struct.pack_into('>I', apm2, 4, num_apm_entries)
    struct.pack_into('>I', apm2, 8, 1)                      # starts at block 1
    struct.pack_into('>I', apm2, 12, hfs_start - 1)
    apm2[16:16+len(b'Apple')] = b'Apple'
    apm2[48:48+len(b'Apple_partition_map')] = b'Apple_partition_map'
    struct.pack_into('>I', apm2, 80, 0)
    struct.pack_into('>I', apm2, 84, hfs_start - 1)
    struct.pack_into('>I', apm2, 88, 0x00000037)            # valid, alloc, readable

    # === Block 3: APM entry - free space (padding) ===
    apm3 = bytearray(BLOCK)
    struct.pack_into('>H', apm3, 0, 0x504D)
    struct.pack_into('>I', apm3, 4, num_apm_entries)
    free_start = hfs_start + hfs_blocks
    free_count = total_blocks - free_start
    if free_count > 0:
        struct.pack_into('>I', apm3, 8, free_start)
        struct.pack_into('>I', apm3, 12, free_count)
        apm3[16:16+len(b'Extra')] = b'Extra'
        apm3[48:48+len(b'Apple_Free')] = b'Apple_Free'
        struct.pack_into('>I', apm3, 80, 0)
        struct.pack_into('>I', apm3, 84, free_count)
        struct.pack_into('>I', apm3, 88, 0x00000000)
    else:
        struct.pack_into('>I', apm3, 8, total_blocks)
        struct.pack_into('>I', apm3, 12, 0)
        apm3[48:48+len(b'Apple_Free')] = b'Apple_Free'

    with open(out_path, 'wb') as f:
        f.write(ddm)                                   # block 0
        f.write(apm1)                                  # block 1
        f.write(apm2)                                  # block 2
        f.write(apm3)                                  # block 3
        f.write(b'\x00' * ((hfs_start - 4) * BLOCK))   # pad to block 96
        f.write(hfs_data)                              # HFS volume
        written = hfs_start * BLOCK + len(hfs_data)
        remaining = total_blocks * BLOCK - written
        if remaining > 0:
            f.write(b'\x00' * remaining)
    return hfs_start, total_blocks


def main():
    args = sys.argv[1:]
    donor = None
    if '--driver-from' in args:
        i = args.index('--driver-from')
        donor = args[i + 1]
        del args[i:i + 2]
    if len(args) != 2:
        print(f"Usage: {sys.argv[0]} <hfs_image> <output.img>"
              " [--driver-from <donor.img>]", file=sys.stderr)
        sys.exit(1)
    hfs_path, out_path = args

    with open(hfs_path, 'rb') as f:
        hfs_data = f.read()

    if len(hfs_data) % BLOCK != 0:
        print(f"Error: HFS image size {len(hfs_data)} is not a multiple of"
              f" {BLOCK}", file=sys.stderr)
        sys.exit(1)

    if donor:
        hfs_start, total_blocks = build_from_donor(hfs_data, out_path, donor)
    else:
        hfs_start, total_blocks = build_driverless(hfs_data, out_path)

    sz = os.path.getsize(out_path)
    print(f"HFS partition: block {hfs_start}, {len(hfs_data) // BLOCK} blocks")
    print(f"Total: {total_blocks} blocks ({sz // 1024}K)")
    print(f"Wrote: {out_path}")


if __name__ == '__main__':
    main()
