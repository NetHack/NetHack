/* NetHack 5.0	revision.h	$NHDT-Date: 1779927286 2026/05/28 00:14:46 $  $NHDT-Branch: NetHack-5.0 $:$NHDT-Revision: 1.2 $ */
/* Copyright (c) Michael Allison, 2026. */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * Supporting revisions to NetHack structs via incremental
 * uplifts, rather than incrementing EDITLEVEL and breaking
 * savefile compatibility.
 *
 * Here is the contrived example struct we'll use for
 * this document:
 *
 * The old revision:           The new revision:
 *
 *       struct mystruct {           struct mystruct {
 *           int field1;                 int field1;
 *           int field2;                 int field2;
 *           char field3;                char field3;
 *           long field4;                long field4;
 *        };                             int newfielda;
 *                                       int newfieldb;
 *                                    };
 *
 * Steps (using mystruct as an example name)
 *
 *  1. Paste an exact copy of the struct declaration as it is/was
 *     in the previous revision into include/revision.h, shrouded by
 *     #if defined(MYSTRUCT_REV0), and tack a '_rev0' suffix onto the
 *     name of the struct. For example,
 *
 *       #if defined(MYSTRUCT_REV0)
 *       struct mystruct_rev0 {
 *           int field1;
 *           int field2;
 *           char field3;
 *           long field4;
 *        };
 *
 *    That goes just ahead of this existing placeholder:
 *
 *       #elif defined(XXX_REV0)
 *
 *  2. Place a comment ahead of the mystruct_rev0 declaration
 *     that explains what has changed, and why the revision
 *     is required.
 *
 *  3. Immediately following the revised struct declaration in the
 *     original header file where it is located, add the following:
 *
 *       #define MYSTRUCT_REV0
 *       #include "revision.h"
 *       #undef MYSTRUCT_REV0
 *
 *  4. In include/savefile.h, add a new prototype for a function
 *     to read the previous revision from the savefile, ideally
 *     immediately following the prototype for the current active
 *     one that hasn't got the '_rev0' suffix:
 *
 *       extern void sfi_mystruct(NHFILE *, struct mystruct *,
 *                                const char *);
 *       extern void sfi_mystruct_rev0(NHFILE *, struct mystruct_rev0  *,
 *                                     const char *);
 *
 *  5. Also in include/savefile.h, in the '#if NH_C < 202300L' block,
 *     add an entry below the one that should already exist for the
 *     struct that hasn't got the '_rev0' suffix:
 *
 *       #define Sfi_mystruct(a,b,c) sfi_rm(a, b, c)
 *       #define Sfi_mystruct_rev0(a, b, c) sfi_mystruct_rev0(a, b, c)
 *
 *  6. Also in include/savefile.h, in the '#define sfi(nhfp, dt, tag)'
 *     generic function definition, add an entry below the one
 *     that should already exist for the struct name that hasn't
 *     got the '_rev0' suffix:
 *
 *       struct mystruct *      : sfi_mystruct,         \
 *       struct mystruct_rev0 * : sfi_mysruct_rev0,     \
 *
 *  7. Also in include/savefile.h, a little further down in the
 *     'Sfi_' macro definitions, add one below the existing
 *     one for the struct without the '_rev0' suffix:
 *
 *       #define Sfi_mystruct(a,b,c) sfi(a, b, c)
 *       #define Sfi_mystruct_rev0(a, b, c) sfi(a, b, c)
 *
 *     Only the input 'Sfi_' is required, because the old revision
 *     will never be written out to a file, only read in from a
 *     a file, so no 'Sfo_' is needed.
 *
 *  8. In include/sfmacros.h, add a new entry below the existing
 *     entry that exists without the '_rev0' suffix:
 *
 *       SF_C(struct, mystruct)
 *       SF_C(struct, mystruct_rev0)
 *
 *  9. In include/sfprocs.h, add a new SF_PROTO_C entry below the
 *     existing entry that already exists without the '_rev0' suffix:
 *
 *       SF_PROTO_C(struct, mystruct);
 *       SF_PROTO_C(struct, mystruct_rev0);
 *
 * 10. Also in include/sfprocs.h, add a new SF_ENTRY_C entry below the
 *     entry that already exists without the '_rev0' suffix:
 *
 *       SF_ENTRY_C(struct, mystruct);
 *       SF_ENTRY_C(struct, mystruct_rev0);
 *
 * 11. In src/sfbase.c, add a prototype for a 'norm_ptrs' stub function
 *     below the entry that already exists without the '_rev0' suffix:
 *
 *       void norm_ptrs_mystruct(struct mystruct *d_mystruct);
 *       void norm_ptrs_mystruct_rev0(struct mystruct_rev0 *d_mystruct);
 *
 * 12. Also in src/sfbase.c, add a 'norm_ptrs' stub function below the
 *     stub function that already exists without the '_rev0' suffix:
 *
 *       void
 *       norm_ptrs_mystruct(struct mystruct *d_mystruct UNUSED)
 *       {
 *       }
 *
 *       void
 *       norm_ptrs_mystruct_rev0(struct mystruct_rev0 *d_mystruct_rev0 UNUSED)
 *       {
 *       }
 *
 * 13. In src/sfstruct.c, add entries to the 'historical' section below the
 *     existing entry that doesn't have a '_rev0' suffix. You need an 'sfo_'
 *     entry and an 'sfi_' entry here, because they have to match function
 *     pointers in another struct. The 'sfo_' function will not be called
 *     from anywhere.
 *
 *         historical_sfo_mystruct,
 *         historical_sfo_mystruct_rev0,
 *     ...
 *         historical_sfi_mystruct,
 *         historical_sfi_mystruct_rev0,
 *
 * 14. In the C source file where the 'Sfi_' call is made for your struct,
 *     the current code likely has a line for reading the struct from
 *     a file, similar the one shown below for the mystruct example:
 *
 *       Sfi_mystruct(nhfp, &svl.mystruct, "mystruct-example");
 *
 *     That single line will need to modified to become a code block similar
 *     to this, so that the uplift function will get called:
 *
 *       if (!gu.uplift_needed_rev0_to_rev1) {
 *            Sfi_mystruct(nhfp, &svl.mystruct, "mystruct-example");
 *          } else {
 *              struct mystruct_rev0 old_mystruct;
 *
 *              Sfi_mystruct_rev0(nhfp, &old_mystruct, "mystruct-example");
 *              uplift_mystruct_rev0_to_mystruct(&old_mystruct, &svl.mystruct);
 *          }
 *       }
 *
 * 15. Shortly after the reading of the struct by the 'Sfi_' function,
 *     if the newer revision of the struct added some new fields, new code
 *     will be needed to initialize the new fields to sane values.
 *     There is no data for the new fields in the existing savefile.
 *
 *        if (gu.uplift_needed_rev0_to_rev1 == 1) {
 *            svl.mystruct.newfielda = sanevalue1;
 *            svl.mystruct.newfieldb = sanevalue2;
 *        }
 *
 * 16. In include/extern.h, add a prototype to the ' ### revision.c ###'
 *     section for the supporting uplift function:
 *
 *       extern void uplift_mystruct_rev0_to_mystruct(struct mystruct_rev0 *,
 *                                                    struct mystruct *);
 *
 * 17. Add the uplift function to src/revision.c.  It needs to do
 *     field-by-field copies of the fields that exist in the old
 *     revision and the new revision. For now, it likely needs to
 *     be handcrafted using your knowledge of the struct's old
 *     and new revisions.
 *
 *     Be sure that any new fields present in the newer revision
 *     of the struct get set to sane values or initialized to
 *     zero. Do whatever suits those fields best, based on your
 *     knowledge of the struct.
 *
 *       void
 *       uplift_mystruct_rev0_to_mystruct(struct mystruct_rev0 *rev0,
 *                                        struct mystruct *rev1)
 *       {
 *           rev1->field1      = rev0->field1;
 *           rev1->field2      = rev0->field2;
 *           rev1->field3      = rev0->field3;
 *           rev1->field4      = rev0->field4;
 *
 *           rev1->newfielda = 0;   // new field
 *           rev1->newfieldb = 42;  // new field
 *       }
 *
 */

#if defined(MYSTRUCT_REV0)
#ifdef DEMO_UPLIFTS
/*
 * struct mystruct_rev0
 *
 * This is the predecessor for 'struct mystruct'
 * Revisions to 'struct mystruct' that 'struct mystruct_rev0' does not have:
 *   int newfielda;
 *   int newfieldb;
 *
 * The uplift function is:
 *     void uplift_mystruct_rev0_to_mystruct(struct *mystruct_rev0,
 *                                           struct *mystruct);
 */

struct mystruct_rev0 {
    int field1;
    int field2;
    char field3;
    long field4;
};
#endif /* DEMO_UPLIFTS */

#elif defined(XXX_REV0)

#else
#error Unproductive inclusion of revision.h
#endif
/* revision.h */
