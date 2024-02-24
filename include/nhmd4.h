/* NetHack 3.7	nhmd4.h	$NHDT-Date: 1708811386 2024/02/24 21:49:46 $	$NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.0 $ */
/*- Copyright (c) Kenneth Lorber, Kensington, Maryland, 2024 */
/* NetHack may be freely redistributed.  See license for details. */

// Derived from:
/*
 * This is an OpenSSL-compatible implementation of the RSA Data Security,
 * Inc. MD4 Message-Digest Algorithm.
 *
 * Written by Solar Designer <solar@openwall.com> in 2001, and placed in
 * the public domain.  See md4.c for more information.
 */

#ifndef NHMD4_H
#define NHMD4_H

#define NHMD4_DIGEST_LENGTH 128
#define NHMD4_RESULTLEN (128 / 8) /* 16 */

typedef uint32_t quint32;

struct nhmd4_context {
    quint32 lo, hi;
    quint32 a, b, c, d;
    unsigned char buffer[64];
    quint32 block[NHMD4_RESULTLEN];
};
typedef struct nhmd4_context NHMD4_CTX;

extern void nhmd4_init(NHMD4_CTX *ctx);
extern void nhmd4_update(NHMD4_CTX *, const unsigned char *, size_t);
extern void nhmd4_final(NHMD4_CTX *, unsigned char result[NHMD4_RESULTLEN]);

#endif /* NHMD4_H */

/*nhmd4.h*/

