/* Atari libc heap tuning. */

#include <malloc.h>           /* _mallocChunkSize (mintlib extension) */

/* Keep __sbrk call counts under the TOS 2.06 ~20-allocation GEMDOS
   cap.  At 256 KB chunks a ~4 MB working set produces ~16 calls. */
static void __attribute__((constructor))
atari_set_malloc_chunk_size(void)
{
    _mallocChunkSize(256L * 1024L);
}
