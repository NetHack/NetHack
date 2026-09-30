#!/usr/bin/env python3
"""Convert sound/wav/*.uu (uuencoded WAV) into classic Mac 'snd ' resources.

Emits a raw resource fork containing one named 'snd ' (format 1,
sampledSynth, bufferCmd, stdSH) resource per input file; the resource
name is the file's basename without .uu (e.g. "sound_Bugle_A",
"se_squeak_D_flat", "sa2_xplevelup"), which is what
sound/mac68ksound/mac68ksound.c looks up via GetNamedResource.

Audio is downmixed to mono, converted to 8-bit unsigned, and resampled
down to at most MAX_RATE, the ceiling the target sound hardware plays.

Usage: wav2snd.py <wav-dir> <output.rsrc>
"""
import binascii
import os
import struct
import sys

MAX_RATE = 22050
BASE_RES_ID = 9000


def uudecode(path):
    out = []
    started = False
    with open(path, 'rb') as f:
        for line in f.read().splitlines():
            if not started:
                if line.startswith(b'begin'):
                    started = True
                continue
            if line.strip() == b'end' or line == b'`' or not line:
                break
            out.append(binascii.a2b_uu(line))
    return b''.join(out)


def parse_wav(data):
    """Return (rate, samples) with samples as a list of mono 8-bit
    unsigned ints. Handles PCM 8/16-bit, any channel count."""
    if data[:4] != b'RIFF' or data[8:12] != b'WAVE':
        raise ValueError('not a RIFF/WAVE file')
    fmt = None
    raw = None
    i = 12
    while i + 8 <= len(data):
        cid = data[i:i + 4]
        sz = struct.unpack('<I', data[i + 4:i + 8])[0]
        if cid == b'fmt ':
            fmt = struct.unpack('<HHIIHH', data[i + 8:i + 24])
        elif cid == b'data':
            raw = data[i + 8:i + 8 + sz]
        i += 8 + sz + (sz & 1)
    if fmt is None or raw is None:
        raise ValueError('missing fmt or data chunk')
    codec, channels, rate, _, _, bits = fmt
    if codec != 1 or bits not in (8, 16):
        raise ValueError('unsupported codec/bits: %d/%d' % (codec, bits))

    samples = []
    if bits == 16:
        n = len(raw) // (2 * channels) * channels
        ints = struct.unpack('<%dh' % n, raw[:n * 2])
        for j in range(0, n, channels):
            v = sum(ints[j:j + channels]) // channels
            samples.append((v >> 8) + 128)
    else:  # 8-bit WAV is already unsigned
        n = len(raw) // channels * channels
        for j in range(0, n, channels):
            v = sum(raw[j:j + channels]) // channels
            samples.append(v)
    return rate, samples


def resample(samples, src_rate, dst_rate):
    """Linear interpolation, no anti-alias filter: decimating below the
    source Nyquist folds high frequencies back into the band."""
    if src_rate <= dst_rate or not samples:
        return samples, src_rate
    ratio = src_rate / dst_rate
    count = int(len(samples) / ratio)
    out = []
    for j in range(count):
        pos = j * ratio
        k = int(pos)
        frac = pos - k
        a = samples[k]
        b = samples[k + 1] if k + 1 < len(samples) else a
        out.append(int(a + (b - a) * frac))
    return out, dst_rate


def make_snd(rate, samples):
    """format-1 'snd ' resource: sampledSynth/initMono, one bufferCmd
    pointing at a standard SoundHeader followed by the samples."""
    sound_header = struct.pack(
        '>IIIIIBB',
        0,                  # samplePtr: samples follow the header
        len(samples),       # length
        (rate << 16) & 0xffffffff,  # sampleRate, 16.16 Fixed
        0,                  # loopStart
        0,                  # loopEnd
        0,                  # encode: stdSH
        60)                 # baseFrequency: middle C
    return struct.pack(
        '>HHHIHHhi',
        1,                  # format 1
        1,                  # one modifier
        5,                  # sampledSynth
        0x00000080,         # initMono
        1,                  # one command
        0x8051,             # bufferCmd | dataOffsetFlag
        0,                  # param1
        20                  # param2: SoundHeader offset in resource
        ) + sound_header + bytes(samples)


def build_rsrc(resources):
    """resources: list of (name, data). Emit a raw resource fork with
    one 'snd ' type."""
    data_blob = b''
    entries = []  # (name, data_offset)
    for name, data in resources:
        entries.append((name, len(data_blob)))
        data_blob += struct.pack('>I', len(data)) + data

    names_blob = b''
    refs_blob = b''
    for i, (name, doff) in enumerate(entries):
        noff = len(names_blob)
        if noff > 0x7fff:
            raise ValueError('name list too long for 16-bit offsets')
        nm = name.encode('mac_roman')
        names_blob += struct.pack('>B', len(nm)) + nm
        refs_blob += struct.pack('>hh', BASE_RES_ID + i, noff)
        refs_blob += struct.pack('>B', 0)          # attributes
        refs_blob += doff.to_bytes(3, 'big')       # 24-bit data offset
        refs_blob += struct.pack('>I', 0)          # handle placeholder

    # type list: count-1, then one entry for 'snd '
    type_list = struct.pack('>H', 0)
    type_list += b'snd ' + struct.pack('>HH', len(entries) - 1, 2 + 8)
    type_list += refs_blob

    map_header = bytes(16 + 4 + 2 + 2)  # header copy, handle, fileref, attrs
    type_list_off = len(map_header) + 4  # == 28
    name_list_off = type_list_off + len(type_list)
    rmap = (map_header
            + struct.pack('>HH', type_list_off, name_list_off)
            + type_list + names_blob)

    data_off = 256
    header = struct.pack('>IIII', data_off, data_off + len(data_blob),
                         len(data_blob), len(rmap))
    return header + bytes(data_off - 16) + data_blob + rmap


def main():
    if len(sys.argv) != 3:
        sys.exit('usage: wav2snd.py <wav-dir> <output.rsrc>')
    wavdir, outpath = sys.argv[1], sys.argv[2]

    resources = []
    total_in = 0
    skipped = 0
    for fn in sorted(os.listdir(wavdir)):
        if not fn.endswith('.uu'):
            continue
        # one unreadable wav must not fail the whole packaging step
        try:
            wav = uudecode(os.path.join(wavdir, fn))
            rate, samples = parse_wav(wav)
        except (ValueError, binascii.Error) as e:
            print('wav2snd: skipping %s: %s' % (fn, e), file=sys.stderr)
            skipped += 1
            continue
        samples, rate = resample(samples, rate, MAX_RATE)
        resources.append((fn[:-3], make_snd(rate, samples)))
        total_in += len(wav)

    if not resources:
        sys.exit('wav2snd: no usable .uu files in %s' % wavdir)
    fork = build_rsrc(resources)
    with open(outpath, 'wb') as f:
        f.write(fork)
    print('wav2snd: %d sounds%s, %d KB wav -> %d KB snd (%s)'
          % (len(resources), ', %d skipped' % skipped if skipped else '',
             total_in // 1024, len(fork) // 1024, outpath))


if __name__ == '__main__':
    main()
