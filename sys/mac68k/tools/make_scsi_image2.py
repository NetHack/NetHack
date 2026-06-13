#!/usr/bin/env python3
"""Wrap a bare HFS volume in an Apple Partition Map SCSI disk image.

With --driver <driver.bin> (the packaged default, sys/mac68k/scsidriver.bin)
the image embeds the port's own MIT-licensed SCSI disk driver in an
Apple_Driver43 partition, so classic Mac OS auto-mounts the disk from the
ROM on real hardware and in emulators (QEMU q800) alike.  The layout and
every ROM-checked field replicate the boot contract reverse-engineered in
the mac-scsi-driver project (see its README, "The boot contract"):

- Block 0: Driver Descriptor Record ('ER'), sbDrvrCount=1,
  DDMap = { ddBlock=64, ddSize=ceil(len/512), ddType=1 }
- Block 1: APM entry, Apple_partition_map  (blocks 1..63)
- Block 2: APM entry, Apple_Driver43       (blocks 64..95) with
  pmBootSize = driver length, pmBootCksum = rotate-left byte sum
  (MANDATORY: without it the ROM silently skips the driver),
  pmProcessor = "68000", and the donor-convention pmPad patch list
- Block 3: APM entry, Apple_HFS            (block 96+)
- Block 4: APM entry, Apple_Free           (tail padding)
- Block 64: the driver blob
- Block 96: the HFS volume

Without --driver the historic driverless layout is written (DDM with
sbDrvrCount=0; HFS entry first).  That is enough for BlueSCSI and real
disks that already carry a driver, but plain emulated SCSI disks will
not auto-mount it.

Usage: make_scsi_image2.py <hfs_image> <output.img> [--driver <driver.bin>]
"""
import struct
import sys
import os

BLOCK = 512

# Apple_Driver43 partition geometry (donor convention; the install-tested
# layout of the mac-scsi-driver project)
DRIVER_START = 64
DRIVER_BLOCKS = 32
HFS_START = 96

# SCSI emulators reject tiny images; 8 MB floor
MIN_BLOCKS = 8 * 1024 * 1024 // BLOCK

# pmPad tail of the Apple_Driver43 map entry (offset 136), cloned from the
# working donor-convention image: Apple's driver patch-descriptor list.
DRIVER_PMPAD = bytes.fromhex('000106000000000000000001000700000001')


def boot_cksum(data):
    """Apple driver-partition boot checksum: byte sum, rotate left 1 each."""
    c = 0
    for b in data:
        c = (c + b) & 0xFFFF
        c = ((c << 1) | (c >> 15)) & 0xFFFF
    return c


def pm_entry(map_entries, start, count, name, ptype, status):
    e = bytearray(BLOCK)
    struct.pack_into('>H', e, 0, 0x504D)          # pmSig
    struct.pack_into('>I', e, 4, map_entries)     # pmMapBlkCnt
    struct.pack_into('>I', e, 8, start)           # pmPyPartStart
    struct.pack_into('>I', e, 12, count)          # pmPartBlkCnt
    e[16:16 + len(name)] = name                   # pmPartName
    e[48:48 + len(ptype)] = ptype                 # pmParType
    struct.pack_into('>I', e, 80, 0)              # pmLgDataStart
    struct.pack_into('>I', e, 84, count)          # pmDataCnt
    struct.pack_into('>I', e, 88, status)         # pmPartStatus
    return e


def build_with_driver(hfs_data, out_path, driver_path):
    hfs_blocks = len(hfs_data) // BLOCK

    with open(driver_path, 'rb') as f:
        driver = f.read()
    if len(driver) > DRIVER_BLOCKS * BLOCK:
        print(f"Error: driver {driver_path} is {len(driver)} bytes; "
              f"max {DRIVER_BLOCKS * BLOCK}", file=sys.stderr)
        sys.exit(1)

    free_start = HFS_START + hfs_blocks
    total_blocks = max(free_start, MIN_BLOCKS)
    free_count = total_blocks - free_start
    if free_count < 32:  # donor convention: a real tail partition
        free_count = 32
        total_blocks = free_start + free_count

    # === Block 0: Driver Descriptor Record ===
    ddm = bytearray(BLOCK)
    struct.pack_into('>H', ddm, 0, 0x4552)        # sbSig 'ER'
    struct.pack_into('>H', ddm, 2, BLOCK)         # sbBlkSize
    struct.pack_into('>I', ddm, 4, total_blocks)  # sbBlkCount
    struct.pack_into('>H', ddm, 16, 1)            # sbDrvrCount
    struct.pack_into('>I', ddm, 18, DRIVER_START)                   # ddBlock
    struct.pack_into('>H', ddm, 22, (len(driver) + BLOCK - 1) // BLOCK)  # ddSize
    struct.pack_into('>H', ddm, 24, 1)            # ddType

    # === APM entries (map, driver, HFS, free) ===
    n = 4
    apm_map = pm_entry(n, 1, DRIVER_START - 1, b'Apple',
                       b'Apple_partition_map', 0x00000037)

    apm_drv = pm_entry(n, DRIVER_START, DRIVER_BLOCKS, b'Macintosh',
                       b'Apple_Driver43', 0x0000037F)
    struct.pack_into('>I', apm_drv, 96, len(driver))         # pmBootSize
    struct.pack_into('>I', apm_drv, 116, boot_cksum(driver)) # pmBootCksum
    apm_drv[120:125] = b'68000'                              # pmProcessor
    apm_drv[136:136 + len(DRIVER_PMPAD)] = DRIVER_PMPAD

    apm_hfs = pm_entry(n, HFS_START, hfs_blocks, b'MacOS',
                       b'Apple_HFS', 0xC00000B7)

    apm_free = pm_entry(n, free_start, free_count, b'Extra',
                        b'Apple_Free', 0x00000037)

    with open(out_path, 'wb') as f:
        f.write(ddm)
        f.write(apm_map)
        f.write(apm_drv)
        f.write(apm_hfs)
        f.write(apm_free)
        f.write(b'\x00' * ((DRIVER_START - 5) * BLOCK))  # pad to block 64
        f.write(driver)
        f.write(b'\x00' * (DRIVER_BLOCKS * BLOCK - len(driver)))
        f.write(hfs_data)
        if free_count > 0:
            f.write(b'\x00' * (free_count * BLOCK))

    print(f"Driver: block {DRIVER_START}, {len(driver)} bytes, "
          f"cksum {boot_cksum(driver):#06x} ({driver_path})")
    return HFS_START, total_blocks


def build_driverless(hfs_data, out_path):
    hfs_blocks = len(hfs_data) // BLOCK
    hfs_start = HFS_START  # match working image
    total_blocks = max(hfs_start + hfs_blocks, MIN_BLOCKS)

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

    # === Block 1: APM entry - HFS partition (first entry!) ===
    apm1 = pm_entry(num_apm_entries, hfs_start, hfs_blocks, b'MacOS',
                    b'Apple_HFS', 0x40000033)

    # === Block 2: APM entry - partition map itself ===
    apm2 = pm_entry(num_apm_entries, 1, hfs_start - 1, b'Apple',
                    b'Apple_partition_map', 0x00000037)

    # === Block 3: APM entry - free space (padding) ===
    free_start = hfs_start + hfs_blocks
    free_count = total_blocks - free_start
    if free_count > 0:
        apm3 = pm_entry(num_apm_entries, free_start, free_count, b'Extra',
                        b'Apple_Free', 0x00000000)
    else:
        apm3 = pm_entry(num_apm_entries, total_blocks, 0, b'',
                        b'Apple_Free', 0x00000000)

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
    driver = None
    if '--driver' in args:
        i = args.index('--driver')
        driver = args[i + 1]
        del args[i:i + 2]
    if len(args) != 2:
        print(f"Usage: {sys.argv[0]} <hfs_image> <output.img>"
              " [--driver <driver.bin>]", file=sys.stderr)
        sys.exit(1)
    hfs_path, out_path = args

    with open(hfs_path, 'rb') as f:
        hfs_data = f.read()

    if len(hfs_data) % BLOCK != 0:
        print(f"Error: HFS image size {len(hfs_data)} is not a multiple of"
              f" {BLOCK}", file=sys.stderr)
        sys.exit(1)

    if driver:
        hfs_start, total_blocks = build_with_driver(hfs_data, out_path,
                                                    driver)
    else:
        hfs_start, total_blocks = build_driverless(hfs_data, out_path)

    sz = os.path.getsize(out_path)
    print(f"HFS partition: block {hfs_start}, {len(hfs_data) // BLOCK} blocks")
    print(f"Total: {total_blocks} blocks ({sz // 1024}K)")
    print(f"Wrote: {out_path}")

if __name__ == '__main__':
    main()
