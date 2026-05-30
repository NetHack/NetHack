/* NetHack 5.0	revision.c	$NHDT-Date: 1779927286 2026/05/28 00:14:46 $  $NHDT-Branch: NetHack-5.0 $:$NHDT-Revision: 1.1 $ */
/* Copyright (c) Michael Allison, 2026. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

boolean revision_increment(
    int file_rev_level,
    int file_critical_byte_count,
    uchar *csc)
{
    if (file_rev_level == 0 && SAVEFILE_REVISION_LEVEL == 1) {
        /*
         * Revision 1
         * We set a flag gu.uplift_needed_rev0_to_rev1 for use by restore routines.
         *
         */
        gu.uplift_needed_rev0_to_rev1 = 1;
        if (csc[file_critical_byte_count - 1] == 0)
            /* below may not be necessary, or even useful */
            csc[file_critical_byte_count - 1] = SAVEFILE_REVISION_LEVEL;
        return TRUE;
    }
    return FALSE;
}

#ifdef DEMO_UPLIFTS

/* original revision 0 is in include/revision.h */

/* revision 1 */
struct mystruct {
    int field1;
    int field2;
    char field3;
    long field4;
    int newfielda;
    int newfieldb;
};

void
uplift_mystruct_rev0_to_mystruct(struct mystruct_rev0 *rev0,
                                 struct mystruct *rev1)
{
    rev1->field1 = rev0->field1;
    rev1->field2 = rev0->field2;
    rev1->field3 = rev0->field3;
    rev1->field4 = rev0->field4;
    rev1->newfielda = 0; // new field
    rev1->newfieldb = 0; // new field
}

void
read_mystruct(struct mystruct *ms)
{
#ifdef SAVEFILE_REVISION_LEVEL

#if (SAVEFILE_REVISION_LEVEL == 0)
    Sfi_mystruct(nhfp, ms, "mystruct-example");
#elif (SAVEFILE_REVISION_LEVEL == 1)

#define MYSTRUCT_REV0
#include "revision.h"
#undef MYSTRUCT_REV0

    if (!gu.uplift_needed_rev0_to_rev1) {
        Sfi_mystruct(nhfp, ms, "mystruct-example");
    } else {
        struct mystruct_rev0 old_mystruct;

        Sfi_mystruct_rev0(nhfp, &old_mystruct, "mystruct-example");
        uplift_mystruct_rev0_to_mystruct(&old_mystruct, ms);
    }

    /* this could be elsewhere in some other sourcefile for example,
     * where it is needed */
    if (gu.uplift_needed_rev0_to_rev1 == 1) {
        /* Provide suitable starting values for fields that were not
         * present in the previous revision that was read */
        ms->newfielda = 0;  // new field in Rev. 1
        ms->newfieldb = 42; // new field in Rev. 1
    }
#endif
#endif /* SAVEFILE_REVISION_LEVEL */
}

#endif  /* DEMO_UPLIFTS */

/*revision.c*/
