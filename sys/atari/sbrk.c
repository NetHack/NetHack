/* sys/atari/sbrk.c — override mintlib's __sbrk so the libc heap is
   satisfied from TT-RAM when present, with automatic GEMDOS fall-back
   to ST-RAM.  Mintlib's archive copy in unix/sbrk.c is displaced by
   this strong definition. */

#include <errno.h>
#include <stdint.h>
#include <malloc.h>           /* _mallocChunkSize (mintlib extension) */
#include <mint/osbind.h>      /* Mxalloc, Malloc */
#include <mint/ostruct.h>     /* MX_PREFTTRAM, MX_GLOBAL */

void *
__sbrk(intptr_t n)
{
    void *p;

    if (n < 0)                /* mintlib's malloc never shrinks */
        return (void *) -1L;

    p = (void *) Mxalloc(n, MX_PREFTTRAM | MX_GLOBAL);
    if ((long) p == -32L || !p) {  /* pre-0x19 GEMDOS, or pools empty */
        p = (void *) Malloc(n);
        if (!p) {
            __set_errno(ENOMEM);
            return (void *) -1L;
        }
    }
    return p;
}

/* Keep __sbrk call counts under the TOS 2.06 ~20-allocation GEMDOS
   cap.  At 256 KB chunks a ~4 MB working set produces ~16 calls. */
static void __attribute__((constructor))
atari_set_malloc_chunk_size(void)
{
    _mallocChunkSize(256L * 1024L);
}
