/* NetHack 3.7	artifact.c	$NHDT-Date: 1654717838 2022/06/08 19:50:38 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.190 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2013. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "artifact.h"
#include "artilist.h"

/*
 * Note:  both artilist[] and artiexist[] have a dummy element #0,
 *        so loops over them should normally start at #1.  The primary
 *        exception is the save & restore code, which doesn't care about
 *        the contents, just the total size.
 */

#define get_artifact(o) \
    (((o) && (o)->oartifact) ? &artilist[(int) (o)->oartifact] : 0)

static boolean bane_applies(const struct artifact *, struct monst *);
static int spec_applies(const struct artifact *, struct monst *);
static int invoke_ok(struct obj *);
static int arti_invoke(struct obj *);
static boolean Mb_hit(struct monst * magr, struct monst *mdef,
                      struct obj *, int *, int, boolean, char *);
static unsigned long abil_to_spfx(long *);
static uchar abil_to_adtyp(long *);
static int glow_strength(int);
static boolean untouchable(struct obj *, boolean);
static int count_surround_traps(coordxy, coordxy);

/* The amount added to the victim's total hit points to insure that the
   victim will be killed even after damage bonus/penalty adjustments.
   Most such penalties are small, and 200 is plenty; the exception is
   half physical damage.  3.3.1 and previous versions tried to use a very
   large number to account for this case; now, we just compute the fatal
   damage by adding it to 2 times the total hit points instead of 1 time.
   Note: this will still break if they have more than about half the number
   of hit points that will fit in a 15 bit integer. */
#define FATAL_DAMAGE_MODIFIER 200

/* artifact tracking; gift and wish imply found; it also gets set for items
   seen on the floor, in containers, and wielded or dropped by monsters */
struct arti_info {
    Bitfield(exists, 1); /* 1 if corresponding artifact has been created */
    Bitfield(found, 1);  /* 1 if artifact is known by hero to exist */
    Bitfield(gift, 1);   /* 1 iff artifact was created as a prayer reward */
    Bitfield(wish, 1);   /* 1 iff artifact was created via wish */
    Bitfield(named, 1);  /* 1 iff artifact was made by naming an item */
    Bitfield(viadip, 1); /* 1 iff dipped long sword became Excalibur */
    Bitfield(lvldef, 1); /* 1 iff created by special level definition */
    Bitfield(bones, 1);  /* 1 iff came from bones file */
    Bitfield(rndm, 1);   /* 1 iff randomly generated */
};
/* array of flags tracking which artifacts exist, indexed by ART_xx;
   ART_xx values are 1..N, element [0] isn't used; no terminator needed */
static struct arti_info artiexist[1 + NROFARTIFACTS];
/* discovery list; for N discovered artifacts, the first N entries are ART_xx
   values in discovery order, the remaining (NROFARTIFACTS-N) slots are 0 */
static xint16 artidisco[NROFARTIFACTS];
/* note: artiexist[] and artidisco[] don't need to be in struct g; they
 * get explicitly initialized at game start so don't need to be part of
 * bulk re-init if game restart ever gets implemented.  They are saved
 * and restored but that is done through this file so they can be local.
 */
static const struct arti_info zero_artiexist = {0}; /* all bits zero */

static void hack_artifacts(void);
static boolean attacks(int, struct obj *);

/* handle some special cases; must be called after u_init() */
static void
hack_artifacts(void)
{
    struct artifact *art;
    int alignmnt = aligns[flags.initalign].value;

    /* Fix up the alignments of "gift" artifacts */
    for (art = artilist + 1; art->otyp; art++)
        if (art->role == Role_switch && art->alignment != A_NONE)
            art->alignment = alignmnt;

    /* Excalibur can be used by any lawful character, not just knights */
    if (!Role_if(PM_KNIGHT))
        artilist[ART_EXCALIBUR].role = NON_PM;

    /* Fix up the quest artifact */
    if (g.urole.questarti) {
        artilist[g.urole.questarti].alignment = alignmnt;
        artilist[g.urole.questarti].role = Role_switch;
    }
    return;
}

/* zero out the artifact existence list */
void
init_artifacts(void)
{
    (void) memset((genericptr_t) artiexist, 0, sizeof artiexist);
    (void) memset((genericptr_t) artidisco, 0, sizeof artidisco);
    hack_artifacts();
}

void
save_artifacts(NHFILE *nhfp)
{
    if (nhfp->structlevel) {
        bwrite(nhfp->fd, (genericptr_t) artiexist, sizeof artiexist);
        bwrite(nhfp->fd, (genericptr_t) artidisco, sizeof artidisco);
    }
}

void
restore_artifacts(NHFILE *nhfp)
{
    if (nhfp->structlevel) {
        mread(nhfp->fd, (genericptr_t) artiexist, sizeof artiexist);
        mread(nhfp->fd, (genericptr_t) artidisco, sizeof artidisco);
    }
    hack_artifacts();   /* redo non-saved special cases */
}

const char *
artiname(int artinum)
{
    if (artinum <= 0 || artinum > NROFARTIFACTS)
        return "";
    return artilist[artinum].name;
}

/*
   Make an artifact.  If a specific alignment is specified, then an object of
   the appropriate alignment is created from scratch, or 0 is returned if
   none is available.  (If at least one aligned artifact has already been
   given, then unaligned ones also become eligible for this.)
   If no alignment is given, then 'otmp' is converted
   into an artifact of matching type, or returned as-is if that's not
   possible.
   For the 2nd case, caller should use ``obj = mk_artifact(obj, A_NONE);''
   for the 1st, ``obj = mk_artifact((struct obj *)0, some_alignment);''.
 */
struct obj *
mk_artifact(
    struct obj *otmp,   /* existing object; ignored if alignment specified */
    aligntyp alignment) /* target alignment, or A_NONE */
{
    const struct artifact *a;
    int m, n, altn;
    boolean by_align = (alignment != A_NONE);
    short o_typ = (by_align || !otmp) ? 0 : otmp->otyp;
    boolean unique = !by_align && otmp && objects[o_typ].oc_unique;
    short eligible[NROFARTIFACTS];

    n = altn = 0;    /* no candidates found yet */
    eligible[0] = 0; /* lint suppression */
    /* gather eligible artifacts */
    for (m = 1, a = &artilist[m]; a->otyp; a++, m++) {
        if (artiexist[m].exists)
            continue;
        if ((a->spfx & SPFX_NOGEN) || unique)
            continue;

        if (!by_align) {
            /* looking for a particular type of item; not producing a
               divine gift so we don't care about role's first choice */
            if (a->otyp == o_typ)
                eligible[n++] = m;
            continue; /* move on to next possibility */
        }

        /* we're looking for an alignment-specific item
           suitable for hero's role+race */
        if ((a->alignment == alignment || a->alignment == A_NONE)
            /* avoid enemies' equipment */
            && (a->race == NON_PM || !race_hostile(&mons[a->race]))) {
            /* when a role-specific first choice is available, use it */
            if (Role_if(a->role)) {
                /* make this be the only possibility in the list */
                eligible[0] = m;
                n = 1;
                break; /* skip all other candidates */
            }
            /* found something to consider for random selection */
            if (a->alignment != A_NONE || u.ugifts > 0) {
                /* right alignment, or non-aligned with at least 1
                   previous gift bestowed, makes this one viable */
                eligible[n++] = m;
            } else {
                /* non-aligned with no previous gifts;
                   if no candidates have been found yet, record
                   this one as a[nother] fallback possibility in
                   case all aligned candidates have been used up
                   (via wishing, naming, bones, random generation) */
                if (!n)
                    eligible[altn++] = m;
                /* [once a regular candidate is found, the list
                   is overwritten and `altn' becomes irrelevant] */
            }
        }
    }

    /* resort to fallback list if main list was empty */
    if (!n)
        n = altn;

    if (n) {
        /* found at least one candidate; pick one at random */
        m = eligible[rn2(n)]; /* [0..n-1] */
        a = &artilist[m];

        /* make an appropriate object if necessary, then christen it */
        if (by_align)
            otmp = mksobj((int) a->otyp, TRUE, FALSE);

        if (otmp) {
            otmp = oname(otmp, a->name, ONAME_NO_FLAGS);
            otmp->oartifact = m;
            /* set existence and reason for creation bits */
            artifact_origin(otmp, ONAME_RANDOM); /* 'random' is default */
        }
    } else {
        /* nothing appropriate could be found; return original object */
        if (by_align)
            otmp = 0; /* (there was no original object) */
    }
    return otmp;
}

/*
 * Returns the full name (with articles and correct capitalization) of an
 * artifact named "name" if one exists, or NULL, it not.
 * The given name must be rather close to the real name for it to match.
 * The object type of the artifact is returned in otyp if the return value
 * is non-NULL.
 */
const char *
artifact_name(
    const char *name, /* string from player that might be an artifact name */
    short *otyp_p,    /* secondary output */
    boolean fuzzy)    /* whether to allow extra or omitted spaces or dashes */
{
    register const struct artifact *a;
    register const char *aname;

    if (!strncmpi(name, "the ", 4))
        name += 4;

    for (a = artilist + 1; a->otyp; a++) {
        aname = a->name;
        if (!strncmpi(aname, "the ", 4))
            aname += 4;
        if (!fuzzy ? !strcmpi(name, aname)
                   : fuzzymatch(name, aname, " -", TRUE)) {
            if (otyp_p)
                *otyp_p = a->otyp;
            return a->name;
        }
    }

    return (char *) 0;
}

boolean
exist_artifact(int otyp, const char *name)
{
    register const struct artifact *a;
    struct arti_info *arex;

    if (otyp && *name)
        for (a = artilist + 1, arex = artiexist + 1; a->otyp; a++, arex++)
            if ((int) a->otyp == otyp && !strcmp(a->name, name))
                return arex->exists ? TRUE : FALSE;
    return FALSE;
}

/* an artifact has just been created or is being "un-created" for a chance
   to be created again later */
void
artifact_exists(
    struct obj *otmp,
    const char *name,
    boolean mod,      /* True: exists, False: being un-created */
    unsigned flgs)    /* ONAME_xyz flags; not relevant if !mod */
{
    register const struct artifact *a;

    if (otmp && *name)
        for (a = artilist + 1; a->otyp; a++)
            if (a->otyp == otmp->otyp && !strcmp(a->name, name)) {
                int m = (int) (a - artilist);

                otmp->oartifact = (char) (mod ? m : 0);
                otmp->age = 0;
                if (otmp->otyp == RIN_INCREASE_DAMAGE)
                    otmp->spe = 0;
                if (mod) { /* means being created rather than un-created */
                    /* one--and only one--of these should always be set */
                    if ((flgs & (ONAME_VIA_NAMING | ONAME_WISH | ONAME_GIFT
                                 | ONAME_VIA_DIP | ONAME_LEVEL_DEF
                                 | ONAME_BONES | ONAME_RANDOM)) == 0)
                        flgs |= ONAME_RANDOM; /* the default origin */
                    /* 'exists' bit will become set (in artifact_origin();
                       there's no ONAME_ flag) and flgs might also contain
                       the know_arti bit (hero knows that artifact exists) */
                    artifact_origin(otmp, flgs);
                } else { /* uncreate */
                    /* clear all the flag bits */
                    artiexist[m] = zero_artiexist;
                }
                break;
            }
    return;
}

/* mark an artifact as 'found' */
void
found_artifact(int a)
{
    if (a < 1 || a > NROFARTIFACTS)
        impossible("found_artifact: invalid artifact index! (%d)", a);
    else if (!artiexist[a].exists)
        impossible("found_artifact: artifact doesn't exist yet? (%d)", a);
    else
        artiexist[a].found = 1;
}

/* if an artifact hasn't already been designated 'found', do that now
   and generate a livelog event about finding it */
void
find_artifact(struct obj *otmp)
{
    int a = otmp->oartifact;

    if (a && !artiexist[a].found) {
        const char *where;

        found_artifact(a); /* artiexist[a].found = 1 */
        /*
         * Unlike costly_spot(), inside_shop() includes the "free spot"
         * in front of the door.  And it doesn't care whether or not
         * there is a shopkeeper present.
         *
         * If hero sees a monster pick up a not-yet-found artifact, it
         * will have its dknown flag set even if far away and will be
         * described as 'found on the floor'.  Similarly for dropping
         * (possibly upon monster's death), dknown will be set and the
         * artifact will be described as 'carried by a monster'.
         * That's handled by caller:  dog_invent(), mpickstuff(), or
         * mdrop_obj() so that we get called before obj->where changes.
         */
        where = ((otmp->where == OBJ_FLOOR)
                 ?  ((inside_shop(otmp->ox, otmp->oy) != NO_ROOM)
                     ? " in a shop"
                     : " on the floor")
                 /* artifacts aren't created in containers but could be
                    inside one if it comes from a bones level */
                 : (otmp->where == OBJ_CONTAINED) ? " in a container"
                   /* perhaps probing, or seeing monster wield artifact */
                   : (otmp->where == OBJ_MINVENT) ? " carried by a monster"
                     /* catchall: probably in inventory, picked up while
                        blind but now seen; there's no previous_where to
                        figure out how it got here */
                     : "");
        livelog_printf(LL_ARTIFACT, "found %s%s",
                       bare_artifactname(otmp), where);
    }
}

int
nartifact_exist(void)
{
    int i, a = 0;

    for (i = 1; i <= NROFARTIFACTS; ++i)
        if (artiexist[i].exists)
            ++a;

    return a;
}

/* set artifact tracking flags;
   calling sequence: oname() -> artifact_exists() -> artifact_origin() or
   mksobj(),others -> mk_artifact() -> artifact_origin(random) possibly
   followed by mksobj(),others -> artifact_origin(non-random origin) */
void
artifact_origin(
    struct obj *arti, /* new artifact */
    unsigned aflags)  /* ONAME_xxx flags, shared by artifact_exists() */
{
    int ct, a = arti->oartifact;

    if (a) {
        /* start by clearing all bits; most are mutually exclusive */
        artiexist[a] = zero_artiexist;
        /* set 'exists' bit back on; not specified via flag bit in aflags */
        artiexist[a].exists = 1;
        /* 'hero knows it exists' is expected for wish, gift, viadip, or
           named and could eventually become set for any of the others */
        if ((aflags & ONAME_KNOW_ARTI) != 0)
            artiexist[a].found = 1;
        /* should be exactly one of wish, gift, via_dip, via_naming,
           level_def (quest), bones, and random (floor or monst's minvent) */
        ct = 0;
        if ((aflags & ONAME_WISH) != 0)
            artiexist[a].wish = 1, ++ct;
        if ((aflags & ONAME_GIFT) != 0)
            artiexist[a].gift = 1, ++ct;
        if ((aflags & ONAME_VIA_DIP) != 0)
            artiexist[a].viadip = 1, ++ct;
        if ((aflags & ONAME_VIA_NAMING) != 0)
            artiexist[a].named = 1, ++ct;
        if ((aflags & ONAME_LEVEL_DEF) != 0)
            artiexist[a].lvldef = 1, ++ct;
        if ((aflags & ONAME_BONES) != 0)
            artiexist[a].bones = 1, ++ct;
        if ((aflags & ONAME_RANDOM) != 0)
            artiexist[a].rndm = 1, ++ct;
        if (ct != 1)
            impossible("invalid artifact origin: %4o", aflags);
    }
}

boolean
spec_ability(struct obj *otmp, unsigned long abil)
{
    const struct artifact *arti = get_artifact(otmp);

    return (boolean) (arti && (arti->spfx & abil) != 0L);
}

/* used so that callers don't need to known about SPFX_ codes */
boolean
confers_luck(struct obj *obj)
{
    /* might as well check for this too */
    if (obj->otyp == LUCKSTONE)
        return TRUE;

    return (boolean) (obj->oartifact && spec_ability(obj, SPFX_LUCK));
}

/* used to check whether a monster is getting reflection from an artifact */
boolean
arti_reflects(struct obj *obj)
{
    const struct artifact *arti = get_artifact(obj);

    if (arti) {
        /* while being worn */
        if ((obj->owornmask & ~W_ART) && (arti->spfx & SPFX_REFLECT))
            return TRUE;
        /* just being carried */
        if (arti->cspfx & SPFX_REFLECT)
            return TRUE;
    }
    return FALSE;
}

/* decide whether this obj is effective when attacking against shades;
   does not consider the bonus for blessed objects versus undead */
boolean
shade_glare(struct obj *obj)
{
    const struct artifact *arti;

    /* any silver object is effective */
    if (objects[obj->otyp].oc_material == SILVER)
        return TRUE;
    /* non-silver artifacts with bonus against undead also are effective */
    arti = get_artifact(obj);
    if (arti && (arti->spfx & SPFX_DFLAG2) && arti->mtype == M2_UNDEAD)
        return TRUE;
    /* [if there was anything with special bonus against noncorporeals,
       it would be effective too] */
    /* otherwise, harmless to shades */
    return FALSE;
}

/* returns 1 if name is restricted for otmp->otyp */
boolean
restrict_name(struct obj *otmp, const char *name)
{
    register const struct artifact *a;
    const char *aname, *odesc, *other;
    boolean sametype[NUM_OBJECTS];
    int i, lo, hi, otyp = otmp->otyp, ocls = objects[otyp].oc_class;

    if (!*name)
        return FALSE;
    if (!strncmpi(name, "the ", 4))
        name += 4;

    /* decide what types of objects are the same as otyp;
       if it's been discovered, then only itself matches;
       otherwise, include all other undiscovered objects
       of the same class which have the same description
       or share the same pool of shuffled descriptions */
    (void) memset((genericptr_t) sametype, 0, sizeof sametype); /* FALSE */
    sametype[otyp] = TRUE;
    if (!objects[otyp].oc_name_known
        && (odesc = OBJ_DESCR(objects[otyp])) != 0) {
        obj_shuffle_range(otyp, &lo, &hi);
        for (i = g.bases[ocls]; i < NUM_OBJECTS; i++) {
            if (objects[i].oc_class != ocls)
                break;
            if (!objects[i].oc_name_known
                && (other = OBJ_DESCR(objects[i])) != 0
                && (!strcmp(odesc, other) || (i >= lo && i <= hi)))
                sametype[i] = TRUE;
        }
    }

    /* Since almost every artifact is SPFX_RESTR, it doesn't cost
       us much to do the string comparison before the spfx check.
       Bug fix:  don't name multiple elven daggers "Sting".
     */
    for (a = artilist + 1; a->otyp; a++) {
        if (!sametype[a->otyp])
            continue;
        aname = a->name;
        if (!strncmpi(aname, "the ", 4))
            aname += 4;
        if (!strcmp(aname, name))
            return (boolean) ((a->spfx & (SPFX_NOGEN | SPFX_RESTR)) != 0
                              || otmp->quan > 1L);
    }

    return FALSE;
}

static boolean
attacks(int adtyp, struct obj *otmp)
{
    const struct artifact *weap;

    if ((weap = get_artifact(otmp)) != 0)
        return (boolean) (weap->attk.adtyp == adtyp);
    return FALSE;
}

boolean
defends(int adtyp, struct obj *otmp)
{
    struct artifact *weap;

    if (!otmp)
        return FALSE;
    if ((weap = get_artifact(otmp)) != 0)
        return (boolean) (weap->defn.adtyp == adtyp);
    if (Is_dragon_armor(otmp)) {
        int otyp = otmp->otyp;

        /* convert mail to scales to simplify testing */
        if (Is_dragon_mail(otmp))
            otyp += GRAY_DRAGON_SCALES - GRAY_DRAGON_SCALE_MAIL;

        switch (adtyp) {
        case AD_MAGM: /* magic missiles => general magic resistance */
            return (otyp == GRAY_DRAGON_SCALES);
        case AD_HALU: /* confers hallucination resistance */
            return (otyp == GOLD_DRAGON_SCALES);
        case AD_FIRE:
      /*case AD_BLND: -- gives infravision but does not prevent blindness */
            return (otyp == RED_DRAGON_SCALES); /* red but not gold */
        case AD_COLD:
      /*case AD_FAMN: -- slows digestion but does not override Famine */
            return (otyp == WHITE_DRAGON_SCALES); /* white but not silver */
        case AD_DRST: /* drain strength => poison */
        case AD_DISE: /* blocks disease but not slime */
            return (otyp == GREEN_DRAGON_SCALES);
        case AD_SLEE: /* sleep */
        case AD_PLYS: /* paralysis => free action */
            return (otyp == ORANGE_DRAGON_SCALES);
        case AD_DISN: /* disintegration */
        case AD_DRLI: /* level drain resistance */
            return (otyp == BLACK_DRAGON_SCALES);
        case AD_ELEC: /* electricity == lightning */
        case AD_SLOW: /* confers speed so blocks speed removal */
            return (otyp == BLUE_DRAGON_SCALES);
        case AD_ACID:
        case AD_STON: /* petrification resistance */
            return (otyp == YELLOW_DRAGON_SCALES);
        default:
            /* SILVER_DRAGON_SCALES don't resist any particular attack type */
            break;
        }
    }
    return FALSE;
}

/* used for monsters */
boolean
defends_when_carried(int adtyp, struct obj *otmp)
{
    const struct artifact *weap;

    if ((weap = get_artifact(otmp)) != 0)
        return (boolean) (weap->cary.adtyp == adtyp);
    return FALSE;
}

/* determine whether an item confers Protection */
boolean
protects(struct obj *otmp, boolean being_worn)
{
    const struct artifact *arti;

    if (being_worn && objects[otmp->otyp].oc_oprop == PROTECTION)
        return TRUE;
    arti = get_artifact(otmp);
    if (!arti)
        return FALSE;
    return (boolean) ((arti->cspfx & SPFX_PROTECT) != 0
                      || (being_worn && (arti->spfx & SPFX_PROTECT) != 0));
}

/*
 * a potential artifact has just been worn/wielded/picked-up or
 * unworn/unwielded/dropped.  Pickup/drop only set/reset the W_ART mask.
 */
void
set_artifact_intrinsic(struct obj *otmp, boolean on, long wp_mask)
{
    long *mask = 0;
    register const struct artifact *art, *oart = get_artifact(otmp);
    register struct obj *obj;
    register uchar dtyp;
    register long spfx;

    if (!oart)
        return;

    /* effects from the defn field */
    dtyp = (wp_mask != W_ART) ? oart->defn.adtyp : oart->cary.adtyp;

    if (dtyp == AD_FIRE)
        mask = &EFire_resistance;
    else if (dtyp == AD_COLD)
        mask = &ECold_resistance;
    else if (dtyp == AD_ELEC)
        mask = &EShock_resistance;
    else if (dtyp == AD_MAGM)
        mask = &EAntimagic;
    else if (dtyp == AD_DISN)
        mask = &EDisint_resistance;
    else if (dtyp == AD_DRST)
        mask = &EPoison_resistance;
    else if (dtyp == AD_DRLI)
        mask = &EDrain_resistance;

    if (mask && wp_mask == W_ART && !on) {
        /* find out if some other artifact also confers this intrinsic;
           if so, leave the mask alone */
        for (obj = g.invent; obj; obj = obj->nobj) {
            if (obj != otmp && obj->oartifact) {
                art = get_artifact(obj);
                if (art && art->cary.adtyp == dtyp) {
                    mask = (long *) 0;
                    break;
                }
            }
        }
    }
    if (mask) {
        if (on)
            *mask |= wp_mask;
        else
            *mask &= ~wp_mask;
    }

    /* intrinsics from the spfx field; there could be more than one */
    spfx = (wp_mask != W_ART) ? oart->spfx : oart->cspfx;
    if (spfx && wp_mask == W_ART && !on) {
        /* don't change any spfx also conferred by other artifacts */
        for (obj = g.invent; obj; obj = obj->nobj)
            if (obj != otmp && obj->oartifact) {
                art = get_artifact(obj);
                if (art)
                    spfx &= ~art->cspfx;
            }
    }

    if (spfx & SPFX_SEARCH) {
        if (on)
            ESearching |= wp_mask;
        else
            ESearching &= ~wp_mask;
    }
    if (spfx & SPFX_HALRES) {
        /* make_hallucinated must (re)set the mask itself to get
         * the display right */
        /* restoring needed because this is the only artifact intrinsic
         * that can print a message--need to guard against being printed
         * when restoring a game
         */
        (void) make_hallucinated((long) !on,
                                 g.program_state.restoring ? FALSE : TRUE,
                                 wp_mask);
    }
    if (spfx & SPFX_ESP) {
        if (on)
            ETelepat |= wp_mask;
        else
            ETelepat &= ~wp_mask;
        see_monsters();
    }
    if (spfx & SPFX_STLTH) {
        if (on)
            EStealth |= wp_mask;
        else
            EStealth &= ~wp_mask;
    }
    if (spfx & SPFX_REGEN) {
        if (on)
            ERegeneration |= wp_mask;
        else
            ERegeneration &= ~wp_mask;
    }
    if (spfx & SPFX_TCTRL) {
        if (on)
            ETeleport_control |= wp_mask;
        else
            ETeleport_control &= ~wp_mask;
    }
    if (spfx & SPFX_WARN) {
        if (spec_m2(otmp)) {
            if (on) {
                EWarn_of_mon |= wp_mask;
                g.context.warntype.obj |= spec_m2(otmp);
            } else {
                EWarn_of_mon &= ~wp_mask;
                g.context.warntype.obj &= ~spec_m2(otmp);
            }
            see_monsters();
        } else {
            if (on)
                EWarning |= wp_mask;
            else
                EWarning &= ~wp_mask;
        }
    }
    if (spfx & SPFX_EREGEN) {
        if (on)
            EEnergy_regeneration |= wp_mask;
        else
            EEnergy_regeneration &= ~wp_mask;
    }
    if (spfx & SPFX_HSPDAM) {
        if (on)
            EHalf_spell_damage |= wp_mask;
        else
            EHalf_spell_damage &= ~wp_mask;
    }
    if (spfx & SPFX_HPHDAM) {
        if (on)
            EHalf_physical_damage |= wp_mask;
        else
            EHalf_physical_damage &= ~wp_mask;
    }
    if (spfx & SPFX_XRAY) {
        /* this assumes that no one else is using xray_range */
        if (on)
            u.xray_range = 3;
        else
            u.xray_range = -1;
        g.vision_full_recalc = 1;
    }
    if ((spfx & SPFX_REFLECT) && (wp_mask & W_WEP)) {
        if (on)
            EReflecting |= wp_mask;
        else
            EReflecting &= ~wp_mask;
    }
    if (spfx & SPFX_PROTECT) {
        if (on)
            EProtection |= wp_mask;
        else
            EProtection &= ~wp_mask;
    }

    if (wp_mask == W_ART && !on && oart->inv_prop) {
        /* might have to turn off invoked power too */
        if (oart->inv_prop <= LAST_PROP
            && (u.uprops[oart->inv_prop].extrinsic & W_ARTI))
            (void) arti_invoke(otmp);
    }
}

/* touch_artifact()'s return value isn't sufficient to tell whether it
   dished out damage, and tracking changes to u.uhp, u.mh, Lifesaved
   when trying to avoid second wounding is too cumbersome */
static boolean touch_blasted; /* for retouch_object() */

/*
 * creature (usually hero) tries to touch (pick up or wield) an artifact obj.
 * Returns 0 if the object refuses to be touched.
 * This routine does not change any object chains.
 * Ignores such things as gauntlets, assuming the artifact is not
 * fooled by such trappings.
 */
int
touch_artifact(struct obj *obj, struct monst *mon)
{
    register const struct artifact *oart = get_artifact(obj);
    boolean badclass, badalign, self_willed, yours;

    touch_blasted = FALSE;
    if (!oart)
        return 1;

    yours = (mon == &g.youmonst);
    /* all quest artifacts are self-willed; if this ever changes, `badclass'
       will have to be extended to explicitly include quest artifacts */
    self_willed = ((oart->spfx & SPFX_INTEL) != 0);
    if (yours) {
        badclass = self_willed
                   && ((oart->role != NON_PM && !Role_if(oart->role))
                       || (oart->race != NON_PM && !Race_if(oart->race)));
        badalign = ((oart->spfx & SPFX_RESTR) != 0
                    && oart->alignment != A_NONE
                    && (oart->alignment != u.ualign.type
                        || u.ualign.record < 0));
    } else if (!is_covetous(mon->data) && !is_mplayer(mon->data)) {
        badclass = self_willed && oart->role != NON_PM
                   && oart != &artilist[ART_EXCALIBUR];
        badalign = (oart->spfx & SPFX_RESTR) && oart->alignment != A_NONE
                   && (oart->alignment != mon_aligntyp(mon));
    } else { /* an M3_WANTSxxx monster or a fake player */
        /* special monsters trying to take the Amulet, invocation tools or
           quest item can touch anything except `spec_applies' artifacts */
        badclass = badalign = FALSE;
    }
    /* weapons which attack specific categories of monsters are
       bad for them even if their alignments happen to match */
    if (!badalign)
        badalign = bane_applies(oart, mon);

    if (((badclass || badalign) && self_willed)
        || (badalign && (!yours || !rn2(4)))) {
        int dmg, tmp;
        char buf[BUFSZ];

        if (!yours)
            return 0;
        You("are blasted by %s power!", s_suffix(the(xname(obj))));
        touch_blasted = TRUE;
        dmg = d((Antimagic ? 2 : 4), (self_willed ? 10 : 4));
        /* add half (maybe quarter) of the usual silver damage bonus */
        if (objects[obj->otyp].oc_material == SILVER && Hate_silver)
            tmp = rnd(10), dmg += Maybe_Half_Phys(tmp);
        Sprintf(buf, "touching %s", oart->name);
        losehp(dmg, buf, KILLED_BY); /* magic damage, not physical */
        exercise(A_WIS, FALSE);
    }

    /* can pick it up unless you're totally non-synch'd with the artifact */
    if (badclass && badalign && self_willed) {
        if (yours) {
            if (!carried(obj))
                pline("%s your grasp!", Tobjnam(obj, "evade"));
            else
                pline("%s beyond your control!", Tobjnam(obj, "are"));
        }
        return 0;
    }

    return 1;
}

/* decide whether an artifact itself is vulnerable to a particular type
   of erosion damage, independent of the properties of its bearer */
boolean
arti_immune(struct obj *obj, int dtyp)
{
    register const struct artifact *weap = get_artifact(obj);

    if (!weap)
        return FALSE;
    if (dtyp == AD_PHYS)
        return FALSE; /* nothing is immune to phys dmg */
    return (boolean) (weap->attk.adtyp == dtyp
                      || weap->defn.adtyp == dtyp
                      || weap->cary.adtyp == dtyp);
}

static boolean
bane_applies(const struct artifact *oart, struct monst *mon)
{
    struct artifact atmp;

    if (oart && (oart->spfx & SPFX_DBONUS) != 0) {
        atmp = *oart;
        atmp.spfx &= SPFX_DBONUS; /* clear other spfx fields */
        if (spec_applies(&atmp, mon))
            return TRUE;
    }
    return FALSE;
}

/* decide whether an artifact's special attacks apply against mtmp */
static int
spec_applies(const struct artifact *weap, struct monst *mtmp)
{
    struct permonst *ptr;
    boolean yours;

    if (!(weap->spfx & (SPFX_DBONUS | SPFX_ATTK)))
        return (weap->attk.adtyp == AD_PHYS);

    yours = (mtmp == &g.youmonst);
    ptr = mtmp->data;

    if (weap->spfx & SPFX_DMONS) {
        return (ptr == &mons[(int) weap->mtype]);
    } else if (weap->spfx & SPFX_DCLAS) {
        return (weap->mtype == (unsigned long) ptr->mlet);
    } else if (weap->spfx & SPFX_DFLAG1) {
        return ((ptr->mflags1 & weap->mtype) != 0L);
    } else if (weap->spfx & SPFX_DFLAG2) {
        return ((ptr->mflags2 & weap->mtype)
                || (yours
                    && ((!Upolyd && (g.urace.selfmask & weap->mtype))
                        || ((weap->mtype & M2_WERE) && u.ulycn >= LOW_PM))));
    } else if (weap->spfx & SPFX_DALIGN) {
        return yours ? (u.ualign.type != weap->alignment)
                     : (ptr->maligntyp == A_NONE
                        || sgn(ptr->maligntyp) != weap->alignment);
    } else if (weap->spfx & SPFX_ATTK) {
        if (defended(mtmp, (int) weap->attk.adtyp))
            return FALSE;

        switch (weap->attk.adtyp) {
        case AD_FIRE:
            return !(yours ? Fire_resistance : resists_fire(mtmp));
        case AD_COLD:
            return !(yours ? Cold_resistance : resists_cold(mtmp));
        case AD_ELEC:
            return !(yours ? Shock_resistance : resists_elec(mtmp));
        case AD_MAGM:
        case AD_STUN:
            return !(yours ? Antimagic : (rn2(100) < ptr->mr));
        case AD_DRST:
            return !(yours ? Poison_resistance : resists_poison(mtmp));
        case AD_DRLI:
            return !(yours ? Drain_resistance : resists_drli(mtmp));
        case AD_STON:
            return !(yours ? Stone_resistance : resists_ston(mtmp));
        default:
            impossible("Weird weapon special attack.");
        }
    }
    return 0;
}

/* return the M2 flags of monster that an artifact's special attacks apply
 * against */
long
spec_m2(struct obj *otmp)
{
    const struct artifact *artifact = get_artifact(otmp);

    if (artifact)
        return artifact->mtype;
    return 0L;
}

/* special attack bonus */
int
spec_abon(struct obj *otmp, struct monst *mon)
{
    const struct artifact *weap = get_artifact(otmp);

    /* no need for an extra check for `NO_ATTK' because this will
       always return 0 for any artifact which has that attribute */

    if (weap && weap->attk.damn && spec_applies(weap, mon))
        return rnd((int) weap->attk.damn);
    return 0;
}

/* special damage bonus */
int
spec_dbon(struct obj *otmp, struct monst *mon, int tmp)
{
    register const struct artifact *weap = get_artifact(otmp);

    if (!weap || (weap->attk.adtyp == AD_PHYS /* check for `NO_ATTK' */
                  && weap->attk.damn == 0 && weap->attk.damd == 0))
        g.spec_dbon_applies = FALSE;
    else if (is_art(otmp, ART_GRIMTOOTH))
        /* Grimtooth has SPFX settings to warn against elves but we want its
           damage bonus to apply to all targets, so bypass spec_applies() */
        g.spec_dbon_applies = TRUE;
    else
        g.spec_dbon_applies = spec_applies(weap, mon);

    if (g.spec_dbon_applies)
        return weap->attk.damd ? rnd((int) weap->attk.damd) : max(tmp, 1);
    return 0;
}

/* add identified artifact to discoveries list */
void
discover_artifact(xint16 m)
{
    int i;

    /* look for this artifact in the discoveries list;
       if we hit an empty slot then it's not present, so add it */
    for (i = 0; i < NROFARTIFACTS; i++)
        if (artidisco[i] == 0 || artidisco[i] == m) {
            artidisco[i] = m;
            return;
        }
    /* there is one slot per artifact, so we should never reach the
       end without either finding the artifact or an empty slot... */
    impossible("couldn't discover artifact (%d)", (int) m);
}

/* used to decide whether an artifact has been fully identified */
boolean
undiscovered_artifact(xint16 m)
{
    int i;

    /* look for this artifact in the discoveries list;
       if we hit an empty slot then it's undiscovered */
    for (i = 0; i < NROFARTIFACTS; i++)
        if (artidisco[i] == m)
            return FALSE;
        else if (artidisco[i] == 0)
            break;
    return TRUE;
}

/* display a list of discovered artifacts; return their count */
int
disp_artifact_discoveries(winid tmpwin) /* supplied by dodiscover() */
{
    int i, m, otyp;
    const char *algnstr;
    char buf[BUFSZ];

    for (i = 0; i < NROFARTIFACTS; i++) {
        if (artidisco[i] == 0)
            break; /* empty slot implies end of list */
        if (tmpwin == WIN_ERR)
            continue; /* for WIN_ERR, we just count */

        if (i == 0)
            putstr(tmpwin, iflags.menu_headings, "Artifacts");
        m = artidisco[i];
        otyp = artilist[m].otyp;
        algnstr = align_str(artilist[m].alignment);
        if (!strcmp(algnstr, "unaligned"))
            algnstr = "non-aligned";

        Sprintf(buf, "  %s [%s %s]", artiname(m),
                algnstr, simple_typename(otyp));
        putstr(tmpwin, 0, buf);
    }
    return i;
}

/* (wizard mode only) show all artifacts and their flags */
void
dump_artifact_info(winid tmpwin)
{
    int m;
    char buf[BUFSZ], buf2[BUFSZ];

    putstr(tmpwin, iflags.menu_headings, "Artifacts");
    for (m = 1; m <= NROFARTIFACTS; ++m) {
        Snprintf(buf2, sizeof buf2,
                "[%s%s%s%s%s%s%s%s%s]", /* 9 bits overall */
                artiexist[m].exists ? "exists;" : "",
                artiexist[m].found  ? " hero knows;" : "",
                /* .exists and .found have different punctuation because
                   they're expected to be combined with one of these */
                artiexist[m].gift   ? " gift"   : "",
                artiexist[m].wish   ? " wish"   : "",
                artiexist[m].named  ? " named"  : "",
                artiexist[m].viadip ? " viadip" : "",
                artiexist[m].lvldef ? " lvldef" : "",
                artiexist[m].bones  ? " bones"  : "",
                artiexist[m].rndm   ? " random" : "");
#if 0   /* 'tmpwin' here is a text window, not a menu */
        if (iflags.menu_tab_sep)
            Sprintf(buf, "  %s\t%s", artiname(m), buf2);
        else
#else
            /* "The Platinum Yendorian Express Card" is 35 characters */
            Snprintf(buf, sizeof buf, "  %-36.36s%s", artiname(m), buf2);
#endif
        putstr(tmpwin, 0, buf);
    }
    return;
}

/*
 * Magicbane's intrinsic magic is incompatible with normal
 * enchantment magic.  Thus, its effects have a negative
 * dependence on spe.  Against low mr victims, it typically
 * does "double athame" damage, 2d4.  Occasionally, it will
 * cast unbalancing magic which effectively averages out to
 * 4d4 damage (3d4 against high mr victims), for spe = 0.
 *
 * Prior to 3.4.1, the cancel (aka purge) effect always
 * included the scare effect too; now it's one or the other.
 * Likewise, the stun effect won't be combined with either
 * of those two; it will be chosen separately or possibly
 * used as a fallback when scare or cancel fails.
 *
 * [Historical note: a change to artifact_hit() for 3.4.0
 * unintentionally made all of Magicbane's special effects
 * be blocked if the defender successfully saved against a
 * stun attack.  As of 3.4.1, those effects can occur but
 * will be slightly less likely than they were in 3.3.x.]
 */

enum mb_effect_indices {
    MB_INDEX_PROBE = 0,
    MB_INDEX_STUN,
    MB_INDEX_SCARE,
    MB_INDEX_CANCEL,

    NUM_MB_INDICES
};

#define MB_MAX_DIEROLL 8 /* rolls above this aren't magical */
static const char *const mb_verb[2][NUM_MB_INDICES] = {
    { "probe", "stun", "scare", "cancel" },
    { "prod", "amaze", "tickle", "purge" },
};

/* called when someone is being hit by Magicbane */
static boolean
Mb_hit(struct monst *magr, /* attacker */
       struct monst *mdef, /* defender */
       struct obj *mb,     /* Magicbane */
       int *dmgptr,        /* extra damage target will suffer */
       int dieroll,        /* d20 that has already scored a hit */
       boolean vis,        /* whether the action can be seen */
       char *hittee)       /* target's name: "you" or mon_nam(mdef) */
{
    struct permonst *old_uasmon;
    const char *verb;
    boolean youattack = (magr == &g.youmonst),
            youdefend = (mdef == &g.youmonst),
            resisted = FALSE, do_stun, do_confuse, result;
    int attack_indx, fakeidx, scare_dieroll = MB_MAX_DIEROLL / 2;

    result = FALSE; /* no message given yet */
    /* the most severe effects are less likely at higher enchantment */
    if (mb->spe >= 3)
        scare_dieroll /= (1 << (mb->spe / 3));
    /* if target successfully resisted the artifact damage bonus,
       reduce overall likelihood of the assorted special effects */
    if (!g.spec_dbon_applies)
        dieroll += 1;

    /* might stun even when attempting a more severe effect, but
       in that case it will only happen if the other effect fails;
       extra damage will apply regardless; 3.4.1: sometimes might
       just probe even when it hasn't been enchanted */
    do_stun = (max(mb->spe, 0) < rn2(g.spec_dbon_applies ? 11 : 7));

    /* the special effects also boost physical damage; increments are
       generally cumulative, but since the stun effect is based on a
       different criterium its damage might not be included; the base
       damage is either 1d4 (athame) or 2d4 (athame+spec_dbon) depending
       on target's resistance check against AD_STUN (handled by caller)
       [note that a successful save against AD_STUN doesn't actually
       prevent the target from ending up stunned] */
    attack_indx = MB_INDEX_PROBE;
    *dmgptr += rnd(4); /* (2..3)d4 */
    if (do_stun) {
        attack_indx = MB_INDEX_STUN;
        *dmgptr += rnd(4); /* (3..4)d4 */
    }
    if (dieroll <= scare_dieroll) {
        attack_indx = MB_INDEX_SCARE;
        *dmgptr += rnd(4); /* (3..5)d4 */
    }
    if (dieroll <= (scare_dieroll / 2)) {
        attack_indx = MB_INDEX_CANCEL;
        *dmgptr += rnd(4); /* (4..6)d4 */
    }

    /* give the hit message prior to inflicting the effects */
    verb = mb_verb[!!Hallucination][attack_indx];
    if (youattack || youdefend || vis) {
        result = TRUE;
        pline_The("magic-absorbing blade %s %s!",
                  vtense((const char *) 0, verb), hittee);
        /* assume probing has some sort of noticeable feedback
           even if it is being done by one monster to another */
        if (attack_indx == MB_INDEX_PROBE && !canspotmon(mdef))
            map_invisible(mdef->mx, mdef->my);
    }

    /* now perform special effects */
    switch (attack_indx) {
    case MB_INDEX_CANCEL:
        old_uasmon = g.youmonst.data;
        /* No mdef->mcan check: even a cancelled monster can be polymorphed
         * into a golem, and the "cancel" effect acts as if some magical
         * energy remains in spellcasting defenders to be absorbed later.
         */
        if (!cancel_monst(mdef, mb, youattack, FALSE, FALSE)) {
            resisted = TRUE;
        } else {
            do_stun = FALSE;
            if (youdefend) {
                if (g.youmonst.data != old_uasmon)
                    *dmgptr = 0; /* rehumanized, so no more damage */
                if (u.uenmax > 0) {
                    u.uenmax--;
                    if (u.uen > 0)
                        u.uen--;
                    g.context.botl = TRUE;
                    You("lose magical energy!");
                }
            } else {
                if (mdef->data == &mons[PM_CLAY_GOLEM])
                    mdef->mhp = 1; /* cancelled clay golems will die */
                if (youattack && attacktype(mdef->data, AT_MAGC)) {
                    u.uenmax++;
                    if (u.uenmax > u.uenpeak)
                        u.uenpeak = u.uenmax;
                    u.uen++;
                    g.context.botl = TRUE;
                    You("absorb magical energy!");
                }
            }
        }
        break;

    case MB_INDEX_SCARE:
        if (youdefend) {
            if (Antimagic) {
                resisted = TRUE;
            } else {
                nomul(-3);
                g.multi_reason = "being scared stiff";
                g.nomovemsg = "";
                if (magr && magr == u.ustuck && sticks(g.youmonst.data)) {
                    set_ustuck((struct monst *) 0);
                    You("release %s!", mon_nam(magr));
                }
            }
        } else {
            if (rn2(2) && resist(mdef, WEAPON_CLASS, 0, NOTELL))
                resisted = TRUE;
            else
                monflee(mdef, 3, FALSE, (mdef->mhp > *dmgptr));
        }
        if (!resisted)
            do_stun = FALSE;
        break;

    case MB_INDEX_STUN:
        do_stun = TRUE; /* (this is redundant...) */
        break;

    case MB_INDEX_PROBE:
        if (youattack && (mb->spe == 0 || !rn2(3 * abs(mb->spe)))) {
            pline_The("%s is insightful.", verb);
            /* pre-damage status */
            probe_monster(mdef);
        }
        break;
    }
    /* stun if that was selected and a worse effect didn't occur */
    if (do_stun) {
        if (youdefend)
            make_stunned(((HStun & TIMEOUT) + 3L), FALSE);
        else
            mdef->mstun = 1;
        /* avoid extra stun message below if we used mb_verb["stun"] above */
        if (attack_indx == MB_INDEX_STUN)
            do_stun = FALSE;
    }
    /* lastly, all this magic can be confusing... */
    do_confuse = !rn2(12);
    if (do_confuse) {
        if (youdefend)
            make_confused((HConfusion & TIMEOUT) + 4L, FALSE);
        else
            mdef->mconf = 1;
    }

    /* now give message(s) describing side-effects; Use fakename
       so vtense() won't be fooled by assigned name ending in 's' */
    fakeidx = youdefend ? 1 : 0;
    if (youattack || youdefend || vis) {
        (void) upstart(hittee); /* capitalize */
        if (resisted) {
            pline("%s %s!", hittee, vtense(fakename[fakeidx], "resist"));
            shieldeff(youdefend ? u.ux : mdef->mx,
                      youdefend ? u.uy : mdef->my);
        }
        if ((do_stun || do_confuse) && Verbose(0,Mb_hit)) {
            char buf[BUFSZ];

            buf[0] = '\0';
            if (do_stun)
                Strcat(buf, "stunned");
            if (do_stun && do_confuse)
                Strcat(buf, " and ");
            if (do_confuse)
                Strcat(buf, "confused");
            pline("%s %s %s%c", hittee, vtense(fakename[fakeidx], "are"), buf,
                  (do_stun && do_confuse) ? '!' : '.');
        }
    }

    return result;
}

DISABLE_WARNING_FORMAT_NONLITERAL

/* Function used when someone attacks someone else with an artifact
 * weapon.  Only adds the special (artifact) damage, and returns a 1 if it
 * did something special (in which case the caller won't print the normal
 * hit message).  This should be called once upon every artifact attack;
 * dmgval() no longer takes artifact bonuses into account.  Possible
 * extension: change the killer so that when an orc kills you with
 * Stormbringer it's "killed by Stormbringer" instead of "killed by an orc".
 */
boolean
artifact_hit(
    struct monst *magr, /* attacker; might be Null if 'mdef' is youmonst */
    struct monst *mdef, /* defender */
    struct obj *otmp,   /* artifact weapon */
    int *dmgptr,        /* output */
    int dieroll)        /* needed for Magicbane and vorpal blades */
{
    boolean youattack = (magr == &g.youmonst);
    boolean youdefend = (mdef == &g.youmonst);
    boolean vis = (!youattack && magr && cansee(magr->mx, magr->my))
                  || (!youdefend && cansee(mdef->mx, mdef->my))
                  || (youattack && engulfing_u(mdef) && !Blind);
    boolean realizes_damage;
    const char *wepdesc;
    static const char you[] = "you";
    char hittee[BUFSZ];

    Strcpy(hittee, youdefend ? you : mon_nam(mdef));

    /* The following takes care of most of the damage, but not all--
     * the exception being for level draining, which is specially
     * handled.  Messages are done in this function, however.
     */
    *dmgptr += spec_dbon(otmp, mdef, *dmgptr);

    if (youattack && youdefend) {
        impossible("attacking yourself with weapon?");
        return FALSE;
    }

    realizes_damage = (youdefend || vis
                       /* feel the effect even if not seen */
                       || (youattack && mdef == u.ustuck));

    /* the four basic attacks: fire, cold, shock and missiles */
    if (attacks(AD_FIRE, otmp)) {
        if (realizes_damage)
            pline_The("fiery blade %s %s%c",
                      !g.spec_dbon_applies
                          ? "hits"
                          : (mdef->data == &mons[PM_WATER_ELEMENTAL])
                                ? "vaporizes part of"
                                : "burns",
                      hittee, !g.spec_dbon_applies ? '.' : '!');
        if (!rn2(4))
            (void) destroy_mitem(mdef, POTION_CLASS, AD_FIRE);
        if (!rn2(4))
            (void) destroy_mitem(mdef, SCROLL_CLASS, AD_FIRE);
        if (!rn2(7))
            (void) destroy_mitem(mdef, SPBOOK_CLASS, AD_FIRE);
        if (!rn2(4))
            ignite_items(mdef->minvent);
        if (youdefend && Slimed)
            burn_away_slime();
        return realizes_damage;
    }
    if (attacks(AD_COLD, otmp)) {
        if (realizes_damage)
            pline_The("ice-cold blade %s %s%c",
                      !g.spec_dbon_applies ? "hits" : "freezes", hittee,
                      !g.spec_dbon_applies ? '.' : '!');
        if (!rn2(4))
            (void) destroy_mitem(mdef, POTION_CLASS, AD_COLD);
        return realizes_damage;
    }
    if (attacks(AD_ELEC, otmp)) {
        if (realizes_damage)
            pline_The("massive hammer hits%s %s%c",
                      !g.spec_dbon_applies ? "" : "!  Lightning strikes",
                      hittee, !g.spec_dbon_applies ? '.' : '!');
        if (g.spec_dbon_applies)
            wake_nearto(mdef->mx, mdef->my, 4 * 4);
        if (!rn2(5))
            (void) destroy_mitem(mdef, RING_CLASS, AD_ELEC);
        if (!rn2(5))
            (void) destroy_mitem(mdef, WAND_CLASS, AD_ELEC);
        return realizes_damage;
    }
    if (attacks(AD_MAGM, otmp)) {
        if (realizes_damage)
            pline_The("imaginary widget hits%s %s%c",
                      !g.spec_dbon_applies
                          ? ""
                          : "!  A hail of magic missiles strikes",
                      hittee, !g.spec_dbon_applies ? '.' : '!');
        return realizes_damage;
    }

    if (attacks(AD_STUN, otmp) && dieroll <= MB_MAX_DIEROLL) {
        /* Magicbane's special attacks (possibly modifies hittee[]) */
        return Mb_hit(magr, mdef, otmp, dmgptr, dieroll, vis, hittee);
    }

    if (!g.spec_dbon_applies) {
        /* since damage bonus didn't apply, nothing more to do;
           no further attacks have side-effects on inventory */
        return FALSE;
    }

    /* We really want "on a natural 20" but Nethack does it in */
    /* reverse from AD&D. */
    if (spec_ability(otmp, SPFX_BEHEAD)) {
        if (is_art(otmp, ART_TSURUGI_OF_MURAMASA) && dieroll == 1) {
            wepdesc = "The razor-sharp blade";
            /* not really beheading, but so close, why add another SPFX */
            if (youattack && engulfing_u(mdef)) {
                You("slice %s wide open!", mon_nam(mdef));
                *dmgptr = 2 * mdef->mhp + FATAL_DAMAGE_MODIFIER;
                return TRUE;
            }
            if (!youdefend) {
                /* allow normal cutworm() call to add extra damage */
                if (g.notonhead)
                    return FALSE;

                if (bigmonst(mdef->data)) {
                    if (youattack)
                        You("slice deeply into %s!", mon_nam(mdef));
                    else if (vis)
                        pline("%s cuts deeply into %s!", Monnam(magr),
                              hittee);
                    *dmgptr *= 2;
                    return TRUE;
                }
                *dmgptr = 2 * mdef->mhp + FATAL_DAMAGE_MODIFIER;
                pline("%s cuts %s in half!", wepdesc, mon_nam(mdef));
                otmp->dknown = TRUE;
                return TRUE;
            } else {
                if (bigmonst(g.youmonst.data)) {
                    pline("%s cuts deeply into you!",
                          magr ? Monnam(magr) : wepdesc);
                    *dmgptr *= 2;
                    return TRUE;
                }

                /* Players with negative AC's take less damage instead
                 * of just not getting hit.  We must add a large enough
                 * value to the damage so that this reduction in
                 * damage does not prevent death.
                 */
                *dmgptr = 2 * (Upolyd ? u.mh : u.uhp) + FATAL_DAMAGE_MODIFIER;
                pline("%s cuts you in half!", wepdesc);
                otmp->dknown = TRUE;
                return TRUE;
            }
        } else if (is_art(otmp, ART_VORPAL_BLADE)
                   && (dieroll == 1 || mdef->data == &mons[PM_JABBERWOCK])) {
            static const char *const behead_msg[2] = { "%s beheads %s!",
                                                       "%s decapitates %s!" };

            if (youattack && engulfing_u(mdef))
                return FALSE;
            wepdesc = artilist[ART_VORPAL_BLADE].name;
            if (!youdefend) {
                if (!has_head(mdef->data) || g.notonhead || u.uswallow) {
                    if (youattack)
                        pline("Somehow, you miss %s wildly.", mon_nam(mdef));
                    else if (vis)
                        pline("Somehow, %s misses wildly.", mon_nam(magr));
                    *dmgptr = 0;
                    return (boolean) (youattack || vis);
                }
                if (noncorporeal(mdef->data) || amorphous(mdef->data)) {
                    pline("%s slices through %s %s.", wepdesc,
                          s_suffix(mon_nam(mdef)), mbodypart(mdef, NECK));
                    return TRUE;
                }
                *dmgptr = 2 * mdef->mhp + FATAL_DAMAGE_MODIFIER;
                pline(behead_msg[rn2(SIZE(behead_msg))], wepdesc,
                      mon_nam(mdef));
                if (Hallucination && !flags.female)
                    pline("Good job Henry, but that wasn't Anne.");
                otmp->dknown = TRUE;
                return TRUE;
            } else {
                if (!has_head(g.youmonst.data)) {
                    pline("Somehow, %s misses you wildly.",
                          magr ? mon_nam(magr) : wepdesc);
                    *dmgptr = 0;
                    return TRUE;
                }
                if (noncorporeal(g.youmonst.data)
                    || amorphous(g.youmonst.data)) {
                    pline("%s slices through your %s.", wepdesc,
                          body_part(NECK));
                    return TRUE;
                }
                *dmgptr = 2 * (Upolyd ? u.mh : u.uhp) + FATAL_DAMAGE_MODIFIER;
                pline(behead_msg[rn2(SIZE(behead_msg))], wepdesc, "you");
                otmp->dknown = TRUE;
                /* Should amulets fall off? */
                return TRUE;
            }
        }
    }
    if (spec_ability(otmp, SPFX_DRLI)) {
        /* some non-living creatures (golems, vortices) are vulnerable to
           life drain effects so can get "<Arti> draws the <life>" feedback */
        const char *life = nonliving(mdef->data) ? "animating force" : "life";

        if (!youdefend) {
            int m_lev = (int) mdef->m_lev, /* will be 0 for 1d4 mon */
                mhpmax = mdef->mhpmax,
                drain = monhp_per_lvl(mdef); /* usually 1d8 */
                /* note: DRLI attack uses 2d6, attacker doesn't get healed */

            /* stop draining HP if it drops too low (still drains level;
               also caller still inflicts regular weapon damage) */
            if (mhpmax - drain <= m_lev)
                drain = (mhpmax > m_lev) ? (mhpmax - (m_lev + 1)) : 0;

            if (vis) {
                /* call distant_name() for possible side-effects even if
                   the result won't be printed */
                char *otmpname = distant_name(otmp, xname);

                if (is_art(otmp, ART_STORMBRINGER))
                    pline_The("%s blade draws the %s from %s!",
                              hcolor(NH_BLACK), life, mon_nam(mdef));
                else
                    pline("%s draws the %s from %s!",
                          The(otmpname), life, mon_nam(mdef));
            }
            if (mdef->m_lev == 0) {
                /* losing a level when at 0 is fatal */
                *dmgptr = 2 * mdef->mhp + FATAL_DAMAGE_MODIFIER;
            } else {
                *dmgptr += drain;
                mdef->mhpmax -= drain;
                mdef->m_lev--;
            }

            if (drain > 0) {
                /* drain: was target's damage, now heal attacker by half */
                drain = (drain + 1) / 2; /* drain/2 rounded up */
                if (youattack) {
                    healup(drain, 0, FALSE, FALSE);
                } else {
                    magr->mhp += drain;
                    if (magr->mhp > magr->mhpmax)
                        magr->mhp = magr->mhpmax;
                }
            }
            return vis;
        } else { /* youdefend */
            int oldhpmax = u.uhpmax;

            if (Blind) {
                You_feel("an %s drain your %s!",
                         is_art(otmp, ART_STORMBRINGER)
                            ? "unholy blade"
                            : "object",
                         life);
            } else {
                /* call distant_name() for possible side-effects even if
                   the result won't be printed */
                char *otmpname = distant_name(otmp, xname);

                if (is_art(otmp, ART_STORMBRINGER))
                    pline_The("%s blade drains your %s!",
                              hcolor(NH_BLACK), life);
                else
                    pline("%s drains your %s!", The(otmpname), life);
            }
            losexp("life drainage");
            if (magr && magr->mhp < magr->mhpmax) {
                magr->mhp += (abs(oldhpmax - u.uhpmax) + 1) / 2;
                if (magr->mhp > magr->mhpmax)
                    magr->mhp = magr->mhpmax;
            }
            return TRUE;
        }
    }
    return FALSE;
}

RESTORE_WARNING_FORMAT_NONLITERAL

/* getobj callback for object to be invoked */
static int
invoke_ok(struct obj *obj)
{
    if (!obj)
        return GETOBJ_EXCLUDE;

    /* artifacts and other special items */
    if (obj->oartifact || objects[obj->otyp].oc_unique
        || (obj->otyp == FAKE_AMULET_OF_YENDOR && !obj->known))
        return GETOBJ_SUGGEST;

    /* synonym for apply, though actually invoking it will do different things
     * depending if it's a regular crystal ball, an artifact one that has an
     * invoke power, and a (theoretical) artifact one that doesn't have an
     * invoke power */
    if (obj->otyp == CRYSTAL_BALL)
        return GETOBJ_SUGGEST;

    return GETOBJ_EXCLUDE;
}

/* the #invoke command */
int
doinvoke(void)
{
    struct obj *obj;

    obj = getobj("invoke", invoke_ok, GETOBJ_PROMPT);
    if (!obj)
        return ECMD_CANCEL;
    if (!retouch_object(&obj, FALSE))
        return ECMD_TIME;
    return arti_invoke(obj);
}

static int
arti_invoke(struct obj *obj)
{
    register const struct artifact *oart = get_artifact(obj);
    if (!obj) {
        impossible("arti_invoke without obj");
        return ECMD_OK;
    }
    if (!oart || !oart->inv_prop) {
        if (obj->otyp == CRYSTAL_BALL)
            use_crystal_ball(&obj);
        else
            pline1(nothing_happens);
        return ECMD_TIME;
    }

    if (oart->inv_prop > LAST_PROP) {
        /* It's a special power, not "just" a property */
        if (obj->age > g.moves) {
            /* the artifact is tired :-) */
            You_feel("that %s %s ignoring you.", the(xname(obj)),
                     otense(obj, "are"));
            /* and just got more so; patience is essential... */
            obj->age += (long) d(3, 10);
            return ECMD_TIME;
        }
        obj->age = g.moves + rnz(100);

        switch (oart->inv_prop) {
        case TAMING: {
            struct obj pseudo;

            pseudo =
                cg.zeroobj; /* neither cursed nor blessed, zero oextra too */
            pseudo.otyp = SCR_TAMING;
            (void) seffects(&pseudo);
            break;
        }
        case HEALING: {
            int healamt = (u.uhpmax + 1 - u.uhp) / 2;
            long creamed = (long) u.ucreamed;

            if (Upolyd)
                healamt = (u.mhmax + 1 - u.mh) / 2;
            if (healamt || Sick || Slimed || Blinded > creamed)
                You_feel("better.");
            else
                goto nothing_special;
            if (healamt > 0) {
                if (Upolyd)
                    u.mh += healamt;
                else
                    u.uhp += healamt;
            }
            if (Sick)
                make_sick(0L, (char *) 0, FALSE, SICK_ALL);
            if (Slimed)
                make_slimed(0L, (char *) 0);
            if (Blinded > creamed)
                make_blinded(creamed, FALSE);
            g.context.botl = TRUE;
            break;
        }
        case ENERGY_BOOST: {
            int epboost = (u.uenmax + 1 - u.uen) / 2;

            if (epboost > 120)
                epboost = 120; /* arbitrary */
            else if (epboost < 12)
                epboost = u.uenmax - u.uen;
            if (epboost) {
                u.uen += epboost;
                g.context.botl = TRUE;
                You_feel("re-energized.");
            } else
                goto nothing_special;
            break;
        }
        case UNTRAP: {
            if (!untrap(TRUE, 0, 0, (struct obj *) 0)) {
                obj->age = 0; /* don't charge for changing their mind */
                return ECMD_OK;
            }
            break;
        }
        case CHARGE_OBJ: {
            struct obj *otmp = getobj("charge", charge_ok,
                                      GETOBJ_PROMPT | GETOBJ_ALLOWCNT);
            boolean b_effect;

            if (!otmp) {
                obj->age = 0;
                return ECMD_CANCEL;
            }
            b_effect = (obj->blessed && (oart->role == Role_switch
                                         || oart->role == NON_PM));
            recharge(otmp, b_effect ? 1 : obj->cursed ? -1 : 0);
            update_inventory();
            break;
        }
        case LEV_TELE:
            level_tele();
            break;
        case CREATE_PORTAL: {
            int i, num_ok_dungeons, last_ok_dungeon = 0;
            d_level newlev;
            winid tmpwin = create_nhwindow(NHW_MENU);
            anything any;
            int clr = 0;

            any = cg.zeroany; /* set all bits to zero */
            start_menu(tmpwin, MENU_BEHAVE_STANDARD);
            /* use index+1 (cant use 0) as identifier */
            for (i = num_ok_dungeons = 0; i < g.n_dgns; i++) {
                if (!g.dungeons[i].dunlev_ureached)
                    continue;
                any.a_int = i + 1;
                add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0,
                         ATR_NONE, clr,
                         g.dungeons[i].dname, MENU_ITEMFLAGS_NONE);
                num_ok_dungeons++;
                last_ok_dungeon = i;
            }
            end_menu(tmpwin, "Open a portal to which dungeon?");
            if (num_ok_dungeons > 1) {
                /* more than one entry; display menu for choices */
                menu_item *selected;
                int n;

                n = select_menu(tmpwin, PICK_ONE, &selected);
                if (n <= 0) {
                    destroy_nhwindow(tmpwin);
                    goto nothing_special;
                }
                i = selected[0].item.a_int - 1;
                free((genericptr_t) selected);
            } else
                i = last_ok_dungeon; /* also first & only OK dungeon */
            destroy_nhwindow(tmpwin);

            /*
             * i is now index into dungeon structure for the new dungeon.
             * Find the closest level in the given dungeon, open
             * a use-once portal to that dungeon and go there.
             * The closest level is either the entry or dunlev_ureached.
             */
            newlev.dnum = i;
            if (g.dungeons[i].depth_start >= depth(&u.uz))
                newlev.dlevel = g.dungeons[i].entry_lev;
            else
                newlev.dlevel = g.dungeons[i].dunlev_ureached;

            if (u.uhave.amulet || In_endgame(&u.uz) || In_endgame(&newlev)
                || newlev.dnum == u.uz.dnum || !next_to_u()) {
                You_feel("very disoriented for a moment.");
            } else {
                if (!Blind)
                    You("are surrounded by a shimmering sphere!");
                else
                    You_feel("weightless for a moment.");
                goto_level(&newlev, FALSE, FALSE, FALSE);
            }
            break;
        }
        case ENLIGHTENING:
            enlightenment(MAGICENLIGHTENMENT, ENL_GAMEINPROGRESS);
            break;
        case CREATE_AMMO: {
            struct obj *otmp = mksobj(ARROW, TRUE, FALSE);

            if (!otmp)
                goto nothing_special;
            otmp->blessed = obj->blessed;
            otmp->cursed = obj->cursed;
            otmp->bknown = obj->bknown;
            if (obj->blessed) {
                if (otmp->spe < 0)
                    otmp->spe = 0;
                otmp->quan += rnd(10);
            } else if (obj->cursed) {
                if (otmp->spe > 0)
                    otmp->spe = 0;
            } else
                otmp->quan += rnd(5);
            otmp->owt = weight(otmp);
            otmp = hold_another_object(otmp, "Suddenly %s out.",
                                       aobjnam(otmp, "fall"), (char *) 0);
            nhUse(otmp);
            break;
        }
        case BANISH: {
            int nvanished = 0, nstayed = 0;
            struct monst *mtmp, *mtmp2;
            d_level dest;

            find_hell(&dest);

            for (mtmp = fmon; mtmp; mtmp = mtmp2) {
                int chance = 1;

                mtmp2 = mtmp->nmon;
                if (DEADMONSTER(mtmp) || !isok(mtmp->mx, mtmp->my))
                    continue;
                if (!is_demon(mtmp->data) && mtmp->data->mlet != S_IMP)
                    continue;
                if (!couldsee(mtmp->mx, mtmp->my))
                    continue;
                if (mtmp->data->msound == MS_NEMESIS)
                    continue;

                if (In_quest(&u.uz) && !g.quest_status.killed_nemesis)
                    chance += 10;
                if (is_dprince(mtmp->data))
                    chance += 2;
                if (is_dlord(mtmp->data))
                    chance++;

                mtmp->msleeping = mtmp->mtame = mtmp->mpeaceful = 0;
                if (chance <= 1 || !rn2(chance)) {
                    if (!Inhell) {
                        nvanished++;
                        /* banish to a random level in Gehennom */
                        dest.dlevel = rn2(dunlevs_in_dungeon(&dest));
                        migrate_mon(mtmp, ledger_no(&dest), MIGR_RANDOM);
                    } else {
                        u_teleport_mon(mtmp, FALSE);
                    }
                } else {
                    nstayed++;
                }
            }

            if (nvanished) {
                pline("%she demon%s disappear%s in a cloud of brimstone!",
                      nstayed ? (nvanished > nstayed ? "Most of t" : "Some of t") : "T",
                      nvanished > 1 ? "s" : "",
                      nvanished > 1 ? "" : "s");
            }
            break;
        }
        }
    } else {
        long eprop = (u.uprops[oart->inv_prop].extrinsic ^= W_ARTI),
             iprop = u.uprops[oart->inv_prop].intrinsic;
        boolean on = (eprop & W_ARTI) != 0; /* true if prop just set */

        if (on && obj->age > g.moves) {
            /* the artifact is tired :-) */
            u.uprops[oart->inv_prop].extrinsic ^= W_ARTI;
            You_feel("that %s %s ignoring you.", the(xname(obj)),
                     otense(obj, "are"));
            /* can't just keep repeatedly trying */
            obj->age += (long) d(3, 10);
            return ECMD_TIME;
        } else if (!on) {
            /* when turning off property, determine downtime */
            /* arbitrary for now until we can tune this -dlc */
            obj->age = g.moves + rnz(100);
        }

        if ((eprop & ~W_ARTI) || iprop) {
 nothing_special:
            /* you had the property from some other source too */
            if (carried(obj))
                You_feel("a surge of power, but nothing seems to happen.");
            return ECMD_TIME;
        }
        switch (oart->inv_prop) {
        case CONFLICT:
            if (on)
                You_feel("like a rabble-rouser.");
            else
                You_feel("the tension decrease around you.");
            break;
        case LEVITATION:
            if (on) {
                float_up();
                spoteffects(FALSE);
            } else
                (void) float_down(I_SPECIAL | TIMEOUT, W_ARTI);
            break;
        case INVIS:
            if (BInvis || Blind)
                goto nothing_special;
            newsym(u.ux, u.uy);
            if (on)
                Your("body takes on a %s transparency...",
                     Hallucination ? "normal" : "strange");
            else
                Your("body seems to unfade...");
            break;
        }
    }

    return ECMD_TIME;
}

/* will freeing this object from inventory cause levitation to end? */
boolean
finesse_ahriman(struct obj *obj)
{
    const struct artifact *oart;
    struct prop save_Lev;
    boolean result;

    /* if we aren't levitating or this isn't an artifact which confers
       levitation via #invoke then freeinv() won't toggle levitation */
    if (!Levitation || (oart = get_artifact(obj)) == 0
        || oart->inv_prop != LEVITATION || !(ELevitation & W_ARTI))
        return FALSE;

    /* arti_invoke(off) -> float_down() clears I_SPECIAL|TIMEOUT & W_ARTI;
       probe ahead to see whether that actually results in floating down;
       (this assumes that there aren't two simultaneously invoked artifacts
       both conferring levitation--safe, since if there were two of them,
       invoking the 2nd would negate the 1st rather than stack with it) */
    save_Lev = u.uprops[LEVITATION];
    HLevitation &= ~(I_SPECIAL | TIMEOUT);
    ELevitation &= ~W_ARTI;
    result = (boolean) !Levitation;
    u.uprops[LEVITATION] = save_Lev;
    return result;
}

/* WAC return TRUE if artifact is always lit */
boolean
artifact_light(struct obj *obj)
{
    /* not artifacts but treat them as if they were because they emit
       light without burning */
    if (obj && (obj->otyp == GOLD_DRAGON_SCALE_MAIL
                || obj->otyp == GOLD_DRAGON_SCALES)
        && (obj->owornmask & W_ARM) != 0L)
        return TRUE;

    return (boolean) (get_artifact(obj) && is_art(obj, ART_SUNSWORD));
}

/* KMH -- Talking artifacts are finally implemented */
void
arti_speak(struct obj *obj)
{
    register const struct artifact *oart = get_artifact(obj);
    const char *line;
    char buf[BUFSZ];

    /* Is this a speaking artifact? */
    if (!oart || !(oart->spfx & SPFX_SPEAK))
        return;

    line = getrumor(bcsign(obj), buf, TRUE);
    if (!*line)
        line = "NetHack rumors file closed for renovation.";
    pline("%s:", Tobjnam(obj, "whisper"));
    verbalize1(line);
    return;
}

boolean
artifact_has_invprop(struct obj *otmp, uchar inv_prop)
{
    const struct artifact *arti = get_artifact(otmp);

    return (boolean) (arti && (arti->inv_prop == inv_prop));
}

/* Return the price sold to the hero of a given artifact or unique item */
long
arti_cost(struct obj *otmp)
{
    if (!otmp->oartifact)
        return (long) objects[otmp->otyp].oc_cost;
    else if (artilist[(int) otmp->oartifact].cost)
        return artilist[(int) otmp->oartifact].cost;
    else
        return (100L * (long) objects[otmp->otyp].oc_cost);
}

static uchar
abil_to_adtyp(long *abil)
{
    struct abil2adtyp_tag {
        long *abil;
        uchar adtyp;
    } abil2adtyp[] = {
        { &EFire_resistance, AD_FIRE },
        { &ECold_resistance, AD_COLD },
        { &EShock_resistance, AD_ELEC },
        { &EAntimagic, AD_MAGM },
        { &EDisint_resistance, AD_DISN },
        { &EPoison_resistance, AD_DRST },
        { &EDrain_resistance, AD_DRLI },
    };
    int k;

    for (k = 0; k < SIZE(abil2adtyp); k++) {
        if (abil2adtyp[k].abil == abil)
            return abil2adtyp[k].adtyp;
    }
    return 0;
}

static unsigned long
abil_to_spfx(long *abil)
{
    static const struct abil2spfx_tag {
        long *abil;
        unsigned long spfx;
    } abil2spfx[] = {
        { &ESearching, SPFX_SEARCH },
        { &EHalluc_resistance, SPFX_HALRES },
        { &ETelepat, SPFX_ESP },
        { &EStealth, SPFX_STLTH },
        { &ERegeneration, SPFX_REGEN },
        { &ETeleport_control, SPFX_TCTRL },
        { &EWarn_of_mon, SPFX_WARN },
        { &EWarning, SPFX_WARN },
        { &EEnergy_regeneration, SPFX_EREGEN },
        { &EHalf_spell_damage, SPFX_HSPDAM },
        { &EHalf_physical_damage, SPFX_HPHDAM },
        { &EReflecting, SPFX_REFLECT },
    };
    int k;

    for (k = 0; k < SIZE(abil2spfx); k++) {
        if (abil2spfx[k].abil == abil)
            return abil2spfx[k].spfx;
    }
    return 0L;
}

/*
 * Return the first item that is conveying a particular intrinsic.
 */
struct obj *
what_gives(long *abil)
{
    struct obj *obj;
    uchar dtyp;
    unsigned long spfx;
    long wornbits;
    long wornmask = (W_ARM | W_ARMC | W_ARMH | W_ARMS
                     | W_ARMG | W_ARMF | W_ARMU
                     | W_AMUL | W_RINGL | W_RINGR | W_TOOL
                     | W_ART | W_ARTI);

    if (u.twoweap)
        wornmask |= W_SWAPWEP;
    dtyp = abil_to_adtyp(abil);
    spfx = abil_to_spfx(abil);
    wornbits = (wornmask & *abil);

    for (obj = g.invent; obj; obj = obj->nobj) {
        if (obj->oartifact
            && (abil != &EWarn_of_mon || g.context.warntype.obj)) {
            const struct artifact *art = get_artifact(obj);

            if (art) {
                if (dtyp) {
                    if (art->cary.adtyp == dtyp /* carried */
                        || (art->defn.adtyp == dtyp /* defends while worn */
                            && (obj->owornmask & ~(W_ART | W_ARTI))))
                        return obj;
                }
                if (spfx) {
                    /* property conferred when carried */
                    if ((art->cspfx & spfx) == spfx)
                        return obj;
                    /* property conferred when wielded or worn */
                    if ((art->spfx & spfx) == spfx && obj->owornmask)
                        return obj;
                }
            }
        } else {
            if (wornbits && wornbits == (wornmask & obj->owornmask))
                return obj;
        }
    }
    return (struct obj *) 0;
}

const char *
glow_color(int arti_indx)
{
    int colornum = artilist[arti_indx].acolor;
    const char *colorstr = clr2colorname(colornum);

    return hcolor(colorstr);
}

/* glow verb; [0] holds the value used when blind */
static const char *const glow_verbs[] = {
    "quiver", "flicker", "glimmer", "gleam"
};

/* relative strength that Sting is glowing (0..3), to select verb */
static int
glow_strength(int count)
{
    /* glow strength should also be proportional to proximity and
       probably difficulty, but we don't have that information and
       gathering it is more trouble than this would be worth */
    return (count > 12) ? 3 : (count > 4) ? 2 : (count > 0);
}

const char *
glow_verb(int count, /* 0 means blind rather than no applicable creatures */
          boolean ingsfx)
{
    static char resbuf[20];

    Strcpy(resbuf, glow_verbs[glow_strength(count)]);
    /* ing_suffix() will double the last consonant for all the words
       we're using and none of them should have that, so bypass it */
    if (ingsfx)
        Strcat(resbuf, "ing");
    return resbuf;
}

/* use for warning "glow" for Sting, Orcrist, and Grimtooth */
void
Sting_effects(int orc_count) /* new count (warn_obj_cnt is old count); -1 is a flag value */
{
    if (u_wield_art(ART_STING)
        || u_wield_art(ART_ORCRIST)
        || u_wield_art(ART_GRIMTOOTH)) {
        int oldstr = glow_strength(g.warn_obj_cnt),
            newstr = glow_strength(orc_count);

        if (orc_count == -1 && g.warn_obj_cnt > 0) {
            /* -1 means that blindness has just been toggled; give a
               'continue' message that eventual 'stop' message will match */
            pline("%s is %s.", bare_artifactname(uwep),
                  glow_verb(Blind ? 0 : g.warn_obj_cnt, TRUE));
        } else if (newstr > 0 && newstr != oldstr) {
            /* goto_level() -> docrt() -> see_monsters() -> Sting_effects();
               if "you materialize on a different level" is pending, give
               it now so that start-glowing message comes after it */
            maybe_lvltport_feedback(); /* usually called by goto_level() */

            /* 'start' message */
            if (!Blind)
                pline("%s %s %s%c", bare_artifactname(uwep),
                      otense(uwep, glow_verb(orc_count, FALSE)),
                      glow_color(uwep->oartifact),
                      (newstr > oldstr) ? '!' : '.');
            else if (oldstr == 0) /* quivers */
                pline("%s %s slightly.", bare_artifactname(uwep),
                      otense(uwep, glow_verb(0, FALSE)));
        } else if (orc_count == 0 && g.warn_obj_cnt > 0) {
            /* 'stop' message */
            pline("%s stops %s.", bare_artifactname(uwep),
                  glow_verb(Blind ? 0 : g.warn_obj_cnt, TRUE));
        }
    }
}

/* called when hero is wielding/applying/invoking a carried item, or
   after undergoing a transformation (alignment change, lycanthropy,
   polymorph) which might affect item access */
int
retouch_object(
    struct obj **objp, /* might be destroyed or unintentionally dropped */
    boolean loseit)    /* whether to drop it if hero can longer touch it */
{
    struct obj *obj = *objp;

    /* allow hero in silver-hating form to try to perform invocation ritual */
    if (obj->otyp == BELL_OF_OPENING
        && invocation_pos(u.ux, u.uy) && !On_stairs(u.ux, u.uy)) {
        return 1;
    }

    if (touch_artifact(obj, &g.youmonst)) {
        char buf[BUFSZ];
        int dmg = 0, tmp;
        boolean ag = (objects[obj->otyp].oc_material == SILVER && Hate_silver),
                bane = bane_applies(get_artifact(obj), &g.youmonst);

        /* nothing else to do if hero can successfully handle this object */
        if (!ag && !bane)
            return 1;

        /* hero can't handle this object, but didn't get touch_artifact()'s
           "<obj> evades your grasp|control" message; give an alternate one */
        You_cant("handle %s%s!", yname(obj),
                 obj->owornmask ? " anymore" : "");
        /* also inflict damage unless touch_artifact() already did so */
        if (!touch_blasted) {
            const char *what = killer_xname(obj);

            if (ag && !obj->oartifact && !bane) {
                /* 'obj' is silver; for rings and wands it ended up that
                   way due to randomization at start of game; showing this
                   game's silver item without stating that it is silver
                   potentially leads to confusion about cause of death */
                if (obj->oclass == RING_CLASS)
                    what = "a silver ring";
                else if (obj->oclass == WAND_CLASS)
                    what = "a silver wand";
                /* for anything else, stick with killer_xname() */
            }
            /* damage is somewhat arbitrary; half the usual 1d20 physical
               for silver, 1d10 magical for <foo>bane, potentially both */
            if (ag)
                tmp = rnd(10), dmg += Maybe_Half_Phys(tmp);
            if (bane)
                dmg += rnd(10);
            Sprintf(buf, "handling %s", what);
            losehp(dmg, buf, KILLED_BY);
            exercise(A_CON, FALSE);
        }
    }

    /* removing a worn item might result in loss of levitation,
       dropping the hero onto a polymorph trap or into water or
       lava and potentially dropping or destroying the item */
    if (obj->owornmask) {
        struct obj *otmp;

        remove_worn_item(obj, FALSE);
        for (otmp = g.invent; otmp; otmp = otmp->nobj)
            if (otmp == obj)
                break;
        if (!otmp)
            *objp = obj = 0;
    }

    /* if we still have it and caller wants us to drop it, do so now */
    if (loseit && obj) {
        if (Levitation) {
            freeinv(obj);
            hitfloor(obj, TRUE);
        } else {
            /* dropx gives a message if a dropped item lands on an altar;
               we provide one for other terrain */
            if (!IS_ALTAR(levl[u.ux][u.uy].typ))
                pline("%s to the %s.", Tobjnam(obj, "fall"),
                      surface(u.ux, u.uy));
            dropx(obj);
        }
        *objp = obj = 0; /* no longer in inventory */
    }
    return 0;
}

/* an item which is worn/wielded or an artifact which conveys
   something via being carried or which has an #invoke effect
   currently in operation undergoes a touch test; if it fails,
   it will be unworn/unwielded and revoked but not dropped */
static boolean
untouchable(struct obj *obj, boolean drop_untouchable)
{
    struct artifact *art;
    boolean beingworn, carryeffect, invoked;
    long wearmask = ~(W_QUIVER | (u.twoweap ? 0L : W_SWAPWEP) | W_BALL);

    beingworn = ((obj->owornmask & wearmask) != 0L
                 /* some items in use don't have any wornmask setting */
                 || (obj->oclass == TOOL_CLASS
                     && (obj->lamplit || (obj->otyp == LEASH && obj->leashmon)
                         || (Is_container(obj) && Has_contents(obj)))));

    if ((art = get_artifact(obj)) != 0) {
        carryeffect = (art->cary.adtyp || art->cspfx);
        invoked = (art->inv_prop > 0 && art->inv_prop <= LAST_PROP
                   && (u.uprops[art->inv_prop].extrinsic & W_ARTI) != 0L);
    } else {
        carryeffect = invoked = FALSE;
    }

    if (beingworn || carryeffect || invoked) {
        if (!retouch_object(&obj, drop_untouchable)) {
            /* "<artifact> is beyond your control" or "you can't handle
               <object>" has been given and it is now unworn/unwielded
               and possibly dropped (depending upon caller); if dropped,
               carried effect was turned off, else we leave that alone;
               we turn off invocation property here if still carried */
            if (invoked && obj)
                (void) arti_invoke(obj); /* reverse #invoke */
            return TRUE;
        }
    }
    return FALSE;
}

/* check all items currently in use (mostly worn) for touchability */
void
retouch_equipment(int dropflag) /* 0==don't drop, 1==drop all, 2==drop weapon */
{
    static int nesting = 0; /* recursion control */
    struct obj *obj;
    boolean dropit, had_gloves = (uarmg != 0);
    int had_rings = (!!uleft + !!uright);

    /*
     * We can potentially be called recursively if losing/unwearing
     * an item causes worn helm of opposite alignment to come off or
     * be destroyed.
     *
     * BUG: if the initial call was due to putting on a helm of
     * opposite alignment and it does come off to trigger recursion,
     * after the inner call executes, the outer call will finish
     * using the non-helm alignment rather than the helm alignment
     * which triggered this in the first place.
     */
    if (!nesting++)
        clear_bypasses(); /* init upon initial entry */

    dropit = (dropflag > 0); /* drop all or drop weapon */
    /* check secondary weapon first, before possibly unwielding primary */
    if (u.twoweap) {
        bypass_obj(uswapwep); /* so loop below won't process it again */
        (void) untouchable(uswapwep, dropit);
    }
    /* check primary weapon next so that they're handled together */
    if (uwep) {
        bypass_obj(uwep); /* so loop below won't process it again */
        (void) untouchable(uwep, dropit);
    }

    /* in case someone is daft enough to add artifact or silver saddle */
    if (u.usteed && (obj = which_armor(u.usteed, W_SADDLE)) != 0) {
        /* untouchable() calls retouch_object() which expects an object in
           hero's inventory, but remove_worn_item() will be harmless for
           saddle and we're suppressing drop, so this works as intended */
        if (untouchable(obj, FALSE))
            dismount_steed(DISMOUNT_THROWN);
    }
    /*
     * TODO?  Force off gloves if either or both rings are going to
     * become unworn; force off cloak [suit] before suit [shirt].
     * The torso handling is hypothetical; the case for gloves is
     * not, due to the possibility of unwearing silver rings.
     */

    dropit = (dropflag == 1); /* all untouchable items */
    /* loss of levitation (silver ring, or Heart of Ahriman invocation)
       might cause hero to lose inventory items (by dropping into lava,
       for instance), so inventory traversal needs to rescan the whole
       g.invent chain each time it moves on to another object; we use bypass
       handling to keep track of which items have already been processed */
    while ((obj = nxt_unbypassed_obj(g.invent)) != 0)
        (void) untouchable(obj, dropit);

    if (had_rings != (!!uleft + !!uright) && uarmg && uarmg->cursed)
        uncurse(uarmg); /* temporary? hack for ring removal plausibility */
    if (had_gloves && !uarmg)
        selftouch("After losing your gloves, you");

    if (!--nesting)
        clear_bypasses(); /* reset upon final exit */
}

static int
count_surround_traps(coordxy x, coordxy y)
{
    struct rm *levp;
    struct obj *otmp;
    struct trap *ttmp;
    coordxy dx, dy;
    int glyph, ret = 0;

    for (dx = x - 1; dx < x + 2; ++dx)
        for (dy = y - 1; dy < y + 2; ++dy) {
            if (!isok(dx, dy))
                continue;
            /* If a trap is shown here, don't count it; the hero
             * should be expecting it.  But if there is a trap here
             * that's not shown, either undiscovered or covered by
             * something, do count it.
             */
            glyph = glyph_at(dx, dy);
            if (glyph_is_trap(glyph))
                continue;
            if ((ttmp = t_at(dx, dy)) != 0) {
                ++ret;
                continue;
            }
            levp = &levl[dx][dy];
            if (IS_DOOR(levp->typ) && (levp->doormask & D_TRAPPED) != 0) {
                ++ret;
                continue;
            }
            for (otmp = g.level.objects[dx][dy]; otmp; otmp = otmp->nexthere)
                if (Is_container(otmp) && otmp->otrapped) {
                    ++ret; /* we're counting locations, so just */
                    break; /* count the first one in a pile     */
                }
        }
    /*
     * [Shouldn't we also check inventory for a trapped container?
     * Even if its trap has already been found, there's no 'tknown'
     * flag to help hero remember that so we have nothing comparable
     * to a shown glyph to justify skipping it.]
     */
    return ret;
}

/* sense adjacent traps if wielding MKoT without wearing gloves */
void
mkot_trap_warn(void)
{
    static const char *const heat[7] = {
        "cool", "slightly warm", "warm", "very warm",
        "hot", "very hot", "like fire"
    };

    if (!uarmg && u_wield_art(ART_MASTER_KEY_OF_THIEVERY)) {
        int idx, ntraps = count_surround_traps(u.ux, u.uy);

        if (ntraps != g.mkot_trap_warn_count) {
            idx = min(ntraps, SIZE(heat) - 1);
            pline_The("Key feels %s%c", heat[idx], (ntraps > 3) ? '!' : '.');
        }
        g.mkot_trap_warn_count = ntraps;
    } else
        g.mkot_trap_warn_count = 0;
}

/* Master Key is magic key if its bless/curse state meets our criteria:
   not cursed for rogues or blessed for non-rogues */
boolean
is_magic_key(struct monst *mon, /* if null, non-rogue is assumed */
             struct obj *obj)
{
    if (is_art(obj, ART_MASTER_KEY_OF_THIEVERY)) {
        if ((mon == &g.youmonst) ? Role_if(PM_ROGUE)
                                 : (mon && mon->data == &mons[PM_ROGUE]))
            return !obj->cursed; /* a rogue; non-cursed suffices for magic */
        /* not a rogue; key must be blessed to behave as a magic one */
        return obj->blessed;
    }
    return FALSE;
}

/* figure out whether 'mon' (usually youmonst) is carrying the magic key */
struct obj *
has_magic_key(struct monst *mon) /* if null, hero assumed */
{
    struct obj *o;
    short key = artilist[ART_MASTER_KEY_OF_THIEVERY].otyp;

    if (!mon)
        mon = &g.youmonst;
    for (o = ((mon == &g.youmonst) ? g.invent : mon->minvent); o;
         o = nxtobj(o, key, FALSE)) {
        if (is_magic_key(mon, o))
            return o;
    }
    return (struct obj *) 0;
}

/*artifact.c*/
