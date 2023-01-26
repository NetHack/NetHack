/* NetHack 3.7	sounds.c	$NHDT-Date: 1674548234 2023/01/24 08:17:14 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.134 $ */
/*      Copyright (c) 1989 Janet Walz, Mike Threepoint */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

static boolean throne_mon_sound(struct monst *);
static boolean beehive_mon_sound(struct monst *);
static boolean morgue_mon_sound(struct monst *);
static boolean zoo_mon_sound(struct monst *);
static boolean temple_priest_sound(struct monst *);
static boolean mon_is_gecko(struct monst *);
static int domonnoise(struct monst *);
static int dochat(void);
static struct monst *responsive_mon_at(int, int);
static int mon_in_room(struct monst *, int);

/* this easily could be a macro, but it might overtax dumb compilers */
static int
mon_in_room(struct monst* mon, int rmtyp)
{
    int rno = levl[mon->mx][mon->my].roomno;
    if (rno >= ROOMOFFSET)
        return gr.rooms[rno - ROOMOFFSET].rtype == rmtyp;
    return FALSE;
}


static boolean
throne_mon_sound(struct monst *mtmp)
{
    if ((mtmp->msleeping || is_lord(mtmp->data)
         || is_prince(mtmp->data)) && !is_animal(mtmp->data)
        && mon_in_room(mtmp, COURT)) {
        static const char *const throne_msg[4] = {
            "the tones of courtly conversation.",
            "a sceptre pounded in judgment.",
            "Someone shouts \"Off with %s head!\"",
            "Queen Beruthiel's cats!",
        };
        int which = rn2(3) + (Hallucination ? 1 : 0);

        if (which != 2) {
            if (which == 0) {
                Soundeffect(se_courtly_conversation, 30);
            } else if (which == 1) {
                Soundeffect(se_sceptor_pounding, 100);
            }
            You_hear1(throne_msg[which]);
        } else {
            DISABLE_WARNING_FORMAT_NONLITERAL
            pline(throne_msg[2], uhis());
            RESTORE_WARNING_FORMAT_NONLITERAL
        }
        return TRUE;
    }
    return FALSE;
}


static boolean
beehive_mon_sound(struct monst *mtmp)
{
    if ((mtmp->data->mlet == S_ANT && is_flyer(mtmp->data))
        && mon_in_room(mtmp, BEEHIVE)) {
        int hallu = Hallucination ? 1 : 0;

        switch (rn2(2) + hallu) {
        case 0:
            Soundeffect(se_low_buzzing, 30);
            You_hear("a low buzzing.");
            break;
        case 1:
            Soundeffect(se_angry_drone, 100);
            You_hear("an angry drone.");
            break;
        case 2:
            Soundeffect(se_bees, 100);
            You_hear("bees in your %sbonnet!",
                     uarmh ? "" : "(nonexistent) ");
            break;
        }
        return TRUE;
    }
    return FALSE;
}

static boolean
morgue_mon_sound(struct monst *mtmp)
{
    if ((is_undead(mtmp->data) || is_vampshifter(mtmp))
        && mon_in_room(mtmp, MORGUE)) {
        int hallu = Hallucination ? 1 : 0;
        const char *hair = body_part(HAIR); /* hair/fur/scales */

        switch (rn2(2) + hallu) {
        case 0:
            You("suddenly realize it is unnaturally quiet.");
            break;
        case 1:
            pline_The("%s on the back of your %s %s up.", hair,
                      body_part(NECK), vtense(hair, "stand"));
            break;
        case 2:
            pline_The("%s on your %s %s to stand up.", hair,
                      body_part(HEAD), vtense(hair, "seem"));
            break;
        }
        return TRUE;
    }
    return FALSE;
}

static boolean
zoo_mon_sound(struct monst *mtmp)
{
    if ((mtmp->msleeping || is_animal(mtmp->data))
        && mon_in_room(mtmp, ZOO)) {
        int hallu = Hallucination ? 1 : 0;
        static const char *const zoo_msg[3] = {
            "a sound reminiscent of an elephant stepping on a peanut.",
            "a sound reminiscent of a seal barking.", "Doctor Dolittle!",
        };
        You_hear1(zoo_msg[rn2(2) + hallu]);
        return TRUE;
    }
    return FALSE;
}

static boolean
temple_priest_sound(struct monst *mtmp)
{
    if (mtmp->ispriest && inhistemple(mtmp)
        /* priest must be active */
        && !helpless(mtmp)
        /* hero must be outside this temple */
        && temple_occupied(u.urooms) != EPRI(mtmp)->shroom) {
        /* Generic temple messages; no attempt to match topic or tone
           to the pantheon involved, let alone to the specific deity.
           These are assumed to be coming from the attending priest;
           asterisk means that the priest must be capable of speech;
           pound sign (octathorpe,&c--don't go there) means that the
           priest and the altar must not be directly visible (we don't
           care if telepathy or extended detection reveals that the
           priest is not currently standing on the altar; he's mobile). */
        static const char *const temple_msg[] = {
            "*someone praising %s.", "*someone beseeching %s.",
            "#an animal carcass being offered in sacrifice.",
            "*a strident plea for donations.",
        };
        const char *msg;
        int hallu = Hallucination ? 1 : 0;
        int trycount = 0,
            ax = EPRI(mtmp)->shrpos.x,
            ay = EPRI(mtmp)->shrpos.y;
        boolean speechless = (mtmp->data->msound <= MS_ANIMAL),
            in_sight = canseemon(mtmp) || cansee(ax, ay);

        do {
            msg = temple_msg[rn2(SIZE(temple_msg) - 1 + hallu)];
            if (strchr(msg, '*') && speechless)
                continue;
            if (strchr(msg, '#') && in_sight)
                continue;
            break; /* msg is acceptable */
        } while (++trycount < 50);
        while (!letter(*msg))
            ++msg; /* skip control flags */
        if (strchr(msg, '%')) {
            DISABLE_WARNING_FORMAT_NONLITERAL
            You_hear(msg, halu_gname(EPRI(mtmp)->shralign));
            RESTORE_WARNING_FORMAT_NONLITERAL
        } else
            You_hear1(msg);
        return TRUE;
    }
    return FALSE;
}

static boolean
oracle_sound(struct monst *mtmp)
{
    if (mtmp->data != &mons[PM_ORACLE])
        return FALSE;

    /* and don't produce silly effects when she's clearly visible */
    if (Hallucination || !canseemon(mtmp)) {
        int hallu = Hallucination ? 1 : 0;
        static const char *const ora_msg[5] = {
            "a strange wind.",     /* Jupiter at Dodona */
            "convulsive ravings.", /* Apollo at Delphi */
            "snoring snakes.",     /* AEsculapius at Epidaurus */
            "someone say \"No more woodchucks!\"",
            "a loud ZOT!" /* both rec.humor.oracle */
        };
        You_hear1(ora_msg[rn2(3) + hallu * 2]);
    }
    return TRUE;
}

void
dosounds(void)
{
    register struct mkroom *sroom;
    register int hallu, vx, vy;
    struct monst *mtmp;

    if (Deaf || !flags.acoustics || u.uswallow || Underwater)
        return;

    hallu = Hallucination ? 1 : 0;

    if (gl.level.flags.nfountains && !rn2(400)) {
        static const char *const fountain_msg[4] = {
            "bubbling water.", "water falling on coins.",
            "the splashing of a naiad.", "a soda fountain!",
        };
        You_hear1(fountain_msg[rn2(3) + hallu]);
    }
    if (gl.level.flags.nsinks && !rn2(300)) {
        static const char *const sink_msg[3] = {
            "a slow drip.", "a gurgling noise.", "dishes being washed!",
        };
        You_hear1(sink_msg[rn2(2) + hallu]);
    }
    if (gl.level.flags.has_court && !rn2(200)) {
        if (get_iter_mons(throne_mon_sound))
            return;
    }
    if (gl.level.flags.has_swamp && !rn2(200)) {
        static const char *const swamp_msg[3] = {
            "hear mosquitoes!", "smell marsh gas!", /* so it's a smell...*/
            "hear Donald Duck!",
        };
        You1(swamp_msg[rn2(2) + hallu]);
        return;
    }
    if (gl.level.flags.has_vault && !rn2(200)) {
        if (!(sroom = search_special(VAULT))) {
            /* strange ... */
            gl.level.flags.has_vault = 0;
            return;
        }
        if (gd_sound())
            switch (rn2(2) + hallu) {
            case 1: {
                boolean gold_in_vault = FALSE;

                for (vx = sroom->lx; vx <= sroom->hx; vx++)
                    for (vy = sroom->ly; vy <= sroom->hy; vy++)
                        if (g_at(vx, vy))
                            gold_in_vault = TRUE;
                if (vault_occupied(u.urooms)
                    != (ROOM_INDEX(sroom) + ROOMOFFSET)) {
                    if (gold_in_vault) {
                        You_hear(!hallu
                                     ? "someone counting gold coins."
                                     : "the quarterback calling the play.");
                    } else {
                        Soundeffect(se_someone_searching, 30);
                        You_hear("someone searching.");
                    }
                    break;
                }
            }
                /*FALLTHRU*/
            case 0:
                Soundeffect(se_guards_footsteps, 30);
                You_hear("the footsteps of a guard on patrol.");
                break;
            case 2:
                You_hear("Ebenezer Scrooge!");
                break;
            }
        return;
    }
    if (gl.level.flags.has_beehive && !rn2(200)) {
        if (get_iter_mons(beehive_mon_sound))
            return;
    }
    if (gl.level.flags.has_morgue && !rn2(200)) {
        if (get_iter_mons(morgue_mon_sound))
            return;
    }
    if (gl.level.flags.has_barracks && !rn2(200)) {
        static const char *const barracks_msg[4] = {
            "blades being honed.", "loud snoring.", "dice being thrown.",
            "General MacArthur!",
        };
        int count = 0;

        for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
            if (DEADMONSTER(mtmp))
                continue;
            if (is_mercenary(mtmp->data)
#if 0 /* don't bother excluding these */
                && !strstri(mtmp->data->pmnames[NEUTRAL], "watch")
                && !strstri(mtmp->data->pmnames[NEUTRAL], "guard")
#endif
                && mon_in_room(mtmp, BARRACKS)
                /* sleeping implies not-yet-disturbed (usually) */
                && (mtmp->msleeping || ++count > 5)) {
                You_hear1(barracks_msg[rn2(3) + hallu]);
                return;
            }
        }
    }
    if (gl.level.flags.has_zoo && !rn2(200)) {
        if (get_iter_mons(zoo_mon_sound))
            return;
    }
    if (gl.level.flags.has_shop && !rn2(200)) {
        if (!(sroom = search_special(ANY_SHOP))) {
            /* strange... */
            gl.level.flags.has_shop = 0;
            return;
        }
        if (tended_shop(sroom)
            && !strchr(u.ushops, (int) (ROOM_INDEX(sroom) + ROOMOFFSET))) {
            static const char *const shop_msg[3] = {
                "someone cursing shoplifters.",
                "the chime of a cash register.", "Neiman and Marcus arguing!",
            };
            You_hear1(shop_msg[rn2(2) + hallu]);
        }
        return;
    }
    if (gl.level.flags.has_temple && !rn2(200)
        && !(Is_astralevel(&u.uz) || Is_sanctum(&u.uz))) {
        if (get_iter_mons(temple_priest_sound))
            return;
    }
    if (Is_oracle_level(&u.uz) && !rn2(400)) {
        if (get_iter_mons(oracle_sound))
            return;
    }
}

static const char *const h_sounds[] = {
    "beep",   "boing",   "sing",   "belche", "creak",   "cough",
    "rattle", "ululate", "pop",    "jingle", "sniffle", "tinkle",
    "eep",    "clatter", "hum",    "sizzle", "twitter", "wheeze",
    "rustle", "honk",    "lisp",   "yodel",  "coo",     "burp",
    "moo",    "boom",    "murmur", "oink",   "quack",   "rumble",
    "twang",  "bellow",  "toot",   "gargle", "hoot",    "warble"
};

const char *
growl_sound(register struct monst* mtmp)
{
    const char *ret;

    switch (mtmp->data->msound) {
    case MS_MEW:
    case MS_HISS:
        ret = "hiss";
        break;
    case MS_BARK:
    case MS_GROWL:
        ret = "growl";
        break;
    case MS_ROAR:
        ret = "roar";
        break;
    case MS_BUZZ:
        ret = "buzz";
        break;
    case MS_SQEEK:
        ret = "squeal";
        break;
    case MS_SQAWK:
        ret = "screech";
        break;
    case MS_NEIGH:
        ret = "neigh";
        break;
    case MS_WAIL:
        ret = "wail";
        break;
    case MS_GROAN:
        ret = "groan";
        break;
    case MS_MOO:
        ret = "low";
        break;
    case MS_SILENT:
        ret = "commotion";
        break;
    default:
        ret = "scream";
    }
    return ret;
}

/* the sounds of a seriously abused pet, including player attacking it */
void
growl(register struct monst* mtmp)
{
    register const char *growl_verb = 0;

    if (helpless(mtmp) || mtmp->data->msound == MS_SILENT)
        return;

    /* presumably nearness and soundok checks have already been made */
    if (Hallucination)
        growl_verb = h_sounds[rn2(SIZE(h_sounds))];
    else
        growl_verb = growl_sound(mtmp);
    if (growl_verb) {
        if (canseemon(mtmp) || !Deaf) {
            pline("%s %s!", Monnam(mtmp), vtense((char *) 0, growl_verb));
            iflags.last_msg = PLNMSG_GROWL;
            if (gc.context.run)
                nomul(0);
        }
        wake_nearto(mtmp->mx, mtmp->my, mtmp->data->mlevel * 18);
    }
}

/* the sounds of mistreated pets */
void
yelp(register struct monst* mtmp)
{
    register const char *yelp_verb = 0;
    enum sound_effect_entries se = se_yelp;

    if (helpless(mtmp) || !mtmp->data->msound)
        return;

    /* presumably nearness and soundok checks have already been made */
    if (Hallucination)
        yelp_verb = h_sounds[rn2(SIZE(h_sounds))];
    else
        switch (mtmp->data->msound) {
        case MS_MEW:
            se = se_feline_yelp;
            yelp_verb = (!Deaf) ? "yowl" : "arch";
            break;
        case MS_BARK:
        case MS_GROWL:
            se = se_canine_yelp;
            yelp_verb = (!Deaf) ? "yelp" : "recoil";
            break;
        case MS_ROAR:
            yelp_verb = (!Deaf) ? "snarl" : "bluff";
            break;
        case MS_SQEEK:
            se = se_squeal;
            yelp_verb = (!Deaf) ? "squeal" : "quiver";
            break;
        case MS_SQAWK:
            se = se_avian_screak;
            yelp_verb = (!Deaf) ? "screak" : "thrash";
            break;
        case MS_WAIL:
            se = se_wail;
            yelp_verb = (!Deaf) ? "wail" : "cringe";
            break;
        }
    if (yelp_verb) {
        Soundeffect(se, 70);  /* Soundeffect() handles Deaf or not Deaf */
        pline("%s %s!", Monnam(mtmp), vtense((char *) 0, yelp_verb));
        if (gc.context.run)
            nomul(0);
        wake_nearto(mtmp->mx, mtmp->my, mtmp->data->mlevel * 12);
    }
}

/* the sounds of distressed pets */
void
whimper(register struct monst* mtmp)
{
    register const char *whimper_verb = 0;
    enum sound_effect_entries se = se_canine_whine;
    if (helpless(mtmp) || !mtmp->data->msound)
        return;

    /* presumably nearness and soundok checks have already been made */
    if (Hallucination)
        whimper_verb = h_sounds[rn2(SIZE(h_sounds))];
    else
        switch (mtmp->data->msound) {
        case MS_MEW:
        case MS_GROWL:
            whimper_verb = "whimper";
            break;
        case MS_BARK:
            whimper_verb = "whine";
            break;
        case MS_SQEEK:
            se = se_squeal;
            whimper_verb = "squeal";
            break;
        }
    if (whimper_verb) {
        if (!Hallucination) {
            Soundeffect(se, 50);
        }
        pline("%s %s.", Monnam(mtmp), vtense((char *) 0, whimper_verb));
        if (gc.context.run)
            nomul(0);
        wake_nearto(mtmp->mx, mtmp->my, mtmp->data->mlevel * 6);
    }
}

/* pet makes "I'm hungry" noises */
void
beg(register struct monst* mtmp)
{
    if (helpless(mtmp)
        || !(carnivorous(mtmp->data) || herbivorous(mtmp->data)))
        return;

    /* presumably nearness and soundok checks have already been made */
    if (!is_silent(mtmp->data) && mtmp->data->msound <= MS_ANIMAL) {
        (void) domonnoise(mtmp);
    } else if (mtmp->data->msound >= MS_HUMANOID) {
        if (!canspotmon(mtmp))
            map_invisible(mtmp->mx, mtmp->my);
        verbalize("I'm hungry.");
    } else {
        /* this is pretty lame but is better than leaving out the block
           of speech types between animal and humanoid; this covers
           MS_SILENT too (if caller lets that get this far) since it's
           excluded by the first two cases */
        if (canspotmon(mtmp))
            pline("%s seems famished.", Monnam(mtmp));
        /* looking famished will be a good trick for a tame skeleton... */
    }
}

/* hero has attacked a peaceful monster within 'mon's view */
const char *
maybe_gasp(struct monst* mon)
{
    static const char *const Exclam[] = {
        "Gasp!", "Uh-oh.", "Oh my!", "What?", "Why?",
    };
    struct permonst *mptr = mon->data;
    int msound = mptr->msound;
    boolean dogasp = FALSE;

    /* other roles' guardians and cross-aligned priests don't gasp */
    if ((msound == MS_GUARDIAN && mptr != &mons[gu.urole.guardnum])
        || (msound == MS_PRIEST && !p_coaligned(mon)))
        msound = MS_SILENT;
    /* co-aligned angels do gasp */
    else if (msound == MS_CUSS && has_emin(mon)
           && (p_coaligned(mon) ? !EMIN(mon)->renegade : EMIN(mon)->renegade))
        msound = MS_HUMANOID;

    /*
     * Only called for humanoids so animal noise handling is ignored.
     */
    switch (msound) {
    case MS_HUMANOID:
    case MS_ARREST: /* Kops */
    case MS_SOLDIER: /* solider, watchman */
    case MS_GUARD: /* vault guard */
    case MS_NURSE:
    case MS_SEDUCE: /* nymph, succubus/incubus */
    case MS_LEADER: /* quest leader */
    case MS_GUARDIAN: /* leader's guards */
    case MS_SELL: /* shopkeeper */
    case MS_ORACLE:
    case MS_PRIEST: /* temple priest, roaming aligned priest (not mplayer) */
    case MS_BOAST: /* giants */
    case MS_IMITATE: /* doppelganger, leocrotta, Aleax */
        dogasp = TRUE;
        break;
    /* issue comprehensible word(s) if hero is similar type of creature */
    case MS_ORC: /* used to be synonym for MS_GRUNT */
    case MS_GRUNT: /* ogres, trolls, gargoyles, one or two others */
    case MS_LAUGH: /* leprechaun, gremlin */
    case MS_ROAR: /* dragon, xorn, owlbear */
    /* capable of speech but only do so if hero is similar type */
    case MS_DJINNI:
    case MS_VAMPIRE: /* vampire in its own form */
    case MS_WERE: /* lycanthrope in human form */
    case MS_SPELL: /* titan, barrow wight, Nazgul, nalfeshnee */
        dogasp = (mptr->mlet == gy.youmonst.data->mlet);
        break;
    /* capable of speech but don't care if you attack peacefuls */
    case MS_BRIBE:
    case MS_CUSS:
    case MS_RIDER:
    case MS_NEMESIS:
    /* can't speak */
    case MS_SILENT:
    default:
        break;
    }
    if (dogasp) {
        return Exclam[rn2(SIZE(Exclam))]; /* [mon->m_id % SIZE(Exclam)]; */
    }
    return (const char *) 0;
}

/* return True if mon is a gecko or seems to look like one (hallucination) */
static boolean
mon_is_gecko(struct monst* mon)
{
    int glyph;

    /* return True if it is actually a gecko */
    if (mon->data == &mons[PM_GECKO])
        return TRUE;
    /* return False if it is a long worm; we might be chatting to its tail
       (not strictly needed; long worms are MS_SILENT so won't get here) */
    if (mon->data == &mons[PM_LONG_WORM])
        return FALSE;
    /* result depends upon whether map spot shows a gecko, which will
       be due to hallucination or to mimickery since mon isn't one */
    glyph = glyph_at(mon->mx, mon->my);
    return (boolean) (glyph_to_mon(glyph) == PM_GECKO);
}

DISABLE_WARNING_FORMAT_NONLITERAL

static int /* check calls to this */
domonnoise(register struct monst* mtmp)
{
    char verbuf[BUFSZ];
    register const char *pline_msg = 0, /* Monnam(mtmp) will be prepended */
        *verbl_msg = 0,                 /* verbalize() */
        *verbl_msg_mcan = 0;            /* verbalize() if cancelled */
    struct permonst *ptr = mtmp->data;
    int msound = ptr->msound, gnomeplan = 0;

    /* presumably nearness and sleep checks have already been made */
    if (Deaf)
        return ECMD_OK;
    if (is_silent(ptr)
        /* shk_chat can handle nonverbal monsters */
        && !mtmp->isshk)
        return ECMD_OK;

    /* leader might be poly'd; if he can still speak, give leader speech */
    if (mtmp->m_id == gq.quest_status.leader_m_id && msound > MS_ANIMAL)
        msound = MS_LEADER;
    /* make sure it's your role's quest guardian; adjust if not */
    else if (msound == MS_GUARDIAN && ptr != &mons[gu.urole.guardnum])
        msound = mons[genus(monsndx(ptr), 1)].msound;
    /* even polymorphed, shopkeepers retain their minds and capitalist bent */
    else if (mtmp->isshk)
        msound = MS_SELL;
    /* some normally non-speaking types can/will speak if hero is similar */
    else if (msound == MS_ORC
             && ((same_race(ptr, gy.youmonst.data)        /* current form, */
                  || same_race(ptr, &mons[Race_switch])) /* unpoly'd form */
                 || Hallucination))
        msound = MS_HUMANOID;
    /* silliness; formerly had a slight chance to interfere with shopping */
    else if (Hallucination && mon_is_gecko(mtmp))
        msound = MS_SELL;

    /* be sure to do this before talking; the monster might teleport away, in
     * which case we want to check its pre-teleport position
     */
    if (!canspotmon(mtmp))
        map_invisible(mtmp->mx, mtmp->my);

    switch (msound) {
    case MS_ORACLE:
        return doconsult(mtmp); /* check this */
    case MS_PRIEST:
        priest_talk(mtmp);
        break;
    case MS_LEADER:
    case MS_NEMESIS:
    case MS_GUARDIAN:
        quest_chat(mtmp);
        break;
    case MS_SELL: /* pitch, pay, total */
        if (!Hallucination || is_silent(ptr) || (mtmp->isshk && !rn2(2))) {
            shk_chat(mtmp);
        } else {
            /* approximation of GEICO's advertising slogan (it actually
               concludes with "save you 15% or more on car insurance.") */
            Sprintf(verbuf, "15 minutes could save you 15 %s.",
                    currency(15L)); /* "zorkmids" */
            verbl_msg = verbuf;
        }
        break;
    case MS_VAMPIRE: {
        /* vampire messages are varied by tameness, peacefulness, and time of
         * night */
        boolean isnight = night();
        boolean kindred = (Upolyd && (u.umonnum == PM_VAMPIRE
                                      || u.umonnum == PM_VAMPIRE_LEADER));
        boolean nightchild =
            (Upolyd && (u.umonnum == PM_WOLF || u.umonnum == PM_WINTER_WOLF
                        || u.umonnum == PM_WINTER_WOLF_CUB));
        const char *racenoun =
            (flags.female && gu.urace.individual.f)
                ? gu.urace.individual.f
                : (gu.urace.individual.m) ? gu.urace.individual.m : gu.urace.noun;

        if (mtmp->mtame) {
            if (kindred) {
                Sprintf(verbuf, "Good %s to you Master%s",
                        isnight ? "evening" : "day",
                        isnight ? "!" : ".  Why do we not rest?");
                verbl_msg = verbuf;
            } else {
                Sprintf(verbuf, "%s%s",
                        nightchild ? "Child of the night, " : "",
                        midnight()
                         ? "I can stand this craving no longer!"
                         : isnight
                          ? "I beg you, help me satisfy this growing craving!"
                          : "I find myself growing a little weary.");
                verbl_msg = verbuf;
            }
        } else if (mtmp->mpeaceful) {
            if (kindred && isnight) {
                Sprintf(verbuf, "Good feeding %s!",
                        flags.female ? "sister" : "brother");
                verbl_msg = verbuf;
            } else if (nightchild && isnight) {
                Sprintf(verbuf, "How nice to hear you, child of the night!");
                verbl_msg = verbuf;
            } else
                verbl_msg = "I only drink... potions.";
        } else {
            static const char *const vampmsg[] = {
                /* These first two (0 and 1) are specially handled below */
                "I vant to suck your %s!",
                "I vill come after %s without regret!",
                /* other famous vampire quotes can follow here if desired */
            };
            int vampindex;

            if (kindred)
                verbl_msg =
                    "This is my hunting ground that you dare to prowl!";
            else if (gy.youmonst.data == &mons[PM_SILVER_DRAGON]
                     || gy.youmonst.data == &mons[PM_BABY_SILVER_DRAGON]) {
                /* Silver dragons are silver in color, not made of silver */
                Sprintf(verbuf, "%s!  Your silver sheen does not frighten me!",
                        gy.youmonst.data == &mons[PM_SILVER_DRAGON]
                            ? "Fool"
                            : "Young Fool");
                verbl_msg = verbuf;
            } else {
                vampindex = rn2(SIZE(vampmsg));
                if (vampindex == 0) {
                    Sprintf(verbuf, vampmsg[vampindex], body_part(BLOOD));
                    verbl_msg = verbuf;
                } else if (vampindex == 1) {
                    Sprintf(verbuf, vampmsg[vampindex],
                            Upolyd ? an(pmname(&mons[u.umonnum],
                                               flags.female ? FEMALE : MALE))
                                   : an(racenoun));
                    verbl_msg = verbuf;
                } else
                    verbl_msg = vampmsg[vampindex];
            }
        }
        break;
    }
    case MS_WERE:
        if (flags.moonphase == FULL_MOON && (night() ^ !rn2(13))) {
            pline("%s throws back %s head and lets out a blood curdling %s!",
                  Monnam(mtmp), mhis(mtmp),
                  (ptr == &mons[PM_HUMAN_WERERAT]) ? "shriek" : "howl");
            Soundeffect(((ptr == &mons[PM_HUMAN_WERERAT]) ? se_scream : se_canine_howl), 80);
            wake_nearto(mtmp->mx, mtmp->my, 11 * 11);
        } else
            pline_msg =
                "whispers inaudibly.  All you can make out is \"moon\".";
        break;
    case MS_BARK:
        if (flags.moonphase == FULL_MOON && night()) {
            pline_msg = "howls.";
        } else if (mtmp->mpeaceful) {
            if (mtmp->mtame
                && (mtmp->mconf || mtmp->mflee || mtmp->mtrapped
                    || gm.moves > EDOG(mtmp)->hungrytime || mtmp->mtame < 5))
                pline_msg = "whines.";
            else if (mtmp->mtame && EDOG(mtmp)->hungrytime > gm.moves + 1000)
                pline_msg = "yips.";
            else {
                if (mtmp->data
                    != &mons[PM_DINGO]) /* dingos do not actually bark */
                    pline_msg = "barks.";
            }
        } else {
            pline_msg = "growls.";
        }
        break;
    case MS_MEW:
        if (mtmp->mtame) {
            if (mtmp->mconf || mtmp->mflee || mtmp->mtrapped
                || mtmp->mtame < 5) {
                Soundeffect(se_feline_yowl, 80);
                pline_msg = "yowls.";
            } else if (gm.moves > EDOG(mtmp)->hungrytime) {
                Soundeffect(se_feline_meow, 80);
                pline_msg = "meows.";
            } else if (EDOG(mtmp)->hungrytime > gm.moves + 1000) {
                Soundeffect(se_feline_purr, 40);
                pline_msg = "purrs.";
            } else {
                Soundeffect(se_feline_mew, 60);
                pline_msg = "mews.";
	    }
            break;
        }
        /*FALLTHRU*/
    case MS_GROWL:
        Soundeffect((mtmp->mpeaceful ? se_snarl : se_growl), 80);
        pline_msg = mtmp->mpeaceful ? "snarls." : "growls!";
        break;
    case MS_ROAR:
        Soundeffect((mtmp->mpeaceful ? se_snarl : se_roar), 80);
        pline_msg = mtmp->mpeaceful ? "snarls." : "roars!";
        break;
    case MS_SQEEK:
        Soundeffect(se_squeak, 80);
        pline_msg = "squeaks.";
        break;
    case MS_SQAWK:
        if (ptr == &mons[PM_RAVEN] && !mtmp->mpeaceful) {
            verbl_msg = "Nevermore!";
        } else {
            Soundeffect(se_squawk, 80);
            pline_msg = "squawks.";
        }
        break;
    case MS_HISS:
        if (!mtmp->mpeaceful) {
            Soundeffect(se_hiss, 80);
            pline_msg = "hisses!";
        } else {
            return ECMD_OK; /* no sound */
        }
        break;
    case MS_BUZZ:
        Soundeffect((mtmp->mpeaceful ? se_buzz : se_angry_drone), 80);
        pline_msg = mtmp->mpeaceful ? "drones." : "buzzes angrily.";
        break;
    case MS_GRUNT:
        Soundeffect(se_grunt, 60);
        pline_msg = "grunts.";
        break;
    case MS_NEIGH:
        if (mtmp->mtame < 5) {
            Soundeffect(se_equine_neigh, 60);
            pline_msg = "neighs.";
        } else if (gm.moves > EDOG(mtmp)->hungrytime) {
            Soundeffect(se_equine_whinny, 60);
            pline_msg = "whinnies.";
        } else {
            Soundeffect(se_equine_whicker, 60);
            pline_msg = "whickers.";
        }
        break;
    case MS_MOO:
        Soundeffect((mtmp->mpeaceful ? se_bovine_moo : se_bovine_bellow), 80);
        pline_msg = mtmp->mpeaceful ? "moos." : "bellows!";
        break;
    case MS_WAIL:
        Soundeffect(se_sad_wailing, 60);
        pline_msg = "wails mournfully.";
        break;
    case MS_GROAN:
        if (!rn2(3)) {
            Soundeffect(se_groan, 60);
            pline_msg = "groans.";
	}
        break;
    case MS_GURGLE:
        Soundeffect(se_gurgle, 60);
        pline_msg = "gurgles.";
        break;
    case MS_BURBLE:
        Soundeffect(se_jabberwock_burble, 60);
        pline_msg = "burbles.";
        break;
    case MS_TRUMPET:
        Soundeffect(se_elephant_trumpet, 60);
        pline_msg = "trumpets!";
        wake_nearto(mtmp->mx, mtmp->my, 11 * 11);
        break;
    case MS_SHRIEK:
        Soundeffect(se_shriek, 60);
        pline_msg = "shrieks.";
        aggravate();
        break;
    case MS_IMITATE:
        pline_msg = "imitates you.";
        break;
    case MS_BONES:
        Soundeffect(se_bone_rattle, 60);
        pline("%s rattles noisily.", Monnam(mtmp));
        You("freeze for a moment.");
        nomul(-2);
        gm.multi_reason = "scared by rattling";
        gn.nomovemsg = 0;
        break;
    case MS_LAUGH: {
        static const char *const laugh_msg[4] = {
            "giggles.", "chuckles.", "snickers.", "laughs.",
        };
        Soundeffect(se_laughter, 60);
        pline_msg = laugh_msg[rn2(4)];
        break;
    }
    case MS_MUMBLE:
        pline_msg = "mumbles incomprehensibly.";
        break;
    case MS_ORC: /* this used to be an alias for grunt, now it is distinct */
        Soundeffect(se_orc_grunt, 60);
        pline_msg = "grunts.";
        break;
    case MS_DJINNI:
        if (mtmp->mtame) {
            verbl_msg = "Sorry, I'm all out of wishes.";
        } else if (mtmp->mpeaceful) {
            if (ptr == &mons[PM_WATER_DEMON])
                pline_msg = "gurgles.";
            else
                verbl_msg = "I'm free!";
        } else {
            if (ptr != &mons[PM_PRISONER])
                verbl_msg = "This will teach you not to disturb me!";
            else /* vague because prisoner might already be out of cell */
                verbl_msg = "Get me out of here.";
        }
        break;
    case MS_BOAST: /* giants */
        if (!mtmp->mpeaceful) {
            switch (rn2(4)) {
            case 0:
                pline("%s boasts about %s gem collection.", Monnam(mtmp),
                      mhis(mtmp));
                break;
            case 1:
                pline_msg = "complains about a diet of mutton.";
                break;
            default:
                pline_msg = "shouts \"Fee Fie Foe Foo!\" and guffaws.";
                wake_nearto(mtmp->mx, mtmp->my, 7 * 7);
                break;
            }
            break;
        }
        /*FALLTHRU*/
    case MS_HUMANOID:
        if (!mtmp->mpeaceful) {
            if (In_endgame(&u.uz) && is_mplayer(ptr))
                mplayer_talk(mtmp);
            else
                pline_msg = "threatens you.";
            break;
        }
        /* Generic peaceful humanoid behavior. */
        if (mtmp->mflee)
            pline_msg = "wants nothing to do with you.";
        else if (mtmp->mhp < mtmp->mhpmax / 4)
            pline_msg = "moans.";
        else if (mtmp->mconf || mtmp->mstun)
            verbl_msg = !rn2(3) ? "Huh?" : rn2(2) ? "What?" : "Eh?";
        else if (!mtmp->mcansee)
            verbl_msg = "I can't see!";
        else if (mtmp->mtrapped) {
            struct trap *t = t_at(mtmp->mx, mtmp->my);

            if (t)
                t->tseen = 1;
            verbl_msg = "I'm trapped!";
        } else if (mtmp->mhp < mtmp->mhpmax / 2)
            pline_msg = "asks for a potion of healing.";
        else if (mtmp->mtame && !mtmp->isminion
                 && gm.moves > EDOG(mtmp)->hungrytime)
            verbl_msg = "I'm hungry.";
        /* Specific monsters' interests */
        else if (is_elf(ptr))
            pline_msg = "curses orcs.";
        else if (is_dwarf(ptr))
            pline_msg = "talks about mining.";
        else if (likes_magic(ptr))
            pline_msg = "talks about spellcraft.";
        else if (ptr->mlet == S_CENTAUR)
            pline_msg = "discusses hunting.";
        else if (is_gnome(ptr)) {
            if (Hallucination && (gnomeplan = rn2(4)) % 2) {
                /* skipped for rn2(4) result of 0 or 2;
                   gag from an early episode of South Park called "Gnomes";
                   initially, Tweek (introduced in that episode) is the only
                   one aware of the tiny gnomes after spotting them sneaking
                   about; they are embarked upon a three-step business plan;
                   a diagram of the plan shows:
                               Phase 1         Phase 2      Phase 3
                         Collect underpants       ?          Profit
                   and they never verbalize step 2 so we don't either */
                verbl_msg = (gnomeplan == 1) ? "Phase one, collect underpants."
                                             : "Phase three, profit!";
            }
            else {
                verbl_msg =
                "Many enter the dungeon, and few return to the sunlit lands.";
            }
        }
        else
            switch (monsndx(ptr)) {
            case PM_HOBBIT:
                /* 3.7: the 'complains' message used to be given if the
                   hobbit's current hit points were at 10 below max or
                   less, but their max is normally less than 10 so it
                   would almost never occur */
                pline_msg = (mtmp->mhp < mtmp->mhpmax
                             && (mtmp->mhpmax <= 10
                                 || mtmp->mhp <= mtmp->mhpmax - 10))
                            ? "complains about unpleasant dungeon conditions."
                            : "asks you about the One Ring.";
                break;
            case PM_ARCHEOLOGIST:
                pline_msg =
                "describes a recent article in \"Spelunker Today\" magazine.";
                break;
            case PM_TOURIST:
                verbl_msg = "Aloha.";
                break;
            default:
                pline_msg = "discusses dungeon exploration.";
                break;
            }
        break;
    case MS_SEDUCE: {
        int swval;

        if (SYSOPT_SEDUCE) {
            if (ptr->mlet != S_NYMPH
                && could_seduce(mtmp, &gy.youmonst, (struct attack *) 0) == 1) {
                (void) doseduce(mtmp);
                break;
            }
            swval = ((poly_gender() != (int) mtmp->female) ? rn2(3) : 0);
        } else
            swval = ((poly_gender() == 0) ? rn2(3) : 0);
        switch (swval) {
        case 2:
            verbl_msg = "Hello, sailor.";
            break;
        case 1:
            pline_msg = "comes on to you.";
            break;
        default:
            pline_msg = "cajoles you.";
        }
    } break;
    case MS_ARREST:
        if (mtmp->mpeaceful)
            verbalize("Just the facts, %s.", flags.female ? "Ma'am" : "Sir");
        else {
            static const char *const arrest_msg[3] = {
                "Anything you say can be used against you.",
                "You're under arrest!", "Stop in the name of the Law!",
            };
            verbl_msg = arrest_msg[rn2(3)];
        }
        break;
    case MS_BRIBE:
        if (mtmp->mpeaceful && !mtmp->mtame) {
            (void) demon_talk(mtmp);
            break;
        }
    /* fall through */
    case MS_CUSS:
        if (!mtmp->mpeaceful)
            cuss(mtmp);
        else if (is_lminion(mtmp))
            verbl_msg = "It's not too late.";
        else
            verbl_msg = "We're all doomed.";
        break;
    case MS_SPELL:
        /* deliberately vague, since it's not actually casting any spell */
        pline_msg = "seems to mutter a cantrip.";
        break;
    case MS_NURSE:
        verbl_msg_mcan = "I hate this job!";
        if (uwep && (uwep->oclass == WEAPON_CLASS || is_weptool(uwep)))
            verbl_msg = "Put that weapon away before you hurt someone!";
        else if (uarmc || uarm || uarmh || uarms || uarmg || uarmf)
            verbl_msg = Role_if(PM_HEALER)
                            ? "Doc, I can't help you unless you cooperate."
                            : "Please undress so I can examine you.";
        else if (uarmu)
            verbl_msg = "Take off your shirt, please.";
        else
            verbl_msg = "Relax, this won't hurt a bit.";
        break;
    case MS_GUARD:
        if (money_cnt(gi.invent))
            verbl_msg = "Please drop that gold and follow me.";
        else
            verbl_msg = "Please follow me.";
        break;
    case MS_SOLDIER: {
        static const char
            *const soldier_foe_msg[3] = {
                "Resistance is useless!", "You're dog meat!", "Surrender!",
            },
            *const soldier_pax_msg[3] = {
                "What lousy pay we're getting here!",
                "The food's not fit for Orcs!",
                "My feet hurt, I've been on them all day!",
            };
        verbl_msg = mtmp->mpeaceful ? soldier_pax_msg[rn2(3)]
                                    : soldier_foe_msg[rn2(3)];
        break;
    }
    case MS_RIDER: {
        const char *tribtitle;
        struct obj *book = 0;
        boolean ms_Death = (ptr == &mons[PM_DEATH]);

        /* 3.6 tribute */
        if (ms_Death && !gc.context.tribute.Deathnotice
            && (book = u_have_novel()) != 0) {
            if ((tribtitle = noveltitle(&book->novelidx)) != 0) {
                Sprintf(verbuf, "Ah, so you have a copy of /%s/.", tribtitle);
                /* no Death featured in these two, so exclude them */
                if (strcmpi(tribtitle, "Snuff")
                    && strcmpi(tribtitle, "The Wee Free Men"))
                    Strcat(verbuf, "  I may have been misquoted there.");
                verbl_msg = verbuf;
            }
            gc.context.tribute.Deathnotice = 1;
        } else if (ms_Death && rn2(3) && Death_quote(verbuf, sizeof verbuf)) {
            verbl_msg = verbuf;
        /* end of tribute addition */

        } else if (ms_Death && !rn2(10)) {
            pline_msg = "is busy reading a copy of Sandman #8.";
        } else
            verbl_msg = "Who do you think you are, War?";
        break;
    } /* case MS_RIDER */
    } /* switch */

    if (pline_msg) {
        pline("%s %s", Monnam(mtmp), pline_msg);
    } else if (mtmp->mcan && verbl_msg_mcan) {
        verbalize1(verbl_msg_mcan);
    } else if (verbl_msg) {
        /* more 3.6 tribute */
        if (ptr == &mons[PM_DEATH]) {
            /* Death talks in CAPITAL LETTERS
               and without quotation marks */
            char tmpbuf[BUFSZ];

            pline1(ucase(strcpy(tmpbuf, verbl_msg)));
        } else {
            verbalize1(verbl_msg);
        }
    }
    return ECMD_TIME;
}

RESTORE_WARNING_FORMAT_NONLITERAL

/* #chat command */
int
dotalk(void)
{
    int result;

    result = dochat();
    return result;
}

static int
dochat(void)
{
    struct monst *mtmp;
    int tx, ty;
    struct obj *otmp;

    if (is_silent(gy.youmonst.data)) {
        pline("As %s, you cannot speak.",
              an(pmname(gy.youmonst.data, flags.female ? FEMALE : MALE)));
        return ECMD_OK;
    }
    if (Strangled) {
        You_cant("speak.  You're choking!");
        return ECMD_OK;
    }
    if (u.uswallow) {
        pline("They won't hear you out there.");
        return ECMD_OK;
    }
    if (Underwater) {
        Your("speech is unintelligible underwater.");
        return ECMD_OK;
    }
    if (!Deaf && !Blind && (otmp = shop_object(u.ux, u.uy)) != 0) {
        /* standing on something in a shop and chatting causes the shopkeeper
           to describe the price(s).  This can inhibit other chatting inside
           a shop, but that shouldn't matter much.  shop_object() returns an
           object iff inside a shop and the shopkeeper is present and willing
           (not angry) and able (not asleep) to speak and the position
           contains any objects other than just gold.
        */
        price_quote(otmp);
        return ECMD_TIME;
    }

    if (!getdir("Talk to whom? (in what direction)")) {
        /* decided not to chat */
        return ECMD_CANCEL;
    }

    if (u.usteed && u.dz > 0) {
        if (helpless(u.usteed)) {
            pline("%s seems not to notice you.", Monnam(u.usteed));
            return ECMD_TIME;
        } else
            return domonnoise(u.usteed);
    }

    if (u.dz) {
        pline("They won't hear you %s there.", u.dz < 0 ? "up" : "down");
        return ECMD_OK;
    }

    if (u.dx == 0 && u.dy == 0) {
        /*
         * Let's not include this.
         * It raises all sorts of questions: can you wear
         * 2 helmets, 2 amulets, 3 pairs of gloves or 6 rings as a marilith,
         * etc...  --KAA
        if (u.umonnum == PM_ETTIN) {
            You("discover that your other head makes boring conversation.");
            return 1;
        }
         */
        pline("Talking to yourself is a bad habit for a dungeoneer.");
        return ECMD_OK;
    }

    tx = u.ux + u.dx;
    ty = u.uy + u.dy;

    if (!isok(tx, ty))
        return ECMD_OK;

    mtmp = m_at(tx, ty);

    if (!mtmp || mtmp->mundetected) {
        if ((otmp = vobj_at(tx, ty)) != 0 && otmp->otyp == STATUE) {
            /* Talking to a statue */
            if (!Blind)
                pline_The("%s seems not to notice you.",
                          /* if hallucinating, you can't tell it's a statue */
                          Hallucination ? rndmonnam((char *) 0) : "statue");
            return ECMD_OK;
        }
        if (!Deaf && (IS_WALL(levl[tx][ty].typ) || levl[tx][ty].typ == SDOOR)) {
            /* Talking to a wall; secret door remains hidden by behaving
               like a wall; IS_WALL() test excludes solid rock even when
               that serves as a wall bordering a corridor */
            if (Blind && !IS_WALL(gl.lastseentyp[tx][ty])) {
                /* when blind, you can only talk to a wall if it has
                   already been mapped as a wall */
                ;
            } else if (!Hallucination) {
                pline("It's like talking to a wall.");
            } else {
                static const char *const walltalk[] = {
                    "gripes about its job.",
                    "tells you a funny joke!",
                    "insults your heritage!",
                    "chuckles.",
                    "guffaws merrily!",
                    "deprecates your exploration efforts.",
                    "suggests a stint of rehab...",
                    "doesn't seem to be interested.",
                };
                int idx = rn2(10);

                if (idx >= SIZE(walltalk))
                    idx = SIZE(walltalk) - 1;
                pline_The("wall %s", walltalk[idx]);
            }
            return ECMD_OK;
        }
    }

    if (!mtmp || mtmp->mundetected
        || M_AP_TYPE(mtmp) == M_AP_FURNITURE
        || M_AP_TYPE(mtmp) == M_AP_OBJECT)
        return ECMD_OK;

    /* sleeping monsters won't talk, except priests (who wake up) */
    if (helpless(mtmp) && !mtmp->ispriest) {
        /* If it is unseen, the player can't tell the difference between
           not noticing him and just not existing, so skip the message. */
        if (canspotmon(mtmp))
            pline("%s seems not to notice you.", Monnam(mtmp));
        return ECMD_OK;
    }

    /* if this monster is waiting for something, prod it into action */
    mtmp->mstrategy &= ~STRAT_WAITMASK;

    if (!Deaf && mtmp->mtame && mtmp->meating) {
        if (!canspotmon(mtmp))
            map_invisible(mtmp->mx, mtmp->my);
        pline("%s is eating noisily.", Monnam(mtmp));
        return ECMD_OK;
    }
    if (Deaf) {
        const char *xresponse = humanoid(gy.youmonst.data)
                    ? "falls on deaf ears"
                    : "is inaudible";

        pline("Any response%s%s %s.",
              canspotmon(mtmp) ? " from " : "",
              canspotmon(mtmp) ? mon_nam(mtmp) : "",
              xresponse);
        return ECMD_OK;
    }
    return domonnoise(mtmp);
}

/* is there a monster at <x,y> that can see the hero and react? */
static struct monst *
responsive_mon_at(int x, int y)
{
    struct monst *mtmp = isok(x, y) ? m_at(x, y) : 0;

    if (mtmp && (helpless(mtmp) /* immobilized monst */
                 || !mtmp->mcansee || !haseyes(mtmp->data) /* blind monst */
                 || (Invis && !perceives(mtmp->data)) /* unseen hero */
                 || (x != mtmp->mx || y != mtmp->my))) /* worm tail */
        mtmp = (struct monst *) 0;
    return mtmp;
}

/* player chose 'uarmh' for #tip (pickup.c); visual #chat, sort of... */
int
tiphat(void)
{
    struct monst *mtmp;
    struct obj *otmp;
    int x, y, range, glyph, vismon, unseen, statue, res;

    if (!uarmh) /* can't get here from there */
        return 0;

    res = uarmh->bknown ? 0 : 1;
    if (cursed(uarmh)) /* "You can't.  It is cursed." */
        return res; /* if learned of curse, use a move */

    /* might choose a position, but dealing with direct lines is simpler */
    if (!getdir("At whom? (in what direction)")) /* bail on ESC */
        return res; /* iffy; now know it's not cursed for sure (since we got
                     * past prior test) but might have already known that */
    res = 1; /* physical action is going to take place */

    /* most helmets have a short wear/take-off delay and we could set
       'multi' to account for that, but we'll pretend that no extra time
       beyond the current move is necessary */
    You("briefly doff your %s.", helm_simple_name(uarmh));

    if (!u.dx && !u.dy) {
        if (u.usteed && u.dz > 0) {
            if (helpless(u.usteed))
                pline("%s doesn't notice.", Monnam(u.usteed));
            else
                (void) domonnoise(u.usteed);
        } else if (u.dz) {
            pline("There's no one %s there.", (u.dz < 0) ? "up" : "down");
        } else {
            pline_The("lout here doesn't acknowledge you...");
        }
        return res;
    }

    mtmp = (struct monst *) 0;
    vismon = unseen = statue = 0, glyph = GLYPH_MON_OFF;
    x = u.ux, y = u.uy;
    for (range = 1; range <= BOLT_LIM + 1; ++range) {
        x += u.dx, y += u.dy;
        if (!isok(x, y) || (range > 1 && !couldsee(x, y))) {
            /* switch back to coordinates for previous interation's 'mtmp' */
            x -= u.dx, y -= u.dy;
            break;
        }
        mtmp = m_at(x, y);
        vismon = (mtmp && canseemon(mtmp));
        glyph = glyph_at(x, y);
        unseen = glyph_is_invisible(glyph);
        statue = (glyph_is_statue(glyph) /* mimic or hallucinatory statue */
                  || (!vismon && !unseen && (otmp = vobj_at(x, y)) != 0
                      && otmp->otyp == STATUE)); /* or actual statue */
        if (vismon && (M_AP_TYPE(mtmp) == M_AP_FURNITURE
                       || M_AP_TYPE(mtmp) == M_AP_OBJECT))
            vismon = 0, mtmp = (struct monst *) 0;
        if (vismon || unseen || (statue && Hallucination)
            /* unseen adjacent monster will respond if able */
            || (range == 1 && mtmp && responsive_mon_at(x, y)
                && !is_silent(mtmp->data))
            /* we check accessible() after m_at() in case there's a
               visible monster phazing through a wall here */
            || !(accessible(x, y) || levl[x][y].typ == IRONBARS))
            break;
    }

    if (unseen || (statue && Hallucination)) {
        pline("That %screature is ignoring you!", unseen ? "unseen " : "");
    } else if (!mtmp || !responsive_mon_at(x, y)) {
        if (vismon) /* 'vismon' is only True when 'mtmp' is non-Null */
            pline("%s seems not to notice you.", Monnam(mtmp));
        else
            goto nada;
    } else { /* 'mtmp' is guaranteed to be non-Null if we get here */
        /* if this monster is waiting for something, prod it into action */
        mtmp->mstrategy &= ~STRAT_WAITMASK;

        if (vismon && humanoid(mtmp->data) && mtmp->mpeaceful && !Conflict) {
            if ((otmp = which_armor(mtmp, W_ARMH)) == 0) {
                pline("%s waves.", Monnam(mtmp));
            } else if (otmp->cursed) {
                pline("%s grasps %s %s but can't remove it.", Monnam(mtmp),
                      mhis(mtmp), helm_simple_name(otmp));
                otmp->bknown = 1;
            } else {
                pline("%s tips %s %s in response.", Monnam(mtmp),
                      mhis(mtmp), helm_simple_name(otmp));
            }
        } else if (vismon && humanoid(mtmp->data)) {
            static const char *const reaction[3] = {
                "curses", "gestures rudely", "gestures offensively",
            };
            int which = !Deaf ? rn2(3) : rn1(2, 1),
                twice = (Deaf || which > 0 || rn2(3)) ? 0 : rn1(2, 1);

            pline("%s %s%s%s at you...", Monnam(mtmp), reaction[which],
                  twice ? " and " : "", twice ? reaction[twice] : "");
        } else if (next2u(x, y) && !Deaf && domonnoise(mtmp)) {
            if (!vismon)
                map_invisible(x, y);
        } else if (vismon) {
            pline("%s doesn't respond.", Monnam(mtmp));
        } else {
 nada:
            pline("%s", nothing_happens);
        }
    }
    return res;
}

#ifdef USER_SOUNDS

typedef struct audio_mapping_rec {
    struct nhregex *regex;
    char *filename;
    int volume;
    int idx;
    struct audio_mapping_rec *next;
} audio_mapping;

static audio_mapping *soundmap = 0;
static audio_mapping *sound_matches_message(const char *);

char *sounddir = 0; /* set in files.c */

/* adds a sound file mapping, returns 0 on failure, 1 on success */
int
add_sound_mapping(const char* mapping)
{
    char text[256];
    char filename[256];
    char filespec[256];
    char msgtyp[11];
    int volume, idx = -1;

    msgtyp[0] = '\0';
    if (sscanf(mapping, "MESG \"%255[^\"]\"%*[\t ]\"%255[^\"]\" %d %d",
               text, filename, &volume, &idx) == 4
        || sscanf(mapping, "MESG %10[^\"] \"%255[^\"]\"%*[\t ]\"%255[^\"]\" %d %d",
                  msgtyp, text, filename, &volume, &idx) == 5
        || sscanf(mapping, "MESG %10[^\"] \"%255[^\"]\"%*[\t ]\"%255[^\"]\" %d",
                  msgtyp, text, filename, &volume) == 4
        || sscanf(mapping, "MESG \"%255[^\"]\"%*[\t ]\"%255[^\"]\" %d",
                  text, filename, &volume) == 3) {
        audio_mapping *new_map;

        if (!sounddir)
            sounddir = dupstr(".");
        if (strlen(sounddir) + 1 + strlen(filename) >= sizeof filespec) {
            raw_print("sound file name too long");
            return 0;
        }
        Snprintf(filespec, sizeof filespec, "%s/%s", sounddir, filename);

        if (idx >= 0 || can_read_file(filespec)) {
            new_map = (audio_mapping *) alloc(sizeof *new_map);
            new_map->regex = regex_init();
            new_map->filename = dupstr(filespec);
            new_map->volume = volume;
            new_map->idx = idx;
            new_map->next = soundmap;

            if (!regex_compile(text, new_map->regex)) {
                char errbuf[BUFSZ];
                char *re_error_desc = regex_error_desc(new_map->regex, errbuf);

                regex_free(new_map->regex);
                free((genericptr_t) new_map->filename);
                free((genericptr_t) new_map);
                raw_print(re_error_desc);
                return 0;
            } else {
                if (*msgtyp) {
                    char tmpbuf[BUFSZ];

                    Sprintf(tmpbuf, "%.10s \"%.230s\"", msgtyp, text);
                    (void) msgtype_parse_add(tmpbuf);
                }
                soundmap = new_map;
            }
        } else {
            Sprintf(text, "cannot read %.243s", filespec);
            raw_print(text);
            return 0;
        }
    } else {
        raw_print("syntax error in SOUND");
        return 0;
    }

    return 1;
}

static audio_mapping *
sound_matches_message(const char* msg)
{
    audio_mapping *snd = soundmap;

    while (snd) {
        if (regex_match(msg, snd->regex))
            return snd;
        snd = snd->next;
    }
    return (audio_mapping *) 0;
}

void
play_sound_for_message(const char* msg)
{
    audio_mapping *snd;

    /* we do this check here first, in order to completely
     * avoid doing the regex search when there won't be a
     * sound anyway, despite a match.
     */
    if (soundprocs.sound_play_usersound) {
        snd = sound_matches_message(msg);
        if (snd) {
            Play_usersound(snd->filename, snd->volume, snd->idx);
        }
    }
}

void
maybe_play_sound(const char* msg)
{
    audio_mapping *snd;

    /* we do this check here first, in order to completely
     * avoid doing the regex search when there won't be a
     * sound anyway, despite a match.
     */
    if (soundprocs.sound_play_usersound) {
        snd = sound_matches_message(msg);
        if (snd) {
            Play_usersound(snd->filename, snd->volume, snd->idx);
        }
    }
}

void
release_sound_mappings(void)
{
    audio_mapping *nextsound = 0;

    while (soundmap) {
        nextsound = soundmap->next;
        regex_free(soundmap->regex);
        free((genericptr_t) soundmap->filename);
        free((genericptr_t) soundmap);
        soundmap = nextsound;
    }

    if (sounddir)
        free((genericptr_t) sounddir), sounddir = 0;
}
#endif /* USER_SOUNDS */

struct sound_procs soundprocs;

#ifdef SND_LIB_PORTAUDIO
extern struct sound_procs portaudio_procs;
#endif
#ifdef SND_LIB_OPENAL
extern struct sound_procs openal_procs;
#endif
#ifdef SND_LIB_SDL_MIXER
extern struct sound_procs sdl_mixer_procs;
#endif
#ifdef SND_LIB_MINIAUDIO
extern struct sound_procs miniaudio_procs;
#endif
#ifdef SND_LIB_FMOD
extern struct sound_procs fmod_procs;
#endif
#ifdef SND_LIB_SOUND_ESCCODES
extern struct sound_procs esccodes_procs;
#endif
#ifdef SND_LIB_VISSOUND
extern struct sound_procs vissound_procs;
#endif
#ifdef SND_LIB_WINDSOUND
extern struct sound_procs windsound_procs;
#endif
#ifdef SND_LIB_MACSOUND
extern struct sound_procs macsound_procs;
#endif
#ifdef SND_LIB_QTSOUND
extern struct sound_procs qtsound_procs;
#endif

struct sound_procs nosound_procs = {
    SOUNDID(nosound),
    0L,
    (void (*)(void)) 0,                     /* init_nhsound    */
    (void (*)(const char *)) 0,             /* exit_nhsound    */
    (void (*)(schar, schar, int32_t)) 0,    /* achievement     */
    (void (*)(char *, int32_t, int32_t)) 0, /* sound effect    */
    (void (*)(int32_t, const char *, int32_t)) 0, /* hero_playnotes  */
    (void (*)(char *, int32_t, int32_t)) 0, /* play_usersound  */
};

/* The order of these array entries must match the
   order of the enum soundlib_ids in sndprocs.h */

static struct sound_choices {
    struct sound_procs *sndprocs;
} soundlib_choices[] = {
    { &nosound_procs },     /* default, built-in */
#ifdef SND_LIB_PORTAUDIO
    { &portaudio_procs },
#endif
#ifdef SND_LIB_OPENAL
    { &openal_procs },
#endif
#ifdef SND_LIB_SDL_MIXER
    { &sdl_mixer_procs },
#endif
#ifdef SND_LIB_MINIAUDIO
    { &miniaudio_procs },
#endif
#ifdef SND_LIB_FMOD
    { &fmod_procs },
#endif
#ifdef SND_LIB_SOUND_ESCCODES
    { &esccodes_procs },
#endif
#ifdef SND_LIB_VISSOUND
    { &vissound_procs },
#endif
#ifdef SND_LIB_WINDSOUND
    { &windsound_procs },
#endif
#ifdef SND_LIB_MACSOUND
    { &macsound_procs },
#endif
#ifdef SND_LIB_QTSOUND
    { &qtsound_procs },
#endif
};

void
activate_chosen_soundlib(void)
{
    int idx = gc.chosen_soundlib;

    if (idx < soundlib_nosound || idx >= SIZE(soundlib_choices))
        panic("activate_chosen_soundlib: invalid soundlib (%d)", idx);

    if (ga.active_soundlib != soundlib_nosound || idx != soundlib_nosound) {
        if (soundprocs.sound_exit_nhsound)
            (*soundprocs.sound_exit_nhsound)("assigning a new sound library");
    }
    soundprocs = *soundlib_choices[idx].sndprocs;
    if (soundprocs.sound_init_nhsound)
        (*soundprocs.sound_init_nhsound)();
    ga.active_soundlib = soundprocs.soundlib_id;
    gc.chosen_soundlib = ga.active_soundlib;
}

void
assign_soundlib(int idx)
{
    if (idx < soundlib_nosound || idx >= SIZE(soundlib_choices))
        panic("assign_soundlib: invalid soundlib (%d)", idx);

    gc.chosen_soundlib
        = (uint32_t) soundlib_choices[idx].sndprocs->soundlib_id;
}

#if 0
static void
choose_soundlib(const char *s)
{
    int i;
    char *tmps = 0;

    for (i = 1; soundlib_choices[i].sndprocs; i++) {
        if (!strcmpi(s, soundlib_choices[i].sndprocs->soundname)) {
            assign_soundlib(i);
            return;
        }
    }
    assign_soundlib((int) soundlib_nosound);

    /* The code below here mimics that in windows.c error handling
       for choosing Window type */

    /* 50: arbitrary, no real soundlib names are anywhere near that long;
       used to prevent potential raw_printf() overflow if user supplies a
       very long string (on the order of 1200 chars) on the command line
       (config file options can't get that big; they're truncated at 1023) */
#define SOUNDLIB_NAME_MAXLEN 50
    if (strlen(s) >= SOUNDLIB_NAME_MAXLEN) {
        tmps = (char *) alloc(SOUNDLIB_NAME_MAXLEN);
        (void) strncpy(tmps, s, SOUNDLIB_NAME_MAXLEN - 1);
        tmps[SOUNDLIB_NAME_MAXLEN - 1] = '\0';
        s = tmps;
    }
#undef SOUNDLIB_NAME_MAXLEN

    if (!soundlib_choices[1].sndprocs) {
        config_error_add(
                   "Soundlib type %s not recognized.  The only choice is: %s",
                   s, soundlib_choices[0].sndprocs->soundname);
    } else {
        char buf[BUFSZ];
        boolean first = TRUE;

        buf[0] = '\0';
        for (i = 0; soundlib_choices[i].sndprocs; i++) {
            Sprintf(eos(buf), "%s%s",
                    first ? "" : ", ",
                    soundlib_choices[i].sndprocs->soundname);
            first = FALSE;
        }
        config_error_add("Soundlib type %s not recognized.  Choices are:  %s",
                         s, buf);
    }
    if (tmps)
        free((genericptr_t) tmps) /*, tmps = 0*/ ;
}
#endif

/* copy up to maxlen-1 characters; 'dest' must be able to hold maxlen;
   treat comma as alternate end of 'src' */
void
get_soundlib_name(char *dest, int maxlen)
{
    int count, idx;
    const char *src;

    idx = ga.active_soundlib;
    if (idx < soundlib_nosound || idx >= SIZE(soundlib_choices))
        panic("get_soundlib_name: invalid active_soundlib (%d)", idx);

    src = soundlib_choices[idx].sndprocs->soundname;
    for (count = 1; count < maxlen; count++) {
        if (*src == ',' || *src == '\0')
            break; /*exit on \0 terminator*/
        *dest++ = *src++;
    }
    *dest = '\0';
}

/*
 * The default sound interface
 *
 * 3rd party sound_procs should be placed in ../sound/x
 * and build procedures should reference them there.
 */

#if 0
static void nosound_init_nhsound(void);
static void nosound_exit_nhsound(const char *);
static void nosound_suspend_nhsound(const char *);
static void nosound_resume_nhsound(void);
static void nosound_achievement(schar, schar, int32_t);
static void nosound_soundeffect(int32_t, int32_t);
static void nosound_play_usersound(char *, int32_t, int32_t);

static void
nosound_init_nhsound(void)
{
}

static void
nosound_exit_nhsound(const char *reason)
{
}

static void
nosound_achievement(schar ach1, schar ach2, int32_t repeat)
{
}

static void
nosound_soundeffect(int32_t seid, int volume)
{
}

static void
nosound_hero_playnotes(int32_t instr, const char *notes, int32_t vol)
{
}

static void
nosound_play_usersound(char *filename, int volume, int idx)
{
}
#endif

#ifdef SND_SOUNDEFFECTS_AUTOMAP
struct soundeffect_automapping {
    enum sound_effect_entries seid;
    const char *base_filename;
};

static const struct soundeffect_automapping
       se_mappings_init[number_of_se_entries] = {
    { se_zero_invalid,                  "" },
    { se_faint_splashing,               "faint_splashing" },
    { se_crackling_of_hellfire,         "crackling_of_hellfire" },
    { se_heart_beat,                    "heart_beat" },
    { se_typing_noise,                  "typing_noise" },
    { se_hollow_sound,                  "hollow_sound" },
    { se_rustling_paper,                "rustling_paper" },
    { se_crushing_sound,                "crushing_sound" },
    { se_splash,                        "splash" },
    { se_chains_rattling_gears_turning, "chains_rattling_gears_turning" },
    { se_smashing_and_crushing,         "smashing_and_crushing" },
    { se_gears_turning_chains_rattling, "gears_turning_chains_rattling" },
    { se_loud_splash,                   "loud_splash" },
    { se_lound_crash,                   "lound_crash" },
    { se_crashing_rock,                 "crashing_rock" },
    { se_sizzling,                      "sizzling" },
    { se_crashing_boulder,              "crashing_boulder" },
    { se_boulder_drop,                  "boulder_drop" },
    { se_item_tumble_downwards,         "item_tumble_downwards" },
    { se_drain_noises,                  "drain_noises" },
    { se_ring_in_drain,                 "ring_in_drain" },
    { se_groans_and_moans,              "groans_and_moans" },
    { se_scratching,                    "scratching" },
    { se_glass_shattering,              "glass_shattering" },
    { se_egg_cracking,                  "egg_cracking" },
    { se_gushing_sound,                 "gushing_sound" },
    { se_glass_crashing,                "glass_crashing" },
    { se_egg_splatting,                 "egg_splatting" },
    { se_sinister_laughter,             "sinister_laughter" },
    { se_blast,                         "blast" },
    { se_stone_breaking,                "stone_breaking" },
    { se_stone_crumbling,               "stone_crumbling" },
    { se_snakes_hissing,                "snakes_hissing" },
    { se_loud_pop,                      "loud_pop" },
    { se_clanking_pipe,                 "clanking_pipe" },
    { se_sewer_song,                    "sewer_song" },
    { se_monster_behind_boulder,        "monster_behind_boulder" },
    { se_wailing_of_the_banshee,        "wailing_of_the_banshee" },
    { se_swoosh,                        "swoosh" },
    { se_explosion,                     "explosion" },
    { se_crashing_sound,                "crashing_sound" },
    { se_someone_summoning,             "someone_summoning" },
    { se_rushing_wind_noise,            "rushing_wind_noise" },
    { se_splat_from_engulf,             "splat_from_engulf" },
    { se_faint_sloshing,                "faint_sloshing" },
    { se_crunching_sound,               "crunching_sound" },
    { se_slurping_sound,                "slurping_sound" },
    { se_masticating_sound,             "masticating_sound" },
    { se_distant_thunder,               "distant_thunder" },
    { se_applause,                      "applause" },
    { se_shrill_whistle,                "shrill_whistle" },
    { se_someone_yells,                 "someone_yells" },
    { se_door_unlock_and_open,          "door_unlock_and_open" },
    { se_door_open,                     "door_open" },
    { se_door_crash_open,               "door_crash_open" },
    { se_dry_throat_rattle,             "dry_throat_rattle" },
    { se_cough,                         "cough" },
    { se_angry_snakes,                  "angry_snakes" },
    { se_zap_then_explosion,            "zap_then_explosion" },
    { se_zap,                           "zap" },
    { se_horn_being_played,             "horn_being_played" },
    { se_mon_chugging_potion,           "mon_chugging_potion" },
    { se_bugle_playing_reveille,        "bugle_playing_reveille" },
    { se_crash_through_floor,           "crash_through_floor" },
    { se_thump,                         "thump" },
    { se_scream,                        "scream" },
    { se_tumbler_click,                 "tumbler_click" },
    { se_gear_turn,                     "gear_turn" },
    { se_divine_music,                  "divine_music" },
    { se_thunderclap,                   "thunderclap" },
    { se_sad_wailing,                   "sad_wailing" },
    { se_maniacal_laughter,             "maniacal_laughter" },
    { se_rumbling_of_earth,             "rumbling_of_earth" },
    { se_clanging_sound,                "clanging_sound" },
    { se_mutter_imprecations,           "mutter_imprecations" },
    { se_mutter_incantation,            "mutter_incantation" },
    { se_angry_voice,                   "angry_voice" },
    { se_sceptor_pounding,              "sceptor_pounding" },
    { se_courtly_conversation,          "courtly_conversation" },
    { se_low_buzzing,                   "low_buzzing" },
    { se_angry_drone,                   "angry_drone" },
    { se_bees,                          "bees" },
    { se_someone_searching,             "someone_searching" },
    { se_guards_footsteps,              "guards_footsteps" },
    { se_faint_chime,                   "faint_chime" },
    { se_loud_click,                    "loud_click" },
    { se_soft_click,                    "soft_click" },
    { se_squeak,                        "squeak" },
    { se_squeak_C,                      "squeak_C" },
    { se_squeak_D_flat,                 "squeak_D_flat" },
    { se_squeak_D,                      "squeak_D" },
    { se_squeak_E_flat,                 "squeak_E_flat" },
    { se_squeak_E,                      "squeak_E" },
    { se_squeak_F,                      "squeak_F" },
    { se_squeak_F_sharp,                "squeak_F_sharp" },
    { se_squeak_G,                      "squeak_G" },
    { se_squeak_G_sharp,                "squeak_G_sharp" },
    { se_squeak_A,                      "squeak_A" },
    { se_squeak_B_flat,                 "squeak_B_flat" },
    { se_squeak_B,                      "squeak_B" },
    { se_someone_bowling,               "someone_bowling" },
    { se_rumbling,                      "rumbling" },
    { se_loud_crash,                    "loud_crash" },
    { se_deafening_roar_atmospheric,    "deafening_roar_atmospheric" },
    { se_low_hum,                       "low_hum" },
    { se_laughter,                      "laughter" },
    { se_cockatrice_hiss,               "cockatrice_hiss" },
    { se_chant,                         "chant" },
    { se_cracking_sound,                "cracking_sound" },
    { se_ripping_sound,                 "ripping_sound" },
    { se_thud,                          "thud" },
    { se_clank,                         "clank" },
    { se_crumbling_sound,               "crumbling_sound" },
    { se_soft_crackling,                "soft_crackling" },
    { se_crackling,                     "crackling" },
    { se_sharp_crack,                   "sharp_crack" },
    { se_wall_of_force,                 "wall_of_force" },
    { se_alarm,                         "alarm" },
    { se_kick_door_it_shatters,         "kick_door_it_shatters" },
    { se_kick_door_it_crashes_open,     "kick_door_it_crashes_open" },
    { se_bubble_rising,                 "bubble_rising" },
    { se_bolt_of_lightning,             "bolt_of_lightning" },
    { se_board_squeak,                  "board_squeak" },
    { se_board_squeaks_loudly,          "board_squeaks_loudly" },
    { se_boing,                         "boing" },
    { se_crashed_ceiling,               "crashed_ceiling" },
    { se_clash,                         "clash" },
    { se_crash_door,                    "crash_door" },
    { se_crash,                         "crash" },
    { se_crash_throne_destroyed,        "crash_throne_destroyed" },
    { se_crash_something_broke,         "crash_something_broke" },
    { se_kadoom_boulder_falls_in,       "kadoom_boulder_falls_in" },
    { se_klunk_pipe,                    "klunk_pipe" },
    { se_kerplunk_boulder_gone,         "kerplunk_boulder_gone" },
    { se_klunk,                         "klunk" },
    { se_klick,                         "klick" },
    { se_kaboom_door_explodes,          "kaboom_door_explodes" },
    { se_kaboom_boom_boom,              "kaboom_boom_boom" },
    { se_kaablamm_of_mine,              "kaablamm_of_mine" },
    { se_kaboom,                        "kaboom" },
    { se_splat_egg,                     "splat_egg" },
    { se_destroy_web,                   "destroy_web" },
    { se_iron_ball_dragging_you,        "iron_ball_dragging_you" },
    { se_iron_ball_hits_you,            "iron_ball_hits_you" },
    { se_lid_slams_open_falls_shut,     "lid_slams_open_falls_shut" },
    { se_chain_shatters,                "chain_shatters" },
    { se_furious_bubbling,              "furious_bubbling" },
    { se_air_crackles,                  "air_crackles" },
    { se_potion_crash_and_break,        "potion_crash_and_break" },
    { se_hiss,                          "hiss" },
    { se_growl,                         "growl" },
    { se_canine_bark,                   "canine_bark" },
    { se_canine_growl,                  "canine_growl" },
    { se_canine_whine,                  "canine_whine" },
    { se_canine_yip,                    "canine_yip" },
    { se_canine_howl,                   "canine_howl" },
    { se_canine_yowl,                   "canine_yowl" },
    { se_canine_yelp,                   "canine_yelp" },
    { se_feline_meow,                   "feline_meow" },
    { se_feline_purr,                   "feline_purr" },
    { se_feline_yip,                    "feline_yip" },
    { se_feline_mew,                    "feline_mew" },
    { se_feline_yowl,                   "feline_yowl" },
    { se_feline_yelp,                   "feline_yelp" },
    { se_roar,                          "roar" },
    { se_snarl,                         "snarl" },
    { se_buzz,                          "buzz" },
    { se_squeek,                        "squeek" },
    { se_squawk,                        "squawk" },
    { se_squeal,                        "squeal" },
    { se_screech,                       "screech" },
    { se_equine_neigh,                  "equine_neigh" },
    { se_equine_whinny,                 "equine_whinny" },
    { se_equine_whicker,                "equine_whicker" },
    { se_bovine_moo,                    "bovine_moo" },
    { se_bovine_bellow,                 "bovine_bellow" },
    { se_wail,                          "wail" },
    { se_groan,                         "groan" },
    { se_grunt,                         "grunt" },
    { se_gurgle,                        "gurgle" },
    { se_elephant_trumpet,              "elephant_trumpet" },
    { se_snake_rattle,                  "snake_rattle" },
    { se_yelp,                          "yelp" },
    { se_jabberwock_burble,             "jabberwock_burble" },
    { se_shriek,                        "shriek" },
    { se_bone_rattle,                   "bone_rattle" },
    { se_orc_grunt,                     "orc_grunt" },
    { se_avian_screak,                  "avian_screak" },
};

static const char *semap_basenames[SIZE(se_mappings_init)];
static boolean basenames_initialized = FALSE;

static void
initialize_semap_basenames(void)
{
    int i;

    /* to avoid things getting out of sequence; seid an index to the name */
    for (i = 1; i < SIZE(se_mappings_init); ++i) {
        if (se_mappings_init[i].seid > 0
                && se_mappings_init[i].seid < SIZE(semap_basenames))
            semap_basenames[se_mappings_init[i].seid]
                                = se_mappings_init[i].base_filename;
    }
}

char *
get_sound_effect_filename(
    int32_t seidint,
    char *buf,
    size_t bufsz,
    int32_t baseflag) /* non-zero means return without
                       * directory or extension suffix */
{
    static const char prefix[] = "se_", suffix[] = ".wav";
    size_t consumes = 0, baselen = 0;
/*    enum sound_effect_entries seid = (enum sound_effect_entries) seidint; */
    char *ourdir = sounddir;       /* sounddir would get set in files.c */

    if (!buf || (!ourdir && baseflag == 0))
        return (char *) 0;

    if (!basenames_initialized) {
        initialize_semap_basenames();
        basenames_initialized = TRUE;
    }

    if (semap_basenames[seidint])
        baselen = strlen(semap_basenames[seidint]);

    consumes = (sizeof prefix - 1) + baselen;
    if (baseflag == 0)
        consumes += (sizeof suffix - 1) + strlen(ourdir);
    consumes += 1 + 1; /* '\0' and '/' */
    if (baselen <= 0 || consumes > bufsz)
        return (char *) 0;

#if 0
    if (!baseflag) {
        Strcpy(buf, ourdir);
        Strcat(buf, "/");
    }
    Strcat(buf, prefix);
    Strcat(buf, semap_basenames[seidint]);
    if (!baseflag) {
        Strcat(buf, suffix);
    }
#else
    if (!baseflag)
        Snprintf(buf, bufsz , "%s/%s%s%s", ourdir, prefix,
                 semap_basenames[seidint], suffix);
    else
        Snprintf(buf, bufsz , "%s%s", prefix, semap_basenames[seidint]);
#endif
    return buf;
}
#endif  /* SND_SOUNDEFFECTS_AUTOMAP */

/*sounds.c*/
