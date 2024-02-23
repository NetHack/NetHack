/* NetHack 3.7  nhmd4.h       $NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$ */
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

#ifndef __NHMD4_H
#define __NHMD4_H

#define NHMD4_DIGEST_LENGTH 128
#define	NHMD4_RESULTLEN (128/8)

typedef uint32_t quint32;

struct nhmd4_context {
	quint32 lo, hi;
	quint32 a, b, c, d;
	unsigned char buffer[64];
	quint32 block[NHMD4_RESULTLEN];
};
typedef struct nhmd4_context NHMD4_CTX;

extern void nhmd4_init(struct nhmd4_context *ctx);
extern void nhmd4_update(struct nhmd4_context *ctx, const unsigned char *data, size_t size);
extern void nhmd4_final(struct nhmd4_context *ctx, unsigned char result[NHMD4_RESULTLEN]);

#endif
