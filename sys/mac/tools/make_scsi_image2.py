#!/usr/bin/env python3
"""Create a SCSI disk image matching the structure of a known-working Mac image.

Based on analysis of a working BlueSCSI image:
- Block 0: DDM with signature 0x4552, driver count=0 (no embedded driver)
- Block 1: APM entry for HFS partition (first entry!)
- Block 2: APM entry for partition map itself
- Block 3: APM entry for driver placeholder (Apple_Driver)
- Block 4: APM entry for free space (Apple_Free)
- Block 96+: HFS volume
"""
import struct
import sys
import os

BLOCK = 512

def main():
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} <hfs_image> <output.img>", file=sys.stderr)
        sys.exit(1)
    hfs_path = sys.argv[1]
    out_path = sys.argv[2]

    with open(hfs_path, 'rb') as f:
        hfs_data = f.read()

    if len(hfs_data) % BLOCK != 0:
        print(f"Error: HFS image size {len(hfs_data)} is not a multiple of"
              f" {BLOCK}", file=sys.stderr)
        sys.exit(1)

    hfs_blocks = len(hfs_data) // BLOCK
    hfs_start = 96  # match working image
    total_blocks = hfs_start + hfs_blocks
    # Round up to at least 8MB
    min_blocks = 8 * 1024 * 1024 // BLOCK  # SCSI emulators reject tiny images; 8 MB floor
    if total_blocks < min_blocks:
        total_blocks = min_blocks

    num_apm_entries = 3

    # === Block 0: DDM ===
    ddm = bytearray(BLOCK)
    struct.pack_into('>H', ddm, 0, 0x4552)       # sbSig
    struct.pack_into('>H', ddm, 2, BLOCK)         # sbBlkSize
    struct.pack_into('>I', ddm, 4, total_blocks)  # sbBlkCount
    struct.pack_into('>H', ddm, 8, 0x0001)        # sbDevType
    struct.pack_into('>H', ddm, 10, 0x0001)       # sbDevId
    struct.pack_into('>I', ddm, 12, 0)            # sbData
    struct.pack_into('>H', ddm, 16, 0)            # sbDrvrCount = 0

    # === Block 1: APM entry - HFS partition ===
    apm1 = bytearray(BLOCK)
    struct.pack_into('>H', apm1, 0, 0x504D)                # pmSig
    struct.pack_into('>I', apm1, 4, num_apm_entries)        # pmMapEntries
    struct.pack_into('>I', apm1, 8, hfs_start)              # pmPyPartStart
    struct.pack_into('>I', apm1, 12, hfs_blocks)            # pmPartBlkCnt
    apm1[16:16+len(b'MacOS')] = b'MacOS'                    # pmPartName
    apm1[48:48+len(b'Apple_HFS')] = b'Apple_HFS'            # pmParType
    struct.pack_into('>I', apm1, 80, 0)                      # pmLgDataStart
    struct.pack_into('>I', apm1, 84, hfs_blocks)             # pmDataCnt
    struct.pack_into('>I', apm1, 88, 0x40000033)             # pmPartStatus (valid, allocated, readable, writable, mounted)

    # === Block 2: APM entry - partition map itself ===
    apm2 = bytearray(BLOCK)
    struct.pack_into('>H', apm2, 0, 0x504D)
    struct.pack_into('>I', apm2, 4, num_apm_entries)
    struct.pack_into('>I', apm2, 8, 1)                       # starts at block 1
    struct.pack_into('>I', apm2, 12, hfs_start - 1)          # covers blocks 1 through hfs_start-1
    apm2[16:16+len(b'Apple')] = b'Apple'
    apm2[48:48+len(b'Apple_partition_map')] = b'Apple_partition_map'
    struct.pack_into('>I', apm2, 80, 0)
    struct.pack_into('>I', apm2, 84, hfs_start - 1)
    struct.pack_into('>I', apm2, 88, 0x00000037)             # valid, allocated, readable

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
        # No free space, make it a zero-length entry
        struct.pack_into('>I', apm3, 8, total_blocks)
        struct.pack_into('>I', apm3, 12, 0)
        apm3[48:48+len(b'Apple_Free')] = b'Apple_Free'

    # === Assemble ===
    with open(out_path, 'wb') as f:
        f.write(ddm)                                    # block 0
        f.write(apm1)                                   # block 1
        f.write(apm2)                                   # block 2
        f.write(apm3)                                   # block 3
        f.write(b'\x00' * ((hfs_start - 4) * BLOCK))   # pad to block 96
        f.write(hfs_data)                               # HFS volume
        # Pad to total size
        written = hfs_start * BLOCK + len(hfs_data)
        remaining = total_blocks * BLOCK - written
        if remaining > 0:
            f.write(b'\x00' * remaining)

    sz = os.path.getsize(out_path)
    print(f"HFS partition: block {hfs_start}, {hfs_blocks} blocks")
    print(f"Total: {total_blocks} blocks ({sz // 1024}K)")
    print(f"Wrote: {out_path}")

if __name__ == '__main__':
    main()
