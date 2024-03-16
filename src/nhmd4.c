/* NetHack 3.7	nhmd4.c	$NHDT-Date: 1708811387 2024/02/24 21:49:47 $	$NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.0 $ */
/*-Copyright (c) Kenneth Lorber, Kensington, Maryland, 2024 */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * Usage is to try to match traceback data with the instance of the
 * program which produced that, not for security related purposes.
 *
 * Derived from:
 */
/*
 * MD4 (RFC-1320) message digest.
 * Modified from MD5 code by Andrey Panin <pazke@donpac.ru>
 *
 * Written by Solar Designer <solar@openwall.com> in 2001, and placed in
 * the public domain.  There's absolutely no warranty.
 *
 * This differs from Colin Plumb's older public domain implementation in
 * that no 32-bit integer data type is required, there's no compile-time
 * endianness configuration, and the function prototypes match OpenSSL's.
 * The primary goals are portability and ease of use.
 *
 * This implementation is meant to be fast, but not as fast as possible.
 * Some known optimizations are not included to reduce source code size
 * and avoid compile-time configuration.
 */
#include "hack.h"
#ifdef CRASHREPORT

#include "nhmd4.h"

staticfn const unsigned char *nhmd4_body(struct nhmd4_context *,
                                 const unsigned char *, size_t);

/* Avoid a conflict from a Lua header */
#ifdef G
#undef G
#endif

/*
 * The basic MD4 functions.
 */
#define F(x, y, z)      ((z) ^ ((x) & ((y) ^ (z))))
#define G(x, y, z)      (((x) & (y)) | ((x) & (z)) | ((y) & (z)))
#define H(x, y, z)      ((x) ^ (y) ^ (z))

/*
 * STEP: the MD4 transformation used for all four rounds.
 * (Joining two expressions with the comma operator provides a sequence
 * point.  C89/C90 and later guarantee that the first will be fully complete
 * before the second starts, making both assignments to 'a' be well defined.)
 */
#define STEP(f, a, b, c, d, x, s) \
    (((a) += f((b), (c), (d)) + (x)),                   \
     ((a) = ((a) << (s)) | ((a) >> (32 - (s)))))

/*
 * SET reads 4 input bytes in little-endian byte order and stores them
 * in a properly aligned word in host byte order.
 *
 * The check for little-endian architectures which tolerate unaligned
 * memory accesses is just an optimization.  Nothing will break if it
 * doesn't work.
 */
#if defined(__i386__) || defined(__x86_64__)
#define SET(n) (*(const quint32 *) &ptr[(n) * 4])
#define GET(n) SET(n)
#else
#define SET(n) \
    (ctx->block[(n)] = \
     ((quint32) ptr[(n) * 4]                          \
      | ((quint32) ptr[(n) * 4 + 1] << 8)             \
      | ((quint32) ptr[(n) * 4 + 2] << 16)            \
      | ((quint32) ptr[(n) * 4 + 3] << 24)))
#define GET(n) (ctx->block[(n)])
#endif

/*
 * This processes one or more 64-byte data blocks, but does NOT update
 * the bit counters.  There're no alignment requirements.
 */
staticfn const unsigned char *
nhmd4_body(
    struct nhmd4_context *ctx,
    const unsigned char *data,
    size_t size)
{
    const unsigned char *ptr;
    quint32 a, b, c, d;
    quint32 saved_a, saved_b, saved_c, saved_d;

    ptr = data;

    a = ctx->a;
    b = ctx->b;
    c = ctx->c;
    d = ctx->d;

    do {
        saved_a = a;
        saved_b = b;
        saved_c = c;
        saved_d = d;

/* Round 1 */
        STEP(F, a, b, c, d, SET( 0),  3);
        STEP(F, d, a, b, c, SET( 1),  7);
        STEP(F, c, d, a, b, SET( 2), 11);
        STEP(F, b, c, d, a, SET( 3), 19);

        STEP(F, a, b, c, d, SET( 4),  3);
        STEP(F, d, a, b, c, SET( 5),  7);
        STEP(F, c, d, a, b, SET( 6), 11);
        STEP(F, b, c, d, a, SET( 7), 19);

        STEP(F, a, b, c, d, SET( 8),  3);
        STEP(F, d, a, b, c, SET( 9),  7);
        STEP(F, c, d, a, b, SET(10), 11);
        STEP(F, b, c, d, a, SET(11), 19);

        STEP(F, a, b, c, d, SET(12),  3);
        STEP(F, d, a, b, c, SET(13),  7);
        STEP(F, c, d, a, b, SET(14), 11);
        STEP(F, b, c, d, a, SET(15), 19);
/* Round 2 */
        STEP(G, a, b, c, d, GET( 0) + 0x5A827999,  3);
        STEP(G, d, a, b, c, GET( 4) + 0x5A827999,  5);
        STEP(G, c, d, a, b, GET( 8) + 0x5A827999,  9);
        STEP(G, b, c, d, a, GET(12) + 0x5A827999, 13);

        STEP(G, a, b, c, d, GET( 1) + 0x5A827999,  3);
        STEP(G, d, a, b, c, GET( 5) + 0x5A827999,  5);
        STEP(G, c, d, a, b, GET( 9) + 0x5A827999,  9);
        STEP(G, b, c, d, a, GET(13) + 0x5A827999, 13);

        STEP(G, a, b, c, d, GET( 2) + 0x5A827999,  3);
        STEP(G, d, a, b, c, GET( 6) + 0x5A827999,  5);
        STEP(G, c, d, a, b, GET(10) + 0x5A827999,  9);
        STEP(G, b, c, d, a, GET(14) + 0x5A827999, 13);

        STEP(G, a, b, c, d, GET( 3) + 0x5A827999,  3);
        STEP(G, d, a, b, c, GET( 7) + 0x5A827999,  5);
        STEP(G, c, d, a, b, GET(11) + 0x5A827999,  9);
        STEP(G, b, c, d, a, GET(15) + 0x5A827999, 13);
/* Round 3 */
        STEP(H, a, b, c, d, GET( 0) + 0x6ED9EBA1,  3);
        STEP(H, d, a, b, c, GET( 8) + 0x6ED9EBA1,  9);
        STEP(H, c, d, a, b, GET( 4) + 0x6ED9EBA1, 11);
        STEP(H, b, c, d, a, GET(12) + 0x6ED9EBA1, 15);

        STEP(H, a, b, c, d, GET( 2) + 0x6ED9EBA1,  3);
        STEP(H, d, a, b, c, GET(10) + 0x6ED9EBA1,  9);
        STEP(H, c, d, a, b, GET( 6) + 0x6ED9EBA1, 11);
        STEP(H, b, c, d, a, GET(14) + 0x6ED9EBA1, 15);

        STEP(H, a, b, c, d, GET( 1) + 0x6ED9EBA1,  3);
        STEP(H, d, a, b, c, GET( 9) + 0x6ED9EBA1,  9);
        STEP(H, c, d, a, b, GET( 5) + 0x6ED9EBA1, 11);
        STEP(H, b, c, d, a, GET(13) + 0x6ED9EBA1, 15);

        STEP(H, a, b, c, d, GET( 3) + 0x6ED9EBA1,  3);
        STEP(H, d, a, b, c, GET(11) + 0x6ED9EBA1,  9);
        STEP(H, c, d, a, b, GET( 7) + 0x6ED9EBA1, 11);
        STEP(H, b, c, d, a, GET(15) + 0x6ED9EBA1, 15);

        a += saved_a;
        b += saved_b;
        c += saved_c;
        d += saved_d;

        ptr += 64;
    } while (size -= 64);

    ctx->a = a;
    ctx->b = b;
    ctx->c = c;
    ctx->d = d;

    return ptr;
}

void
nhmd4_init(
    struct nhmd4_context *ctx)
{
    ctx->a = 0x67452301;
    ctx->b = 0xefcdab89;
    ctx->c = 0x98badcfe;
    ctx->d = 0x10325476;

    ctx->lo = 0;
    ctx->hi = 0;
}

void
nhmd4_update(
    struct nhmd4_context *ctx,
    const unsigned char *data,
    size_t size)
{
    /* @UNSAFE */
    quint32 saved_lo;
    unsigned long used, free;

    saved_lo = ctx->lo;
    if ((ctx->lo = (saved_lo + size) & 0x1fffffff) < saved_lo)
        ctx->hi++;
    ctx->hi += (quint32)(size >> 29);

    used = saved_lo & 0x3f;

    if (used) {
        free = 64 - used;

        if (size < free) {
            memcpy(&ctx->buffer[used], data, size);
            return;
        }

        memcpy(&ctx->buffer[used], data, free);
        data = (const unsigned char *) data + free;
        size -= free;
        nhmd4_body(ctx, ctx->buffer, 64);
    }

    if (size >= 64) {
        data = nhmd4_body(ctx, data, size & ~0x3fUL);
        size &= 0x3fUL;
    }

    memcpy(ctx->buffer, data, size);
}

void
nhmd4_final(
    struct nhmd4_context *ctx,
    unsigned char result[NHMD4_RESULTLEN])
{
    /* @UNSAFE */
    unsigned long used, free;

    used = ctx->lo & 0x3fUL;

    ctx->buffer[used++] = 0x80;

    free = 64 - used;

    if (free < 8) {
        memset(&ctx->buffer[used], 0, free);
        nhmd4_body(ctx, ctx->buffer, 64);
        used = 0;
        free = 64;
    }

    memset(&ctx->buffer[used], 0, free - 8);

    ctx->lo <<= 3;
    ctx->buffer[56] = ctx->lo;
    ctx->buffer[57] = ctx->lo >> 8;
    ctx->buffer[58] = ctx->lo >> 16;
    ctx->buffer[59] = ctx->lo >> 24;
    ctx->buffer[60] = ctx->hi;
    ctx->buffer[61] = ctx->hi >> 8;
    ctx->buffer[62] = ctx->hi >> 16;
    ctx->buffer[63] = ctx->hi >> 24;

    nhmd4_body(ctx, ctx->buffer, 64);

    result[0] = ctx->a;
    result[1] = ctx->a >> 8;
    result[2] = ctx->a >> 16;
    result[3] = ctx->a >> 24;
    result[4] = ctx->b;
    result[5] = ctx->b >> 8;
    result[6] = ctx->b >> 16;
    result[7] = ctx->b >> 24;
    result[8] = ctx->c;
    result[9] = ctx->c >> 8;
    result[10] = ctx->c >> 16;
    result[11] = ctx->c >> 24;
    result[12] = ctx->d;
    result[13] = ctx->d >> 8;
    result[14] = ctx->d >> 16;
    result[15] = ctx->d >> 24;

    memset(ctx, 0, sizeof *ctx);
}

#undef F
#undef G
#undef H
#undef STEP
#undef SET
#undef GET

#endif /* CRASHREPORT */

/*nhmd4.c*/
