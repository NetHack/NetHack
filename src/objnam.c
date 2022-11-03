/* NetHack 3.7	objnam.c	$NHDT-Date: 1654557302 2022/06/06 23:15:02 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.368 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2011. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

/* "an uncursed greased partly eaten guardian naga hatchling [corpse]" */
#define PREFIX 80 /* (56) */
#define SCHAR_LIM 127
#define NUMOBUF 12

struct _readobjnam_data {
    struct obj *otmp;
    char *bp;
    char *origbp;
    char oclass;
    char *un, *dn, *actualn;
    const char *name;
    char *p;
    int cnt, spe, spesgn, typ, very, rechrg;
    int blessed, uncursed, iscursed, ispoisoned, isgreased;
    int eroded, eroded2, erodeproof, locked, unlocked, broken, real, fake;
    int halfeaten, mntmp, contents;
    int islit, unlabeled, ishistoric, isdiluted, trapped;
    int doorless, open, closed, looted;
    int tmp, tinv, tvariety, mgend;
    int wetness, gsize;
    int ftype;
    boolean zombify;
    char globbuf[BUFSZ];
    char fruitbuf[BUFSZ];
};

static char *strprepend(char *, const char *);
static char *nextobuf(void);
static void releaseobuf(char *);
static char *xname_flags(struct obj *, unsigned);
static char *minimal_xname(struct obj *);
static void add_erosion_words(struct obj *, char *);
static char *doname_base(struct obj *obj, unsigned);
static boolean singplur_lookup(char *, char *, boolean,
                               const char *const *);
static char *singplur_compound(char *);
static boolean ch_ksound(const char *basestr);
static boolean badman(const char *, boolean);
static boolean wishymatch(const char *, const char *, boolean);
static short rnd_otyp_by_wpnskill(schar);
static short rnd_otyp_by_namedesc(const char *, char, int);
static struct obj *wizterrainwish(struct _readobjnam_data *);
static void readobjnam_init(char *, struct _readobjnam_data *);
static int readobjnam_preparse(struct _readobjnam_data *);
static void readobjnam_parse_charges(struct _readobjnam_data *);
static int readobjnam_postparse1(struct _readobjnam_data *);
static int readobjnam_postparse2(struct _readobjnam_data *);
static int readobjnam_postparse3(struct _readobjnam_data *);

struct Jitem {
    int item;
    const char *name;
};

#define BSTRCMPI(base, ptr, str) ((ptr) < base || strcmpi((ptr), str))
#define BSTRNCMPI(base, ptr, str, num) \
    ((ptr) < base || strncmpi((ptr), str, num))
#define Strcasecpy(dst, src) (void) strcasecpy(dst, src)

/* true for gems/rocks that should have " stone" appended to their names */
#define GemStone(typ)                                                  \
    (typ == FLINT                                                      \
     || (objects[typ].oc_material == GEMSTONE                          \
         && (typ != DILITHIUM_CRYSTAL && typ != RUBY && typ != DIAMOND \
             && typ != SAPPHIRE && typ != BLACK_OPAL && typ != EMERALD \
             && typ != OPAL)))

static struct Jitem Japanese_items[] = { { SHORT_SWORD, "wakizashi" },
                                             { BROADSWORD, "ninja-to" },
                                             { FLAIL, "nunchaku" },
                                             { GLAIVE, "naginata" },
                                             { LOCK_PICK, "osaku" },
                                             { WOODEN_HARP, "koto" },
                                             { KNIFE, "shito" },
                                             { PLATE_MAIL, "tanko" },
                                             { HELMET, "kabuto" },
                                             { LEATHER_GLOVES, "yugake" },
                                             { FOOD_RATION, "gunyoki" },
                                             { POT_BOOZE, "sake" },
                                             { 0, "" } };

static const char *Japanese_item_name(int i);

static char *
strprepend(char *s,const char * pref)
{
    register int i = (int) strlen(pref);

    if (i > PREFIX) {
        impossible("PREFIX too short (for %d).", i);
        return s;
    }
    s -= i;
    (void) strncpy(s, pref, i); /* do not copy trailing 0 */
    return s;
}

/* manage a pool of BUFSZ buffers, so callers don't have to */
static char NEARDATA obufs[NUMOBUF][BUFSZ];
static int obufidx = 0;

static char *
nextobuf(void)
{
    obufidx = (obufidx + 1) % NUMOBUF;
    return obufs[obufidx];
}

/* put the most recently allocated buffer back if possible */
static void
releaseobuf(char *bufp)
{
    /* caller may not know whether bufp is the most recently allocated
       buffer; if it isn't, do nothing; note that because of the somewhat
       obscure PREFIX handling for object name formatting by xname(),
       the pointer our caller has and is passing to us might be into the
       middle of an obuf rather than the address returned by nextobuf() */
    if (bufp >= obufs[obufidx]
        && bufp < obufs[obufidx] + sizeof obufs[obufidx]) /* obufs[][BUFSZ] */
        obufidx = (obufidx - 1 + NUMOBUF) % NUMOBUF;
}

/* used by display_pickinv (invent.c, main whole-inventory routine) to
   release each successive doname() result in order to try to avoid
   clobbering all the obufs when 'perm_invent' is enabled and updated
   while one or more obufs have been allocated but not released yet */
void
maybereleaseobuf(char *obuffer)
{
    releaseobuf(obuffer);

    /*
     * An example from 3.6.x where all obufs got clobbered was when a
     * monster used a bullwhip to disarm the hero of a two-handed weapon:
     * "The ogre lord yanks Cleaver from your corpses!"
     |
     | hand = body_part(HAND);
     | if (use_plural)      // switches 'hand' from static buffer to an obuf
     |   hand = makeplural(hand);
      ...
     | release_worn_item(); // triggers full inventory update for perm_invent
      ...
     | pline(..., hand);    // the obuf[] for "hands" was clobbered with the
     |                      //+ partial formatting of an item from invent
     *
     * Another example was from writing a scroll without room in invent to
     * hold it after being split from a stack of blank scrolls:
     * "Oops!  food rations out of your grasp!"
     * hold_another_object() was passed 'the(aobjnam(newscroll, "slip"))'
     * as an argument and that should have yielded
     * "Oops!  The scroll of <foo> slips out of your grasp!"
     * but attempting to add the item to inventory triggered update for
     * perm_invent and the result from 'the(...)' was clobbered by partial
     * formatting of some inventory item.  [It happened in a shop and the
     * shk claimed ownership of the new scroll, but that wasn't relevant.]
     * That got fixed earlier, by delaying update_inventory() during
     * hold_another_object() rather than by avoiding using all the obufs.
     */
}

char *
obj_typename(int otyp)
{
    char *buf = nextobuf();
    struct objclass *ocl = &objects[otyp];
    const char *actualn = OBJ_NAME(*ocl);
    const char *dn = OBJ_DESCR(*ocl);
    const char *un = ocl->oc_uname;
    int nn = ocl->oc_name_known;

    if (Role_if(PM_SAMURAI) && Japanese_item_name(otyp))
        actualn = Japanese_item_name(otyp);
    switch (ocl->oc_class) {
    case COIN_CLASS:
        Strcpy(buf, "coin");
        break;
    case POTION_CLASS:
        Strcpy(buf, "potion");
        break;
    case SCROLL_CLASS:
        Strcpy(buf, "scroll");
        break;
    case WAND_CLASS:
        Strcpy(buf, "wand");
        break;
    case SPBOOK_CLASS:
        if (otyp != SPE_NOVEL) {
            Strcpy(buf, "spellbook");
        } else {
            Strcpy(buf, !nn ? "book" : "novel");
            nn = 0;
        }
        break;
    case RING_CLASS:
        Strcpy(buf, "ring");
        break;
    case AMULET_CLASS:
        if (nn)
            Strcpy(buf, actualn);
        else
            Strcpy(buf, "amulet");
        if (un)
            Sprintf(eos(buf), " called %s", un);
        if (dn)
            Sprintf(eos(buf), " (%s)", dn);
        return buf;
    default:
        if (nn) {
            Strcpy(buf, actualn);
            if (GemStone(otyp))
                Strcat(buf, " stone");
            if (un)
                Sprintf(eos(buf), " called %s", un);
            if (dn)
                Sprintf(eos(buf), " (%s)", dn);
        } else {
            Strcpy(buf, dn ? dn : actualn);
            if (ocl->oc_class == GEM_CLASS)
                Strcat(buf,
                       (ocl->oc_material == MINERAL) ? " stone" : " gem");
            if (un)
                Sprintf(eos(buf), " called %s", un);
        }
        return buf;
    }
    /* here for ring/scroll/potion/wand */
    if (nn) {
        if (ocl->oc_unique)
            Strcpy(buf, actualn); /* avoid spellbook of Book of the Dead */
        else
            Sprintf(eos(buf), " of %s", actualn);
    }
    if (un)
        Sprintf(eos(buf), " called %s", un);
    if (dn)
        Sprintf(eos(buf), " (%s)", dn);
    return buf;
}

/* less verbose result than obj_typename(); either the actual name
   or the description (but not both); user-assigned name is ignored */
char *
simple_typename(int otyp)
{
    char *bufp, *pp, *save_uname = objects[otyp].oc_uname;

    objects[otyp].oc_uname = 0; /* suppress any name given by user */
    bufp = obj_typename(otyp);
    objects[otyp].oc_uname = save_uname;
    if ((pp = strstri(bufp, " (")) != 0)
        *pp = '\0'; /* strip the appended description */
    return bufp;
}

/* typename for debugging feedback where data involved might be suspect */
char *
safe_typename(int otyp)
{
    unsigned save_nameknown;
    char *res = 0;

    if (otyp < STRANGE_OBJECT || otyp >= NUM_OBJECTS
        || !OBJ_NAME(objects[otyp])) {
        res = nextobuf();
        Sprintf(res, "glorkum[%d]", otyp);
        impossible("safe_typename: %s", res);
    } else {
        /* force it to be treated as fully discovered */
        save_nameknown = objects[otyp].oc_name_known;
        objects[otyp].oc_name_known = 1;
        res = simple_typename(otyp);
        objects[otyp].oc_name_known = save_nameknown;
    }
    return res;
}

boolean
obj_is_pname(struct obj* obj)
{
    if (!obj->oartifact || !has_oname(obj))
        return FALSE;
    if (!g.program_state.gameover && !iflags.override_ID) {
        if (not_fully_identified(obj))
            return FALSE;
    }
    return TRUE;
}

/* Give the name of an object seen at a distance.  Unlike xname/doname,
   we usually don't want to set dknown if it's not set already. */
char *
distant_name(
    struct obj *obj, /* object to be formatted */
    char *(*func)(OBJ_P)) /* formatting routine (usually xname or doname) */
{
    char *str;
    coordxy ox = 0, oy = 0;
        /*
         * (r * r): square of the x or y distance;
         * (r * r) * 2: sum of squares of both x and y distances
         * (r * r) * 2 - r: instead of a square extending from the hero,
         * round the corners (so shorter distance imposed for diagonal).
         *
         * distu() matrix convering a range of 3+ for one quadrant:
         *  16 17  -  -  -
         *   9 10 13 18  -
         *   4  5  8 13  -
         *   1  2  5 10 17
         *   @  1  4  9 16
         * Theoretical r==1 would yield 1.
         * r==2 yields 6, functionally equivalent to 5, a knight's jump,
         * r==3, the xray range of the Eyes of the Overworld, yields 15.
         */
    int r = (u.xray_range > 2) ? u.xray_range : 2,
        neardist = (r * r) * 2 - r; /* same as r*r + r*(r-1) */

    /* this maybe-nearby part used to be replicated in multiple callers */
    if (get_obj_location(obj, &ox, &oy, 0) && cansee(ox, oy)
        && (obj->oartifact || distu(ox, oy) <= neardist)) {
        /* side-effects:  treat as having been seen up close;
           cansee() is True hence hero isn't Blind so if 'func' is
           the usual doname or xname, obj->dknown will become set
           and then for an artifact, find_artifact() will be called */
        str = (*func)(obj);
    } else {
        /* prior to 3.6.1, this used to save current blindness state,
           explicitly set state to hero-is-blind, make the call (which
           won't set obj->dknown when blind), then restore the saved
           value; but the Eyes of the Overworld override blindness and
           would let characters wearing them get obj->dknown set for
           distant items, so the external flag was added */
        ++g.distantname;
        str = (*func)(obj);
        --g.distantname;
    }
    return str;
}

/* convert player specified fruit name into corresponding fruit juice name
   ("slice of pizza" -> "pizza juice" rather than "slice of pizza juice") */
char *
fruitname(
    boolean juice) /* whether or not to append " juice" to the name */
{
    char *buf = nextobuf();
    const char *fruit_nam = strstri(g.pl_fruit, " of ");

    if (fruit_nam)
        fruit_nam += 4; /* skip past " of " */
    else
        fruit_nam = g.pl_fruit; /* use it as is */

    Sprintf(buf, "%s%s", makesingular(fruit_nam), juice ? " juice" : "");
    return buf;
}

/* look up a named fruit by index (1..127) */
struct fruit *
fruit_from_indx(int indx)
{
    struct fruit *f;

    for (f = g.ffruit; f; f = f->nextf)
        if (f->fid == indx)
            break;
    return f;
}

/* look up a named fruit by name */
struct fruit *
fruit_from_name(
    const char *fname,
    boolean exact, /* False: prefix or exact match, True: exact match only */
    int *highest_fid) /* optional output; only valid if 'fname' isn't found */
{
    struct fruit *f, *tentativef;
    char *altfname;
    unsigned k;
    /*
     * note: named fruits are case-senstive...
     */

    if (highest_fid)
        *highest_fid = 0;
    /* first try for an exact match */
    for (f = g.ffruit; f; f = f->nextf)
        if (!strcmp(f->fname, fname))
            return f;
        else if (highest_fid && f->fid > *highest_fid)
            *highest_fid = f->fid;

    /* didn't match as-is; if caller is willing to accept a prefix
       match, try to find one; we want to find the longest prefix that
       matches, not the first */
    if (!exact) {
        tentativef = 0;
        for (f = g.ffruit; f; f = f->nextf) {
            k = Strlen(f->fname);
            if (!strncmp(f->fname, fname, k)
                && (!fname[k] || fname[k] == ' ')
                && (!tentativef || k > strlen(tentativef->fname)))
                tentativef = f;
        }
        f = tentativef;
    }
    /* if we still don't have a match, try singularizing the target;
       for exact match, that's trivial, but for prefix, it's hard */
    if (!f) {
        altfname = makesingular(fname);
        for (f = g.ffruit; f; f = f->nextf) {
            if (!strcmp(f->fname, altfname))
                break;
        }
        releaseobuf(altfname);
    }
    if (!f && !exact) {
        char fnamebuf[BUFSZ], *p;
        unsigned fname_k = Strlen(fname); /* length of assumed plural fname */

        tentativef = 0;
        for (f = g.ffruit; f; f = f->nextf) {
            k = Strlen(f->fname);
            /* reload fnamebuf[] each iteration in case it gets modified;
               there's no need to recalculate fname_k */
            Strcpy(fnamebuf, fname);
            /* bug? if singular of fname is longer than plural,
               failing the 'fname_k > k' test could skip a viable
               candidate; unfortunately, we can't singularize until
               after stripping off trailing stuff and we can't get
               accurate fname_k until fname has been singularized;
               compromise and use 'fname_k >= k' instead of '>',
               accepting 1 char length discrepancy without risking
               false match (I hope...) */
            if (fname_k >= k && (p = strchr(&fnamebuf[k], ' ')) != 0) {
                *p = '\0'; /* truncate at 1st space past length of f->fname */
                altfname = makesingular(fnamebuf);
                k = Strlen(altfname); /* actually revised 'fname_k' */
                if (!strcmp(f->fname, altfname)
                    && (!tentativef || k > strlen(tentativef->fname)))
                    tentativef = f;
                releaseobuf(altfname); /* avoid churning through all obufs */
            }
        }
        f = tentativef;
    }
    return f;
}

/* sort the named-fruit linked list by fruit index number */
void
reorder_fruit(boolean forward)
{
    struct fruit *f, *allfr[1 + 127];
    int i, j, k = SIZE(allfr);

    for (i = 0; i < k; ++i)
        allfr[i] = (struct fruit *) 0;
    for (f = g.ffruit; f; f = f->nextf) {
        /* without sanity checking, this would reduce to 'allfr[f->fid]=f' */
        j = f->fid;
        if (j < 1 || j >= k) {
            impossible("reorder_fruit: fruit index (%d) out of range", j);
            return; /* don't sort after all; should never happen... */
        } else if (allfr[j]) {
            impossible("reorder_fruit: duplicate fruit index (%d)", j);
            return;
        }
        allfr[j] = f;
    }
    g.ffruit = 0; /* reset linked list; we're rebuilding it from scratch */
    /* slot [0] will always be empty; must start 'i' at 1 to avoid
       [k - i] being out of bounds during first iteration */
    for (i = 1; i < k; ++i) {
        /* for forward ordering, go through indices from high to low;
           for backward ordering, go from low to high */
        j = forward ? (k - i) : i;
        if (allfr[j]) {
            allfr[j]->nextf = g.ffruit;
            g.ffruit = allfr[j];
        }
    }
}

char *
xname(struct obj* obj)
{
    return xname_flags(obj, CXN_NORMAL);
}

static char *
xname_flags(
    register struct obj *obj,
    unsigned cxn_flags) /* bitmask of CXN_xxx values */
{
    register char *buf;
    char *obufp;
    int typ = obj->otyp;
    struct objclass *ocl = &objects[typ];
    int nn = ocl->oc_name_known, omndx = obj->corpsenm;
    const char *actualn = OBJ_NAME(*ocl);
    const char *dn = OBJ_DESCR(*ocl);
    const char *un = ocl->oc_uname;
    boolean pluralize = (obj->quan != 1L) && !(cxn_flags & CXN_SINGULAR);
    boolean known, dknown, bknown;

    buf = nextobuf() + PREFIX; /* leave room for "17 -3 " */
    if (Role_if(PM_SAMURAI) && Japanese_item_name(typ))
        actualn = Japanese_item_name(typ);
    /* As of 3.6.2: this used to be part of 'dn's initialization, but it
       needs to come after possibly overriding 'actualn' */
    if (!dn)
        dn = actualn;

    buf[0] = '\0';
    /*
     * clean up known when it's tied to oc_name_known, eg after AD_DRIN
     * This is only required for unique objects since the article
     * printed for the object is tied to the combination of the two
     * and printing the wrong article gives away information.
     */
    if (!nn && ocl->oc_uses_known && ocl->oc_unique)
        obj->known = 0;
    if (!Blind && !g.distantname)
        obj->dknown = 1;
    if (Role_if(PM_CLERIC))
        obj->bknown = 1; /* avoid set_bknown() to bypass update_inventory() */

    if (iflags.override_ID) {
        known = dknown = bknown = TRUE;
        nn = 1;
    } else {
        known = obj->known;
        dknown = obj->dknown;
        bknown = obj->bknown;
    }

    /*
     * Maybe find a previously unseen artifact.
     *
     * Assumption 1: if an artifact object is being formatted, it is
     *  being shown to the hero (on floor, or looking into container,
     *  or probing a monster, or seeing a monster wield it).
     * Assumption 2: if in a pile that has been stepped on, the
     *  artifact won't be noticed for cases where the pile to too deep
     *  to be auto-shown, unless the player explicitly looks at that
     *  spot (via ':').  Might need to make an exception somehow (at
     *  the point where the decision whether to auto-show gets made?)
     *  when an artifact is on the top of the pile.
     * Assumption 3: since this is used for livelog events, not being
     *  100% correct won't negatively affect the player's current game.
     *
     * We use the real obj->dknown rather than the override_ID variant
     * so that wizard-mode ^I doesn't cause a not-yet-seen artifact in
     * inventory (picked up while blind, still blind) to become found.
     */
    if (obj->oartifact && obj->dknown)
        find_artifact(obj);

    if (obj_is_pname(obj))
        goto nameit;
    switch (obj->oclass) {
    case AMULET_CLASS:
        if (!dknown)
            Strcpy(buf, "amulet");
        else if (typ == AMULET_OF_YENDOR || typ == FAKE_AMULET_OF_YENDOR)
            /* each must be identified individually */
            Strcpy(buf, known ? actualn : dn);
        else if (nn)
            Strcpy(buf, actualn);
        else if (un)
            Sprintf(buf, "amulet called %s", un);
        else
            Sprintf(buf, "%s amulet", dn);
        break;
    case WEAPON_CLASS:
        if (is_poisonable(obj) && obj->opoisoned)
            Strcpy(buf, "poisoned ");
        /*FALLTHRU*/
    case VENOM_CLASS:
    case TOOL_CLASS:
        if (typ == LENSES)
            Strcpy(buf, "pair of ");
        else if (is_wet_towel(obj))
            Strcpy(buf, (obj->spe < 3) ? "moist " : "wet ");

        if (!dknown)
            Strcat(buf, dn);
        else if (nn)
            Strcat(buf, actualn);
        else if (un) {
            Strcat(buf, dn);
            Strcat(buf, " called ");
            Strcat(buf, un);
        } else
            Strcat(buf, dn);

        if (typ == FIGURINE && omndx != NON_PM) {
            char anbuf[10]; /* [4] would be enough: 'a','n',' ','\0' */
            const char *pm_name = obj_pmname(obj);

            Sprintf(eos(buf), " of %s%s", just_an(anbuf, pm_name), pm_name);
        } else if (is_wet_towel(obj)) {
            if (wizard)
                Sprintf(eos(buf), " (%d)", obj->spe);
        }
        break;
    case ARMOR_CLASS:
        /* depends on order of the dragon scales objects */
        if (typ >= GRAY_DRAGON_SCALES && typ <= YELLOW_DRAGON_SCALES) {
            Sprintf(buf, "set of %s", actualn);
            break;
        }
        if (is_boots(obj) || is_gloves(obj))
            Strcpy(buf, "pair of ");

        if (obj->otyp >= ELVEN_SHIELD && obj->otyp <= ORCISH_SHIELD
            && !dknown) {
            Strcpy(buf, "shield");
            break;
        }
        if (obj->otyp == SHIELD_OF_REFLECTION && !dknown) {
            Strcpy(buf, "smooth shield");
            break;
        }

        if (nn) {
            Strcat(buf, actualn);
        } else if (un) {
            if (is_boots(obj))
                Strcat(buf, "boots");
            else if (is_gloves(obj))
                Strcat(buf, "gloves");
            else if (is_cloak(obj))
                Strcpy(buf, "cloak");
            else if (is_helmet(obj))
                Strcpy(buf, "helmet");
            else if (is_shield(obj))
                Strcpy(buf, "shield");
            else
                Strcpy(buf, "armor");
            Strcat(buf, " called ");
            Strcat(buf, un);
        } else
            Strcat(buf, dn);
        break;
    case FOOD_CLASS:
        /* we could include partly-eaten-hack on fruit but don't need to */
        if (typ == SLIME_MOLD) {
            struct fruit *f = fruit_from_indx(obj->spe);

            if (!f) {
                impossible("Bad fruit #%d?", obj->spe);
                Strcpy(buf, "fruit");
            } else {
                Strcpy(buf, f->fname);
                if (pluralize) {
                    /* ick: already pluralized fruit names are allowed--we
                       want to try to avoid adding a redundant plural suffix;
                       double ick: makesingular() and makeplural() each use
                       and return an obuf but we don't want any particular
                       xname() call to consume more than one of those
                       [note: makeXXX() will be fully evaluated and done with
                       'buf' before strcpy() touches its output buffer] */
                    Strcpy(buf, obufp = makesingular(buf));
                    releaseobuf(obufp);
                    Strcpy(buf, obufp = makeplural(buf));
                    releaseobuf(obufp);

                    pluralize = FALSE;
                }
            }
            break;
        }
        if (iflags.partly_eaten_hack && obj->oeaten) {
            /* normally "partly eaten" is supplied by doname() when
               appropriate and omitted by xname(); shrink_glob() wants
               it but uses Yname2() -> yname() -> xname() rather than
               doname() so we've added an external flag to request it */
            Strcat(buf, "partly eaten ");
        }
        if (obj->globby) { /* 3.7 added "medium" to replace no-prefix */
            Sprintf(eos(buf), "%s %s", (obj->owt <= 100) ? "small"
                                       : (obj->owt <= 300) ? "medium"
                                         : (obj->owt <= 500) ? "large"
                                           : "very large",
                    actualn);
            break;
        }

        Strcpy(buf, actualn);
        if (typ == TIN && known)
            tin_details(obj, omndx, buf);
        break;
    case COIN_CLASS:
    case CHAIN_CLASS:
        Strcpy(buf, actualn);
        break;
    case ROCK_CLASS:
        if (typ == STATUE && omndx != NON_PM) {
            char anbuf[10];
            const char *statue_pmname = obj_pmname(obj);

            Sprintf(buf, "%s%s of %s%s",
                    (Role_if(PM_ARCHEOLOGIST)
                     && (obj->spe & CORPSTAT_HISTORIC)) ? "historic " : "",
                    actualn,
                    type_is_pname(&mons[omndx]) ? ""
                      : the_unique_pm(&mons[omndx]) ? "the "
                        : just_an(anbuf, statue_pmname),
                    statue_pmname);
        } else {
            /* sometimes caller wants "next boulder" rather than just
               "boulder" (when pushing against a pile of more than one);
               originally we just tested for non-0 but checking for 1 is
               more robust because the default value for that overloaded
               field (obj->corpsenm) is NON_PM (-1) rather than 0 */
            if (typ == BOULDER && obj->next_boulder == 1)
                Strcat(strcpy(buf, "next "), actualn);
            else
                Strcpy(buf, actualn);
        }
        break;
    case BALL_CLASS:
        Sprintf(buf, "%sheavy iron ball",
                (obj->owt > ocl->oc_weight) ? "very " : "");
        break;
    case POTION_CLASS:
        if (dknown && obj->odiluted)
            Strcpy(buf, "diluted ");
        if (nn || un || !dknown) {
            Strcat(buf, "potion");
            if (!dknown)
                break;
            if (nn) {
                Strcat(buf, " of ");
                if (typ == POT_WATER && bknown
                    && (obj->blessed || obj->cursed)) {
                    Strcat(buf, obj->blessed ? "holy " : "unholy ");
                }
                Strcat(buf, actualn);
            } else {
                Strcat(buf, " called ");
                Strcat(buf, un);
            }
        } else {
            Strcat(buf, dn);
            Strcat(buf, " potion");
        }
        break;
    case SCROLL_CLASS:
        Strcpy(buf, "scroll");
        if (!dknown)
            break;
        if (nn) {
            Strcat(buf, " of ");
            Strcat(buf, actualn);
        } else if (un) {
            Strcat(buf, " called ");
            Strcat(buf, un);
        } else if (ocl->oc_magic) {
            Strcat(buf, " labeled ");
            Strcat(buf, dn);
        } else {
            Strcpy(buf, dn);
            Strcat(buf, " scroll");
        }
        break;
    case WAND_CLASS:
        if (!dknown)
            Strcpy(buf, "wand");
        else if (nn)
            Sprintf(buf, "wand of %s", actualn);
        else if (un)
            Sprintf(buf, "wand called %s", un);
        else
            Sprintf(buf, "%s wand", dn);
        break;
    case SPBOOK_CLASS:
        if (typ == SPE_NOVEL) { /* 3.6 tribute */
            if (!dknown)
                Strcpy(buf, "book");
            else if (nn)
                Strcpy(buf, actualn);
            else if (un)
                Sprintf(buf, "novel called %s", un);
            else
                Sprintf(buf, "%s book", dn);
            break;
            /* end of tribute */
        } else if (!dknown) {
            Strcpy(buf, "spellbook");
        } else if (nn) {
            if (typ != SPE_BOOK_OF_THE_DEAD)
                Strcpy(buf, "spellbook of ");
            Strcat(buf, actualn);
        } else if (un) {
            Sprintf(buf, "spellbook called %s", un);
        } else
            Sprintf(buf, "%s spellbook", dn);
        break;
    case RING_CLASS:
        if (!dknown)
            Strcpy(buf, "ring");
        else if (nn)
            Sprintf(buf, "ring of %s", actualn);
        else if (un)
            Sprintf(buf, "ring called %s", un);
        else
            Sprintf(buf, "%s ring", dn);
        break;
    case GEM_CLASS: {
        const char *rock = (ocl->oc_material == MINERAL) ? "stone" : "gem";

        if (!dknown) {
            Strcpy(buf, rock);
        } else if (!nn) {
            if (un)
                Sprintf(buf, "%s called %s", rock, un);
            else
                Sprintf(buf, "%s %s", dn, rock);
        } else {
            Strcpy(buf, actualn);
            if (GemStone(typ))
                Strcat(buf, " stone");
        }
        break;
    }
    default:
        Sprintf(buf, "glorkum %d %d %d", obj->oclass, typ, obj->spe);
        impossible("xname_flags: %s", buf);
        break;
    }
    if (pluralize) {
        /* (see fruit name handling in case FOOD_CLASS above) */
        Strcpy(buf, obufp = makeplural(buf));
        releaseobuf(obufp);
    }

    /* maybe give some extra information which isn't shown during play */
    if (g.program_state.gameover) {
        const char *lbl;
        char tmpbuf[BUFSZ];

        /* disclose without breaking illiterate conduct, but mainly tip off
           players who aren't aware that something readable is present */
        switch (obj->otyp) {
        case T_SHIRT:
        case ALCHEMY_SMOCK:
            Sprintf(eos(buf), " with text \"%s\"",
                    (obj->otyp == T_SHIRT) ? tshirt_text(obj, tmpbuf)
                                           : apron_text(obj, tmpbuf));
            break;
        case CANDY_BAR:
            lbl = candy_wrapper_text(obj);
            if (*lbl)
                Sprintf(eos(buf), " labeled \"%s\"", lbl);
            break;
        case HAWAIIAN_SHIRT:
            Sprintf(eos(buf), " with %s motif",
                    an(hawaiian_motif(obj, tmpbuf)));
            break;
        default:
            break;
        }
    }

    if (has_oname(obj) && dknown) {
        Strcat(buf, " named ");
 nameit:
        obufp = eos(buf);
        Strcat(buf, ONAME(obj));
        /* downcase "The" in "<quest-artifact-item> named The ..." */
        if (obj->oartifact && !strncmp(obufp, "The ", 4))
            *obufp = lowc(*obufp); /* = 't'; */
    }

    if (!strncmpi(buf, "the ", 4))
        buf += 4;
    return buf;
}

/* similar to simple_typename but minimal_xname operates on a particular
   object rather than its general type; it formats the most basic info:
     potion                     -- if description not known
     brown potion               -- if oc_name_known not set
     potion of object detection -- if discovered
 */
static char *
minimal_xname(struct obj *obj)
{
    char *bufp;
    struct obj bareobj;
    struct objclass saveobcls;
    int otyp = obj->otyp;

    /* suppress user-supplied name */
    saveobcls.oc_uname = objects[otyp].oc_uname;
    objects[otyp].oc_uname = 0;
    /* suppress actual name if object's description is unknown */
    saveobcls.oc_name_known = objects[otyp].oc_name_known;
    if (iflags.override_ID)
        objects[otyp].oc_name_known = 1;
    else if (!obj->dknown)
        objects[otyp].oc_name_known = 0;

    /* caveat: this makes a lot of assumptions about which fields
       are required in order for xname() to yield a sensible result */
    bareobj = cg.zeroobj;
    bareobj.otyp = otyp;
    bareobj.oclass = obj->oclass;
    bareobj.dknown = (obj->dknown || iflags.override_ID) ? 1 : 0;
    /* suppress known except for amulets (needed for fakes and real A-of-Y) */
    bareobj.known = (obj->oclass == AMULET_CLASS)
                        ? obj->known
                        /* default is "on" for types which don't use it */
                        : !objects[otyp].oc_uses_known;
    bareobj.quan = 1L;         /* don't want plural */
    /* for a boulder, leave corpsenm as 0; non-zero produces "next boulder" */
    if (otyp != BOULDER)
        bareobj.corpsenm = NON_PM; /* suppress statue and figurine details */
    /* but suppressing fruit details leads to "bad fruit #0"
       [perhaps we should force "slime mold" rather than use xname?] */
    if (obj->otyp == SLIME_MOLD)
        bareobj.spe = obj->spe;

    /* bufp will be an obuf[] and a pointer into middle of that is viable */
    bufp = distant_name(&bareobj, xname);
    /* undo forced setting of bareobj.blessed for cleric (preist[ess]) */
    if (!strncmp(bufp, "uncursed ", 9))
        bufp += 9;

    objects[otyp].oc_uname = saveobcls.oc_uname;
    objects[otyp].oc_name_known = saveobcls.oc_name_known;
    return bufp;
}

/* xname() output augmented for multishot missile feedback */
char *
mshot_xname(struct obj* obj)
{
    char tmpbuf[BUFSZ];
    char *onm = xname(obj);

    if (g.m_shot.n > 1 && g.m_shot.o == obj->otyp) {
        /* "the Nth arrow"; value will eventually be passed to an() or
           The(), both of which correctly handle this "the " prefix */
        Sprintf(tmpbuf, "the %d%s ", g.m_shot.i, ordin(g.m_shot.i));
        onm = strprepend(onm, tmpbuf);
    }
    return onm;
}

/* used for naming "the unique_item" instead of "a unique_item" */
boolean
the_unique_obj(struct obj* obj)
{
    boolean known = (obj->known || iflags.override_ID);

    if (!obj->dknown && !iflags.override_ID)
        return FALSE;
    else if (obj->otyp == FAKE_AMULET_OF_YENDOR && !known)
        return TRUE; /* lie */
    else
        return (boolean) (objects[obj->otyp].oc_unique
                          && (known || obj->otyp == AMULET_OF_YENDOR));
}

/* should monster type be prefixed with "the"? (mostly used for corpses) */
boolean
the_unique_pm(struct permonst* ptr)
{
    boolean uniq;

    /* even though monsters with personal names are unique, we want to
       describe them as "Name" rather than "the Name" */
    if (type_is_pname(ptr))
        return FALSE;

    uniq = (ptr->geno & G_UNIQ) ? TRUE : FALSE;
    /* high priest is unique if it includes "of <deity>", otherwise not
       (caller needs to handle the 1st possibility; we assume the 2nd);
       worm tail should be irrelevant but is included for completeness */
    if (ptr == &mons[PM_HIGH_CLERIC] || ptr == &mons[PM_LONG_WORM_TAIL])
        uniq = FALSE;
    /* Wizard no longer needs this; he's flagged as unique these days */
    if (ptr == &mons[PM_WIZARD_OF_YENDOR])
        uniq = TRUE;
    return uniq;
}

static void
add_erosion_words(struct obj* obj, char* prefix)
{
    boolean iscrys = (obj->otyp == CRYSKNIFE);
    boolean rknown;

    rknown = (iflags.override_ID == 0) ? obj->rknown : TRUE;

    if (!is_damageable(obj) && !iscrys)
        return;

    /* The only cases where any of these bits do double duty are for
     * rotted food and diluted potions, which are all not is_damageable().
     */
    if (obj->oeroded && !iscrys) {
        switch (obj->oeroded) {
        case 2:
            Strcat(prefix, "very ");
            break;
        case 3:
            Strcat(prefix, "thoroughly ");
            break;
        }
        Strcat(prefix, is_rustprone(obj) ? "rusty " : "burnt ");
    }
    if (obj->oeroded2 && !iscrys) {
        switch (obj->oeroded2) {
        case 2:
            Strcat(prefix, "very ");
            break;
        case 3:
            Strcat(prefix, "thoroughly ");
            break;
        }
        Strcat(prefix, is_corrodeable(obj) ? "corroded " : "rotted ");
    }
    if (rknown && obj->oerodeproof)
        Strcat(prefix, iscrys
                          ? "fixed "
                          : is_rustprone(obj)
                             ? "rustproof "
                             : is_corrodeable(obj)
                                ? "corrodeproof " /* "stainless"? */
                                : is_flammable(obj)
                                   ? "fireproof "
                                   : "");
}

/* used to prevent rust on items where rust makes no difference */
boolean
erosion_matters(struct obj* obj)
{
    switch (obj->oclass) {
    case TOOL_CLASS:
        /* it's possible for a rusty weptool to be polymorphed into some
           non-weptool iron tool, in which case the rust implicitly goes
           away, but it's also possible for it to be polymorphed into a
           non-iron tool, in which case rust also implicitly goes away,
           so there's no particular reason to try to handle the first
           instance differently [this comment belongs in poly_obj()...] */
        return is_weptool(obj) ? TRUE : FALSE;
    case WEAPON_CLASS:
    case ARMOR_CLASS:
    case BALL_CLASS:
    case CHAIN_CLASS:
        return TRUE;
    default:
        break;
    }
    return FALSE;
}

#define DONAME_WITH_PRICE 1
#define DONAME_VAGUE_QUAN 2

/* core of doname() */
static char *
doname_base(
    struct obj* obj,       /* object to format */
    unsigned doname_flags) /* special case requests */
{
    boolean ispoisoned = FALSE,
            with_price = (doname_flags & DONAME_WITH_PRICE) != 0,
            vague_quan = (doname_flags & DONAME_VAGUE_QUAN) != 0;
    boolean known, dknown, cknown, bknown, lknown,
            fake_arti, force_the;
    char prefix[PREFIX];
    char tmpbuf[PREFIX + 1]; /* for when we have to add something at
                              * the start of prefix instead of the
                              * end (Strcat is used on the end) */
    const char *aname = 0;
    int omndx = obj->corpsenm;
    register char *bp = xname(obj);

    if (iflags.override_ID) {
        known = dknown = cknown = bknown = lknown = TRUE;
    } else {
        known = obj->known;
        dknown = obj->dknown;
        cknown = obj->cknown;
        bknown = obj->bknown;
        lknown = obj->lknown;
    }

    /* When using xname, we want "poisoned arrow", and when using
     * doname, we want "poisoned +0 arrow".  This kludge is about the only
     * way to do it, at least until someone overhauls xname() and doname(),
     * combining both into one function taking a parameter.
     */
    /* must check opoisoned--someone can have a weirdly-named fruit */
    if (!strncmp(bp, "poisoned ", 9) && obj->opoisoned) {
        bp += 9;
        ispoisoned = TRUE;
    }

    /* fruits are allowed to be given artifact names; when that happens,
       format the name like the corresponding artifact, which may or may not
       want "the" prefix and when it doesn't, avoid "a"/"an" prefix too */
    fake_arti = (obj->otyp == SLIME_MOLD
                 && (aname = artifact_name(bp, (short *) 0, FALSE)) != 0);
    force_the = (fake_arti && !strncmpi(aname, "the ", 4));

    prefix[0] = '\0';
    if (obj->quan != 1L) {
        if (dknown || !vague_quan)
            Sprintf(prefix, "%ld ", obj->quan);
        else
            Strcpy(prefix, "some ");
    } else if (obj->otyp == CORPSE) {
        /* skip article prefix for corpses [else corpse_xname()
           would have to be taught how to strip it off again] */
        ;
    } else if (force_the || obj_is_pname(obj) || the_unique_obj(obj)) {
        if (!strncmpi(bp, "the ", 4))
            bp += 4;
        Strcpy(prefix, "the ");
    } else if (!fake_arti) {
        /* default prefix */
        Strcpy(prefix, "a ");
    }

    /* "empty" goes at the beginning, but item count goes at the end */
    if (cknown
        /* bag of tricks: include "empty" prefix if it's known to
           be empty but its precise number of charges isn't known
           (when that is known, suffix of "(n:0)" will be appended,
           making the prefix be redundant; note that 'known' flag
           isn't set when emptiness gets discovered because then
           charging magic would yield known number of new charges);
           horn of plenty isn't a container but is close enough */
        && ((obj->otyp == BAG_OF_TRICKS || obj->otyp == HORN_OF_PLENTY)
             ? (obj->spe == 0 && !known)
             /* not a bag of tricks or horn of plenty: it's empty if
                it is a container that has no contents */
             : ((Is_container(obj) || obj->otyp == STATUE)
                && !Has_contents(obj))))
        Strcat(prefix, "empty ");

    if (bknown && obj->oclass != COIN_CLASS
        && (obj->otyp != POT_WATER || !objects[POT_WATER].oc_name_known
            || (!obj->cursed && !obj->blessed))) {
        /* allow 'blessed clear potion' if we don't know it's holy water;
         * always allow "uncursed potion of water"
         */
        if (obj->cursed)
            Strcat(prefix, "cursed ");
        else if (obj->blessed)
            Strcat(prefix, "blessed ");
        else if (!flags.implicit_uncursed
            /* For most items with charges or +/-, if you know how many
             * charges are left or what the +/- is, then you must have
             * totally identified the item, so "uncursed" is unnecessary,
             * because an identified object not described as "blessed" or
             * "cursed" must be uncursed.
             *
             * If the charges or +/- is not known, "uncursed" must be
             * printed to avoid ambiguity between an item whose curse
             * status is unknown, and an item known to be uncursed.
             */
                 || ((!known || !objects[obj->otyp].oc_charged
                      || obj->oclass == ARMOR_CLASS
                      || obj->oclass == RING_CLASS)
#ifdef MAIL_STRUCTURES
                     && obj->otyp != SCR_MAIL
#endif
                     && obj->otyp != FAKE_AMULET_OF_YENDOR
                     && obj->otyp != AMULET_OF_YENDOR
                     && !Role_if(PM_CLERIC)))
            Strcat(prefix, "uncursed ");
    }

    if (lknown && Is_box(obj)) {
        if (obj->obroken)
            /* 3.6.0 used "unlockable" here but that could be misunderstood
               to mean "capable of being unlocked" rather than the intended
               "not capable of being locked" */
            Strcat(prefix, "broken ");
        else if (obj->olocked)
            Strcat(prefix, "locked ");
        else
            Strcat(prefix, "unlocked ");
    }

    if (obj->greased)
        Strcat(prefix, "greased ");

    if (cknown && Has_contents(obj)) {
        /* we count the number of separate stacks, which corresponds
           to the number of inventory slots needed to be able to take
           everything out if no merges occur */
        long itemcount = count_contents(obj, FALSE, FALSE, TRUE, FALSE);

        Sprintf(eos(bp), " containing %ld item%s", itemcount,
                plur(itemcount));
    }

    switch (is_weptool(obj) ? WEAPON_CLASS : obj->oclass) {
    case AMULET_CLASS:
        if (obj->owornmask & W_AMUL)
            Strcat(bp, " (being worn)");
        break;
    case ARMOR_CLASS:
        if (obj->owornmask & W_ARMOR) {
            Strcat(bp, (obj == uskin) ? " (embedded in your skin)"
                       /* in case of perm_invent update while Wear/Takeoff
                          is in progress; check doffing() before donning()
                          because donning() returns True for both cases */
                       : doffing(obj) ? " (being doffed)"
                         : donning(obj) ? " (being donned)"
                           : " (being worn)");
            /* slippery fingers is an intrinsic condition of the hero
               rather than extrinsic condition of objects, but gloves
               are described as slippery when hero has slippery fingers */
            if (obj == uarmg && Glib) /* just appended "(something)",
                                       * change to "(something; slippery)" */
                Strcpy(strrchr(bp, ')'), "; slippery)");
            else if (!Blind && obj->lamplit && artifact_light(obj))
                Sprintf(strrchr(bp, ')'), ", %s lit)",
                        arti_light_description(obj));
        }
        /*FALLTHRU*/
    case WEAPON_CLASS:
        if (ispoisoned)
            Strcat(prefix, "poisoned ");
        add_erosion_words(obj, prefix);
        if (known) {
            Strcat(prefix, sitoa(obj->spe));
            Strcat(prefix, " ");
        }
        break;
    case TOOL_CLASS:
        if (obj->owornmask & (W_TOOL | W_SADDLE)) { /* blindfold */
            Strcat(bp, " (being worn)");
            break;
        }
        if (obj->otyp == LEASH && obj->leashmon != 0) {
            struct monst *mlsh = find_mid(obj->leashmon, FM_FMON);

            if (!mlsh) {
                impossible("leashed monster not on this level");
                obj->leashmon = 0;
            } else {
                Sprintf(eos(bp), " (attached to %s)",
                        noit_mon_nam(mlsh));
            }
            break;
        }
        if (obj->otyp == CANDELABRUM_OF_INVOCATION) {
            Sprintf(eos(bp), " (%d of 7 candle%s%s)",
                    obj->spe, plur(obj->spe),
                    !obj->lamplit ? " attached" : ", lit");
            break;
        } else if (obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP
                   || obj->otyp == BRASS_LANTERN || Is_candle(obj)) {
            if (Is_candle(obj)
                && obj->age < 20L * (long) objects[obj->otyp].oc_cost)
                Strcat(prefix, "partly used ");
            if (obj->lamplit)
                Strcat(bp, " (lit)");
            break;
        }
        if (objects[obj->otyp].oc_charged)
            goto charges;
        break;
    case WAND_CLASS:
 charges:
        if (known)
            Sprintf(eos(bp), " (%d:%d)", (int) obj->recharged, obj->spe);
        break;
    case POTION_CLASS:
        if (obj->otyp == POT_OIL && obj->lamplit)
            Strcat(bp, " (lit)");
        break;
    case RING_CLASS:
 ring:
        if (obj->owornmask & W_RINGR)
            Strcat(bp, " (on right ");
        if (obj->owornmask & W_RINGL)
            Strcat(bp, " (on left ");
        if (obj->owornmask & W_RING) {
            Strcat(bp, body_part(HAND));
            Strcat(bp, ")");
        }
        if (known && objects[obj->otyp].oc_charged) {
            Strcat(prefix, sitoa(obj->spe));
            Strcat(prefix, " ");
        }
        break;
    case FOOD_CLASS:
        if (obj->oeaten)
            Strcat(prefix, "partly eaten ");
        if (obj->otyp == CORPSE) {
            /* (quan == 1) => want corpse_xname() to supply article,
               (quan != 1) => already have count or "some" as prefix;
               "corpse" is already in the buffer returned by xname() */
            unsigned cxarg = (((obj->quan != 1L) ? 0 : CXN_ARTICLE)
                              | CXN_NOCORPSE);
            char *cxstr = corpse_xname(obj, prefix, cxarg);

            Sprintf(prefix, "%s ", cxstr);
            /* avoid having doname(corpse) consume an extra obuf */
            releaseobuf(cxstr);
        } else if (obj->otyp == EGG) {
#if 0 /* corpses don't tell if they're stale either */
            if (known && stale_egg(obj))
                Strcat(prefix, "stale ");
#endif
            if (omndx >= LOW_PM
                && (known || (g.mvitals[omndx].mvflags & MV_KNOWS_EGG))) {
                Strcat(prefix, mons[omndx].pmnames[NEUTRAL]);
                Strcat(prefix, " ");
                if (obj->spe == 1)
                    Strcat(bp, " (laid by you)");
            }
        }
        if (obj->otyp == MEAT_RING)
            goto ring;
        break;
    case BALL_CLASS:
    case CHAIN_CLASS:
        add_erosion_words(obj, prefix);
        if (obj->owornmask & (W_BALL | W_CHAIN))
            Sprintf(eos(bp), " (%s to you)",
                    (obj->owornmask & W_BALL) ? "chained" : "attached");
        break;
    }

    if ((obj->otyp == STATUE || obj->otyp == CORPSE || obj->otyp == FIGURINE)
        && wizard && iflags.wizmgender) {
        int cgend = (obj->spe & CORPSTAT_GENDER),
            mgend = ((cgend == CORPSTAT_MALE) ? MALE
                     : (cgend == CORPSTAT_FEMALE) ? FEMALE
                       : NEUTRAL);
        Sprintf(eos(bp), " (%s)",
                cgend != CORPSTAT_RANDOM ? genders[mgend].adj
                                         : "unspecified gender");
    }

    if ((obj->owornmask & W_WEP) && !g.mrg_to_wielded) {
        boolean twoweap_primary = (obj == uwep && u.twoweap),
                tethered = (obj->otyp == AKLYS);


        /* use alternate phrasing for non-weapons and for wielded ammo
           (arrows, bolts), or missiles (darts, shuriken, boomerangs)
           except when those are being actively dual-wielded where the
           regular phrasing will list them as "in right hand" to
           contrast with secondary weapon's "in left hand" */
        if ((obj->quan != 1L
             || ((obj->oclass == WEAPON_CLASS)
                 ? (is_ammo(obj) || is_missile(obj))
                 : !is_weptool(obj)))
            && !twoweap_primary) {
            Strcat(bp, " (wielded)");
        } else {
            const char *hand_s = body_part(HAND);

            if (bimanual(obj))
                hand_s = makeplural(hand_s);
            /* note: Sting's glow message, if added, will insert text
               in front of "(weapon in hand)"'s closing paren */
            Sprintf(eos(bp), " (%s%s in %s%s)",
                    tethered ? "tethered " : "", /* aklys */
                    /* avoid "tethered wielded in right hand" for twoweapon */
                    (twoweap_primary && !tethered) ? "wielded" : "weapon",
                    twoweap_primary ? "right " : "", hand_s);
            if (!Blind) {
                if (g.warn_obj_cnt && obj == uwep
                    && (EWarn_of_mon & W_WEP) != 0L)
                    /* we know bp[] ends with ')'; overwrite that */
                    Sprintf(eos(bp) - 1, ", %s %s)",
                            glow_verb(g.warn_obj_cnt, TRUE),
                            glow_color(obj->oartifact));
                else if (obj->lamplit && artifact_light(obj))
                    /* as above, overwrite known closing paren */
                    Sprintf(eos(bp) - 1, ", %s lit)",
                            arti_light_description(obj));
            }
        }
    }
    if (obj->owornmask & W_SWAPWEP) {
        if (u.twoweap)
            Sprintf(eos(bp), " (wielded in left %s)", body_part(HAND));
        else
            /* TODO: rephrase this when obj isn't a weapon or weptool */
            Sprintf(eos(bp), " (alternate weapon%s; not wielded)",
                    plur(obj->quan));
    }
    if (obj->owornmask & W_QUIVER) {
        switch (obj->oclass) {
        case WEAPON_CLASS:
            if (is_ammo(obj)) {
                if (objects[obj->otyp].oc_skill == -P_BOW) {
                    /* Ammo for a bow */
                    Strcat(bp, " (in quiver)");
                    break;
                } else {
                    /* Ammo not for a bow */
                    Strcat(bp, " (in quiver pouch)");
                    break;
                }
            } else {
                /* Weapons not considered ammo */
                Strcat(bp, " (at the ready)");
                break;
            }
        /* Small things and ammo not for a bow */
        case RING_CLASS:
        case AMULET_CLASS:
        case WAND_CLASS:
        case COIN_CLASS:
        case GEM_CLASS:
            Strcat(bp, " (in quiver pouch)");
            break;
        default: /* odd things */
            Strcat(bp, " (at the ready)");
        }
    }
    /* treat 'restoring' like suppress_price because shopkeeper and
       bill might not be available yet while restore is in progress
       (objects won't normally be formatted during that time, but if
       'perm_invent' is enabled then they might be [not any more...]) */
    if (iflags.suppress_price || g.program_state.restoring) {
        ; /* don't attempt to obtain any shop pricing, even if 'with_price' */
    } else if (is_unpaid(obj)) { /* in inventory or in container in invent */
        long quotedprice = unpaid_cost(obj, TRUE);

        Sprintf(eos(bp), " (%s, %ld %s)",
                obj->unpaid ? "unpaid" : "contents",
                quotedprice, currency(quotedprice));
    } else if (with_price) { /* on floor or in container on floor */
        int nochrg = 0;
        long price = get_cost_of_shop_item(obj, &nochrg);

        if (price > 0L)
            Sprintf(eos(bp), " (%s, %ld %s)",
                    nochrg ? "contents" : "for sale",
                    price, currency(price));
        else if (nochrg > 0)
            Sprintf(eos(bp), " (no charge)");
    }
    if (!strncmp(prefix, "a ", 2)) {
        /* save current prefix, without "a "; might be empty */
        Strcpy(tmpbuf, prefix + 2);
        /* set prefix[] to "", "a ", or "an " */
        (void) just_an(prefix, *tmpbuf ? tmpbuf : bp);
        /* append remainder of original prefix */
        Strcat(prefix, tmpbuf);
    }

    /* show weight for items (debug tourist info);
       "aum" is stolen from Crawl's "Arbitrary Unit of Measure" */
    if (wizard && iflags.wizweight) {
        /* wizard mode user has asked to see object weights */
        if (with_price && (*(eos(bp)-1) == ')'))
            Sprintf(eos(bp)-1, ", %u aum)", obj->owt);
        else
            Sprintf(eos(bp), " (%u aum)", obj->owt);
    }
    bp = strprepend(bp, prefix);
    return bp;
}

char *
doname(struct obj* obj)
{
    return doname_base(obj, (unsigned) 0);
}

/* Name of object including price. */
char *
doname_with_price(struct obj* obj)
{
    return doname_base(obj, DONAME_WITH_PRICE);
}

/* "some" instead of precise quantity if obj->dknown not set */
char *
doname_vague_quan(struct obj* obj)
{
    /* Used by farlook.
     * If it hasn't been seen up close and quantity is more than one,
     * use "some" instead of the quantity: "some gold pieces" rather
     * than "25 gold pieces".  This is suboptimal, to put it mildly,
     * because lookhere and pickup report the precise amount.
     * Picking the item up while blind also shows the precise amount
     * for inventory display, then dropping it while still blind leaves
     * obj->dknown unset so the count reverts to "some" for farlook.
     *
     * TODO: add obj->qknown flag for 'quantity known' on stackable
     * items; it could overlay obj->cknown since no containers stack.
     */
    return doname_base(obj, DONAME_VAGUE_QUAN);
}

/* used from invent.c */
boolean
not_fully_identified(struct obj* otmp)
{
    /* gold doesn't have any interesting attributes [yet?] */
    if (otmp->oclass == COIN_CLASS)
        return FALSE; /* always fully ID'd */
    /* check fundamental ID hallmarks first */
    if (!otmp->known || !otmp->dknown
#ifdef MAIL_STRUCTURES
        || (!otmp->bknown && otmp->otyp != SCR_MAIL)
#else
        || !otmp->bknown
#endif
        || !objects[otmp->otyp].oc_name_known)
        return TRUE;
    if ((!otmp->cknown && (Is_container(otmp) || otmp->otyp == STATUE))
        || (!otmp->lknown && Is_box(otmp)))
        return TRUE;
    if (otmp->oartifact && undiscovered_artifact(otmp->oartifact))
        return TRUE;
    /* otmp->rknown is the only item of interest if we reach here */
    /*
     *  Note:  if a revision ever allows scrolls to become fireproof or
     *  rings to become shockproof, this checking will need to be revised.
     *  `rknown' ID only matters if xname() will provide the info about it.
     */
    if (otmp->rknown
        || (otmp->oclass != ARMOR_CLASS && otmp->oclass != WEAPON_CLASS
            && !is_weptool(otmp)            /* (redundant) */
            && otmp->oclass != BALL_CLASS)) /* (useless) */
        return FALSE;
    else /* lack of `rknown' only matters for vulnerable objects */
        return (boolean) (is_rustprone(otmp) || is_corrodeable(otmp)
                          || is_flammable(otmp));
}

/* format a corpse name (xname() omits monster type; doname() calls us);
   eatcorpse() also uses us for death reason when eating tainted glob */
char *
corpse_xname(
    struct obj *otmp,
    const char *adjective,
    unsigned cxn_flags) /* bitmask of CXN_xxx values */
{
    /* some callers [aobjnam()] rely on prefix area that xname() sets aside */
    char *nambuf = nextobuf() + PREFIX;
    int omndx = otmp->corpsenm;
    boolean ignore_quan = (cxn_flags & CXN_SINGULAR) != 0,
            /* suppress "the" from "the unique monster corpse" */
        no_prefix = (cxn_flags & CXN_NO_PFX) != 0,
            /* include "the" for "the woodchuck corpse */
        the_prefix = (cxn_flags & CXN_PFX_THE) != 0,
            /* include "an" for "an ogre corpse */
        any_prefix = (cxn_flags & CXN_ARTICLE) != 0,
            /* leave off suffix (do_name() appends "corpse" itself) */
        omit_corpse = (cxn_flags & CXN_NOCORPSE) != 0,
        possessive = FALSE,
        glob = (otmp->otyp != CORPSE && otmp->globby);
    const char *mnam;

    if (glob) {
        mnam = OBJ_NAME(objects[otmp->otyp]); /* "glob of <monster>" */
    } else if (omndx == NON_PM) { /* paranoia */
        mnam = "thing";
    } else {
        mnam = obj_pmname(otmp);
        if (the_unique_pm(&mons[omndx]) || type_is_pname(&mons[omndx])) {
            mnam = s_suffix(mnam);
            possessive = TRUE;
            /* don't precede personal name like "Medusa" with an article */
            if (type_is_pname(&mons[omndx]))
                no_prefix = TRUE;
            /* always precede non-personal unique monster name like
               "Oracle" with "the" unless explicitly overridden */
            else if (the_unique_pm(&mons[omndx]) && !no_prefix)
                the_prefix = TRUE;
        }
    }
    if (no_prefix)
        the_prefix = any_prefix = FALSE;
    else if (the_prefix)
        any_prefix = FALSE; /* mutually exclusive */

    *nambuf = '\0';
    /* can't use the() the way we use an() below because any capitalized
       Name causes it to assume a personal name and return Name as-is;
       that's usually the behavior wanted, but here we need to force "the"
       to precede capitalized unique monsters (pnames are handled above) */
    if (the_prefix)
        Strcat(nambuf, "the ");
    /* note: over time, various instances of the(mon_name()) have crept
       into the code, so the() has been modified to deal with capitalized
       monster names; we could switch to using it below like an() */

    if (!adjective || !*adjective) {
        /* normal case:  newt corpse */
        Strcat(nambuf, mnam);
    } else {
        /* adjective positioning depends upon format of monster name */
        if (possessive) /* Medusa's cursed partly eaten corpse */
            Sprintf(eos(nambuf), "%s %s", mnam, adjective);
        else /* cursed partly eaten troll corpse */
            Sprintf(eos(nambuf), "%s %s", adjective, mnam);
        /* in case adjective has a trailing space, squeeze it out */
        mungspaces(nambuf);
        /* doname() might include a count in the adjective argument;
           if so, don't prepend an article */
        if (digit(*adjective))
            any_prefix = FALSE;
    }

    if (glob) {
        ; /* omit_corpse doesn't apply; quantity is always 1 */
    } else if (!omit_corpse) {
        Strcat(nambuf, " corpse");
        /* makeplural(nambuf) => append "s" to "corpse" */
        if (otmp->quan > 1L && !ignore_quan) {
            Strcat(nambuf, "s");
            any_prefix = FALSE; /* avoid "a newt corpses" */
        }
    }

    /* it's safe to overwrite our nambuf[] after an() has copied its
       old value into another buffer; and once _that_ has been copied,
       the obuf[] returned by an() can be made available for re-use */
    if (any_prefix) {
        char *obufp;

        Strcpy(nambuf, obufp = an(nambuf));
        releaseobuf(obufp);
    }
    return nambuf;
}

/* xname doesn't include monster type for "corpse"; cxname does */
char *
cxname(struct obj* obj)
{
    if (obj->otyp == CORPSE)
        return corpse_xname(obj, (const char *) 0, CXN_NORMAL);
    return xname(obj);
}

/* like cxname, but ignores quantity */
char *
cxname_singular(struct obj* obj)
{
    if (obj->otyp == CORPSE)
        return corpse_xname(obj, (const char *) 0, CXN_SINGULAR);
    return xname_flags(obj, CXN_SINGULAR);
}

/* treat an object as fully ID'd when it might be used as reason for death */
char *
killer_xname(struct obj* obj)
{
    struct obj save_obj;
    unsigned save_ocknown;
    char *buf, *save_ocuname, *save_oname = (char *) 0;

    /* bypass object twiddling for artifacts */
    if (obj->oartifact)
        return bare_artifactname(obj);

    /* remember original settings for core of the object;
       oextra structs other than oname don't matter here--since they
       aren't modified they don't need to be saved and restored */
    save_obj = *obj;
    if (has_oname(obj))
        save_oname = ONAME(obj);

    /* killer name should be more specific than general xname; however, exact
       info like blessed/cursed and rustproof makes things be too verbose */
    obj->known = obj->dknown = 1;
    obj->bknown = obj->rknown = obj->greased = 0;
    /* if character is a priest[ess], bknown will get toggled back on */
    if (obj->otyp != POT_WATER)
        obj->blessed = obj->cursed = 0;
    else
        obj->bknown = 1; /* describe holy/unholy water as such */
    /* "killed by poisoned <obj>" would be misleading when poison is
       not the cause of death and "poisoned by poisoned <obj>" would
       be redundant when it is, so suppress "poisoned" prefix */
    obj->opoisoned = 0;
    /* strip user-supplied name; artifacts keep theirs */
    if (!obj->oartifact && save_oname)
        ONAME(obj) = (char *) 0;
    /* temporarily identify the type of object */
    save_ocknown = objects[obj->otyp].oc_name_known;
    objects[obj->otyp].oc_name_known = 1;
    save_ocuname = objects[obj->otyp].oc_uname;
    objects[obj->otyp].oc_uname = 0; /* avoid "foo called bar" */

    /* format the object */
    if (obj->otyp == CORPSE) {
        buf = corpse_xname(obj, (const char *) 0, CXN_NORMAL);
    } else if (obj->otyp == SLIME_MOLD) {
        /* concession to "most unique deaths competition" in the annual
           devnull tournament, suppress player supplied fruit names because
           those can be used to fake other objects and dungeon features */
        buf = nextobuf();
        Sprintf(buf, "deadly slime mold%s", plur(obj->quan));
    } else {
        buf = xname(obj);
    }
    /* apply an article if appropriate; caller should always use KILLED_BY */
    if (obj->quan == 1L && !strstri(buf, "'s ") && !strstri(buf, "s' "))
        buf = (obj_is_pname(obj) || the_unique_obj(obj)) ? the(buf) : an(buf);

    objects[obj->otyp].oc_name_known = save_ocknown;
    objects[obj->otyp].oc_uname = save_ocuname;
    *obj = save_obj; /* restore object's core settings */
    if (!obj->oartifact && save_oname)
        ONAME(obj) = save_oname;

    return buf;
}

/* xname,doname,&c with long results reformatted to omit some stuff */
char *
short_oname(
    struct obj *obj,
    char *(*func)(OBJ_P),    /* main formatting routine */
    char *(*altfunc)(OBJ_P), /* alternate for shortest result */
    unsigned lenlimit)
{
    struct obj save_obj;
    char unamebuf[12], onamebuf[12], *save_oname, *save_uname, *outbuf;

    outbuf = (*func)(obj);
    if ((unsigned) strlen(outbuf) <= lenlimit)
        return outbuf;

    /* shorten called string to fairly small amount */
    save_uname = objects[obj->otyp].oc_uname;
    if (save_uname && strlen(save_uname) >= sizeof unamebuf) {
        (void) strncpy(unamebuf, save_uname, sizeof unamebuf - 4);
        Strcpy(unamebuf + sizeof unamebuf - 4, "...");
        objects[obj->otyp].oc_uname = unamebuf;
        releaseobuf(outbuf);
        outbuf = (*func)(obj);
        objects[obj->otyp].oc_uname = save_uname; /* restore called string */
        if ((unsigned) strlen(outbuf) <= lenlimit)
            return outbuf;
    }

    /* shorten named string to fairly small amount */
    save_oname = has_oname(obj) ? ONAME(obj) : 0;
    if (save_oname && strlen(save_oname) >= sizeof onamebuf) {
        (void) strncpy(onamebuf, save_oname, sizeof onamebuf - 4);
        Strcpy(onamebuf + sizeof onamebuf - 4, "...");
        ONAME(obj) = onamebuf;
        releaseobuf(outbuf);
        outbuf = (*func)(obj);
        ONAME(obj) = save_oname; /* restore named string */
        if ((unsigned) strlen(outbuf) <= lenlimit)
            return outbuf;
    }

    /* shorten both called and named strings;
       unamebuf and onamebuf have both already been populated */
    if (save_uname && strlen(save_uname) >= sizeof unamebuf && save_oname
        && strlen(save_oname) >= sizeof onamebuf) {
        objects[obj->otyp].oc_uname = unamebuf;
        ONAME(obj) = onamebuf;
        releaseobuf(outbuf);
        outbuf = (*func)(obj);
        if ((unsigned) strlen(outbuf) <= lenlimit) {
            objects[obj->otyp].oc_uname = save_uname;
            ONAME(obj) = save_oname;
            return outbuf;
        }
    }

    /* still long; strip several name-lengthening attributes;
       called and named strings are still in truncated form */
    save_obj = *obj;
    obj->bknown = obj->rknown = obj->greased = 0;
    obj->oeroded = obj->oeroded2 = 0;
    releaseobuf(outbuf);
    outbuf = (*func)(obj);
    if (altfunc && (unsigned) strlen(outbuf) > lenlimit) {
        /* still long; use the alternate function (usually one of
           the jackets around minimal_xname()) */
        releaseobuf(outbuf);
        outbuf = (*altfunc)(obj);
    }
    /* restore the object */
    *obj = save_obj;
    if (save_oname)
        ONAME(obj) = save_oname;
    if (save_uname)
        objects[obj->otyp].oc_uname = save_uname;

    /* use whatever we've got, whether it's too long or not */
    return outbuf;
}

/*
 * Used if only one of a collection of objects is named (e.g. in eat.c).
 */
const char *
singular(struct obj* otmp, char* (*func)(OBJ_P))
{
    long savequan;
    char *nam;

    /* using xname for corpses does not give the monster type */
    if (otmp->otyp == CORPSE && func == xname)
        func = cxname;

    savequan = otmp->quan;
    otmp->quan = 1L;
    nam = (*func)(otmp);
    otmp->quan = savequan;
    return nam;
}

/* pick "", "a ", or "an " as article for 'str'; used by an() and doname() */
char *
just_an(char *outbuf, const char *str)
{
    char c0;

    *outbuf = '\0';
    c0 = lowc(*str);
    if (!str[1] || str[1] == ' ') {
        /* single letter; might be used for named fruit or a musical note */
        Strcpy(outbuf, strchr("aefhilmnosx", c0) ? "an " : "a ");
    } else if (!strncmpi(str, "the ", 4) || !strcmpi(str, "molten lava")
               || !strcmpi(str, "iron bars") || !strcmpi(str, "ice")) {
        ; /* no article */
    } else {
        /* normal case is "an <vowel>" or "a <consonant>" */
        if ((strchr(vowels, c0) /* some exceptions warranting "a <vowel>" */
             /* 'wun' initial sound */
             && (strncmpi(str, "one", 3) || (str[3] && !strchr("-_ ", str[3])))
             /* long 'u' initial sound */
             && strncmpi(str, "eu", 2) /* "eucalyptus leaf" */
             && strncmpi(str, "uke", 3) && strncmpi(str, "ukulele", 7)
             && strncmpi(str, "unicorn", 7) && strncmpi(str, "uranium", 7)
             && strncmpi(str, "useful", 6)) /* "useful tool" */
            || (c0 == 'x' && !strchr(vowels, lowc(str[1]))))
            Strcpy(outbuf, "an ");
        else
            Strcpy(outbuf, "a ");
    }
    return outbuf;
}

char *
an(const char* str)
{
    char *buf = nextobuf();

    if (!str || !*str) {
        impossible("Alphabet soup: 'an(%s)'.", str ? "\"\"" : "<null>");
        return strcpy(buf, "an []");
    }
    (void) just_an(buf, str);
    return strcat(buf, str);
}

char *
An(const char* str)
{
    char *tmp = an(str);

    *tmp = highc(*tmp);
    return tmp;
}

/*
 * Prepend "the" if necessary; assumes str is a subject derived from xname.
 * Use type_is_pname() for monster names, not the().  the() is idempotent.
 */
char *
the(const char* str)
{
    const char *aname;
    char *buf = nextobuf();
    boolean insert_the = FALSE;

    if (!str || !*str) {
        impossible("Alphabet soup: 'the(%s)'.", str ? "\"\"" : "<null>");
        return strcpy(buf, "the []");
    }
    if (!strncmpi(str, "the ", 4)) {
        buf[0] = lowc(*str);
        Strcpy(&buf[1], str + 1);
        return buf;
    } else if (*str < 'A' || *str > 'Z'
               /* some capitalized monster names want "the", others don't */
               || CapitalMon(str)
               /* treat named fruit as not a proper name, even if player
                  has assigned a capitalized proper name as his/her fruit,
                  unless it matches an artifact name */
               || (fruit_from_name(str, TRUE, (int *) 0)
                   && ((aname = artifact_name(str, (short *) 0, FALSE)) == 0
                       || strncmpi(aname, "the ", 4) == 0))) {
        /* not a proper name, needs an article */
        insert_the = TRUE;
    } else {
        /* Probably a proper name, might not need an article */
        register char *tmp, *named, *called;
        int l;

        /* some objects have capitalized adjectives in their names */
        if (((tmp = strrchr(str, ' ')) != 0 || (tmp = strrchr(str, '-')) != 0)
            && (tmp[1] < 'A' || tmp[1] > 'Z')) {
            /* insert "the" unless we have an apostrophe (where we assume
               we're dealing with "Unique's corpse" when "Unique" wasn't
               caught by CapitalMon() above) */
            insert_the = !strchr(str, '\'');
        } else if (tmp && strchr(str, ' ') < tmp) { /* has spaces */
            /* it needs an article if the name contains "of" */
            tmp = strstri(str, " of ");
            named = strstri(str, " named ");
            called = strstri(str, " called ");
            if (called && (!named || called < named))
                named = called;

            if (tmp && (!named || tmp < named)) /* found an "of" */
                insert_the = TRUE;
            /* stupid special case: lacks "of" but needs "the" */
            else if (!named && (l = Strlen(str)) >= 31
                     && !strcmp(&str[l - 31],
                                "Platinum Yendorian Express Card"))
                insert_the = TRUE;
        }
    }
    if (insert_the)
        Strcpy(buf, "the ");
    else
        buf[0] = '\0';
    Strcat(buf, str);

    return buf;
}

char *
The(const char *str)
{
    char *tmp = the(str);

    *tmp = highc(*tmp);
    return tmp;
}

/* returns "count cxname(otmp)" or just cxname(otmp) if count == 1 */
char *
aobjnam(struct obj *otmp, const char *verb)
{
    char prefix[PREFIX];
    char *bp = cxname(otmp);

    if (otmp->quan != 1L) {
        Sprintf(prefix, "%ld ", otmp->quan);
        bp = strprepend(bp, prefix);
    }
    if (verb) {
        Strcat(bp, " ");
        Strcat(bp, otense(otmp, verb));
    }
    return bp;
}

/* combine yname and aobjnam eg "your count cxname(otmp)" */
char *
yobjnam(struct obj* obj, const char *verb)
{
    char *s = aobjnam(obj, verb);

    /* leave off "your" for most of your artifacts, but prepend
     * "your" for unique objects and "foo of bar" quest artifacts */
    if (!carried(obj) || !obj_is_pname(obj)
        || obj->oartifact >= ART_ORB_OF_DETECTION) {
        char *outbuf = shk_your(nextobuf(), obj);
        int space_left = BUFSZ - 1 - Strlen(outbuf);

        s = strncat(outbuf, s, space_left);
    }
    return s;
}

/* combine Yname2 and aobjnam eg "Your count cxname(otmp)" */
char *
Yobjnam2(struct obj* obj, const char *verb)
{
    register char *s = yobjnam(obj, verb);

    *s = highc(*s);
    return s;
}

/* like aobjnam, but prepend "The", not count, and use xname */
char *
Tobjnam(struct obj* otmp, const char *verb)
{
    char *bp = The(xname(otmp));

    if (verb) {
        Strcat(bp, " ");
        Strcat(bp, otense(otmp, verb));
    }
    return bp;
}

/* capitalized variant of doname() */
char *
Doname2(struct obj* obj)
{
    char *s = doname(obj);

    *s = highc(*s);
    return s;
}

#if 0 /* stalled-out work in progress */
/* Doname2() for itemized buying of 'obj' from a shop */
char *
payDoname(struct obj* obj)
{
    static const char and_contents[] = " and its contents";
    char *p = doname(obj);

    if (Is_container(obj) && !obj->cknown) {
        if (obj->unpaid) {
            if ((int) strlen(p) + sizeof and_contents - 1 < BUFSZ - PREFIX)
                Strcat(p, and_contents);
            *p = highc(*p);
        } else {
            p = strprepend(p, "Contents of ");
        }
    } else {
        *p = highc(*p);
    }
    return p;
}
#endif /*0*/

/* returns "[your ]xname(obj)" or "Foobar's xname(obj)" or "the xname(obj)" */
char *
yname(struct obj* obj)
{
    char *s = cxname(obj);

    /* leave off "your" for most of your artifacts, but prepend
     * "your" for unique objects and "foo of bar" quest artifacts */
    if (!carried(obj) || !obj_is_pname(obj)
        || obj->oartifact >= ART_ORB_OF_DETECTION) {
        char *outbuf = shk_your(nextobuf(), obj);
        int space_left = BUFSZ - 1 - Strlen(outbuf);

        s = strncat(outbuf, s, space_left);
    }

    return s;
}

/* capitalized variant of yname() */
char *
Yname2(struct obj* obj)
{
    char *s = yname(obj);

    *s = highc(*s);
    return s;
}

/* returns "your minimal_xname(obj)"
 * or "Foobar's minimal_xname(obj)"
 * or "the minimal_xname(obj)"
 */
char *
ysimple_name(struct obj* obj)
{
    char *outbuf = nextobuf();
    char *s = shk_your(outbuf, obj); /* assert( s == outbuf ); */
    int space_left = BUFSZ - 1 - Strlen(s);

    return strncat(s, minimal_xname(obj), space_left);
}

/* capitalized variant of ysimple_name() */
char *
Ysimple_name2(struct obj* obj)
{
    char *s = ysimple_name(obj);

    *s = highc(*s);
    return s;
}

/* "scroll" or "scrolls" */
char *
simpleonames(struct obj* obj)
{
    char *obufp, *simpleoname = minimal_xname(obj);

    if (obj->quan != 1L) {
        /* 'simpleoname' points to an obuf; makeplural() will allocate
           another one and only that one can be explicitly released for
           re-use, so this is slightly convoluted to cope with that;
           makeplural() will be fully evaluated and done with its input
           argument before strcpy() touches its output argument */
        Strcpy(simpleoname, obufp = makeplural(simpleoname));
        releaseobuf(obufp);
    }
    return simpleoname;
}

/* "a scroll" or "scrolls"; "a silver bell" or "the Bell of Opening" */
char *
ansimpleoname(struct obj* obj)
{
    char *obufp, *simpleoname = simpleonames(obj);
    int otyp = obj->otyp;

    /* prefix with "the" if a unique item, or a fake one imitating same,
       has been formatted with its actual name (we let typename() handle
       any `known' and `dknown' checking necessary) */
    if (otyp == FAKE_AMULET_OF_YENDOR)
        otyp = AMULET_OF_YENDOR;
    if (objects[otyp].oc_unique
        && !strcmp(simpleoname, OBJ_NAME(objects[otyp]))) {
        /* the() will allocate another obuf[]; we want to avoid using two */
        Strcpy(simpleoname, obufp = the(simpleoname));
        releaseobuf(obufp);
    } else if (obj->quan == 1L) {
        /* simpleoname[] is singular if quan==1, plural otherwise;
           an() will allocate another obuf[]; we want to avoid using two */
        Strcpy(simpleoname, obufp = an(simpleoname));
        releaseobuf(obufp);
    }
    return simpleoname;
}

/* "the scroll" or "the scrolls" */
char *
thesimpleoname(struct obj *obj)
{
    char *obufp, *simpleoname = simpleonames(obj);

    /* the() will allocate another obuf[]; we want to avoid using two */
    Strcpy(simpleoname, obufp = the(simpleoname));
    releaseobuf(obufp);
    return simpleoname;
}

/* basic name of obj, as if it has been discovered; for some types of
   items, we can't just use OBJ_NAME() because it doesn't always include
   the class (for instance "light" when we want "spellbook of light");
   minimal_xname() uses xname() to get that */
char *
actualoname(struct obj *obj)
{
    char *res;

    iflags.override_ID = TRUE;
    res = minimal_xname(obj);
    iflags.override_ID = FALSE;
    return res;
}

/* artifact's name without any object type or known/dknown/&c feedback */
char *
bare_artifactname(struct obj *obj)
{
    char *outbuf;

    if (obj->oartifact) {
        outbuf = nextobuf();
        Strcpy(outbuf, artiname(obj->oartifact));
        if (!strncmp(outbuf, "The ", 4))
            outbuf[0] = lowc(outbuf[0]);
    } else {
        outbuf = xname(obj);
    }
    return outbuf;
}

static const char *const wrp[] = {
    "wand",   "ring",      "potion",     "scroll", "gem",
    "amulet", "spellbook", "spell book",
    /* for non-specific wishes */
    "weapon", "armor",     "tool",       "food",   "comestible",
};
static const char wrpsym[] = { WAND_CLASS,   RING_CLASS,   POTION_CLASS,
                               SCROLL_CLASS, GEM_CLASS,    AMULET_CLASS,
                               SPBOOK_CLASS, SPBOOK_CLASS, WEAPON_CLASS,
                               ARMOR_CLASS,  TOOL_CLASS,   FOOD_CLASS,
                               FOOD_CLASS };

/* return form of the verb (input plural) if xname(otmp) were the subject */
char *
otense(struct obj* otmp,const char * verb)
{
    char *buf;

    /*
     * verb is given in plural (without trailing s).  Return as input
     * if the result of xname(otmp) would be plural.  Don't bother
     * recomputing xname(otmp) at this time.
     */
    if (!is_plural(otmp))
        return vtense((char *) 0, verb);

    buf = nextobuf();
    Strcpy(buf, verb);
    return buf;
}

/* various singular words that vtense would otherwise categorize as plural;
   also used by makesingular() to catch some special cases */
static const char *const special_subjs[] = {
    "erinys",  "manes", /* this one is ambiguous */
    "Cyclops", "Hippocrates",     "Pelias",    "aklys",
    "amnesia", "detect monsters", "paralysis", "shape changers",
    "nemesis", 0
    /* note: "detect monsters" and "shape changers" are normally
       caught via "<something>(s) of <whatever>", but they can be
       wished for using the shorter form, so we include them here
       to accommodate usage by makesingular during wishing */
};

/* return form of the verb (input plural) for present tense 3rd person subj */
char *
vtense(const char* subj, const char* verb)
{
    char *buf = nextobuf(), *bspot;
    int len, ltmp;
    const char *sp, *spot;
    const char *const *spec;

    /*
     * verb is given in plural (without trailing s).  Return as input
     * if subj appears to be plural.  Add special cases as necessary.
     * Many hard cases can already be handled by using otense() instead.
     * If this gets much bigger, consider decomposing makeplural.
     * Note: monster names are not expected here (except before corpse).
     *
     * Special case: allow null sobj to get the singular 3rd person
     * present tense form so we don't duplicate this code elsewhere.
     */
    if (subj) {
        if (!strncmpi(subj, "a ", 2) || !strncmpi(subj, "an ", 3))
            goto sing;
        spot = (const char *) 0;
        for (sp = subj; (sp = strchr(sp, ' ')) != 0; ++sp) {
            if (!strncmpi(sp, " of ", 4) || !strncmpi(sp, " from ", 6)
                || !strncmpi(sp, " called ", 8) || !strncmpi(sp, " named ", 7)
                || !strncmpi(sp, " labeled ", 9)) {
                if (sp != subj)
                    spot = sp - 1;
                break;
            }
        }
        len = (int) strlen(subj);
        if (!spot)
            spot = subj + len - 1;

        /*
         * plural: anything that ends in 's', but not '*us' or '*ss'.
         * Guess at a few other special cases that makeplural creates.
         */
        if ((lowc(*spot) == 's' && spot != subj
             && !strchr("us", lowc(*(spot - 1))))
            || !BSTRNCMPI(subj, spot - 3, "eeth", 4)
            || !BSTRNCMPI(subj, spot - 3, "feet", 4)
            || !BSTRNCMPI(subj, spot - 1, "ia", 2)
            || !BSTRNCMPI(subj, spot - 1, "ae", 2)) {
            /* check for special cases to avoid false matches */
            len = (int) (spot - subj) + 1;
            for (spec = special_subjs; *spec; spec++) {
                ltmp = Strlen(*spec);
                if (len == ltmp && !strncmpi(*spec, subj, len))
                    goto sing;
                /* also check for <prefix><space><special_subj>
                   to catch things like "the invisible erinys" */
                if (len > ltmp && *(spot - ltmp) == ' '
                    && !strncmpi(*spec, spot - ltmp + 1, ltmp))
                    goto sing;
            }

            return strcpy(buf, verb);
        }
        /*
         * 3rd person plural doesn't end in telltale 's';
         * 2nd person singular behaves as if plural.
         */
        if (!strcmpi(subj, "they") || !strcmpi(subj, "you"))
            return strcpy(buf, verb);
    }

 sing:
    Strcpy(buf, verb);
    len = (int) strlen(buf);
    bspot = buf + len - 1;

    if (!strcmpi(buf, "are")) {
        Strcasecpy(buf, "is");
    } else if (!strcmpi(buf, "have")) {
        Strcasecpy(bspot - 1, "s");
    } else if (strchr("zxs", lowc(*bspot))
               || (len >= 2 && lowc(*bspot) == 'h'
                   && strchr("cs", lowc(*(bspot - 1))))
               || (len == 2 && lowc(*bspot) == 'o')) {
        /* Ends in z, x, s, ch, sh; add an "es" */
        Strcasecpy(bspot + 1, "es");
    } else if (lowc(*bspot) == 'y' && !strchr(vowels, lowc(*(bspot - 1)))) {
        /* like "y" case in makeplural */
        Strcasecpy(bspot, "ies");
    } else {
        Strcasecpy(bspot + 1, "s");
    }

    return buf;
}

struct sing_plur {
    const char *sing, *plur;
};

/* word pairs that don't fit into formula-based transformations;
   also some suffices which have very few--often one--matches or
   which aren't systematically reversible (knives, staves) */
static const struct sing_plur one_off[] = {
    { "child",
      "children" },      /* (for wise guys who give their food funny names) */
    { "cubus", "cubi" }, /* in-/suc-cubus */
    { "culus", "culi" }, /* homunculus */
    { "djinni", "djinn" },
    { "erinys", "erinyes" },
    { "foot", "feet" },
    { "fungus", "fungi" },
    { "goose", "geese" },
    { "knife", "knives" },
    { "labrum", "labra" }, /* candelabrum */
    { "louse", "lice" },
    { "mouse", "mice" },
    { "mumak", "mumakil" },
    { "nemesis", "nemeses" },
    { "ovum", "ova" },
    { "ox", "oxen" },
    { "passerby", "passersby" },
    { "rtex", "rtices" }, /* vortex */
    { "serum", "sera" },
    { "staff", "staves" },
    { "tooth", "teeth" },
    { 0, 0 }
};

static const char *const as_is[] = {
    /* makesingular() leaves these plural due to how they're used */
    "boots",   "shoes",     "gloves",    "lenses",   "scales",
    "eyes",    "gauntlets", "iron bars",
    /* both singular and plural are spelled the same */
    "bison",   "deer",      "elk",       "fish",      "fowl",
    "tuna",    "yaki",      "-hai",      "krill",     "manes",
    "moose",   "ninja",     "sheep",     "ronin",     "roshi",
    "shito",   "tengu",     "ki-rin",    "Nazgul",    "gunyoki",
    "piranha", "samurai",   "shuriken",  "haggis",    "Bordeaux",
    0,
    /* Note:  "fish" and "piranha" are collective plurals, suitable
       for "wiped out all <foo>".  For "3 <foo>", they should be
       "fishes" and "piranhas" instead.  We settle for collective
       variant instead of attempting to support both. */
};

/* singularize/pluralize decisions common to both makesingular & makeplural */
static boolean
singplur_lookup(
char *basestr, char *endstring,    /* base string, pointer to eos(string) */
boolean to_plural,            /* true => makeplural, false => makesingular */
const char *const *alt_as_is) /* another set like as_is[] */
{
    const struct sing_plur *sp;
    const char *same, *other, *const *as;
    int al;
    int baselen = Strlen(basestr);

    for (as = as_is; *as; ++as) {
        al = (int) strlen(*as);
        if (!BSTRCMPI(basestr, endstring - al, *as))
            return TRUE;
    }
    if (alt_as_is) {
        for (as = alt_as_is; *as; ++as) {
            al = (int) strlen(*as);
            if (!BSTRCMPI(basestr, endstring - al, *as))
                return TRUE;
        }
    }

   /* Leave "craft" as a suffix as-is (aircraft, hovercraft);
      "craft" itself is (arguably) not included in our likely context */
   if ((baselen > 5) && (!BSTRCMPI(basestr, endstring - 5, "craft")))
       return TRUE;
   /* avoid false hit on one_off[].plur == "lice" or .sing == "goose";
       if more of these turn up, one_off[] entries will need to flagged
       as to which are whole words and which are matchable as suffices
       then matching in the loop below will end up becoming more complex */
    if (!strcmpi(basestr, "slice")
        || !strcmpi(basestr, "mongoose")) {
        if (to_plural)
            Strcasecpy(endstring, "s");
        return TRUE;
    }
    /* skip "ox" -> "oxen" entry when pluralizing "<something>ox"
       unless it is muskox */
    if (to_plural && baselen > 2 && !strcmpi(endstring - 2, "ox")
        && !(baselen > 5 && !strcmpi(endstring - 6, "muskox"))) {
        /* "fox" -> "foxes" */
        Strcasecpy(endstring, "es");
        return TRUE;
    }
    if (to_plural) {
        if (baselen > 2 && !strcmpi(endstring - 3, "man")
            && badman(basestr, to_plural)) {
            Strcasecpy(endstring, "s");
            return TRUE;
        }
    } else {
        if (baselen > 2 && !strcmpi(endstring - 3, "men")
            && badman(basestr, to_plural))
            return TRUE;
    }
    for (sp = one_off; sp->sing; sp++) {
        /* check whether endstring already matches */
        same = to_plural ? sp->plur : sp->sing;
        al = (int) strlen(same);
        if (!BSTRCMPI(basestr, endstring - al, same))
            return TRUE; /* use as-is */
        /* check whether it matches the inverse; if so, transform it */
        other = to_plural ? sp->sing : sp->plur;
        al = (int) strlen(other);
        if (!BSTRCMPI(basestr, endstring - al, other)) {
            Strcasecpy(endstring - al, same);
            return TRUE; /* one_off[] transformation */
        }
    }
    return FALSE;
}

/* searches for common compounds, ex. lump of royal jelly */
static char *
singplur_compound(char *str)
{
    /* if new entries are added, be sure to keep compound_start[] in sync */
    static const char *const compounds[] =
        {
          " of ",     " labeled ", " called ",
          " named ",  " above", /* lurkers above */
          " versus ", " from ",    " in ",
          " on ",     " a la ",    " with", /* " with "? */
          " de ",     " d'",       " du ",
          " au ",     "-in-",      "-at-",
          0
        }, /* list of first characters for all compounds[] entries */
        compound_start[] = " -";

    const char *const *cmpd;
    char *p;

    for (p = str; *p; ++p) {
        /* substring starting at p can only match if *p is found
           within compound_start[] */
        if (!strchr(compound_start, *p))
            continue;

        /* check current substring against all words in the compound[] list */
        for (cmpd = compounds; *cmpd; ++cmpd)
            if (!strncmpi(p, *cmpd, (int) strlen(*cmpd)))
                return p;
    }
    /* wasn't recognized as a compound phrase */
    return 0;
}

/* Plural routine; once upon a time it may have been chiefly used for
 * user-defined fruits, but it is now used extensively throughout the
 * program.
 *
 * For fruit, we have to try to account for everything reasonable the
 * player has; something unreasonable can still break the code.
 * However, it's still a lot more accurate than "just add an 's' at the
 * end", which Rogue uses...
 *
 * Also used for plural monster names ("Wiped out all homunculi." or the
 * vanquished monsters list) and body parts.  A lot of unique monsters have
 * names which get mangled by makeplural and/or makesingular.  They're not
 * genocidable, and vanquished-mon handling does its own special casing
 * (for uniques who've been revived and re-killed), so we don't bother
 * trying to get those right here.
 *
 * Also misused by muse.c to convert 1st person present verbs to 2nd person.
 * 3.6.0: made case-insensitive.
 */
char *
makeplural(const char* oldstr)
{
    register char *spot;
    char lo_c, *str = nextobuf();
    const char *excess = (char *) 0;
    int len, i;

    if (oldstr)
        while (*oldstr == ' ')
            oldstr++;
    if (!oldstr || !*oldstr) {
        impossible("plural of null?");
        Strcpy(str, "s");
        return str;
    }
    /* makeplural() is sometimes used on monsters rather than objects
       and sometimes pronouns are used for monsters, so check those;
       unfortunately, "her" (which matches genders[1].him and [1].his)
       and "it" (which matches genders[2].he and [2].him) are ambiguous;
       we'll live with that; caller can fix things up if necessary */
    *str = '\0';
    for (i = 0; i <= 2; ++i) {
        if (!strcmpi(genders[i].he, oldstr))
            Strcpy(str, genders[3].he); /* "they" */
        else if (!strcmpi(genders[i].him, oldstr))
            Strcpy(str, genders[3].him); /* "them" */
        else if (!strcmpi(genders[i].his, oldstr))
            Strcpy(str, genders[3].his); /* "their" */
        if (*str) {
            if (oldstr[0] == highc(oldstr[0]))
                str[0] = highc(str[0]);
            return str;
        }
    }

    Strcpy(str, oldstr);

    /*
     * Skip changing "pair of" to "pairs of".  According to Webster, usual
     * English usage is use pairs for humans, e.g. 3 pairs of dancers,
     * and pair for objects and non-humans, e.g. 3 pair of boots.  We don't
     * refer to pairs of humans in this game so just skip to the bottom.
     */
    if (!strncmpi(str, "pair of ", 8))
        goto bottom;

    /* look for "foo of bar" so that we can focus on "foo" */
    if ((spot = singplur_compound(str)) != 0) {
        excess = oldstr + (int) (spot - str);
        *spot = '\0';
    } else
        spot = eos(str);

    spot--;
    while (spot > str && *spot == ' ')
        spot--; /* Strip blanks from end */
    *(spot + 1) = '\0';
    /* Now spot is the last character of the string */

    len = Strlen(str);

    /* Single letters */
    if (len == 1 || !letter(*spot)) {
        Strcpy(spot + 1, "'s");
        goto bottom;
    }

    /* dispense with some words which don't need pluralization */
    {
        static const char *const already_plural[] = {
            "ae",  /* algae, larvae, &c */
            "eaux", /* chateaux, gateaux */
            "matzot", 0,
        };

        /* spot+1: synch up with makesingular's usage */
        if (singplur_lookup(str, spot + 1, TRUE, already_plural))
            goto bottom;

        /* more of same, but not suitable for blanket loop checking */
        if ((len == 2 && !strcmpi(str, "ya"))
            || (len >= 3 && !strcmpi(spot - 2, " ya")))
            goto bottom;
    }

    /* man/men ("Wiped out all cavemen.") */
    if (len >= 3 && !strcmpi(spot - 2, "man")
        /* exclude shamans and humans etc */
        && !badman(str, TRUE)) {
        Strcasecpy(spot - 1, "en");
        goto bottom;
    }
    if (lowc(*spot) == 'f') { /* (staff handled via one_off[]) */
        lo_c = lowc(*(spot - 1));
        if (len >= 3 && !strcmpi(spot - 2, "erf")) {
            /* avoid "nerf" -> "nerves", "serf" -> "serves" */
            ; /* fall through to default (append 's') */
        } else if (strchr("lr", lo_c) || strchr(vowels, lo_c)) {
            /* [aeioulr]f to [aeioulr]ves */
            Strcasecpy(spot, "ves");
            goto bottom;
        }
    }
    /* ium/ia (mycelia, baluchitheria) */
    if (len >= 3 && !strcmpi(spot - 2, "ium")) {
        Strcasecpy(spot - 2, "ia");
        goto bottom;
    }
    /* algae, larvae, hyphae (another fungus part) */
    if ((len >= 4 && !strcmpi(spot - 3, "alga"))
        || (len >= 5
            && (!strcmpi(spot - 4, "hypha") || !strcmpi(spot - 4, "larva")))
        || (len >= 6 && !strcmpi(spot - 5, "amoeba"))
        || (len >= 8 && (!strcmpi(spot - 7, "vertebra")))) {
        /* a to ae */
        Strcasecpy(spot + 1, "e");
        goto bottom;
    }
    /* fungus/fungi, homunculus/homunculi, but buses, lotuses, wumpuses */
    if (len > 3 && !strcmpi(spot - 1, "us")
        && !((len >= 5 && !strcmpi(spot - 4, "lotus"))
             || (len >= 6 && !strcmpi(spot - 5, "wumpus")))) {
        Strcasecpy(spot - 1, "i");
        goto bottom;
    }
    /* sis/ses (nemesis) */
    if (len >= 3 && !strcmpi(spot - 2, "sis")) {
        Strcasecpy(spot - 1, "es");
        goto bottom;
    }
    /* -eau/-eaux (gateau, chapeau...) */
    if (len >= 3 && !strcmpi(spot - 2, "eau")
        /* 'bureaus' is the more common plural of 'bureau' */
        && BSTRCMPI(str, spot - 5, "bureau")) {
        Strcasecpy(spot + 1, "x");
        goto bottom;
    }
    /* matzoh/matzot, possible food name */
    if (len >= 6
        && (!strcmpi(spot - 5, "matzoh") || !strcmpi(spot - 5, "matzah"))) {
        Strcasecpy(spot - 1, "ot"); /* oh/ah -> ot */
        goto bottom;
    }
    if (len >= 5
        && (!strcmpi(spot - 4, "matzo") || !strcmpi(spot - 4, "matza"))) {
        Strcasecpy(spot, "ot"); /* o/a -> ot */
        goto bottom;
    }

    /* note: ox/oxen, VAX/VAXen, goose/geese */

    lo_c = lowc(*spot);

    /* codex/spadix/neocortex and the like */
    if (len >= 5
        && (!strcmpi(spot - 2, "dex")
            ||!strcmpi(spot - 2, "dix")
            ||!strcmpi(spot - 2, "tex"))
           /* indices would have been ok too, but stick with indexes */
        && (strcmpi(spot - 4,"index") != 0)) {
        Strcasecpy(spot - 1, "ices"); /* ex|ix -> ices */
        goto bottom;
    }
    /* Ends in z, x, s, ch, sh; add an "es" */
    if (strchr("zxs", lo_c)
        || (len >= 2 && lo_c == 'h' && strchr("cs", lowc(*(spot - 1)))
            /* 21st century k-sound */
            && !(len >= 4 && lowc(*(spot - 1)) == 'c' && ch_ksound(str)))
        /* Kludge to get "tomatoes" and "potatoes" right */
        || (len >= 4 && !strcmpi(spot - 2, "ato"))
        || (len >= 5 && !strcmpi(spot - 4, "dingo"))) {
        Strcasecpy(spot + 1, "es"); /* append es */
        goto bottom;
    }
    /* Ends in y preceded by consonant (note: also "qu") change to "ies" */
    if (lo_c == 'y' && !strchr(vowels, lowc(*(spot - 1)))) {
        Strcasecpy(spot, "ies"); /* y -> ies */
        goto bottom;
    }
    /* Default: append an 's' */
    Strcasecpy(spot + 1, "s");

 bottom:
    if (excess)
        Strcat(str, excess);
    return str;
}

/*
 * Singularize a string the user typed in; this helps reduce the complexity
 * of readobjnam, and is also used in pager.c to singularize the string
 * for which help is sought.
 *
 * "Manes" is ambiguous: monster type (keep s), or horse body part (drop s)?
 * Its inclusion in as_is[]/special_subj[] makes it get treated as the former.
 *
 * A lot of unique monsters have names ending in s; plural, or singular
 * from plural, doesn't make much sense for them so we don't bother trying.
 * 3.6.0: made case-insensitive.
 */
char *
makesingular(const char* oldstr)
{
    register char *p, *bp;
    const char *excess = 0;
    char *str = nextobuf();

    if (oldstr)
        while (*oldstr == ' ')
            oldstr++;
    if (!oldstr || !*oldstr) {
        impossible("singular of null?");
        str[0] = '\0';
        return str;
    }
    /* makeplural() of pronouns isn't reversible but at least we can
       force a singular value */
    *str = '\0';
    if (!strcmpi(genders[3].he, oldstr)) /* "they" */
        Strcpy(str, genders[2].he); /* "it" */
    else if (!strcmpi(genders[3].him, oldstr)) /* "them" */
        Strcpy(str, genders[2].him); /* also "it" */
    else if (!strcmpi(genders[3].his, oldstr)) /* "their" */
        Strcpy(str, genders[2].his); /* "its" */
    if (*str) {
        if (oldstr[0] == highc(oldstr[0]))
            str[0] = highc(str[0]);
        return str;
    }

    bp = strcpy(str, oldstr);

    /* check for "foo of bar" so that we can focus on "foo" */
    if ((p = singplur_compound(bp)) != 0) {
        excess = oldstr + (int) (p - bp);
        *p = '\0';
    } else
        p = eos(bp);

    /* dispense with some words which don't need singularization */
    if (singplur_lookup(bp, p, FALSE, special_subjs))
        goto bottom;

    /* remove -s or -es (boxes) or -ies (rubies) */
    if (p >= bp + 1 && lowc(p[-1]) == 's') {
        if (p >= bp + 2 && lowc(p[-2]) == 'e') {
            if (p >= bp + 3 && lowc(p[-3]) == 'i') { /* "ies" */
                if (!BSTRCMPI(bp, p - 7, "cookies")
                    || (!BSTRCMPI(bp, p - 4, "pies")
                        /* avoid false match for "harpies" */
                        && (p - 4 == bp || p[-5] == ' '))
                    /* alternate djinni/djinn spelling; not really needed */
                    || (!BSTRCMPI(bp, p - 6, "genies")
                        /* avoid false match for "progenies" */
                        && (p - 6 == bp || p[-7] == ' '))
                    || !BSTRCMPI(bp, p - 5, "mbies") /* zombie */
                    || !BSTRCMPI(bp, p - 5, "yries")) /* valkyrie */
                    goto mins;
                Strcasecpy(p - 3, "y"); /* ies -> y */
                goto bottom;
            }
            /* wolves, but f to ves isn't fully reversible */
            if (p - 4 >= bp && (strchr("lr", lowc(*(p - 4)))
                                || strchr(vowels, lowc(*(p - 4))))
                && !BSTRCMPI(bp, p - 3, "ves")) {
                if (!BSTRCMPI(bp, p - 6, "cloves")
                    || !BSTRCMPI(bp, p - 6, "nerves"))
                    goto mins;
                Strcasecpy(p - 3, "f"); /* ves -> f */
                goto bottom;
            }
            /* note: nurses, axes but boxes, wumpuses */
            if (!BSTRCMPI(bp, p - 4, "eses")
                || !BSTRCMPI(bp, p - 4, "oxes") /* boxes, foxes */
                || !BSTRCMPI(bp, p - 4, "nxes") /* lynxes */
                || !BSTRCMPI(bp, p - 4, "ches")
                || !BSTRCMPI(bp, p - 4, "uses") /* lotuses */
                || !BSTRCMPI(bp, p - 4, "shes") /* splashes [of venom] */
                || !BSTRCMPI(bp, p - 4, "sses") /* priestesses */
                || !BSTRCMPI(bp, p - 5, "atoes") /* tomatoes */
                || !BSTRCMPI(bp, p - 7, "dingoes")
                || !BSTRCMPI(bp, p - 7, "Aleaxes")) {
                *(p - 2) = '\0'; /* drop es */
                goto bottom;
            } /* else fall through to mins */

            /* ends in 's' but not 'es' */
        } else if (!BSTRCMPI(bp, p - 2, "us")) { /* lotus, fungus... */
            if (BSTRCMPI(bp, p - 6, "tengus") /* but not these... */
                && BSTRCMPI(bp, p - 7, "hezrous"))
                goto bottom;
        } else if (!BSTRCMPI(bp, p - 2, "ss")
                   || !BSTRCMPI(bp, p - 5, " lens")
                   || (p - 4 == bp && !strcmpi(p - 4, "lens"))) {
            goto bottom;
        }
 mins:
        *(p - 1) = '\0'; /* drop s */

    } else { /* input doesn't end in 's' */

        if (!BSTRCMPI(bp, p - 3, "men")
            && !badman(bp, FALSE)) {
            Strcasecpy(p - 2, "an");
            goto bottom;
        }
        /* matzot -> matzo, algae -> alga */
        if (!BSTRCMPI(bp, p - 6, "matzot") || !BSTRCMPI(bp, p - 2, "ae")
            || !BSTRCMPI(bp, p - 4, "eaux")) {
            *(p - 1) = '\0'; /* drop t/e/x */
            goto bottom;
        }
        /* balactheria -> balactherium */
        if (p - 4 >= bp && !strcmpi(p - 2, "ia")
            && strchr("lr", lowc(*(p - 3))) && lowc(*(p - 4)) == 'e') {
            Strcasecpy(p - 1, "um"); /* a -> um */
        }

        /* here we cannot find the plural suffix */
    }

 bottom:
    /* if we stripped off a suffix (" of bar" from "foo of bar"),
       put it back now [strcat() isn't actually 100% safe here...] */
    if (excess)
        Strcat(bp, excess);

    return bp;
}


static boolean
ch_ksound(const char *basestr)
{
    /* these are some *ch words/suffixes that make a k-sound. They pluralize by
       adding 's' rather than 'es' */
    static const char *const ch_k[] = {
        "monarch",     "poch",    "tech",     "mech",      "stomach", "psych",
        "amphibrach",  "anarch",  "atriarch", "azedarach", "broch",
        "gastrotrich", "isopach", "loch",     "oligarch",  "peritrich",
        "sandarach",   "sumach",  "symposiarch",
    };
    int i, al;
    const char *endstr;

    if (!basestr || strlen(basestr) < 4)
        return FALSE;

    endstr = eos((char *) basestr);
    for (i = 0; i < SIZE(ch_k); i++) {
        al = (int) strlen(ch_k[i]);
        if (!BSTRCMPI(basestr, endstr - al, ch_k[i]))
            return TRUE;
    }
    return FALSE;
}

static boolean
badman(
    const char *basestr,
    boolean to_plural)  /* True: makeplural, False: makesingular */
{
    /* these are all the prefixes for *man that don't have a *men plural */
    static const char *const no_men[] = {
        "albu", "antihu", "anti", "ata", "auto", "bildungsro", "cai", "cay",
        "ceru", "corner", "decu", "des", "dura", "fir", "hanu", "het",
        "infrahu", "inhu", "nonhu", "otto", "out", "prehu", "protohu",
        "subhu", "superhu", "talis", "unhu", "sha",
        "hu", "un", "le", "re", "so", "to", "at", "a",
    };
    /* these are all the prefixes for *men that don't have a *man singular */
    static const char *const no_man[] = {
        "abdo", "acu", "agno", "ceru", "cogno", "cycla", "fleh", "grava",
        "hegu", "preno", "sonar", "speci", "dai", "exa", "fla", "sta", "teg",
        "tegu", "vela", "da", "hy", "lu", "no", "nu", "ra", "ru", "se", "vi",
        "ya", "o", "a",
    };
    int i, al;
    const char *endstr, *spot;

    if (!basestr || strlen(basestr) < 4)
        return FALSE;

    endstr = eos((char *) basestr);

    if (to_plural) {
        for (i = 0; i < SIZE(no_men); i++) {
            al = (int) strlen(no_men[i]);
            spot = endstr - (al + 3);
            if (!BSTRNCMPI(basestr, spot, no_men[i], al)
                && (spot == basestr || *(spot - 1) == ' '))
                return TRUE;
        }
    } else {
        for (i = 0; i < SIZE(no_man); i++) {
            al = (int) strlen(no_man[i]);
            spot = endstr - (al + 3);
            if (!BSTRNCMPI(basestr, spot, no_man[i], al)
                && (spot == basestr || *(spot - 1) == ' '))
                return TRUE;
        }
    }
    return FALSE;
}

/* compare user string against object name string using fuzzy matching */
static boolean
wishymatch(
    const char *u_str,      /* from user, so might be variant spelling */
    const char *o_str,      /* from objects[], so is in canonical form */
    boolean retry_inverted) /* optional extra "of" handling */
{
    static NEARDATA const char detect_SP[] = "detect ",
                               SP_detection[] = " detection";
    char *p, buf[BUFSZ];

    /* ignore spaces & hyphens and upper/lower case when comparing */
    if (fuzzymatch(u_str, o_str, " -", TRUE))
        return TRUE;

    if (retry_inverted) {
        const char *u_of, *o_of;

        /* when just one of the strings is in the form "foo of bar",
           convert it into "bar foo" and perform another comparison */
        u_of = strstri(u_str, " of ");
        o_of = strstri(o_str, " of ");
        if (u_of && !o_of) {
            Strcpy(buf, u_of + 4);
            copynchars(eos(strcat(buf, " ")), u_str, (int) (u_of - u_str));
            if (fuzzymatch(buf, o_str, " -", TRUE))
                return TRUE;
        } else if (o_of && !u_of) {
            Strcpy(buf, o_of + 4);
            copynchars(eos(strcat(buf, " ")), o_str, (int) (o_of - o_str));
            if (fuzzymatch(u_str, buf, " -", TRUE))
                return TRUE;
        }
    }

    /* [note: if something like "elven speed boots" ever gets added, these
       special cases should be changed to call wishymatch() recursively in
       order to get the "of" inversion handling] */
    if (!strncmp(o_str, "dwarvish ", 9)) {
        if (!strncmpi(u_str, "dwarven ", 8))
            return fuzzymatch(u_str + 8, o_str + 9, " -", TRUE);
    } else if (!strncmp(o_str, "elven ", 6)) {
        if (!strncmpi(u_str, "elvish ", 7))
            return fuzzymatch(u_str + 7, o_str + 6, " -", TRUE);
        else if (!strncmpi(u_str, "elfin ", 6))
            return fuzzymatch(u_str + 6, o_str + 6, " -", TRUE);
    } else if (strstri(o_str, "helm") && strstri(u_str, "helmet")) {
        copynchars(buf, u_str, (int) sizeof buf - 1);
        (void) strsubst(buf, "helmet", "helm");
        return wishymatch(buf, o_str,  TRUE);
    } else if (strstri(o_str, "gauntlets") && strstri(u_str, "gloves")) {
        /* -3: room to replace shorter "gloves" with longer "gauntlets" */
        copynchars(buf, u_str, (int) sizeof buf - 1 - 3);
        (void) strsubst(buf, "gloves", "gauntlets");
        return wishymatch(buf, o_str, TRUE);
    } else if (!strncmp(o_str, detect_SP, sizeof detect_SP - 1)) {
        /* check for "detect <foo>" vs "<foo> detection" */
        if ((p = strstri(u_str, SP_detection)) != 0
            && !*(p + sizeof SP_detection - 1)) {
            /* convert "<foo> detection" into "detect <foo>" */
            *p = '\0';
            Strcat(strcpy(buf, detect_SP), u_str);
            /* "detect monster" -> "detect monsters" */
            if (!strcmpi(u_str, "monster"))
                Strcat(buf, "s");
            *p = ' ';
            return fuzzymatch(buf, o_str, " -", TRUE);
        }
    } else if (strstri(o_str, SP_detection)) {
        /* and the inverse, "<foo> detection" vs "detect <foo>" */
        if (!strncmpi(u_str, detect_SP, sizeof detect_SP - 1)) {
            /* convert "detect <foo>s" into "<foo> detection" */
            p = makesingular(u_str + sizeof detect_SP - 1);
            Strcat(strcpy(buf, p), SP_detection);
            /* caller may be looping through objects[], so avoid
               churning through all the obufs */
            releaseobuf(p);
            return fuzzymatch(buf, o_str, " -", TRUE);
        }
    } else if (strstri(o_str, "ability")) {
        /* when presented with "foo of bar", makesingular() used to
           singularize both foo & bar, but now only does so for foo */
        /* catch "{potion(s),ring} of {gain,restore,sustain} abilities" */
        if ((p = strstri(u_str, "abilities")) != 0
            && !*(p + sizeof "abilities" - 1)) {
            (void) strncpy(buf, u_str, (unsigned) (p - u_str));
            Strcpy(buf + (p - u_str), "ability");
            return fuzzymatch(buf, o_str, " -", TRUE);
        }
    } else if (!strcmp(o_str, "aluminum")) {
        /* this special case doesn't really fit anywhere else... */
        /* (note that " wand" will have been stripped off by now) */
        if (!strcmpi(u_str, "aluminium"))
            return fuzzymatch(u_str + 9, o_str + 8, " -", TRUE);
    }

    return FALSE;
}

struct o_range {
    const char *name, oclass;
    int f_o_range, l_o_range;
};

/* wishable subranges of objects */
static NEARDATA const struct o_range o_ranges[] = {
    { "bag", TOOL_CLASS, SACK, BAG_OF_TRICKS },
    { "lamp", TOOL_CLASS, OIL_LAMP, MAGIC_LAMP },
    { "candle", TOOL_CLASS, TALLOW_CANDLE, WAX_CANDLE },
    { "horn", TOOL_CLASS, TOOLED_HORN, HORN_OF_PLENTY },
    { "shield", ARMOR_CLASS, SMALL_SHIELD, SHIELD_OF_REFLECTION },
    { "hat", ARMOR_CLASS, FEDORA, DUNCE_CAP },
    { "helm", ARMOR_CLASS, ELVEN_LEATHER_HELM, HELM_OF_TELEPATHY },
    { "gloves", ARMOR_CLASS, LEATHER_GLOVES, GAUNTLETS_OF_DEXTERITY },
    { "gauntlets", ARMOR_CLASS, LEATHER_GLOVES, GAUNTLETS_OF_DEXTERITY },
    { "boots", ARMOR_CLASS, LOW_BOOTS, LEVITATION_BOOTS },
    { "shoes", ARMOR_CLASS, LOW_BOOTS, IRON_SHOES },
    { "cloak", ARMOR_CLASS, MUMMY_WRAPPING, CLOAK_OF_DISPLACEMENT },
    { "shirt", ARMOR_CLASS, HAWAIIAN_SHIRT, T_SHIRT },
    { "dragon scales", ARMOR_CLASS, GRAY_DRAGON_SCALES,
      YELLOW_DRAGON_SCALES },
    { "dragon scale mail", ARMOR_CLASS, GRAY_DRAGON_SCALE_MAIL,
      YELLOW_DRAGON_SCALE_MAIL },
    { "sword", WEAPON_CLASS, SHORT_SWORD, KATANA },
    { "venom", VENOM_CLASS, BLINDING_VENOM, ACID_VENOM },
    { "gray stone", GEM_CLASS, LUCKSTONE, FLINT },
    { "grey stone", GEM_CLASS, LUCKSTONE, FLINT },
};

/* alternate spellings; if the difference is only the presence or
   absence of spaces and/or hyphens (such as "pickaxe" vs "pick axe"
   vs "pick-axe") then there is no need for inclusion in this list;
   likewise for ``"of" inversions'' ("boots of speed" vs "speed boots") */
static const struct alt_spellings {
    const char *sp;
    int ob;
} spellings[] = {
    { "pickax", PICK_AXE },
    { "whip", BULLWHIP },
    { "saber", SILVER_SABER },
    { "silver sabre", SILVER_SABER },
    { "smooth shield", SHIELD_OF_REFLECTION },
    { "grey dragon scale mail", GRAY_DRAGON_SCALE_MAIL },
    { "grey dragon scales", GRAY_DRAGON_SCALES },
    { "iron ball", HEAVY_IRON_BALL },
    { "lantern", BRASS_LANTERN },
    { "mattock", DWARVISH_MATTOCK },
    { "amulet of poison resistance", AMULET_VERSUS_POISON },
    { "amulet of protection", AMULET_OF_GUARDING },
    { "amulet of telepathy", AMULET_OF_ESP },
    { "helm of esp", HELM_OF_TELEPATHY },
    { "gauntlets of ogre power", GAUNTLETS_OF_POWER },
    { "gauntlets of giant strength", GAUNTLETS_OF_POWER },
    { "elven chain mail", ELVEN_MITHRIL_COAT },
    { "potion of sleep", POT_SLEEPING },
    { "scroll of recharging", SCR_CHARGING },
    { "recharging", SCR_CHARGING },
    { "stone", ROCK },
    { "camera", EXPENSIVE_CAMERA },
    { "tee shirt", T_SHIRT },
    { "can", TIN },
    { "can opener", TIN_OPENER },
    { "kelp", KELP_FROND },
    { "eucalyptus", EUCALYPTUS_LEAF },
    { "lembas", LEMBAS_WAFER },
    { "cookie", FORTUNE_COOKIE },
    { "pie", CREAM_PIE },
    { "huge meatball", ENORMOUS_MEATBALL }, /* likely conflated name */
    { "huge chunk of meat", ENORMOUS_MEATBALL }, /* original name */
    { "marker", MAGIC_MARKER },
    { "hook", GRAPPLING_HOOK },
    { "grappling iron", GRAPPLING_HOOK },
    { "grapnel", GRAPPLING_HOOK },
    { "grapple", GRAPPLING_HOOK },
    { "protection from shape shifters", RIN_PROTECTION_FROM_SHAPE_CHAN },
    /* if we ever add other sizes, move this to o_ranges[] with "bag" */
    { "box", LARGE_BOX },
    /* normally we wouldn't have to worry about unnecessary <space>, but
       " stone" will get stripped off, preventing a wishymatch; that actually
       lets "flint stone" be a match, so we also accept bogus "flintstone" */
    { "luck stone", LUCKSTONE },
    { "load stone", LOADSTONE },
    { "touch stone", TOUCHSTONE },
    { "flintstone", FLINT },
    { (const char *) 0, 0 },
};

static short
rnd_otyp_by_wpnskill(schar skill)
{
    int i, n = 0;
    short otyp = STRANGE_OBJECT;

    for (i = g.bases[WEAPON_CLASS];
         i < NUM_OBJECTS && objects[i].oc_class == WEAPON_CLASS; i++)
        if (objects[i].oc_skill == skill) {
            n++;
            otyp = i;
        }
    if (n > 0) {
        n = rn2(n);
        for (i = g.bases[WEAPON_CLASS];
             i < NUM_OBJECTS && objects[i].oc_class == WEAPON_CLASS; i++)
            if (objects[i].oc_skill == skill)
                if (--n < 0)
                    return i;
    }
    return otyp;
}

static short
rnd_otyp_by_namedesc(
    const char *name,
    char oclass,
    int xtra_prob) /* add to item's chance of being chosen; non-zero causes
                    * 0% random generation items to also be considered */
{
    int i, n = 0;
    short validobjs[NUM_OBJECTS];
    register const char *zn, *of;
    boolean check_of;
    int lo, hi, minglob, maxglob, prob, maxprob = 0;

    if (!name || !*name)
        return STRANGE_OBJECT;

    /* only skip "foo of" for "foo of bar" if target doesn't contain " of " */
    check_of = (strstri(name, " of ") == 0);
    minglob = GLOB_OF_GRAY_OOZE;
    maxglob = GLOB_OF_BLACK_PUDDING;

    (void) memset((genericptr_t) validobjs, 0, sizeof validobjs);
    if (oclass) {
        lo = g.bases[(uchar) oclass];
        hi = g.bases[(uchar) oclass + 1] - 1;
    } else {
        lo = STRANGE_OBJECT + 1;
        hi = NUM_OBJECTS - 1;
    }
    /* FIXME:
     * When this spans classes (the !oclass case), the item
     * probabilities are not very useful because they don't take
     * the class generation probability into account.  [If 10%
     * of spellbooks were blank and 1% of scrolls were blank,
     * "blank" would have 10/11 chance to yield a book even though
     * scrolls are supposed to be much more common than books.]
     */
    for (i = lo; i <= hi; ++i) {
        /* don't match extra descriptions (w/o real name) */
        if ((zn = OBJ_NAME(objects[i])) == 0)
            continue;
        if (wishymatch(name, zn, TRUE) /* objects[] name */
            /* let "<bar>" match "<foo> of <bar>" (already does if foo is
               an object class, but this is for lump of royal jelly,
               clove of garlic, bag of tricks, &c) with a few exceptions:
               for "opening", don't match "bell of opening"; for monster
               type ooze/pudding/slime don't match glob of same since that
               ought to match "corpse/egg/figurine of type" too but won't */
            || (check_of
                && i != BELL_OF_OPENING
                && (i < minglob || i > maxglob)
                && (of = strstri(zn, " of ")) != 0
                && wishymatch(name, of + 4, FALSE)) /* partial name */
            || ((zn = OBJ_DESCR(objects[i])) != 0
                && wishymatch(name, zn, FALSE)) /* objects[] description */
            /* "cloth" should match "piece of cloth"; there's only one
               description containing " of " so no special case handling */
            || (zn && check_of && (of = strstri(zn, " of ")) != 0
                && wishymatch(name, of + 4, FALSE)) /* partial description */
            || ((zn = objects[i].oc_uname) != 0
                && wishymatch(name, zn, FALSE)) /* user-called name */
            ) {
            validobjs[n++] = (short) i;
            maxprob += (objects[i].oc_prob + xtra_prob);
        }
    }

    if (n > 0 && maxprob) {
        prob = rn2(maxprob);
        for (i = 0; i < n - 1; i++)
            if ((prob -= (objects[validobjs[i]].oc_prob + xtra_prob)) < 0)
                break;
        return validobjs[i];
    }
    return STRANGE_OBJECT;
}

int
shiny_obj(char oclass)
{
    return (int) rnd_otyp_by_namedesc("shiny", oclass, 0);
}

/* in wizard mode, readobjnam() can accept wishes for traps and terrain */
static struct obj *
wizterrainwish(struct _readobjnam_data *d)
{
    struct rm *lev;
    boolean madeterrain = FALSE, badterrain = FALSE, didblock;
    int trap, oldtyp;
    coordxy x = u.ux, y = u.uy;
    char *bp = d->bp, *p = d->p;

    for (trap = NO_TRAP + 1; trap < TRAPNUM; trap++) {
        struct trap *t;
        const char *tname;

        tname = trapname(trap, TRUE);
        if (!str_start_is(bp, tname, TRUE))
            continue;
        /* found it; avoid stupid mistakes */
        if (is_hole(trap) && !Can_fall_thru(&u.uz))
            trap = ROCKTRAP;
        if ((t = maketrap(x, y, trap)) != 0) {
            trap = t->ttyp;
            tname = trapname(trap, TRUE);
            pline("%s%s.", An(tname),
                  (trap != MAGIC_PORTAL) ? "" : " to nowhere");
        } else
            pline("Creation of %s failed.", an(tname));
        return (struct obj *) &cg.zeroobj;
    }

    /* furniture and terrain (use at your own risk; can clobber stairs
       or place furniture on existing traps which shouldn't be allowed) */
    lev = &levl[x][y];
    oldtyp = lev->typ;
    didblock = does_block(x, y, lev);
    p = eos(bp);
    if (!BSTRCMPI(bp, p - 8, "fountain")) {
        lev->typ = FOUNTAIN;
        g.level.flags.nfountains++;
        lev->looted = d->looted ? F_LOOTED : 0; /* overlays 'flags' */
        lev->blessedftn = !strncmpi(bp, "magic ", 6);
        pline("A %sfountain.", lev->blessedftn ? "magic " : "");
        madeterrain = TRUE;
    } else if (!BSTRCMPI(bp, p - 6, "throne")) {
        lev->typ = THRONE;
        lev->looted = d->looted ? T_LOOTED : 0; /* overlays 'flags' */
        pline("A throne.");
        madeterrain = TRUE;
    } else if (!BSTRCMPI(bp, p - 4, "sink")) {
        lev->typ = SINK;
        g.level.flags.nsinks++;
        lev->looted = d->looted ? (S_LPUDDING | S_LDWASHER | S_LRING) : 0;
        pline("A sink.");
        madeterrain = TRUE;

    /* ("water" matches "potion of water" rather than terrain) */
    } else if (!BSTRCMPI(bp, p - 4, "pool")
               || !BSTRCMPI(bp, p - 4, "moat")
               || !BSTRCMPI(bp, p - 13, "wall of water")) {
        long save_prop;
        const char *new_water;

        lev->typ = !BSTRCMPI(bp, p - 4, "pool") ? POOL
                   : !BSTRCMPI(bp, p - 4, "moat") ? MOAT
                     : WATER;
        lev->flags = 0;
        del_engr_at(x, y);
        save_prop = EHalluc_resistance;
        EHalluc_resistance = 1;
        new_water = waterbody_name(x, y);
        EHalluc_resistance = save_prop;
        pline("%s.", An(new_water));
        /* Must manually make kelp! */
        water_damage_chain(g.level.objects[x][y], TRUE);
        madeterrain = TRUE;

    /* also matches "molten lava" */
    } else if (!BSTRCMPI(bp, p - 4, "lava")) {
        lev->typ = LAVAPOOL;
        lev->flags = 0;
        del_engr_at(x, y);
        pline("A pool of molten lava.");
        if (!(Levitation || Flying))
            pooleffects(FALSE);
        madeterrain = TRUE;
    } else if (!BSTRCMPI(bp, p - 3, "ice")) {
        lev->typ = ICE;
        lev->flags = 0;
        del_engr_at(x, y);

        if (!strncmpi(bp, "melting ", 8))
            start_melt_ice_timeout(x, y, 0L);

        pline("Ice.");
        madeterrain = TRUE;
    } else if (!BSTRCMPI(bp, p - 5, "altar")) {
        aligntyp al;

        lev->typ = ALTAR;
        if (!strncmpi(bp, "chaotic ", 8))
            al = A_CHAOTIC;
        else if (!strncmpi(bp, "neutral ", 8))
            al = A_NEUTRAL;
        else if (!strncmpi(bp, "lawful ", 7))
            al = A_LAWFUL;
        else if (!strncmpi(bp, "unaligned ", 10))
            al = A_NONE;
        else /* -1 - A_CHAOTIC, 0 - A_NEUTRAL, 1 - A_LAWFUL */
            al = !rn2(6) ? A_NONE : (rn2((int) A_LAWFUL + 2) - 1);
        lev->altarmask = Align2amask(al); /* overlays 'flags' */
        pline("%s altar.", An(align_str(al)));
        madeterrain = TRUE;
    } else if (!BSTRCMPI(bp, p - 5, "grave")
               || !BSTRCMPI(bp, p - 9, "headstone")) {
        make_grave(x, y, (char *) 0);
        if (IS_GRAVE(lev->typ)) {
            lev->looted = 0; /* overlays 'flags' */
            lev->disturbed = d->looted ? 1 : 0;
            pline("A %sgrave.", lev->disturbed ? "disturbed " : "");
            madeterrain = TRUE;
        } else {
            pline("Can't place a grave here.");
            badterrain = TRUE;
        }
    } else if (!BSTRCMPI(bp, p - 4, "tree")) {
        lev->typ = TREE;
        lev->looted = d->looted ? (TREE_LOOTED | TREE_SWARM) : 0;
        pline("A tree.");
        madeterrain = TRUE;
    } else if (!BSTRCMPI(bp, p - 4, "bars")) {
        lev->typ = IRONBARS;
        lev->flags = 0;
        /* [FIXME: if this isn't a wall or door location where 'horizontal'
            is already set up, that should be calculated for this spot.
            Unforutnately, it can be tricky; placing one in open space
            and then another adjacent might need to recalculate first one.] */
        pline("Iron bars.");
        madeterrain = TRUE;
    } else if (!BSTRCMPI(bp, p - 5, "cloud")) {
        lev->typ = CLOUD;
        lev->flags = 0;
        pline("A cloud.");
        madeterrain = TRUE;
    } else if (!BSTRCMPI(bp, p - 4, "door")
               || (d->doorless && !BSTRCMPI(bp, p - 7, "doorway"))) {
        char dbuf[40];
        unsigned old_wall_info;
        boolean secret = !BSTRCMPI(bp, p - 11, "secret door");

        /* require door or wall so that the 'horizontal' flag will
           already have the correct value; player might choose to put
           DOOR on top of existing DOOR or SDOOR on top of existing SDOOR
           to control its trapped state; iron bars are surrogate walls;
           a previously dug wall looks like corridor but is actually a
           doorless doorway so will be acceptable here */
        if (lev->typ == DOOR || lev->typ == SDOOR
            || (IS_WALL(lev->typ) && lev->typ != DBWALL)
            || lev->typ == IRONBARS) {
            /* remember previous wall info [is this right for iron bars?] */
            old_wall_info = (lev->typ != DOOR) ? lev->wall_info : 0;
            /* set the new terrain type */
            lev->typ = secret ? SDOOR : DOOR;
            lev->wall_info = 0; /* overlays 'flags' */
            /* lev->horizontal stays as-is */
            if (Is_rogue_level(&u.uz)) {
                /* all doors on the rogue level are doorless; locking magic
                   there converts them into walls rather than closed doors */
                d->doorless = 1;
                d->locked = d->closed = d->open = d->broken = 0;
            }
            /* if not locked, secret doors are implicitly closed but
               mustn't be set that way explicitly because they use both
               doormask and wall_info which both overload rm[x][y].flags
               (CLOSED overlaps wall_info bits, LOCKED and TRAPPED don't);
               conversion from SDOOR to DOOR changes NODOOR to CLOSED */
            lev->doormask = d->locked ? D_LOCKED
                            : (d->doorless || secret) ? D_NODOOR
                              : d->open ? D_ISOPEN
                                : d->broken ? D_BROKEN
                                  : D_CLOSED;
            /* SDOOR uses wall_info, restore relevant bits.
             * FIXME? if we're changing a regular door into a secret door,
             * old_wall_info bits will be 0 instead of being set properly.
             * Probably only matters if player uses Passes_walls and a wish
             * to turn a T- or cross-wall into a door, losing wall info,
             * and then another wish to turn that door into a secret door. */
            if (secret)
                lev->wall_info |= (old_wall_info & WM_MASK);
            /* set up trapped flag; open door states aren't eligible */
            if (d->trapped == 2 /* 2: wish includes explicit "untrapped" */
                || secret /* secret doors can't trapped due to their use
                           * of both doormask and wall_info; those both
                           * overlay rm->flags and partially conflict */
                || (lev->doormask & (D_LOCKED | D_CLOSED)) == 0)
                d->trapped = 0;
            if (d->trapped)
                lev->doormask |= D_TRAPPED;
            /* feedback */
            dbuf[0] = '\0';
            if (lev->doormask & D_TRAPPED)
                Strcat(dbuf, "trapped ");
            if (lev->doormask & D_LOCKED)
                Strcat(dbuf, "locked ");
            if (lev->typ == SDOOR) {
                Strcat(dbuf, "secret door");
            } else {
                /* these should be mutually exclusive but we describe them
                   as if they're independent to maybe catch future bugs... */
                if (lev->doormask & D_CLOSED)
                    Strcat(dbuf, "closed ");
                if (lev->doormask & D_ISOPEN)
                    Strcat(dbuf, "open ");
                if (lev->doormask & D_BROKEN)
                    Strcat(dbuf, "broken ");
                if ((lev->doormask & ~D_TRAPPED) == D_NODOOR)
                    Strcat(dbuf, "doorless doorway");
                else
                    Strcat(dbuf, "door");
            }
            pline("%s.", upstart(an(dbuf)));
            madeterrain = TRUE;
        } else {
            Strcpy(dbuf, secret ? "secret door" : "door");
            pline("%s requires door or wall location.", upstart(dbuf));
            badterrain = TRUE;
        }
    } else if (!BSTRCMPI(bp, p - 4, "wall")
                         && (bp == p - 4 || p[-4] == ' ')) {
        schar wall = HWALL;

        if ((isok(u.ux, u.uy-1) && IS_WALL(levl[u.ux][u.uy-1].typ))
            || (isok(u.ux, u.uy+1) && IS_WALL(levl[u.ux][u.uy+1].typ)))
            wall = VWALL;
        madeterrain = TRUE;
        lev->typ = wall;
        fix_wall_spines(max(0,u.ux-1), max(0,u.uy-1),
                        min(COLNO,u.ux+1), min(ROWNO,u.uy+1));
        pline("A wall.");
    } else if (!BSTRCMPI(bp, p - 15, "secret corridor")) {
        if (lev->typ == CORR) {
            lev->typ = SCORR;
            /* neither CORR nor SCORR uses 'flags' or 'horizontal' */
            pline("Secret corridor.");
            madeterrain = TRUE;
        } else {
            pline("Secret corridor requires corridor location.");
            badterrain = TRUE;
        }
    }

    if (madeterrain) {
        feel_newsym(x, y); /* map the spot where the wish occurred */

        /* hero started at <x,y> but might not be there anymore (create
           lava, decline to die, and get teleported away to safety) */
        if (u.uinwater && !is_pool(u.ux, u.uy)) {
            set_uinwater(0); /* u.uinwater = 0; leave the water */
            docrt();
            /* [block/unblock_point was handled by docrt -> vision_recalc] */
        } else {
            if (u.utrap && u.utraptype == TT_LAVA && !is_lava(u.ux, u.uy))
                reset_utrap(FALSE);

            if (does_block(x, y, lev)) {
                if (!didblock)
                    block_point(x, y);
            } else {
                if (didblock)
                    unblock_point(x, y);
            }
        }

        /* fixups for replaced terrain that aren't handled above;
           for fountain placed on fountain or sink placed on sink, the
           increment above gets canceled out by the decrement here;
           otherwise if fountain or sink was replaced, there's one less */
        if (IS_FOUNTAIN(oldtyp))
            g.level.flags.nfountains--;
        else if (IS_SINK(oldtyp))
            g.level.flags.nsinks--;
        /* horizontal is overlaid by fountain->blessedftn, grave->disturbed */
        if (IS_FOUNTAIN(oldtyp) || IS_GRAVE(oldtyp)
            || IS_WALL(oldtyp) || oldtyp == IRONBARS
            || IS_DOOR(oldtyp) || oldtyp == SDOOR) {
            /* when new terrain is a fountain, 'blessedftn' was explicitly
               set above; likewise for grave and 'disturbed'; when it's a
               door, the old type was a wall or a door and we retain the
               'horizontal' value from those */
            if (!IS_FOUNTAIN(lev->typ) && !IS_GRAVE(lev->typ)
                && !IS_DOOR(lev->typ) && lev->typ != SDOOR)
                lev->horizontal = 0; /* also clears blessedftn, disturbed */
        }
        /* note: lev->lit and lev->nondiggable retain their values even
           though those might not make sense with the new terrain */

        /* might have changed terrain from something that blocked
           levitation and flying to something that doesn't (levitating
           while in xorn form and replacing solid stone with furniture) */
        switch_terrain();
    }
    if (madeterrain || badterrain) {
        /* cast 'const' away; caller won't modify this */
        return (struct obj *) &cg.zeroobj;
    }

    return (struct obj *) 0;
}

#define TIN_UNDEFINED 0
#define TIN_EMPTY 1
#define TIN_SPINACH 2

static void
readobjnam_init(char *bp, struct _readobjnam_data *d)
{
    d->otmp = (struct obj *) 0;
    d->cnt = d->spe = d->spesgn = d->typ = 0;
    d->very = d->rechrg = d->blessed = d->uncursed = d->iscursed
        = d->ispoisoned = d->isgreased = d->eroded = d->eroded2
        = d->erodeproof = d->halfeaten = d->islit = d->unlabeled
        = d->ishistoric = d->isdiluted /* statues, potions */
          /* box/chest and wizard mode door */
        = d->trapped = d->locked = d->unlocked = d->broken
        = d->open = d->closed = d->doorless /* wizard mode door */
        = d->looted /* wizard mode fountain/sink/throne/tree and grave */
        = d->real = d->fake = 0; /* Amulet */
    d->tvariety = RANDOM_TIN;
    d->mgend = -1; /* not specified, aka random */
    d->mntmp = NON_PM;
    d->contents = TIN_UNDEFINED;
    d->oclass = 0;
    d->actualn = d->dn = d->un = 0;
    d->wetness = 0;
    d->gsize = 0;
    d->zombify = FALSE;
    d->bp = d->origbp = bp;
    d->p = (char *) 0;
    d->name = (const char *) 0;
    d->ftype = g.context.current_fruit;
    (void) memset(d->globbuf, '\0', sizeof d->globbuf);
    (void) memset(d->fruitbuf, '\0', sizeof d->fruitbuf);
}

/* return 1 if d->bp is empty or contains only various qualifiers like
   "blessed", "rustproof", and so on, or 0 if anything else is present */
static int
readobjnam_preparse(struct _readobjnam_data *d)
{
    char *save_bp = 0;
    int more_l = 0, res = 1;

    for (;;) {
        register int l;

        if (!d->bp || !*d->bp)
            break;
        res = 0;

        if (!strncmpi(d->bp, "an ", l = 3) || !strncmpi(d->bp, "a ", l = 2)) {
            d->cnt = 1;
        } else if (!strncmpi(d->bp, "the ", l = 4)) {
            ; /* just increment `bp' by `l' below */
        } else if (!d->cnt && digit(*d->bp) && strcmp(d->bp, "0")) {
            d->cnt = atoi(d->bp);
            while (digit(*d->bp))
                d->bp++;
            while (*d->bp == ' ')
                d->bp++;
            l = 0;
        } else if (*d->bp == '+' || *d->bp == '-') {
            d->spesgn = (*d->bp++ == '+') ? 1 : -1;
            d->spe = atoi(d->bp);
            while (digit(*d->bp))
                d->bp++;
            while (*d->bp == ' ')
                d->bp++;
            l = 0;
        } else if (!strncmpi(d->bp, "blessed ", l = 8)
                   || !strncmpi(d->bp, "holy ", l = 5)) {
            d->blessed = 1, d->uncursed = d->iscursed = 0;
        } else if (!strncmpi(d->bp, "cursed ", l = 7)
                   || !strncmpi(d->bp, "unholy ", l = 7)) {
            d->iscursed = 1, d->blessed = d->uncursed = 0;
        } else if (!strncmpi(d->bp, "uncursed ", l = 9)) {
            d->uncursed = 1, d->blessed = d->iscursed = 0;
        } else if (!strncmpi(d->bp, "rustproof ", l = 10)
                   || !strncmpi(d->bp, "erodeproof ", l = 11)
                   || !strncmpi(d->bp, "corrodeproof ", l = 13)
                   || !strncmpi(d->bp, "fixed ", l = 6)
                   || !strncmpi(d->bp, "fireproof ", l = 10)
                   || !strncmpi(d->bp, "rotproof ", l = 9)) {
            d->erodeproof = 1;
        } else if (!strncmpi(d->bp, "lit ", l = 4)
                   || !strncmpi(d->bp, "burning ", l = 8)) {
            d->islit = 1;
        } else if (!strncmpi(d->bp, "unlit ", l = 6)
                   || !strncmpi(d->bp, "extinguished ", l = 13)) {
            d->islit = 0;

        /* "wet" and "moist" are only applicable for towels */
        } else if (!strncmpi(d->bp, "moist ", l = 6)
                   || !strncmpi(d->bp, "wet ", l = 4)) {
            if (!strncmpi(d->bp, "wet ", 4))
                d->wetness = 3 + rn2(3); /* 3..5 */
            else
                d->wetness = rnd(2); /* 1..2 */

        /* "unlabeled" and "blank" are synonymous */
        } else if (!strncmpi(d->bp, "unlabeled ", l = 10)
                   || !strncmpi(d->bp, "unlabelled ", l = 11)
                   || !strncmpi(d->bp, "blank ", l = 6)) {
            d->unlabeled = 1;
        } else if (!strncmpi(d->bp, "poisoned ", l = 9)) {
            d->ispoisoned = 1;

        /* "trapped" recognized but not honored outside wizard mode */
        } else if (!strncmpi(d->bp, "trapped ", l = 8)) {
            d->trapped = 0; /* undo any previous "untrapped" */
            if (wizard)
                d->trapped = 1;
        } else if (!strncmpi(d->bp, "untrapped ", l = 10)) {
            d->trapped = 2; /* not trapped */

        /* locked, unlocked, broken: box/chest lock states, also door states;
           open, closed, doorless: additional door states */
        } else if (!strncmpi(d->bp, "locked ", l = 7)) {
            d->locked = d->closed = 1,
                d->unlocked = d->broken = d->open = d->doorless = 0;
        } else if (!strncmpi(d->bp, "unlocked ", l = 9)) {
            d->unlocked = d->closed = 1,
                d->locked = d->broken = d->open = d->doorless = 0;
        } else if (!strncmpi(d->bp, "broken ", l = 7)) {
            d->broken = 1,
                d->locked = d->unlocked = d->open = d->closed
                = d->doorless = 0;
        } else if (!strncmpi(d->bp, "open ", l = 5)) {
            d->open = 1,
                d->closed = d->locked = d->broken = d->doorless = 0;
        } else if (!strncmpi(d->bp, "closed ", l = 7)) {
            d->closed = 1,
                d->open = d->locked = d->broken = d->doorless = 0;
        } else if (!strncmpi(d->bp, "doorless ", l = 9)) {
            d->doorless = 1,
                d->open = d->closed = d->locked = d->unlocked = d->broken = 0;
        /* looted: fountain/sink/throne/tree; disturbed: grave */
        } else if (!strncmpi(d->bp, "looted ", l = 7)
                   /* overload disturbed grave with looted fountain here
                      even though they're separate in struct rm */
                   || !strncmpi(d->bp, "disturbed ", l = 10)) {
            d->looted = 1;
        } else if (!strncmpi(d->bp, "greased ", l = 8)) {
            d->isgreased = 1;
        } else if (!strncmpi(d->bp, "zombifying ", l = 11)) {
            d->zombify = TRUE;
        } else if (!strncmpi(d->bp, "very ", l = 5)) {
            /* very rusted very heavy iron ball */
            d->very = 1;
        } else if (!strncmpi(d->bp, "thoroughly ", l = 11)) {
            d->very = 2;
        } else if (!strncmpi(d->bp, "rusty ", l = 6)
                   || !strncmpi(d->bp, "rusted ", l = 7)
                   || !strncmpi(d->bp, "burnt ", l = 6)
                   || !strncmpi(d->bp, "burned ", l = 7)) {
            d->eroded = 1 + d->very;
            d->very = 0;
        } else if (!strncmpi(d->bp, "corroded ", l = 9)
                   || !strncmpi(d->bp, "rotted ", l = 7)) {
            d->eroded2 = 1 + d->very;
            d->very = 0;
        } else if (!strncmpi(d->bp, "partly eaten ", l = 13)
                   || !strncmpi(d->bp, "partially eaten ", l = 16)) {
            d->halfeaten = 1;
        } else if (!strncmpi(d->bp, "historic ", l = 9)) {
            d->ishistoric = 1;
        } else if (!strncmpi(d->bp, "diluted ", l = 8)) {
            d->isdiluted = 1;
        } else if (!strncmpi(d->bp, "empty ", l = 6)) {
            d->contents = TIN_EMPTY;
        } else if (!strncmpi(d->bp, "small ", l = 6)) { /* glob sizes */
            /* "small" might be part of monster name (mimic, if wishing
               for its corpse) rather than prefix for glob size; when
               used for globs, it might be either "small glob of <foo>" or
               "small <foo> glob" and user might add 's' even though plural
               doesn't accomplish anything because globs don't stack */
            if (strncmpi(d->bp + l, "glob", 4) && !strstri(d->bp + l, " glob"))
                break;
            d->gsize = 1;
        } else if (!strncmpi(d->bp, "medium ", l = 7)) {
            /* 3.7: in 3.6, "medium" was only used during wishing and the
               mid-size glob had no adjective when formatted, but as of
               3.7, "medium" has become an explicit part of the name for
               combined globs of at least 5 individual ones (owt >= 100)
               and less than 15 (owt < 300) */
            d->gsize = 2;
        } else if (!strncmpi(d->bp, "large ", l = 6)) {
            /* "large" might be part of monster name (dog, cat, koboold,
               mimic) or object name (box, round shield) rather than
               prefix for glob size */
            if (strncmpi(d->bp + l, "glob", 4) && !strstri(d->bp + l, " glob"))
                break;
            /* "very large " had "very " peeled off on previous iteration */
            d->gsize = (d->very != 1) ? 3 : 4;
        } else if (!strncmpi(d->bp, "real ", l = 5)) {
            /* accept "real Amulet of Yendor" with "blessed" or "cursed"
               or useless "erodeproof" before or after "real" ... */
            d->real = 1; /* don't negate 'fake' here; "real fake amulet" and
                       * "fake real amulet" will both yield fake amulet
                       * (so will "real amulet" outside of wizard mode) */
        } else if (!strncmpi(d->bp, "fake ", l = 5)) {
            /* ... and "fake Amulet of Yendor" likewise */
            d->fake = 1, d->real = 0;
            /* ['real' isn't actually needed (unless we someday add
               "real gem" for random non-glass, non-stone)] */
        } else if (!strncmpi(d->bp, "female ", l = 7)) {
            d->mgend = FEMALE;
            /* if after "corpse/statue/figurine of", remove from string */
            if (save_bp)
                strsubst(d->bp, "female ", ""), l = 0;
        } else if (!strncmpi(d->bp, "male ", l = 5)) {
            d->mgend = MALE;
            if (save_bp)
                strsubst(d->bp, "male ", ""), l = 0;
        } else if (!strncmpi(d->bp, "neuter ", l = 7)) {
            d->mgend = NEUTRAL;
            if (save_bp)
                strsubst(d->bp, "neuter ", ""), l = 0;

        /*
         * Corpse/statue/figurine gender hack:  in order to accept
         * "statue of a female gnome ruler" for gnome queen we need
         * to recognize and skip over "statue of [a ]".  Otherwise
         * we would only accept "female gnome ruler statue" and the
         * viable but silly "female statue of a gnome ruler".
         */
        } else if ((!strncmpi(d->bp, "corpse ", l = 7)
                    || !strncmpi(d->bp, "statue ", l = 7)
                    || !strncmpi(d->bp, "figurine ", l = 9))
                   && !strncmpi(d->bp + l, "of ", more_l = 3)) {
            save_bp = d->bp; /* we'll backtrack to here later */
            l += more_l, more_l = 0;
            if (!strncmpi(d->bp + l, "a ", more_l = 2)
                || !strncmpi(d->bp + l, "an ", more_l = 3)
                || !strncmpi(d->bp + l, "the ", more_l = 4))
                l += more_l;
        } else {
            break;
        }
        d->bp += l;
    }
    if (save_bp)
        d->bp = save_bp;
    return res;
}

static void
readobjnam_parse_charges(struct _readobjnam_data *d)
{
    if (strlen(d->bp) > 1 && (d->p = strrchr(d->bp, '(')) != 0) {
        boolean keeptrailingchars = TRUE;
        int idx = 0;

        if (d->p > d->bp && d->p[-1] == ' ')
            idx = -1;
        d->p[idx] = '\0'; /* terminate bp */
        ++d->p; /* advance past '(' */
        if (!strncmpi(d->p, "lit)", 4)) {
            d->islit = 1;
            d->p += 4 - 1; /* point at ')' */
        } else {
            d->spe = atoi(d->p);
            while (digit(*d->p))
                d->p++;
            if (*d->p == ':') {
                d->p++;
                d->rechrg = d->spe;
                d->spe = atoi(d->p);
                while (digit(*d->p))
                    d->p++;
            }
            if (*d->p != ')') {
                d->spe = d->rechrg = 0;
                /* mis-matched parentheses; rest of string will be ignored
                 * [probably we should restore everything back to '('
                 * instead since it might be part of "named ..."]
                 */
                keeptrailingchars = FALSE;
            } else {
                d->spesgn = 1;
            }
        }
        if (keeptrailingchars) {
            char *pp = eos(d->bp);

            /* 'pp' points at 'pb's terminating '\0',
               'p' points at ')' and will be incremented past it */
            do {
                *pp++ = *++d->p;
            } while (*d->p);
        }
    }
    /*
     * otmp->spe is type schar, so we don't want spe to be any bigger or
     * smaller.  Also, spe should always be positive --some cheaters may
     * try to confuse atoi().
     */
    if (d->spe < 0) {
        d->spesgn = -1; /* cheaters get what they deserve */
        d->spe = abs(d->spe);
    }
    /* cap on obj->spe is independent of (and less than) SCHAR_LIM */
    if (d->spe > SPE_LIM)
        d->spe = SPE_LIM; /* slime mold uses d.ftype, so not affected */
    if (d->rechrg < 0 || d->rechrg > 7)
        d->rechrg = 7; /* recharge_limit */
}

static int
readobjnam_postparse1(struct _readobjnam_data *d)
{
    int i;

    /* now we have the actual name, as delivered by xname, say
     *  green potions called whisky
     *  scrolls labeled "QWERTY"
     *  egg
     *  fortune cookies
     *  very heavy iron ball named hoei
     *  wand of wishing
     *  elven cloak
     */
    if ((d->p = strstri(d->bp, " named ")) != 0) {
        *d->p = 0;
        d->name = d->p + 7;
    }
    if ((d->p = strstri(d->bp, " called ")) != 0) {
        *d->p = 0;
        d->un = d->p + 8;
        /* "helmet called telepathy" is not "helmet" (a specific type)
         * "shield called reflection" is not "shield" (a general type)
         */
        for (i = 0; i < SIZE(o_ranges); i++)
            if (!strcmpi(d->bp, o_ranges[i].name)) {
                d->oclass = o_ranges[i].oclass;
                return 1; /*goto srch;*/
            }
    }
    if ((d->p = strstri(d->bp, " labeled ")) != 0) {
        *d->p = 0;
        d->dn = d->p + 9;
    } else if ((d->p = strstri(d->bp, " labelled ")) != 0) {
        *d->p = 0;
        d->dn = d->p + 10;
    }
    if ((d->p = strstri(d->bp, " of spinach")) != 0) {
        *d->p = 0;
        d->contents = TIN_SPINACH;
    }
    /* real vs fake is only useful for wizard mode but we'll accept its
       parsing in normal play (result is never real Amulet for that case) */
    if ((d->p = strstri(d->bp, OBJ_DESCR(objects[AMULET_OF_YENDOR]))) != 0
        && (d->p == d->bp || d->p[-1] == ' ')) {
        char *s = d->bp;

        /* "Amulet of Yendor" matches two items, name of real Amulet
           and description of fake one; player can explicitly specify
           "real" to disambiguate, but not specifying "fake" achieves
           the same thing; "real" and "fake" are parsed above with other
           prefixes so that combinations like "blessed real" and "real
           blessed" work as expected; also accept partial specification
           of the full name of the fake; unlike the prefix recognition
           loop above, these have to be in the right order when more
           than one is present (similar to worthless glass gems below) */
        if (!strncmpi(s, "cheap ", 6))
            d->fake = 1, s += 6;
        if (!strncmpi(s, "plastic ", 8))
            d->fake = 1, s += 8;
        if (!strncmpi(s, "imitation ", 10))
            d->fake = 1, s += 10;
        nhUse(s); /* suppress potential assigned-but-not-used complaint */
        /* when 'fake' is True, it overrides 'real' if both were given;
           when it is False, force 'real' whether that was specified or not */
        d->real = !d->fake;
        d->typ = d->real ? AMULET_OF_YENDOR : FAKE_AMULET_OF_YENDOR;
        return 2; /*goto typfnd;*/
    }

    /*
     * Skip over "pair of ", "pairs of", "set of" and "sets of".
     *
     * Accept "3 pair of boots" as well as "3 pairs of boots".  It is
     * valid English either way.  See makeplural() for more on pair/pairs.
     *
     * We should only double count if the object in question is not
     * referred to as a "pair of".  E.g. We should double if the player
     * types "pair of spears", but not if the player types "pair of
     * lenses".  Luckily (?) all objects that are referred to as pairs
     * -- boots, gloves, and lenses -- are also not mergable, so cnt is
     * ignored anyway.
     */
    if (!strncmpi(d->bp, "pair of ", 8)) {
        d->bp += 8;
        d->cnt *= 2;
    } else if (!strncmpi(d->bp, "pairs of ", 9)) {
        d->bp += 9;
        if (d->cnt > 1)
            d->cnt *= 2;
    } else if (!strncmpi(d->bp, "set of ", 7)) {
        d->bp += 7;
    } else if (!strncmpi(d->bp, "sets of ", 8)) {
        d->bp += 8;
    }

    /* Intercept pudding globs here; they're a valid wish target,
     * but we need them to not get treated like a corpse.
     * If a count is specified, it will be used to magnify weight
     * rather than to specify quantity (which is always 1 for globs).
     */
    i = (int) strlen(d->bp);
    d->p = (char *) 0;
    /* check for "glob", "<foo> glob", and "glob of <foo>" */
    if (!strcmpi(d->bp, "glob") || !BSTRCMPI(d->bp, d->bp + i - 5, " glob")
        || !strcmpi(d->bp, "globs")
        || !BSTRCMPI(d->bp, d->bp + i - 6, " globs")
        || (d->p = strstri(d->bp, "glob of ")) != 0
        || (d->p = strstri(d->bp, "globs of ")) != 0) {
        d->mntmp = name_to_mon(!d->p ? d->bp
                                     : (strstri(d->p, " of ") + 4), (int *) 0);
        /* if we didn't recognize monster type, pick a valid one at random */
        if (d->mntmp == NON_PM)
            d->mntmp = rn1(PM_BLACK_PUDDING - PM_GRAY_OOZE, PM_GRAY_OOZE);
        /* normally this would be done when makesingular() changes the value
           but canonical form here is already singular so that won't happen */
        if (d->cnt < 2 && strstri(d->bp, "globs"))
            d->cnt = 2; /* affects otmp->owt but not otmp->quan for globs */
        /* construct canonical spelling in case name_to_mon() recognized a
           variant (grey ooze) or player used inverted syntax (<foo> glob);
           if player has given a valid monster type but not valid glob type,
           object name lookup won't find it and wish attempt will fail */
        Sprintf(d->globbuf, "glob of %s", mons[d->mntmp].pmnames[NEUTRAL]);
        d->bp = d->globbuf;
        d->mntmp = NON_PM; /* not useful for "glob of <foo>" object lookup */
        d->oclass = FOOD_CLASS;
        d->actualn = d->bp, d->dn = 0;
        return 1; /*goto srch;*/
    } else {
        /*
         * Find corpse type using "of" (figurine of an orc, tin of orc meat)
         * Don't check if it's a wand or spellbook.
         * (avoid "wand/finger of death" confusion).
         * Don't match "ogre" or "giant" monster name inside alternate item
         * names "gauntlets of ogre power" and "gauntlets of giant strength"
         * (or the alternate spelling of those, "gloves of ...").
         */
        if (!strstri(d->bp, "wand ") && !strstri(d->bp, "spellbook ")
            && !strstri(d->bp, "gauntlets ") && !strstri(d->bp, "gloves ")
            && !strstri(d->bp, "finger ")) {
            if ((d->p = strstri(d->bp, "tin of ")) != 0) {
                if (!strcmpi(d->p + 7, "spinach")) {
                    d->contents = TIN_SPINACH;
                    d->mntmp = NON_PM;
                } else {
                    d->tmp = tin_variety_txt(d->p + 7, &d->tinv);
                    d->tvariety = d->tinv;
                    d->mntmp = name_to_mon(d->p + 7 + d->tmp, &d->mgend);
                }
                d->typ = TIN;
                return 2; /*goto typfnd;*/
            } else if ((d->p = strstri(d->bp, " of ")) != 0
                       && ((d->mntmp = name_to_mon(d->p + 4, &d->mgend))
                           >= LOW_PM))
                *d->p = 0;
        }
    }
    /* Find corpse type w/o "of" (red dragon scale mail, yeti corpse) */
    if (strncmpi(d->bp, "samurai sword", 13)  /* not the "samurai" monster! */
        && strncmpi(d->bp, "wizard lock", 11) /* not the "wizard" monster! */
        && strncmpi(d->bp, "death wand", 10)  /* 'of inversion', not Rider */
        && strncmpi(d->bp, "master key", 10)  /* not the "master" rank */
        && strncmpi(d->bp, "ninja-to", 8)     /* not the "ninja" rank */
        && strncmpi(d->bp, "magenta", 7)) {   /* not the "mage" rank */
        const char *rest = 0;

        if (d->mntmp < LOW_PM && strlen(d->bp) > 2
            && ((d->mntmp = name_to_monplus(d->bp, &rest, &d->mgend))
                >= LOW_PM)) {
            char *obp = d->bp;

            /* 'rest' is a pointer past the matching portion; if that was
               an alternate name or a rank title rather than the canonical
               monster name we wouldn't otherwise know how much to skip */
            d->bp = (char *) rest; /* cast away const */

            if (*d->bp == ' ') {
                d->bp++;
            } else if (!strncmpi(d->bp, "s ", 2)
                       || (d->bp > d->origbp
                           && !strncmpi(d->bp - 1, "s' ", 3))) {
                d->bp += 2;
            } else if (!strncmpi(d->bp, "es ", 3)
                       || !strncmpi(d->bp, "'s ", 3)) {
                d->bp += 3;
            } else if (!*d->bp && !d->actualn && !d->dn && !d->un
                       && !d->oclass) {
                /* no referent; they don't really mean a monster type */
                d->bp = obp;
                d->mntmp = NON_PM;
            }
        }
    }

    /* first change to singular if necessary */
    if (*d->bp
        /* we want "tricks" to match "bag of tricks" [rnd_otyp_by_namedesc()]
           but that wouldn't work if it gets singularized to "trick"
           ["tricks bag" matches whether or not this exception is present
           because singularize operates on "bag" and wishymatch()'s
           'of inversion' finds a match] */
        && strcmpi(d->bp, "tricks")
        /* an odd potential wish; fail rather than get a false match with
           "cloth" because it might yield a "cloth spellbook" rather than
           a "piece of cloth" cloak [maybe we should give random armor?] */
        && strcmpi(d->bp, "clothes")
        ) {
        char *sng = makesingular(d->bp);

        if (strcmp(d->bp, sng)) {
            if (d->cnt == 1)
                d->cnt = 2;
            Strcpy(d->bp, sng);
        }
    }

    /* Alternate spellings (pick-ax, silver sabre, &c) */
    {
        const struct alt_spellings *as = spellings;

        while (as->sp) {
            if (wishymatch(d->bp, as->sp, TRUE)) {
                d->typ = as->ob;
                return 2; /*goto typfnd;*/
            }
            as++;
        }
        /* can't use spellings list for this one due to shuffling */
        if (!strncmpi(d->bp, "grey spell", 10))
            *(d->bp + 2) = 'a';

        if ((d->p = strstri(d->bp, "armour")) != 0) {
            /* skip past "armo", then copy remainder beyond "u" */
            d->p += 4;
            while ((*d->p = *(d->p + 1)) != '\0')
                ++d->p; /* self terminating */
        }
    }

    /* dragon scales - assumes order of dragons */
    if (!strcmpi(d->bp, "scales") && d->mntmp >= PM_GRAY_DRAGON
        && d->mntmp <= PM_YELLOW_DRAGON) {
        d->typ = GRAY_DRAGON_SCALES + d->mntmp - PM_GRAY_DRAGON;
        d->mntmp = NON_PM; /* no monster */
        return 2; /*goto typfnd;*/
    }

    d->p = eos(d->bp);
    if (!BSTRCMPI(d->bp, d->p - 10, "holy water")) {
        /* this isn't needed for "[un]holy water" because adjective parsing
           handles holy==blessed and unholy==cursed and leaves "water" for
           the object type, but it is needed for "potion of [un]holy water"
           since that parsing stops when it reaches "potion"; also, neither
           "holy water" nor "unholy water" is an actual type of potion */
        if (!BSTRNCMPI(d->bp, d->p - 10 - 2, "un", 2))
            d->iscursed = 1, d->blessed = d->uncursed = 0; /* unholy water */
        else
            d->blessed = 1, d->iscursed = d->uncursed = 0; /* holy water */
        d->typ = POT_WATER;
        return 2; /*goto typfnd;*/
    }
    /* accept "paperback" or "paperback book", reject "paperback spellbook" */
    if (!strncmpi(d->bp, "paperback", 9)) {
        char *dbp = d->bp + 9; /* just past "paperback" */

        if (!*dbp || !strncmpi(dbp, " book", 5)) {
            d->typ = SPE_NOVEL;
            return 2; /*goto typfnd;*/
        } else {
            d->otmp = (struct obj *) 0;
            return 3;
        }
    }
    if (d->unlabeled && !BSTRCMPI(d->bp, d->p - 6, "scroll")) {
        d->typ = SCR_BLANK_PAPER;
        return 2; /*goto typfnd;*/
    }
    if (d->unlabeled && !BSTRCMPI(d->bp, d->p - 9, "spellbook")) {
        d->typ = SPE_BLANK_PAPER;
        return 2; /*goto typfnd;*/
    }
    /* specific food rather than color of gem/potion/spellbook[/scales] */
    if (!BSTRCMPI(d->bp, d->p - 6, "orange") && d->mntmp == NON_PM) {
        d->typ = ORANGE;
        return 2; /*goto typfnd;*/
    }
    /*
     * NOTE: Gold pieces are handled as objects nowadays, and therefore
     * this section should probably be reconsidered as well as the entire
     * gold/money concept.  Maybe we want to add other monetary units as
     * well in the future. (TH)
     */
    if (!BSTRCMPI(d->bp, d->p - 10, "gold piece")
        || !BSTRCMPI(d->bp, d->p - 7, "zorkmid")
        || !strcmpi(d->bp, "gold") || !strcmpi(d->bp, "money")
        || !strcmpi(d->bp, "coin") || *d->bp == GOLD_SYM) {
        if (d->cnt > 5000 && !wizard)
            d->cnt = 5000;
        else if (d->cnt < 1)
            d->cnt = 1;
        d->otmp = mksobj(GOLD_PIECE, FALSE, FALSE);
        d->otmp->quan = (long) d->cnt;
        d->otmp->owt = weight(d->otmp);
        g.context.botl = 1;
        return 3; /*return otmp;*/
    }

    /* check for single character object class code ("/" for wand, &c) */
    if (strlen(d->bp) == 1 && (i = def_char_to_objclass(*d->bp)) < MAXOCLASSES
        && i > ILLOBJ_CLASS && (i != VENOM_CLASS || wizard)) {
        d->oclass = i;
        return 4; /*goto any;*/
    }

    /* Search for class names: XXXXX potion, scroll of XXXXX.  Avoid */
    /* false hits on, e.g., rings for "ring mail". */
    if (strncmpi(d->bp, "enchant ", 8)
        && strncmpi(d->bp, "destroy ", 8)
        && strncmpi(d->bp, "detect food", 11)
        && strncmpi(d->bp, "food detection", 14)
        && strncmpi(d->bp, "ring mail", 9)
        && strncmpi(d->bp, "studded leather armor", 21)
        && strncmpi(d->bp, "leather armor", 13)
        && strncmpi(d->bp, "tooled horn", 11)
        && strncmpi(d->bp, "food ration", 11)
        && strncmpi(d->bp, "meat ring", 9))
        for (i = 0; i < (int) (sizeof wrpsym); i++) {
            register int j = Strlen(wrp[i]);

            /* check for "<class> [ of ] something" */
            if (!strncmpi(d->bp, wrp[i], j)) {
                d->oclass = wrpsym[i];
                if (d->oclass != AMULET_CLASS) {
                    d->bp += j;
                    if (!strncmpi(d->bp, " of ", 4))
                        d->actualn = d->bp + 4;
                    /* else if(*bp) ?? */
                } else
                    d->actualn = d->bp;
                return 1; /*goto srch;*/
            }
            /* check for "something <class>" */
            if (!BSTRCMPI(d->bp, d->p - j, wrp[i])) {
                d->oclass = wrpsym[i];
                /* for "foo amulet", leave the class name so that
                   wishymatch() can do "of inversion" to try matching
                   "amulet of foo"; other classes don't include their
                   class name in their full object names (where
                   "potion of healing" is just "healing", for instance) */
                if (d->oclass != AMULET_CLASS) {
                    d->p -= j;
                    *d->p = '\0';
                    if (d->p > d->bp && d->p[-1] == ' ')
                        d->p[-1] = '\0';
                } else {
                    /* amulet without "of"; convoluted wording but better a
                       special case that's handled than one that's missing */
                    if (!strncmpi(d->bp, "versus poison ", 14)) {
                        d->typ = AMULET_VERSUS_POISON;
                        return 2; /*goto typfnd;*/
                    }
                }
                d->actualn = d->dn = d->bp;
                return 1; /*goto srch;*/
            }
        }

    /* Wishing in wizard mode can create traps and furniture.
     * Part I:  distinguish between trap and object for the two
     * types of traps which have corresponding objects:  bear trap
     * and land mine.  "beartrap" (object) and "bear trap" (trap)
     * have a difference in spelling which we used to exploit by
     * adding a special case in wishymatch(), but "land mine" is
     * spelled the same either way so needs different handing.
     * Since we need something else for land mine, we've dropped
     * the bear trap hack so that both are handled exactly the
     * same.  To get an armed trap instead of a disarmed object,
     * the player can prefix either the object name or the trap
     * name with "trapped " (which ordinarily applies to chests
     * and tins), or append something--anything at all except for
     * " object", but " trap" is suggested--to either the trap
     * name or the object name.
     */
    if (wizard && (!strncmpi(d->bp, "bear", 4)
                   || !strncmpi(d->bp, "land", 4))) {
        boolean beartrap = (lowc(*d->bp) == 'b');
        char *zp = d->bp + 4; /* skip "bear"/"land" */

        if (*zp == ' ')
            ++zp; /* embedded space is optional */
        if (!strncmpi(zp, beartrap ? "trap" : "mine", 4)) {
            zp += 4;
            if (d->trapped == 2 || !strcmpi(zp, " object")) {
                /* "untrapped <foo>" or "<foo> object" */
                d->typ = beartrap ? BEARTRAP : LAND_MINE;
                return 2; /*goto typfnd;*/
            } else if (d->trapped == 1 || *zp != '\0') {
                /* "trapped <foo>" or "<foo> trap" (actually "<foo>*") */
                /* use canonical trap spelling, skip object matching */
                Strcpy(d->bp, trapname(beartrap ? BEAR_TRAP : LANDMINE, TRUE));
                return 5; /*goto wiztrap;*/
            }
            /* [no prefix or suffix; we're going to end up matching
               the object name and getting a disarmed trap object] */
        }
    }

    return 0;
}

static int
readobjnam_postparse2(struct _readobjnam_data *d)
{
    int i;

    /* "grey stone" check must be before general "stone" */
    for (i = 0; i < SIZE(o_ranges); i++)
        if (!strcmpi(d->bp, o_ranges[i].name)) {
            d->typ = rnd_class(o_ranges[i].f_o_range, o_ranges[i].l_o_range);
            return 2; /*goto typfnd;*/
        }

    if (!BSTRCMPI(d->bp, d->p - 6, " stone")
        || !BSTRCMPI(d->bp, d->p - 4, " gem")) {
        d->p[!strcmpi(d->p - 4, " gem") ? -4 : -6] = '\0';
        d->oclass = GEM_CLASS;
        d->dn = d->actualn = d->bp;
        return 1; /*goto srch;*/
    } else if (!strcmpi(d->bp, "looking glass")) {
        ; /* avoid false hit on "* glass" */
    } else if (!BSTRCMPI(d->bp, d->p - 6, " glass")
               || !strcmpi(d->bp, "glass")) {
        register char *s = d->bp;

        /* treat "broken glass" as a non-existent item; since "broken" is
           also a chest/box prefix it might have been stripped off above */
        if (d->broken || strstri(s, "broken")) {
            d->otmp = (struct obj *) 0;
            return 3; /* return otmp */
        }
        if (!strncmpi(s, "worthless ", 10))
            s += 10;
        if (!strncmpi(s, "piece of ", 9))
            s += 9;
        if (!strncmpi(s, "colored ", 8))
            s += 8;
        else if (!strncmpi(s, "coloured ", 9))
            s += 9;
        if (!strcmpi(s, "glass")) { /* choose random color */
            /* 9 different kinds */
            d->typ = LAST_GEM + rnd(NUM_GLASS_GEMS);
            if (objects[d->typ].oc_class == GEM_CLASS)
                return 2; /*goto typfnd;*/
            else
                d->typ = 0; /* somebody changed objects[]? punt */
        } else { /* try to construct canonical form */
            char tbuf[BUFSZ];

            Strcpy(tbuf, "worthless piece of ");
            Strcat(tbuf, s); /* assume it starts with the color */
            Strcpy(d->bp, tbuf);
        }
    }

    d->actualn = d->bp;
    if (!d->dn)
        d->dn = d->actualn; /* ex. "skull cap" */

    return 0;
}

static int
readobjnam_postparse3(struct _readobjnam_data *d)
{
    int i;

    /* check real names of gems first */
    if (!d->oclass && d->actualn) {
        for (i = g.bases[GEM_CLASS]; i <= LAST_GEM; i++) {
            register const char *zn;

            if ((zn = OBJ_NAME(objects[i])) != 0 && !strcmpi(d->actualn, zn)) {
                d->typ = i;
                return 2; /*goto typfnd;*/
            }
        }
        /* "tin of foo" would be caught above, but plain "tin" has
           a random chance of yielding "tin wand" unless we do this */
        if (!strcmpi(d->actualn, "tin")) {
            d->typ = TIN;
            return 2; /*goto typfnd;*/
        }
    }

    if (((d->typ = rnd_otyp_by_namedesc(d->actualn, d->oclass, 1))
         != STRANGE_OBJECT)
        || (d->dn != d->actualn
            && ((d->typ = rnd_otyp_by_namedesc(d->dn, d->oclass, 1))
                != STRANGE_OBJECT))
        || ((d->typ = rnd_otyp_by_namedesc(d->un, d->oclass, 1))
             != STRANGE_OBJECT)
        || (d->origbp != d->actualn
            && ((d->typ = rnd_otyp_by_namedesc(d->origbp, d->oclass, 1))
                != STRANGE_OBJECT)))
        return 2; /*goto typfnd;*/
    d->typ = 0;

    if (d->actualn) {
        struct Jitem *j = Japanese_items;

        while (j->item) {
            if (d->actualn && !strcmpi(d->actualn, j->name)) {
                d->typ = j->item;
                return 2; /*goto typfnd;*/
            }
            j++;
        }
    }
    /* if we've stripped off "armor" and failed to match anything
       in objects[], append "mail" and try again to catch misnamed
       requests like "plate armor" and "yellow dragon scale armor" */
    if (d->oclass == ARMOR_CLASS && !strstri(d->bp, "mail")) {
        /* modifying bp's string is ok; we're about to resort
           to random armor if this also fails to match anything */
        Strcat(d->bp, " mail");
        return 6; /*goto retry;*/
    }
    if (!strcmpi(d->bp, "spinach")) {
        d->contents = TIN_SPINACH;
        d->typ = TIN;
        return 2; /*goto typfnd;*/
    }
    /* Fruits must not mess up the ability to wish for real objects (since
     * you can leave a fruit in a bones file and it will be added to
     * another person's game), so they must be checked for last, after
     * stripping all the possible prefixes and seeing if there's a real
     * name in there.  So we have to save the full original name.  However,
     * it's still possible to do things like "uncursed burnt Alaska",
     * or worse yet, "2 burned 5 course meals", so we need to loop to
     * strip off the prefixes again, this time stripping only the ones
     * possible on food.
     * We could get even more detailed so as to allow food names with
     * prefixes that _are_ possible on food, so you could wish for
     * "2 3 alarm chilis".  Currently this isn't allowed; options.c
     * automatically sticks 'candied' in front of such names.
     */
    /* Note: not strcmpi.  2 fruits, one capital, one not, are possible.
       Also not strncmp.  We used to ignore trailing text with it, but
       that resulted in "grapefruit" matching "grape" if the latter came
       earlier than the former in the fruit list. */
    {
        char *fp;
        int l, cntf;
        int blessedf, iscursedf, uncursedf, halfeatenf;
        struct fruit *f;

        blessedf = iscursedf = uncursedf = halfeatenf = 0;
        cntf = 0;

        fp = d->fruitbuf;
        for (;;) {
            if (!fp || !*fp)
                break;
            if (!strncmpi(fp, "an ", l = 3) || !strncmpi(fp, "a ", l = 2)) {
                cntf = 1;
            } else if (!cntf && digit(*fp)) {
                cntf = atoi(fp);
                while (digit(*fp))
                    fp++;
                while (*fp == ' ')
                    fp++;
                l = 0;
            } else if (!strncmpi(fp, "blessed ", l = 8)) {
                blessedf = 1;
            } else if (!strncmpi(fp, "cursed ", l = 7)) {
                iscursedf = 1;
            } else if (!strncmpi(fp, "uncursed ", l = 9)) {
                uncursedf = 1;
            } else if (!strncmpi(fp, "partly eaten ", l = 13)
                       || !strncmpi(fp, "partially eaten ", l = 16)) {
                halfeatenf = 1;
            } else
                break;
            fp += l;
        }

        for (f = g.ffruit; f; f = f->nextf) {
            /* match type: 0=none, 1=exact, 2=singular, 3=plural */
            int ftyp = 0;

            if (!strcmp(fp, f->fname))
                ftyp = 1;
            else if (!strcmp(fp, makesingular(f->fname)))
                ftyp = 2;
            else if (!strcmp(fp, makeplural(f->fname)))
                ftyp = 3;
            if (ftyp) {
                d->typ = SLIME_MOLD;
                d->blessed = blessedf;
                d->iscursed = iscursedf;
                d->uncursed = uncursedf;
                d->halfeaten = halfeatenf;
                /* adjust count if user explicitly asked for
                   singular amount (can't happen unless fruit
                   has been given an already pluralized name)
                   or for plural amount */
                if (ftyp == 2 && !cntf)
                    cntf = 1;
                else if (ftyp == 3 && !cntf)
                    cntf = 2;
                d->cnt = cntf;
                d->ftype = f->fid;
                return 2; /*goto typfnd;*/
            }
        }
    }

    if (!d->oclass && d->actualn) {
        short objtyp;

        /* Perhaps it's an artifact specified by name, not type */
        d->name = artifact_name(d->actualn, &objtyp, TRUE);
        if (d->name) {
            d->typ = objtyp;
            return 2; /*goto typfnd;*/
        }
    }

    return 0;
}


/*
 * Return something wished for.  Specifying a null pointer for
 * the user request string results in a random object.  Otherwise,
 * if asking explicitly for "nothing" (or "nil") return no_wish;
 * if not an object return &cg.zeroobj; if an error (no matching object),
 * return null.
 */
struct obj *
readobjnam(char *bp, struct obj *no_wish)
{
    struct _readobjnam_data d;

    readobjnam_init(bp, &d);
    if (!bp)
        goto any;

    /* first, remove extra whitespace they may have typed */
    (void) mungspaces(bp);
    /* allow wishing for "nothing" to preserve wishless conduct...
       [now requires "wand of nothing" if that's what was really wanted] */
    if (!strcmpi(bp, "nothing") || !strcmpi(bp, "nil")
        || !strcmpi(bp, "none"))
        return no_wish;
    /* save the [nearly] unmodified choice string */
    Strcpy(d.fruitbuf, bp);

    if (readobjnam_preparse(&d))
        goto any;

    if (!d.cnt)
        d.cnt = 1; /* will be changed to 2 if makesingular() changes string */

    readobjnam_parse_charges(&d);

    switch (readobjnam_postparse1(&d)) {
    default:
    case 0: break;
    case 1: goto srch;
    case 2: goto typfnd;
    case 3: return d.otmp;
    case 4: goto any;
    case 5: goto wiztrap;
    }

 retry:
    switch (readobjnam_postparse2(&d)) {
    default:
    case 0: break;
    case 1: goto srch;
    case 2: goto typfnd;
    case 3: return d.otmp;
    case 4: goto any;
    case 5: goto wiztrap;
    }

 srch:
    switch (readobjnam_postparse3(&d)) {
    default:
    case 0: break;
    case 1: goto srch;
    case 2: goto typfnd;
    case 3: return d.otmp;
    case 4: goto any;
    case 5: goto wiztrap;
    case 6: goto retry;
    }

    /*
     * Let wizards wish for traps and furniture.
     * Must come after objects check so wizards can still wish for
     * trap objects like beartraps.
     * Disallow such topology tweaks for WIZKIT startup wishes.
     */
 wiztrap:
    if (wizard && !g.program_state.wizkit_wishing && !d.oclass) {
        /* [inline code moved to separate routine to unclutter readobjnam] */
        if ((d.otmp = wizterrainwish(&d)) != 0)
            return d.otmp;
    }

    if (!d.oclass && !d.typ) {
        if (!strncmpi(d.bp, "polearm", 7)) {
            d.typ = rnd_otyp_by_wpnskill(P_POLEARMS);
            goto typfnd;
        } else if (!strncmpi(d.bp, "hammer", 6)) {
            d.typ = rnd_otyp_by_wpnskill(P_HAMMER);
            goto typfnd;
        }
    }

    if (!d.oclass)
        return ((struct obj *) 0);
 any:
    if (!d.oclass)
        d.oclass = wrpsym[rn2((int) sizeof wrpsym)];
 typfnd:
    if (d.typ)
        d.oclass = objects[d.typ].oc_class;

    /* handle some objects that are only allowed in wizard mode */
    if (d.typ && !wizard) {
        switch (d.typ) {
        case AMULET_OF_YENDOR:
            d.typ = FAKE_AMULET_OF_YENDOR;
            break;
        case CANDELABRUM_OF_INVOCATION:
            d.typ = rnd_class(TALLOW_CANDLE, WAX_CANDLE);
            break;
        case BELL_OF_OPENING:
            d.typ = BELL;
            break;
        case SPE_BOOK_OF_THE_DEAD:
            d.typ = SPE_BLANK_PAPER;
            break;
        case MAGIC_LAMP:
            d.typ = OIL_LAMP;
            break;
        default:
            /* catch any other non-wishable objects (venom) */
            if (objects[d.typ].oc_nowish)
                return (struct obj *) 0;
            break;
        }
    }

    /* if asking for corpse of a monster which leaves behind a glob, give
       glob instead of rejecting the monster type to create random corpse */
    if (d.typ == CORPSE && d.mntmp >= LOW_PM
        && mons[d.mntmp].mlet == S_PUDDING) {
        d.typ = GLOB_OF_GRAY_OOZE + (d.mntmp - PM_GRAY_OOZE);
        d.mntmp = NON_PM; /* not used for globs */
    }
    /*
     * Create the object, then fine-tune it.
     */
    d.otmp = d.typ ? mksobj(d.typ, TRUE, FALSE) : mkobj(d.oclass, FALSE);
    d.typ = d.otmp->otyp, d.oclass = d.otmp->oclass; /* what we actually got */

    /* if player specified a reasonable count, maybe honor it;
       quantity for gold is handled elsewhere and d.cnt is 0 for it here */
    if (d.otmp->globby) {
        /* for globs, calculate weight based on gsize, then multiply by cnt;
           asking for 2 globs or for 2 small globs produces 1 small glob
           weighing 40au instead of normal 20au; asking for 5 medium globs
           might produce 1 very large glob weighing 600au */
        d.otmp->quan = 1L; /* always 1 for globs */
        d.otmp->owt = weight(d.otmp);
        /* gsize 0: unspecified => small;
           1: small (1..5) => keep default owt for 1, yielding 20;
           2: medium (6..15) => use weight for 6, yielding 120;
           3: large (16..25) => 320; 4: very large (26+) => 520 */
        if (d.gsize > 1)
            d.otmp->owt += ((unsigned) (5 + (d.gsize - 2) * 10)
                            * d.otmp->owt);  /* 20 + {5|15|25} times 20 */
        /* limit overall weight which limits shrink-away time which in turn
           affects how long some of it will remain available to be eaten */
        if (d.cnt > 1) {
            int rn1cnt = rn1(5, 2); /* 2..6 */

            if (rn1cnt > 6 - d.gsize)
                rn1cnt = 6 - d.gsize;
            if (d.cnt > rn1cnt
                && (!wizard || g.program_state.wizkit_wishing
                    || yn("Override glob weight limit?") != 'y'))
                d.cnt = rn1cnt;
            d.otmp->owt *= (unsigned) d.cnt;
        }
        /* note: the owt assignment below will not change glob's weight */
        d.cnt = 0;
    } else if (d.cnt > 0) {
        if (objects[d.typ].oc_merge
            && (wizard /* quantity isn't restricted when debugging */
                /* note: in normal play, explicitly asking for 1 might
                   fail the 'cnt < rnd(6)' test and could produce more
                   than 1 if mksobj() creates the item that way */
                || d.cnt < rnd(6)
                || (d.cnt <= 7 && Is_candle(d.otmp))
                || (d.cnt <= 20
                    && (d.typ == ROCK || d.typ == FLINT || is_missile(d.otmp)
                        /* WEAPON_CLASS test excludes gems, gray stones */
                        || (d.oclass == WEAPON_CLASS && is_ammo(d.otmp))))))
            d.otmp->quan = (long) d.cnt;
    }

    if (d.islit && (d.typ == OIL_LAMP || d.typ == MAGIC_LAMP
                    || d.typ == BRASS_LANTERN
                    || Is_candle(d.otmp) || d.typ == POT_OIL)) {
        place_object(d.otmp, u.ux, u.uy); /* make it viable light source */
        begin_burn(d.otmp, FALSE);
        obj_extract_self(d.otmp); /* now release it for caller's use */
    }

    if (d.spesgn == 0) {
        /* spe not specifed; retain the randomly assigned value */
        d.spe = d.otmp->spe;
    } else if (wizard) {
        ; /* no restrictions except SPE_LIM */
    } else if (d.oclass == ARMOR_CLASS || d.oclass == WEAPON_CLASS
               || is_weptool(d.otmp)
               || (d.oclass == RING_CLASS && objects[d.typ].oc_charged)) {
        if (d.spe > rnd(5) && d.spe > d.otmp->spe)
            d.spe = 0;
        if (d.spe > 2 && Luck < 0)
            d.spesgn = -1;
    } else {
        /* crystal ball cancels like a wand, to (n:-1) */
        if (d.oclass == WAND_CLASS || d.typ == CRYSTAL_BALL) {
            if (d.spe > 1 && d.spesgn == -1)
                d.spe = 1;
        } else {
            if (d.spe > 0 && d.spesgn == -1)
                d.spe = 0;
        }
        if (d.spe > d.otmp->spe)
            d.spe = d.otmp->spe;
    }

    if (d.spesgn == -1)
        d.spe = -d.spe;

    /* set otmp->spe.  This may, or may not, use d.spe... */
    switch (d.typ) {
    case TIN:
        d.otmp->spe = 0; /* default: not spinach */
        if (d.contents == TIN_EMPTY) {
            d.otmp->corpsenm = NON_PM;
        } else if (d.contents == TIN_SPINACH) {
            d.otmp->corpsenm = NON_PM;
            d.otmp->spe = 1; /* spinach after all */
        }
        break;
    case TOWEL:
        if (d.wetness)
            d.otmp->spe = d.wetness;
        break;
    case SLIME_MOLD:
        d.otmp->spe = d.ftype;
    /* Fall through */
    case SKELETON_KEY:
    case CHEST:
    case LARGE_BOX:
    case HEAVY_IRON_BALL:
    case IRON_CHAIN:
        break;
    case STATUE: /* otmp->cobj already done in mksobj() */
    case FIGURINE:
    case CORPSE: {
        struct permonst *P = (d.mntmp >= LOW_PM) ? &mons[d.mntmp] : 0;

        d.otmp->spe = !P ? CORPSTAT_RANDOM
                      /* if neuter, force neuter regardless of wish request */
                      : is_neuter(P) ? CORPSTAT_NEUTER
                        /* not neuter, honor wish unless it conflicts */
                        : (d.mgend == FEMALE && !is_male(P)) ? CORPSTAT_FEMALE
                          : (d.mgend == MALE && !is_female(P)) ? CORPSTAT_MALE
                            /* unspecified or wish conflicts */
                            : CORPSTAT_RANDOM;
        if (P && d.otmp->spe == CORPSTAT_RANDOM)
            d.otmp->spe = is_male(P) ? CORPSTAT_MALE
                          : is_female(P) ? CORPSTAT_FEMALE
                            : rn2(2) ? CORPSTAT_MALE : CORPSTAT_FEMALE;
        if (d.ishistoric && d.typ == STATUE)
            d.otmp->spe |= CORPSTAT_HISTORIC;
        break;
    };
#ifdef MAIL_STRUCTURES
    /* scroll of mail:  0: delivered in-game via external event (or randomly
       for fake mail); 1: from bones or wishing; 2: written with marker */
    case SCR_MAIL:
        /*FALLTHRU*/
#endif
    /* splash of venom:  0: normal, and transitory; 1: wishing */
    case ACID_VENOM:
    case BLINDING_VENOM:
        d.otmp->spe = 1;
        break;
    case WAN_WISHING:
        if (!wizard) {
            d.otmp->spe = (rn2(10) ? -1 : 0);
            break;
        }
        /*FALLTHRU*/
    default:
        d.otmp->spe = d.spe;
    }

    /* set otmp->corpsenm or dragon scale [mail] */
    if (d.mntmp >= LOW_PM) {
        int humanwere;

        if (d.mntmp == PM_LONG_WORM_TAIL)
            d.mntmp = PM_LONG_WORM;
        /* werecreatures in beast form are all flagged no-corpse so for
           corpses and tins, switch to their corresponding human form;
           for figurines, override the can't-be-human restriction instead */
        if (d.typ != FIGURINE && is_were(&mons[d.mntmp])
            && (g.mvitals[d.mntmp].mvflags & G_NOCORPSE) != 0
            && (humanwere = counter_were(d.mntmp)) != NON_PM)
            d.mntmp = humanwere;

        switch (d.typ) {
        case TIN:
            if (dead_species(d.mntmp, FALSE)) {
                d.otmp->corpsenm = NON_PM; /* it's empty */
            } else if ((!(mons[d.mntmp].geno & G_UNIQ) || wizard)
                       && !(g.mvitals[d.mntmp].mvflags & G_NOCORPSE)
                       && mons[d.mntmp].cnutrit != 0) {
                d.otmp->corpsenm = d.mntmp;
            }
            break;
        case CORPSE:
            if ((!(mons[d.mntmp].geno & G_UNIQ) || wizard)
                && !(g.mvitals[d.mntmp].mvflags & G_NOCORPSE)) {
                if (mons[d.mntmp].msound == MS_GUARDIAN)
                    d.mntmp = genus(d.mntmp, 1);
                set_corpsenm(d.otmp, d.mntmp);
            }
            if (d.zombify && zombie_form(&mons[d.mntmp])) {
                (void) start_timer(rn1(5, 10), TIMER_OBJECT,
                                   ZOMBIFY_MON, obj_to_any(d.otmp));
            }
            break;
        case EGG:
            d.mntmp = can_be_hatched(d.mntmp);
            /* this also sets hatch timer if appropriate */
            set_corpsenm(d.otmp, d.mntmp);
            break;
        case FIGURINE:
            if (!(mons[d.mntmp].geno & G_UNIQ)
                && (!is_human(&mons[d.mntmp]) || is_were(&mons[d.mntmp]))
#ifdef MAIL_STRUCTURES
                && d.mntmp != PM_MAIL_DAEMON
#endif
                )
                d.otmp->corpsenm = d.mntmp;
            break;
        case STATUE:
            d.otmp->corpsenm = d.mntmp;
            if (Has_contents(d.otmp) && verysmall(&mons[d.mntmp]))
                delete_contents(d.otmp); /* no spellbook */
            break;
        case SCALE_MAIL:
            /* Dragon mail - depends on the order of objects & dragons. */
            if (d.mntmp >= PM_GRAY_DRAGON && d.mntmp <= PM_YELLOW_DRAGON)
                d.otmp->otyp = GRAY_DRAGON_SCALE_MAIL
                              + d.mntmp - PM_GRAY_DRAGON;
            break;
        }
    }

    /* set blessed/cursed -- setting the fields directly is safe
     * since weight() is called below and addinv() will take care
     * of luck */
    if (d.iscursed) {
        curse(d.otmp);
    } else if (d.uncursed) {
        d.otmp->blessed = 0;
        d.otmp->cursed = (Luck < 0 && !wizard);
    } else if (d.blessed) {
        d.otmp->blessed = (Luck >= 0 || wizard);
        d.otmp->cursed = (Luck < 0 && !wizard);
    } else if (d.spesgn < 0) {
        curse(d.otmp);
    }

    /* set eroded and erodeproof */
    if (erosion_matters(d.otmp)) {
        if (d.eroded && (is_flammable(d.otmp) || is_rustprone(d.otmp)))
            d.otmp->oeroded = d.eroded;
        if (d.eroded2 && (is_corrodeable(d.otmp) || is_rottable(d.otmp)))
            d.otmp->oeroded2 = d.eroded2;
        /*
         * 3.6.1: earlier versions included `&& !eroded && !eroded2' here,
         * but damageproof combined with damaged is feasible (eroded
         * armor modified by confused reading of cursed destroy armor)
         * so don't prevent player from wishing for such a combination.
         */
        if (d.erodeproof
            && (is_damageable(d.otmp) || d.otmp->otyp == CRYSKNIFE))
            d.otmp->oerodeproof = (Luck >= 0 || wizard);
    }

    /* set otmp->recharged */
    if (d.oclass == WAND_CLASS) {
        /* prevent wishing abuse */
        if (d.otmp->otyp == WAN_WISHING && !wizard)
            d.rechrg = 1;
        d.otmp->recharged = (unsigned) d.rechrg;
    }

    /* set poisoned */
    if (d.ispoisoned) {
        if (is_poisonable(d.otmp))
            d.otmp->opoisoned = (Luck >= 0);
        else if (d.oclass == FOOD_CLASS)
            /* try to taint by making it as old as possible */
            d.otmp->age = 1L;
    }
    /* and [un]trapped */
    if (d.trapped) {
        if (Is_box(d.otmp) || d.typ == TIN)
            d.otmp->otrapped = (d.trapped == 1);
    }
    /* empty for containers rather than for tins */
    if (d.contents == TIN_EMPTY) {
        if (d.otmp->otyp == BAG_OF_TRICKS || d.otmp->otyp == HORN_OF_PLENTY) {
            if (d.otmp->spe > 0)
                d.otmp->spe = 0;
        } else if (Has_contents(d.otmp)) {
            /* this assumes that artifacts can't be randomly generated
               inside containers */
            delete_contents(d.otmp);
            d.otmp->owt = weight(d.otmp);
        }
    }
    /* set locked/unlocked/broken */
    if (Is_box(d.otmp)) {
        if (d.locked) {
            d.otmp->olocked = 1, d.otmp->obroken = 0;
        } else if (d.unlocked) {
            d.otmp->olocked = 0, d.otmp->obroken = 0;
        } else if (d.broken) {
            d.otmp->olocked = 0, d.otmp->obroken = 1;
        }
        if (d.otmp->obroken)
            d.otmp->otrapped = 0;
    }

    if (d.isgreased)
        d.otmp->greased = 1;

    if (d.isdiluted && d.otmp->oclass == POTION_CLASS)
        d.otmp->odiluted = (d.otmp->otyp != POT_WATER);

    /* set tin variety */
    if (d.otmp->otyp == TIN && d.tvariety >= 0 && (rn2(4) || wizard))
        set_tin_variety(d.otmp, d.tvariety);

    if (d.name) {
        const char *aname, *novelname;
        short objtyp;

        /* an artifact name might need capitalization fixing */
        aname = artifact_name(d.name, &objtyp, TRUE);
        if (aname && objtyp == d.otmp->otyp)
            d.name = aname;

        /* 3.6 tribute - fix up novel */
        if (d.otmp->otyp == SPE_NOVEL
            && (novelname = lookup_novel(d.name, &d.otmp->novelidx)) != 0)
            d.name = novelname;

        d.otmp = oname(d.otmp, d.name, ONAME_WISH);
        /* name==aname => wished for artifact (otmp->oartifact => got it) */
        if (d.otmp->oartifact || d.name == aname) {
            d.otmp->quan = 1L;
            u.uconduct.wisharti++; /* KMH, conduct */
        }
    }

    /* more wishing abuse: don't allow wishing for certain artifacts */
    /* and make them pay; charge them for the wish anyway! */
    if ((is_quest_artifact(d.otmp)
         || (d.otmp->oartifact && rn2(nartifact_exist()) > 1)) && !wizard) {
        artifact_exists(d.otmp, safe_oname(d.otmp), FALSE, ONAME_NO_FLAGS);
        obfree(d.otmp, (struct obj *) 0);
        d.otmp = (struct obj *) &cg.zeroobj;
        pline("For a moment, you feel %s in your %s, but it disappears!",
              something, makeplural(body_part(HAND)));
        return d.otmp;
    }

    if (d.halfeaten && d.otmp->oclass == FOOD_CLASS) {
        unsigned nut = obj_nutrition(d.otmp);

        /* do this adjustment before setting up object's weight; skip
           "partly eaten" for food with 0 nutrition (wraith corpse) or for
           anything that couldn't take more than one bite (1 nutrition;
           ought to check for one-bite instead but that's complicated) */
        if (nut > 1) {
            d.otmp->oeaten = nut;
            consume_oeaten(d.otmp, 1);
        }
    }
    d.otmp->owt = weight(d.otmp);
    if (d.very && d.otmp->otyp == HEAVY_IRON_BALL)
        d.otmp->owt += IRON_BALL_W_INCR;

    return d.otmp;
}

int
rnd_class(int first, int last)
{
    int i, x, sum = 0;

    if (last > first) {
        for (i = first; i <= last; i++)
            sum += objects[i].oc_prob;
        if (!sum) /* all zero, so equal probability */
            return rn1(last - first + 1, first);

        x = rnd(sum);
        for (i = first; i <= last; i++)
            if ((x -= objects[i].oc_prob) <= 0)
                return i;
    }
    return (first == last) ? first : STRANGE_OBJECT;
}

static const char *
Japanese_item_name(int i)
{
    struct Jitem *j = Japanese_items;

    while (j->item) {
        if (i == j->item)
            return j->name;
        j++;
    }
    return (const char *) 0;
}

const char *
suit_simple_name(struct obj *suit)
{
    const char *suitnm, *esuitp;

    if (suit) {
        if (Is_dragon_mail(suit))
            return "dragon mail"; /* <color> dragon scale mail */
        else if (Is_dragon_scales(suit))
            return "dragon scales";
        suitnm = OBJ_NAME(objects[suit->otyp]);
        esuitp = eos((char *) suitnm);
        if (strlen(suitnm) > 5 && !strcmp(esuitp - 5, " mail"))
            return "mail"; /* most suits fall into this category */
        else if (strlen(suitnm) > 7 && !strcmp(esuitp - 7, " jacket"))
            return "jacket"; /* leather jacket */
    }
    /* "suit" is lame but "armor" is ambiguous and "body armor" is absurd */
    return "suit";
}

const char *
cloak_simple_name(struct obj *cloak)
{
    if (cloak) {
        switch (cloak->otyp) {
        case ROBE:
            return "robe";
        case MUMMY_WRAPPING:
            return "wrapping";
        case ALCHEMY_SMOCK:
            return (objects[cloak->otyp].oc_name_known && cloak->dknown)
                       ? "smock"
                       : "apron";
        default:
            break;
        }
    }
    return "cloak";
}

/* helm vs hat for messages */
const char *
helm_simple_name(struct obj *helmet)
{
    /*
     *  There is some wiggle room here; the result has been chosen
     *  for consistency with the "protected by hard helmet" messages
     *  given for various bonks on the head:  headgear that provides
     *  such protection is a "helm", that which doesn't is a "hat".
     *
     *      elven leather helm / leather hat    -> hat
     *      dwarvish iron helm / hard hat       -> helm
     *  The rest are completely straightforward:
     *      fedora, cornuthaum, dunce cap       -> hat
     *      all other types of helmets          -> helm
     */
    return (helmet && !is_metallic(helmet)) ? "hat" : "helm";
}

/* gloves vs gauntlets; depends upon discovery state */
const char *
gloves_simple_name(struct obj *gloves)
{
    static const char gauntlets[] = "gauntlets";

    if (gloves && gloves->dknown) {
        int otyp = gloves->otyp;
        struct objclass *ocl = &objects[otyp];
        const char *actualn = OBJ_NAME(*ocl),
                   *descrpn = OBJ_DESCR(*ocl);

        if (strstri(objects[otyp].oc_name_known ? actualn : descrpn,
                    gauntlets))
            return gauntlets;
    }
    return "gloves";
}

/* boots vs shoes; depends upon discovery state */
const char *
boots_simple_name(struct obj *boots)
{
    static const char shoes[] = "shoes";

    if (boots && boots->dknown) {
        int otyp = boots->otyp;
        struct objclass *ocl = &objects[otyp];
        const char *actualn = OBJ_NAME(*ocl),
                   *descrpn = OBJ_DESCR(*ocl);

        if (strstri(descrpn, shoes)
            || (objects[otyp].oc_name_known && strstri(actualn, shoes)))
            return shoes;
    }
    return "boots";
}

/* simplified shield for messages */
const char *
shield_simple_name(struct obj *shield)
{
    if (shield) {
        /* xname() describes unknown (unseen) reflection as smooth */
        if (shield->otyp == SHIELD_OF_REFLECTION)
            return shield->dknown ? "silver shield" : "smooth shield";
        /*
         * We might distinguish between wooden vs metallic or
         * light vs heavy to give small benefit to spell casters.
         * Fighter types probably care more about the former for
         * vulnerability to fire or rust.
         *
         * We could do that both ways: light wooden shield, light
         * metallic shield (there aren't any), heavy wooden shield,
         * and heavy metallic shield but that's getting away from
         * "simple name" which is intended to be shorter as well
         * as less detailed than xname().
         */
#if 0
        /* spellcasting uses a division like this */
        return (weight(shield) > (int) objects[SMALL_SHIELD].oc_weight)
               ? "heavy shield"
               : "light shield";
#endif
    }
    return "shield";
}

/* for completness */
const char *
shirt_simple_name(struct obj *shirt UNUSED)
{
    return "shirt";
}

const char *
mimic_obj_name(struct monst *mtmp)
{
    if (M_AP_TYPE(mtmp) == M_AP_OBJECT) {
        if (mtmp->mappearance == GOLD_PIECE)
            return "gold";
        if (mtmp->mappearance != STRANGE_OBJECT)
            return simple_typename(mtmp->mappearance);
    }
    return "whatcha-may-callit";
}

/*
 * Construct a query prompt string, based around an object name, which is
 * guaranteed to fit within [QBUFSZ].  Takes an optional prefix, three
 * choices for filling in the middle (two object formatting functions and a
 * last resort literal which should be very short), and an optional suffix.
 */
char *
safe_qbuf(
    char *qbuf, /* output buffer */
    const char *qprefix,
    const char *qsuffix,
    struct obj *obj,
    char *(*func)(OBJ_P),
    char *(*altfunc)(OBJ_P),
    const char *lastR)
{
    char *bufp, *endp;
    /* convert size_t (or int for ancient systems) to ordinary unsigned */
    unsigned len, lenlimit,
        len_qpfx = (unsigned) (qprefix ? strlen(qprefix) : 0),
        len_qsfx = (unsigned) (qsuffix ? strlen(qsuffix) : 0),
        len_lastR = (unsigned) strlen(lastR);

    lenlimit = QBUFSZ - 1;
    endp = qbuf + lenlimit;
    /* sanity check, aimed mainly at paniclog (it's conceivable for
       the result of short_oname() to be shorter than the length of
       the last resort string, but we ignore that possibility here) */
    if (len_qpfx > lenlimit)
        impossible("safe_qbuf: prefix too long (%u characters).", len_qpfx);
    else if (len_qpfx + len_qsfx > lenlimit)
        impossible("safe_qbuf: suffix too long (%u + %u characters).",
                   len_qpfx, len_qsfx);
    else if (len_qpfx + len_lastR + len_qsfx > lenlimit)
        impossible("safe_qbuf: filler too long (%u + %u + %u characters).",
                   len_qpfx, len_lastR, len_qsfx);

    /* the output buffer might be the same as the prefix if caller
       has already partially filled it */
    if (qbuf == qprefix) {
        /* prefix is already in the buffer */
        *endp = '\0';
    } else if (qprefix) {
        /* put prefix into the buffer */
        (void) strncpy(qbuf, qprefix, lenlimit);
        *endp = '\0';
    } else {
        /* no prefix; output buffer starts out empty */
        qbuf[0] = '\0';
    }
    len = (unsigned) strlen(qbuf);

    if (len + len_lastR + len_qsfx > lenlimit) {
        /* too long; skip formatting, last resort output is truncated */
        if (len < lenlimit) {
            (void) strncpy(&qbuf[len], lastR, lenlimit - len);
            *endp = '\0';
            len = (unsigned) strlen(qbuf);
            if (qsuffix && len < lenlimit) {
                (void) strncpy(&qbuf[len], qsuffix, lenlimit - len);
                *endp = '\0';
                /* len = (unsigned) strlen(qbuf); */
            }
        }
    } else {
        /* suffix and last resort are guaranteed to fit */
        len += len_qsfx; /* include the pending suffix */
        /* format the object */
        bufp = short_oname(obj, func, altfunc, lenlimit - len);
        if (len + strlen(bufp) <= lenlimit)
            Strcat(qbuf, bufp); /* formatted name fits */
        else
            Strcat(qbuf, lastR); /* use last resort */
        releaseobuf(bufp);

        if (qsuffix)
            Strcat(qbuf, qsuffix);
    }
    /* assert( strlen(qbuf) < QBUFSZ ); */
    return qbuf;
}

/*objnam.c*/
