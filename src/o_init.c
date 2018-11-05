/* NetHack 3.6	o_init.c	$NHDT-Date: 1528332336 2018/06/07 00:45:36 $  $NHDT-Branch: NetHack-3.6.2 $:$NHDT-Revision: 1.24 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2011. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "lev.h" /* save & restore info */

STATIC_DCL void FDECL(setgemprobs, (d_level *));
STATIC_DCL void FDECL(shuffle, (int, int, BOOLEAN_P));
STATIC_DCL void NDECL(shuffle_all);
STATIC_DCL boolean FDECL(interesting_to_discover, (int));
STATIC_DCL char *FDECL(oclass_to_name, (CHAR_P, char *));

static NEARDATA short disco[NUM_OBJECTS] = DUMMY;

#ifdef USE_TILES
STATIC_DCL void NDECL(shuffle_tiles);
extern short glyph2tile[]; /* from tile.c */

/* Shuffle tile assignments to match descriptions, so a red potion isn't
 * displayed with a blue tile and so on.
 *
 * Tile assignments are not saved, and shouldn't be so that a game can
 * be resumed on an otherwise identical non-tile-using binary, so we have
 * to reshuffle the assignments from oc_descr_idx information when a game
 * is restored.  So might as well do that the first time instead of writing
 * another routine.
 */
STATIC_OVL void
shuffle_tiles()
{
    int i;
    short tmp_tilemap[NUM_OBJECTS];

    for (i = 0; i < NUM_OBJECTS; i++)
        tmp_tilemap[i] = glyph2tile[objects[i].oc_descr_idx + GLYPH_OBJ_OFF];

    for (i = 0; i < NUM_OBJECTS; i++)
        glyph2tile[i + GLYPH_OBJ_OFF] = tmp_tilemap[i];
}
#endif /* USE_TILES */

STATIC_OVL void
setgemprobs(dlev)
d_level *dlev;
{
    int j, first, lev;

    if (dlev)
        lev = (ledger_no(dlev) > maxledgerno()) ? maxledgerno()
                                                : ledger_no(dlev);
    else
        lev = 0;
    first = bases[GEM_CLASS];

    for (j = 0; j < 9 - lev / 3; j++)
        objects[first + j].oc_prob = 0;
    first += j;
    if (first > LAST_GEM || objects[first].oc_class != GEM_CLASS
        || OBJ_NAME(objects[first]) == (char *) 0) {
        raw_printf("Not enough gems? - first=%d j=%d LAST_GEM=%d", first, j,
                   LAST_GEM);
        wait_synch();
    }
    for (j = first; j <= LAST_GEM; j++)
        objects[j].oc_prob = (171 + j - first) / (LAST_GEM + 1 - first);
}

/* shuffle descriptions on objects o_low to o_high */
STATIC_OVL void
shuffle(o_low, o_high, domaterial)
int o_low, o_high;
boolean domaterial;
{
    int i, j, num_to_shuffle;
    short sw;
    int color;

    for (num_to_shuffle = 0, j = o_low; j <= o_high; j++)
        if (!objects[j].oc_name_known)
            num_to_shuffle++;
    if (num_to_shuffle < 2)
        return;

    for (j = o_low; j <= o_high; j++) {
        if (objects[j].oc_name_known)
            continue;
        do
            i = j + rn2(o_high - j + 1);
        while (objects[i].oc_name_known);
        sw = objects[j].oc_descr_idx;
        objects[j].oc_descr_idx = objects[i].oc_descr_idx;
        objects[i].oc_descr_idx = sw;
        sw = objects[j].oc_tough;
        objects[j].oc_tough = objects[i].oc_tough;
        objects[i].oc_tough = sw;
        color = objects[j].oc_color;
        objects[j].oc_color = objects[i].oc_color;
        objects[i].oc_color = color;

        /* shuffle material */
        if (domaterial) {
            sw = objects[j].oc_material;
            objects[j].oc_material = objects[i].oc_material;
            objects[i].oc_material = sw;
        }
    }
}

void
init_objects()
{
    register int i, first, last, sum;
    register char oclass;
#ifdef TEXTCOLOR
#define COPY_OBJ_DESCR(o_dst, o_src) \
    o_dst.oc_descr_idx = o_src.oc_descr_idx, o_dst.oc_color = o_src.oc_color
#else
#define COPY_OBJ_DESCR(o_dst, o_src) o_dst.oc_descr_idx = o_src.oc_descr_idx
#endif

    /* bug fix to prevent "initialization error" abort on Intel Xenix.
     * reported by mikew@semike
     */
    for (i = 0; i < MAXOCLASSES; i++)
        bases[i] = 0;
    /* initialize object descriptions */
    for (i = 0; i < NUM_OBJECTS; i++)
        objects[i].oc_name_idx = objects[i].oc_descr_idx = i;
    /* init base; if probs given check that they add up to 1000,
       otherwise compute probs */
    first = 0;
    while (first < NUM_OBJECTS) {
        oclass = objects[first].oc_class;
        last = first + 1;
        while (last < NUM_OBJECTS && objects[last].oc_class == oclass)
            last++;
        bases[(int) oclass] = first;

        if (oclass == GEM_CLASS) {
            setgemprobs((d_level *) 0);

            if (rn2(2)) { /* change turquoise from green to blue? */
                COPY_OBJ_DESCR(objects[TURQUOISE], objects[SAPPHIRE]);
            }
            if (rn2(2)) { /* change aquamarine from green to blue? */
                COPY_OBJ_DESCR(objects[AQUAMARINE], objects[SAPPHIRE]);
            }
            switch (rn2(4)) { /* change fluorite from violet? */
            case 0:
                break;
            case 1: /* blue */
                COPY_OBJ_DESCR(objects[FLUORITE], objects[SAPPHIRE]);
                break;
            case 2: /* white */
                COPY_OBJ_DESCR(objects[FLUORITE], objects[DIAMOND]);
                break;
            case 3: /* green */
                COPY_OBJ_DESCR(objects[FLUORITE], objects[EMERALD]);
                break;
            }
        }
    check:
        sum = 0;
        for (i = first; i < last; i++)
            sum += objects[i].oc_prob;
        if (sum == 0) {
            for (i = first; i < last; i++)
                objects[i].oc_prob = (1000 + i - first) / (last - first);
            goto check;
        }
        if (sum != 1000)
            error("init-prob error for class %d (%d%%)", oclass, sum);
        first = last;
    }
    /* shuffle descriptions */
    shuffle_all();
#ifdef USE_TILES
    shuffle_tiles();
#endif
    objects[WAN_NOTHING].oc_dir = rn2(2) ? NODIR : IMMEDIATE;
}

/* retrieve the range of objects that otyp shares descriptions with */
void
obj_shuffle_range(otyp, lo_p, hi_p)
int otyp;         /* input: representative item */
int *lo_p, *hi_p; /* output: range that item belongs among */
{
    int i, ocls = objects[otyp].oc_class;

    /* default is just the object itself */
    *lo_p = *hi_p = otyp;

    switch (ocls) {
    case ARMOR_CLASS:
        if (otyp >= HELMET && otyp <= HELM_OF_TELEPATHY)
            *lo_p = HELMET, *hi_p = HELM_OF_TELEPATHY;
        else if (otyp >= LEATHER_GLOVES && otyp <= GAUNTLETS_OF_DEXTERITY)
            *lo_p = LEATHER_GLOVES, *hi_p = GAUNTLETS_OF_DEXTERITY;
        else if (otyp >= CLOAK_OF_PROTECTION && otyp <= CLOAK_OF_DISPLACEMENT)
            *lo_p = CLOAK_OF_PROTECTION, *hi_p = CLOAK_OF_DISPLACEMENT;
        else if (otyp >= SPEED_BOOTS && otyp <= LEVITATION_BOOTS)
            *lo_p = SPEED_BOOTS, *hi_p = LEVITATION_BOOTS;
        break;
    case POTION_CLASS:
        /* potion of water has the only fixed description */
        *lo_p = bases[POTION_CLASS];
        *hi_p = POT_WATER - 1;
        break;
    case AMULET_CLASS:
    case SCROLL_CLASS:
    case SPBOOK_CLASS:
        /* exclude non-magic types and also unique ones */
        *lo_p = bases[ocls];
        for (i = *lo_p; objects[i].oc_class == ocls; i++)
            if (objects[i].oc_unique || !objects[i].oc_magic)
                break;
        *hi_p = i - 1;
        break;
    case RING_CLASS:
    case WAND_CLASS:
    case VENOM_CLASS:
        /* entire class */
        *lo_p = bases[ocls];
        for (i = *lo_p; objects[i].oc_class == ocls; i++)
            continue;
        *hi_p = i - 1;
        break;
    }

    /* artifact checking might ask about item which isn't part of any range
       but fell within the classes that do have ranges specified above */
    if (otyp < *lo_p || otyp > *hi_p)
        *lo_p = *hi_p = otyp;
    return;
}

/* randomize object descriptions */
STATIC_OVL void
shuffle_all()
{
    /* entire classes; obj_shuffle_range() handles their exceptions */
    static char shuffle_classes[] = {
        AMULET_CLASS, POTION_CLASS, RING_CLASS,  SCROLL_CLASS,
        SPBOOK_CLASS, WAND_CLASS,   VENOM_CLASS,
    };
    /* sub-class type ranges (one item from each group) */
    static short shuffle_types[] = {
        HELMET, LEATHER_GLOVES, CLOAK_OF_PROTECTION, SPEED_BOOTS,
    };
    int first, last, idx;

    /* do whole classes (amulets, &c) */
    for (idx = 0; idx < SIZE(shuffle_classes); idx++) {
        obj_shuffle_range(bases[(int) shuffle_classes[idx]], &first, &last);
        shuffle(first, last, TRUE);
    }
    /* do type ranges (helms, &c) */
    for (idx = 0; idx < SIZE(shuffle_types); idx++) {
        obj_shuffle_range(shuffle_types[idx], &first, &last);
        shuffle(first, last, FALSE);
    }
    return;
}

/* find the object index for snow boots; used [once] by slippery ice code */
int
find_skates()
{
    register int i;
    register const char *s;

    for (i = SPEED_BOOTS; i <= LEVITATION_BOOTS; i++)
        if ((s = OBJ_DESCR(objects[i])) != 0 && !strcmp(s, "snow boots"))
            return i;

    impossible("snow boots not found?");
    return -1; /* not 0, or caller would try again each move */
}

/* level dependent initialization */
void
oinit()
{
    setgemprobs(&u.uz);
}

void
savenames(fd, mode)
int fd, mode;
{
    register int i;
    unsigned int len;

    if (perform_bwrite(mode)) {
        bwrite(fd, (genericptr_t) bases, sizeof bases);
        bwrite(fd, (genericptr_t) disco, sizeof disco);
        bwrite(fd, (genericptr_t) objects,
               sizeof(struct objclass) * NUM_OBJECTS);
    }
    /* as long as we use only one version of Hack we
       need not save oc_name and oc_descr, but we must save
       oc_uname for all objects */
    for (i = 0; i < NUM_OBJECTS; i++)
        if (objects[i].oc_uname) {
            if (perform_bwrite(mode)) {
                len = strlen(objects[i].oc_uname) + 1;
                bwrite(fd, (genericptr_t) &len, sizeof len);
                bwrite(fd, (genericptr_t) objects[i].oc_uname, len);
            }
            if (release_data(mode)) {
                free((genericptr_t) objects[i].oc_uname);
                objects[i].oc_uname = 0;
            }
        }
}

void
restnames(fd)
register int fd;
{
    register int i;
    unsigned int len;

    mread(fd, (genericptr_t) bases, sizeof bases);
    mread(fd, (genericptr_t) disco, sizeof disco);
    mread(fd, (genericptr_t) objects, sizeof(struct objclass) * NUM_OBJECTS);
    for (i = 0; i < NUM_OBJECTS; i++)
        if (objects[i].oc_uname) {
            mread(fd, (genericptr_t) &len, sizeof len);
            objects[i].oc_uname = (char *) alloc(len);
            mread(fd, (genericptr_t) objects[i].oc_uname, len);
        }
#ifdef USE_TILES
    shuffle_tiles();
#endif
}

void
discover_object(oindx, mark_as_known, credit_hero)
register int oindx;
boolean mark_as_known;
boolean credit_hero;
{
    if (!objects[oindx].oc_name_known) {
        register int dindx, acls = objects[oindx].oc_class;

        /* Loop thru disco[] 'til we find the target (which may have been
           uname'd) or the next open slot; one or the other will be found
           before we reach the next class...
         */
        for (dindx = bases[acls]; disco[dindx] != 0; dindx++)
            if (disco[dindx] == oindx)
                break;
        disco[dindx] = oindx;

        if (mark_as_known) {
            objects[oindx].oc_name_known = 1;
            if (credit_hero)
                exercise(A_WIS, TRUE);
        }
        /* moves==1L => initial inventory, gameover => final disclosure */
        if (moves > 1L && !program_state.gameover)
            update_inventory();
    }
}

/* if a class name has been cleared, we may need to purge it from disco[] */
void
undiscover_object(oindx)
register int oindx;
{
    if (!objects[oindx].oc_name_known) {
        register int dindx, acls = objects[oindx].oc_class;
        register boolean found = FALSE;

        /* find the object; shift those behind it forward one slot */
        for (dindx = bases[acls]; dindx < NUM_OBJECTS && disco[dindx] != 0
                                  && objects[dindx].oc_class == acls;
             dindx++)
            if (found)
                disco[dindx - 1] = disco[dindx];
            else if (disco[dindx] == oindx)
                found = TRUE;

        /* clear last slot */
        if (found)
            disco[dindx - 1] = 0;
        else
            impossible("named object not in disco");
        update_inventory();
    }
}

STATIC_OVL boolean
interesting_to_discover(i)
register int i;
{
    /* Pre-discovered objects are now printed with a '*' */
    return (boolean) (objects[i].oc_uname != (char *) 0
                      || (objects[i].oc_name_known
                          && OBJ_DESCR(objects[i]) != (char *) 0));
}

/* items that should stand out once they're known */
static short uniq_objs[] = {
    AMULET_OF_YENDOR, SPE_BOOK_OF_THE_DEAD, CANDELABRUM_OF_INVOCATION,
    BELL_OF_OPENING,
};

/* the '\' command - show discovered object types */
int
dodiscovered() /* free after Robert Viduya */
{
    register int i, dis;
    int ct = 0;
    char *s, oclass, prev_class, classes[MAXOCLASSES], buf[BUFSZ];
    winid tmpwin;

    tmpwin = create_nhwindow(NHW_MENU);
    putstr(tmpwin, 0, "Discoveries");
    putstr(tmpwin, 0, "");

    /* gather "unique objects" into a pseudo-class; note that they'll
       also be displayed individually within their regular class */
    for (i = dis = 0; i < SIZE(uniq_objs); i++)
        if (objects[uniq_objs[i]].oc_name_known) {
            if (!dis++)
                putstr(tmpwin, iflags.menu_headings, "Unique items");
            Sprintf(buf, "  %s", OBJ_NAME(objects[uniq_objs[i]]));
            putstr(tmpwin, 0, buf);
            ++ct;
        }
    /* display any known artifacts as another pseudo-class */
    ct += disp_artifact_discoveries(tmpwin);

    /* several classes are omitted from packorder; one is of interest here */
    Strcpy(classes, flags.inv_order);
    if (!index(classes, VENOM_CLASS))
        (void) strkitten(classes, VENOM_CLASS); /* append char to string */

    for (s = classes; *s; s++) {
        oclass = *s;
        prev_class = oclass + 1; /* forced different from oclass */
        for (i = bases[(int) oclass];
             i < NUM_OBJECTS && objects[i].oc_class == oclass; i++) {
            if ((dis = disco[i]) != 0 && interesting_to_discover(dis)) {
                ct++;
                if (oclass != prev_class) {
                    putstr(tmpwin, iflags.menu_headings,
                           let_to_name(oclass, FALSE, FALSE));
                    prev_class = oclass;
                }
                Sprintf(buf, "%s %s",
                        (objects[dis].oc_pre_discovered ? "*" : " "),
                        obj_typename(dis));
                putstr(tmpwin, 0, buf);
            }
        }
    }
    if (ct == 0) {
        You("haven't discovered anything yet...");
    } else
        display_nhwindow(tmpwin, TRUE);
    destroy_nhwindow(tmpwin);

    return 0;
}

/* lower case let_to_name() output, which differs from def_oc_syms[].name */
STATIC_OVL char *
oclass_to_name(oclass, buf)
char oclass;
char *buf;
{
    char *s;

    Strcpy(buf, let_to_name(oclass, FALSE, FALSE));
    for (s = buf; *s; ++s)
        *s = lowc(*s);
    return buf;
}

/* the '`' command - show discovered object types for one class */
int
doclassdisco()
{
    static NEARDATA const char
        prompt[] = "View discoveries for which sort of objects?",
        havent_discovered_any[] = "haven't discovered any %s yet.",
        unique_items[] = "unique items",
        artifact_items[] = "artifacts";
    char *s, c, oclass, menulet, allclasses[MAXOCLASSES],
        discosyms[2 + MAXOCLASSES + 1], buf[BUFSZ];
    int i, ct, dis, xtras;
    boolean traditional;
    winid tmpwin = WIN_ERR;
    anything any;
    menu_item *pick_list = 0;

    discosyms[0] = '\0';
    traditional = (flags.menu_style == MENU_TRADITIONAL
                   || flags.menu_style == MENU_COMBINATION);
    if (!traditional) {
        tmpwin = create_nhwindow(NHW_MENU);
        start_menu(tmpwin);
    }
    any = zeroany;
    menulet = 'a';

    /* check whether we've discovered any unique objects */
    for (i = 0; i < SIZE(uniq_objs); i++)
        if (objects[uniq_objs[i]].oc_name_known) {
            Strcat(discosyms, "u");
            if (!traditional) {
                any.a_int = 'u';
                add_menu(tmpwin, NO_GLYPH, &any, menulet++, 0, ATR_NONE,
                         unique_items, MENU_UNSELECTED);
            }
            break;
        }

    /* check whether we've discovered any artifacts */
    if (disp_artifact_discoveries(WIN_ERR) > 0) {
        Strcat(discosyms, "a");
        if (!traditional) {
            any.a_int = 'a';
            add_menu(tmpwin, NO_GLYPH, &any, menulet++, 0, ATR_NONE,
                     artifact_items, MENU_UNSELECTED);
        }
    }

    /* collect classes with discoveries, in packorder ordering; several
       classes are omitted from packorder and one is of interest here */
    Strcpy(allclasses, flags.inv_order);
    if (!index(allclasses, VENOM_CLASS))
        (void) strkitten(allclasses, VENOM_CLASS); /* append char to string */
    /* construct discosyms[] */
    for (s = allclasses; *s; ++s) {
        oclass = *s;
        c = def_oc_syms[(int) oclass].sym;
        for (i = bases[(int) oclass];
             i < NUM_OBJECTS && objects[i].oc_class == oclass; ++i)
            if ((dis = disco[i]) != 0 && interesting_to_discover(dis)) {
                if (!index(discosyms, c)) {
                    Sprintf(eos(discosyms), "%c", c);
                    if (!traditional) {
                        any.a_int = c;
                        add_menu(tmpwin, NO_GLYPH, &any, menulet++, c,
                                 ATR_NONE, oclass_to_name(oclass, buf),
                                 MENU_UNSELECTED);
                    }
                }
            }
    }

    /* there might not be anything for us to do... */
    if (!discosyms[0]) {
        You(havent_discovered_any, "items");
        if (tmpwin != WIN_ERR)
            destroy_nhwindow(tmpwin);
        return 0;
    }

    /* have player choose a class */
    c = '\0'; /* class not chosen yet */
    if (traditional) {
        /* we'll prompt even if there's only one viable class; we add all
           nonviable classes as unseen acceptable choices so player can ask
           for discoveries of any class whether it has discoveries or not */
        for (s = allclasses, xtras = 0; *s; ++s) {
            c = def_oc_syms[(int) *s].sym;
            if (!index(discosyms, c)) {
                if (!xtras++)
                    (void) strkitten(discosyms, '\033');
                (void) strkitten(discosyms, c);
            }
        }
        /* get the class (via its symbol character) */
        c = yn_function(prompt, discosyms, '\0');
        savech(c);
        if (!c)
            clear_nhwindow(WIN_MESSAGE);
    } else {
        /* menustyle:full or menustyle:partial */
        if (!discosyms[1] && flags.menu_style == MENU_PARTIAL) {
            /* only one class; menustyle:partial normally jumps past class
               filtering straight to final menu so skip class filter here */
            c = discosyms[0];
        } else {
            /* more than one choice, or menustyle:full which normally has
               an intermediate class selection menu before the final menu */
            end_menu(tmpwin, prompt);
            i = select_menu(tmpwin, PICK_ONE, &pick_list);
            if (i > 0) {
                c = pick_list[0].item.a_int;
                free((genericptr_t) pick_list);
            } /* else c stays 0 */
        }
        destroy_nhwindow(tmpwin);
    }
    if (!c)
        return 0; /* player declined to make a selection */

    /*
     * show discoveries for object class c
     */
    tmpwin = create_nhwindow(NHW_MENU);
    ct = 0;
    switch (c) {
    case 'u':
        putstr(tmpwin, iflags.menu_headings,
               upstart(strcpy(buf, unique_items)));
        for (i = 0; i < SIZE(uniq_objs); i++)
            if (objects[uniq_objs[i]].oc_name_known) {
                Sprintf(buf, "  %s", OBJ_NAME(objects[uniq_objs[i]]));
                putstr(tmpwin, 0, buf);
                ++ct;
            }
        if (!ct)
            You(havent_discovered_any, unique_items);
        break;
    case 'a':
        /* disp_artifact_discoveries() includes a header */
        ct = disp_artifact_discoveries(tmpwin);
        if (!ct)
            You(havent_discovered_any, artifact_items);
        break;
    default:
        oclass = def_char_to_objclass(c);
        Sprintf(buf, "Discovered %s", let_to_name(oclass, FALSE, FALSE));
        putstr(tmpwin, iflags.menu_headings, buf);
        for (i = bases[(int) oclass];
             i < NUM_OBJECTS && objects[i].oc_class == oclass; ++i) {
            if ((dis = disco[i]) != 0 && interesting_to_discover(dis)) {
                Sprintf(buf, "%s %s",
                        objects[dis].oc_pre_discovered ? "*" : " ",
                        obj_typename(dis));
                putstr(tmpwin, 0, buf);
                ++ct;
            }
        }
        if (!ct)
            You(havent_discovered_any, oclass_to_name(oclass, buf));
        break;
    }
    if (ct)
        display_nhwindow(tmpwin, TRUE);
    destroy_nhwindow(tmpwin);
    return 0;
}

/* put up nameable subset of discoveries list as a menu */
void
rename_disco()
{
    register int i, dis;
    int ct = 0, mn = 0, sl;
    char *s, oclass, prev_class;
    winid tmpwin;
    anything any;
    menu_item *selected = 0;

    any = zeroany;
    tmpwin = create_nhwindow(NHW_MENU);
    start_menu(tmpwin);

    /*
     * Skip the "unique objects" section (each will appear within its
     * regular class if it is nameable) and the artifacts section.
     * We assume that classes omitted from packorder aren't nameable
     * so we skip venom too.
     */

    /* for each class, show discoveries in that class */
    for (s = flags.inv_order; *s; s++) {
        oclass = *s;
        prev_class = oclass + 1; /* forced different from oclass */
        for (i = bases[(int) oclass];
             i < NUM_OBJECTS && objects[i].oc_class == oclass; i++) {
            dis = disco[i];
            if (!dis || !interesting_to_discover(dis))
                continue;
            ct++;
            if (!objtyp_is_callable(dis))
                continue;
            mn++;

            if (oclass != prev_class) {
                any.a_int = 0;
                add_menu(tmpwin, NO_GLYPH, &any, 0, 0, iflags.menu_headings,
                         let_to_name(oclass, FALSE, FALSE),
                         MENU_UNSELECTED);
                prev_class = oclass;
            }
            any.a_int = dis;
            add_menu(tmpwin, NO_GLYPH, &any, 0, 0, ATR_NONE,
                     obj_typename(dis), MENU_UNSELECTED);
        }
    }
    if (ct == 0) {
        You("haven't discovered anything yet...");
    } else if (mn == 0) {
        pline("None of your discoveries can be assigned names...");
    } else {
        end_menu(tmpwin, "Pick an object type to name");
        dis = STRANGE_OBJECT;
        sl = select_menu(tmpwin, PICK_ONE, &selected);
        if (sl > 0) {
            dis = selected[0].item.a_int;
            free((genericptr_t) selected);
        }
        if (dis != STRANGE_OBJECT) {
            struct obj odummy;

            odummy = zeroobj;
            odummy.otyp = dis;
            odummy.oclass = objects[dis].oc_class;
            odummy.quan = 1L;
            odummy.known = !objects[dis].oc_uses_known;
            odummy.dknown = 1;
            docall(&odummy);
        }
    }
    destroy_nhwindow(tmpwin);
    return;
}

/*o_init.c*/
