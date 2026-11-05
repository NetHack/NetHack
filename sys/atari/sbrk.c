/* $NHDT-Header: NetHack 5.0 sbrk.c 2026/09/30_13:29:51 NetHack-5.0:1.2 $ */
/* Copyright (c) Ingo Paschke, 2026 */
/* NetHack may be freely redistributed.  See license for details. */

/* Atari libc heap tuning. */

#include <malloc.h>           /* _mallocChunkSize (mintlib extension) */

/* Keep __sbrk call counts under the TOS 2.06 ~20-allocation GEMDOS
   cap.  At 256 KB chunks a ~4 MB working set produces ~16 calls. */
static void __attribute__((constructor))
atari_set_malloc_chunk_size(void)
{
    _mallocChunkSize(256L * 1024L);
}
