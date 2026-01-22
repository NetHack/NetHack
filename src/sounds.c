/* NetHack 3.7	sounds.c	$NHDT-Date: 1736530208 2025/01/10 09:30:08 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.165 $ */
/*      Copyright (c) 1989 Janet Walz, Mike Threepoint */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "i18n.h"

staticfn boolean throne_mon_sound(struct monst *);
staticfn boolean beehive_mon_sound(struct monst *);
staticfn boolean morgue_mon_sound(struct monst *);
staticfn boolean zoo_mon_sound(struct monst *);
staticfn boolean temple_priest_sound(struct monst *);
staticfn boolean mon_is_gecko(struct monst *);
staticfn int dochat(void);
staticfn struct monst *responsive_mon_at(int, int);
staticfn int mon_in_room(struct monst *, int);
staticfn boolean oracle_sound(struct monst *);

/* this easily could be a macro, but it might overtax dumb compilers */
staticfn int
mon_in_room(struct monst *mon, int rmtyp)
{
    int rno = levl[mon->mx][mon->my].roomno;
    if (rno >= ROOMOFFSET)
        return svr.rooms[rno - ROOMOFFSET].rtype == rmtyp;
    return FALSE;
}


staticfn boolean
throne_mon_sound(struct monst *mtmp)
{
    if ((mtmp->msleeping || is_lord(mtmp->data)
         || is_prince(mtmp->data)) && !is_animal(mtmp->data)
        && mon_in_room(mtmp, COURT)) {
        static const char *const throne_msg[4] = {
            N_("the tones of courtly conversation."),
            N_("a sceptre pounded in judgment."),
            N_("Someone shouts \"Off with %s head!\""),
            N_("Queen Beruthiel's cats!"),
        };
        int which = rn2(3) + (Hallucination ? 1 : 0);

        if (which != 2) {
            if (which == 0) {
                Soundeffect(se_courtly_conversation, 30);
            } else if (which == 1) {
                Soundeffect(se_sceptor_pounding, 100);
            }
            You_hear1(_(throne_msg[which]));
        } else {
            DISABLE_WARNING_FORMAT_NONLITERAL
            pline(_(throne_msg[2]), uhis());
            RESTORE_WARNING_FORMAT_NONLITERAL
        }
        return TRUE;
    }
    return FALSE;
}


staticfn boolean
beehive_mon_sound(struct monst *mtmp)
{
    if ((mtmp->data->mlet == S_ANT && is_flyer(mtmp->data))
        && mon_in_room(mtmp, BEEHIVE)) {
        int hallu = Hallucination ? 1 : 0;

        switch (rn2(2) + hallu) {
        case 0:
            Soundeffect(se_low_buzzing, 30);
            You_hear(_("a low buzzing."));
            break;
        case 1:
            Soundeffect(se_angry_drone, 100);
            You_hear(_("an angry drone."));
            break;
        case 2:
            Soundeffect(se_bees, 100);
            You_hear(_("bees in your %sbonnet!"),
                     uarmh ? "" : "(nonexistent) ");
            break;
        }
        return TRUE;
    }
    return FALSE;
}

staticfn boolean
morgue_mon_sound(struct monst *mtmp)
{
    if ((is_undead(mtmp->data) || is_vampshifter(mtmp))
        && mon_in_room(mtmp, MORGUE)) {
        int hallu = Hallucination ? 1 : 0;
        const char *hair = body_part(HAIR); /* hair/fur/scales */

        switch (rn2(2) + hallu) {
        case 0:
            You(_("suddenly realize it is unnaturally quiet."));
            break;
        case 1:
            pline_The(_("%s on the back of your %s %s up."), hair,
                      body_part(NECK), vtense(hair, _("stand")));
            break;
        case 2:
            pline_The(_("%s on your %s %s to stand up."), hair,
                      body_part(HEAD), vtense(hair, _("seem")));
            break;
        }
        return TRUE;
    }
    return FALSE;
}

staticfn boolean
zoo_mon_sound(struct monst *mtmp)
{
    if ((mtmp->msleeping || is_animal(mtmp->data))
        && mon_in_room(mtmp, ZOO)) {
        int hallu = Hallucination ? 1 : 0, selection = rn2(2) + hallu;
        static const char *const zoo_msg[3] = {
            N_("a sound reminiscent of an elephant stepping on a peanut."),
            N_("a sound reminiscent of a seal barking."), N_("Doctor Dolittle!"),
        };
        You_hear1(_(zoo_msg[selection]));
        return TRUE;
    }
    return FALSE;
}

staticfn boolean
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
            N_("*someone praising %s."), N_("*someone beseeching %s."),
            N_("#an animal carcass being offered in sacrifice."),
            N_("*a strident plea for donations."),
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
            You_hear(_(msg), halu_gname(EPRI(mtmp)->shralign));
            RESTORE_WARNING_FORMAT_NONLITERAL
        } else
            You_hear1(_(msg));
        return TRUE;
    }
    return FALSE;
}

staticfn boolean
oracle_sound(struct monst *mtmp)
{
    if (mtmp->data != &mons[PM_ORACLE])
        return FALSE;

    /* and don't produce silly effects when she's clearly visible */
    if (Hallucination || !canseemon(mtmp)) {
        int hallu = Hallucination ? 1 : 0;
        static const char *const ora_msg[5] = {
            N_("a strange wind."),     /* Jupiter at Dodona */
            N_("convulsive ravings."), /* Apollo at Delphi */
            N_("snoring snakes."),     /* AEsculapius at Epidaurus */
            N_("someone say \"No more woodchucks!\""),
            N_("a loud ZOT!") /* both rec.humor.oracle */
        };
        You_hear1(_(ora_msg[rn2(3) + hallu * 2]));
    }
    return TRUE;
}

void
dosounds(void)
{
    struct mkroom *sroom;
    int hallu, vx, vy;
    struct monst *mtmp;

    if (Deaf || !flags.acoustics || u.uswallow || Underwater)
        return;

    hallu = Hallucination ? 1 : 0;

    if (svl.level.flags.nfountains && !rn2(400)) {
        static const char *const fountain_msg[4] = {
            N_("bubbling water."), N_("water falling on coins."),
            N_("the splashing of a naiad."), N_("a soda fountain!"),
        };
        You_hear1(_(fountain_msg[rn2(3) + hallu]));
    }
    if (svl.level.flags.nsinks && !rn2(300)) {
        static const char *const sink_msg[3] = {
            N_("a slow drip."), N_("a gurgling noise."), N_("dishes being washed!"),
        };
        You_hear1(_(sink_msg[rn2(2) + hallu]));
    }
    if (svl.level.flags.has_court && !rn2(200)) {
        if (get_iter_mons(throne_mon_sound))
            return;
    }
    if (svl.level.flags.has_swamp && !rn2(200)) {
        static const char *const swamp_msg[3] = {
            N_("hear mosquitoes!"), N_("smell marsh gas!"), /* so it's a smell...*/
            N_("hear Donald Duck!"),
        };
        You1(_(swamp_msg[rn2(2) + hallu]));
        return;
    }
    if (svl.level.flags.has_vault && !rn2(200)) {
        if (!(sroom = search_special(VAULT))) {
            /* strange ... */
            svl.level.flags.has_vault = 0;
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
                                     ? _("someone counting gold coins.")
                                     : _("the quarterback calling the play."));
                    } else {
                        Soundeffect(se_someone_searching, 30);
                        You_hear(_("someone searching."));
                    }
                    break;
                }
            }
            FALLTHROUGH;
                /*FALLTHRU*/
            case 0:
                Soundeffect(se_guards_footsteps, 30);
                You_hear(_("the footsteps of a guard on patrol."));
                break;
            case 2:
                You_hear(_("Ebenezer Scrooge!"));
                break;
            }
        return;
    }
    if (svl.level.flags.has_beehive && !rn2(200)) {
        if (get_iter_mons(beehive_mon_sound))
            return;
    }
    if (svl.level.flags.has_morgue && !rn2(200)) {
        if (get_iter_mons(morgue_mon_sound))
            return;
    }
    if (svl.level.flags.has_barracks && !rn2(200)) {
        static const char *const barracks_msg[4] = {
            N_("blades being honed."), N_("loud snoring."), N_("dice being thrown."),
            N_("General MacArthur!"),
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
                You_hear1(_(barracks_msg[rn2(3) + hallu]));
                return;
            }
        }
    }
    if (svl.level.flags.has_zoo && !rn2(200)) {
        if (get_iter_mons(zoo_mon_sound))
            return;
    }
    if (svl.level.flags.has_shop && !rn2(200)) {
        if (!(sroom = search_special(ANY_SHOP))) {
            /* strange... */
            svl.level.flags.has_shop = 0;
            return;
        }
        if (tended_shop(sroom)
            && !strchr(u.ushops, (int) (ROOM_INDEX(sroom) + ROOMOFFSET))) {
            static const char *const shop_msg[3] = {
                N_("someone cursing shoplifters."),
                N_("the chime of a cash register."), N_("Neiman and Marcus arguing!"),
            };
            You_hear1(_(shop_msg[rn2(2) + hallu]));
            noisy_shop(sroom);
        }
        return;
    }
    if (svl.level.flags.has_temple && !rn2(200)
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
    N_("beep"),   N_("boing"),   N_("sing"),   N_("belche"), N_("creak"),   N_("cough"),
    N_("rattle"), N_("ululate"), N_("pop"),    N_("jingle"), N_("sniffle"), N_("tinkle"),
    N_("eep"),    N_("clatter"), N_("hum"),    N_("sizzle"), N_("twitter"), N_("wheeze"),
    N_("rustle"), N_("honk"),    N_("lisp"),   N_("yodel"),  N_("coo"),     N_("burp"),
    N_("moo"),    N_("boom"),    N_("murmur"), N_("oink"),   N_("quack"),   N_("rumble"),
    N_("twang"),  N_("toot"),    N_("gargle"), N_("hoot"),   N_("warble")
};

const char *
growl_sound(struct monst *mtmp)
{
    const char *ret;

    switch (mtmp->data->msound) {
    case MS_MEW:
    case MS_HISS:
        ret = _("hiss");
        break;
    case MS_BARK:
    case MS_GROWL:
        ret = _("growl");
        break;
    case MS_ROAR:
        ret = _("roar");
        break;
    case MS_BELLOW:
        ret = _("bellow");
        break;
    case MS_BUZZ:
        ret = _("buzz");
        break;
    case MS_SQEEK:
        ret = _("squeal");
        break;
    case MS_SQAWK:
        ret = _("screech");
        break;
    case MS_NEIGH:
        ret = _("neigh");
        break;
    case MS_WAIL:
        ret = _("wail");
        break;
    case MS_GROAN:
        ret = _("groan");
        break;
    case MS_MOO:
        ret = _("low");
        break;
    case MS_SILENT:
        ret = _("commotion");
        break;
    default:
        ret = _("scream");
    }
    return ret;
}

/* the sounds of a seriously abused pet, including player attacking it */
void
growl(struct monst *mtmp)
{
    const char *growl_verb = 0;

    if (helpless(mtmp) || mtmp->data->msound == MS_SILENT)
        return;

    /* presumably nearness and soundok checks have already been made */
    if (Hallucination)
        growl_verb = _(ROLL_FROM(h_sounds));
    else
        growl_verb = growl_sound(mtmp);
    if (growl_verb) {
        if (canseemon(mtmp) || !Deaf) {
            pline(_("%s %s!"), Monnam(mtmp), vtense((char *) 0, growl_verb));
            iflags.last_msg = PLNMSG_GROWL;
            if (svc.context.run)
                nomul(0);
        }
        wake_nearto(mtmp->mx, mtmp->my, mtmp->data->mlevel * 18);
    }
}

/* the sounds of mistreated pets */
void
yelp(struct monst *mtmp)
{
    const char *yelp_verb = 0;
    enum sound_effect_entries se = se_yelp;

    if (helpless(mtmp) || !mtmp->data->msound)
        return;

    /* presumably nearness and soundok checks have already been made */
    if (Hallucination)
        yelp_verb = _(ROLL_FROM(h_sounds));
    else
        switch (mtmp->data->msound) {
        case MS_MEW:
            se = se_feline_yelp;
            yelp_verb = (!Deaf) ? _("yowl") : _("arch");
            break;
        case MS_BARK:
        case MS_GROWL:
            se = se_canine_yelp;
            yelp_verb = (!Deaf) ? _("yelp") : _("recoil");
            break;
        case MS_ROAR:
            yelp_verb = (!Deaf) ? _("snarl") : _("bluff");
            break;
        case MS_SQEEK:
            se = se_squeal;
            yelp_verb = (!Deaf) ? _("squeal") : _("quiver");
            break;
        case MS_SQAWK:
            se = se_avian_screak;
            yelp_verb = (!Deaf) ? _("screak") : _("thrash");
            break;
        case MS_WAIL:
            se = se_wail;
            yelp_verb = (!Deaf) ? _("wail") : _("cringe");
            break;
        }
    if (yelp_verb) {
        Soundeffect(se, 70);  /* Soundeffect() handles Deaf or not Deaf */
        pline(_("%s %s!"), Monnam(mtmp), vtense((char *) 0, yelp_verb));
        if (svc.context.run)
            nomul(0);
        wake_nearto(mtmp->mx, mtmp->my, mtmp->data->mlevel * 12);
    }
#ifndef SND_LIB_INTEGRATED
    nhUse(se);
#endif
}

/* the sounds of distressed pets */
void
whimper(struct monst *mtmp)
{
    const char *whimper_verb = 0;
    enum sound_effect_entries se = se_canine_whine;
    if (helpless(mtmp) || !mtmp->data->msound)
        return;

    /* presumably nearness and soundok checks have already been made */
    if (Hallucination)
        whimper_verb = _(ROLL_FROM(h_sounds));
    else
        switch (mtmp->data->msound) {
        case MS_MEW:
        case MS_GROWL:
            whimper_verb = _("whimper");
            break;
        case MS_BARK:
            whimper_verb = _("whine");
            break;
        case MS_SQEEK:
            se = se_squeal;
            whimper_verb = _("squeal");
            break;
        }
    if (whimper_verb) {
        if (!Hallucination) {
            Soundeffect(se, 50);
        }
        pline(_("%s %s."), Monnam(mtmp), vtense((char *) 0, whimper_verb));
        if (svc.context.run)
            nomul(0);
        wake_nearto(mtmp->mx, mtmp->my, mtmp->data->mlevel * 6);
    }
#ifndef SND_LIB_INTEGRATED
    nhUse(se);
#endif
}

/* pet makes "I'm hungry" noises */
void
beg(struct monst *mtmp)
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
        SetVoice(mtmp, 0, 80, 0);
        verbalize(_("I'm hungry."));
    } else {
        /* this is pretty lame but is better than leaving out the block
           of speech types between animal and humanoid; this covers
           MS_SILENT too (if caller lets that get this far) since it's
           excluded by the first two cases */
        if (canspotmon(mtmp))
            pline(_("%s seems famished."), Monnam(mtmp));
        /* looking famished will be a good trick for a tame skeleton... */
    }
}

/* hero has attacked a peaceful monster within 'mon's view */
const char *
maybe_gasp(struct monst *mon)
{
    static const char *const Exclam[] = {
        N_("Gasp!"), N_("Uh-oh."), N_("Oh my!"), N_("What?"), N_("Why?"),
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
    case MS_BELLOW: /* crocodile */
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
        return _(ROLL_FROM(Exclam)); /* [mon->m_id % SIZE(Exclam)]; */
    }
    return (const char *) 0;
}

/* for egg hatching; caller will apply "ing" suffix
   [the old message when a carried egg hatches was
   "its cries sound like {mommy,daddy}"
   regardless of what type of sound--if any--the creature made] */
const char *
cry_sound(struct monst *mtmp)
{
    const char *ret = 0;
    struct permonst *ptr = mtmp->data;

    /* a relatively small subset of MS_ sound values are used by oviparous
       species so we don't try to supply something for every MS_ type */
    switch (ptr->msound) {
    default:
    case MS_SILENT: /* insects, arthropods, worms, sea creatures */
        /* "chitter": have silent critters make some noise
           or the mommy/daddy gag when hatching doesn't work */
        ret = (ptr->mlet == S_EEL) ? _("gurgle") : _("chitter");
        break;
    case MS_HISS: /* chickatrice, pyrolisk, snakes */
        ret = _("hiss");
        break;
    case MS_ROAR: /* baby dragons; have them growl instead of roar */
    case MS_GROWL: /* (none) */
        ret = _("growl");
        break;
    case MS_CHIRP: /* adult crocodiles bellow, babies chirp */
        ret = _("chirp");
        break;
    case MS_BUZZ: /* killer bees */
        ret = _("buzz");
        break;
    case MS_SQAWK: /* ravens */
        ret = _("screech");
        break;
    case MS_GRUNT: /* gargoyles */
        ret = _("grunt");
        break;
    case MS_MUMBLE: /* naga hatchlingss */
        ret = _("mumble");
        break;
    }
    return ret;
}

/* return True if mon is a gecko or seems to look like one (hallucination) */
staticfn boolean
mon_is_gecko(struct monst *mon)
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

int /* check calls to this */
domonnoise(struct monst *mtmp)
{
    char verbuf[BUFSZ];
    const char *pline_msg = 0, /* Monnam(mtmp) will be prepended */
        *verbl_msg = 0,                 /* verbalize() */
        *verbl_msg_mcan = 0;            /* verbalize() if cancelled */
    struct permonst *ptr = mtmp->data;
    int msound = ptr->msound, gnomeplan = 0;

    /* presumably nearness and sleep checks have already been made */
    if (Deaf)
        return ECMD_OK;
    /* shk_chat can handle nonverbal monsters */
    if (is_silent(ptr) && !mtmp->isshk)
        return ECMD_OK;

    /* leader might be poly'd; if he can still speak, give leader speech */
    if (mtmp->m_id == svq.quest_status.leader_m_id && msound > MS_ANIMAL)
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
    else if (msound == MS_MOO && !mtmp->mtame)
        msound = MS_BELLOW;
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
           night */
        boolean isnight = night();
        boolean kindred = (Upolyd && (u.umonnum == PM_VAMPIRE
                                      || u.umonnum == PM_VAMPIRE_LEADER));
        boolean nightchild = (Upolyd && (u.umonnum == PM_WOLF
                                         || u.umonnum == PM_WINTER_WOLF
                                         || u.umonnum == PM_WINTER_WOLF_CUB));
        const char *racenoun = (flags.female && gu.urace.individual.f)
                               ? gu.urace.individual.f
                               : (gu.urace.individual.m)
                                 ? gu.urace.individual.m
                                 : gu.urace.noun;

        if (mtmp->mtame) {
            if (kindred) {
                Sprintf(verbuf, _("Good %s to you Master%s"),
                        isnight ? _("evening") : _("day"),
                        isnight ? _("!") : _(".  Why do we not rest?"));
                verbl_msg = verbuf;
            } else {
                Sprintf(verbuf, "%s%s",
                        nightchild ? _("Child of the night, ") : "",
                        midnight()
                         ? _("I can stand this craving no longer!")
                         : isnight
                          ? _("I beg you, help me satisfy this growing craving!")
                          : _("I find myself growing a little weary."));
                verbl_msg = verbuf;
            }
        } else if (mtmp->mpeaceful) {
            if (kindred && isnight) {
                Sprintf(verbuf, _("Good feeding %s!"),
                        flags.female ? _("sister") : _("brother"));
                verbl_msg = verbuf;
            } else if (nightchild && isnight) {
                Sprintf(verbuf, _("How nice to hear you, child of the night!"));
                verbl_msg = verbuf;
            } else
                verbl_msg = _("I only drink... potions.");
        } else {
            int vampindex;

            if (kindred) {
                verbl_msg = _("This is my hunting ground"
                            " that you dare to prowl!");
            } else if (gy.youmonst.data == &mons[PM_SILVER_DRAGON]
                       || gy.youmonst.data == &mons[PM_BABY_SILVER_DRAGON]) {
                /* Silver dragons are silver in color, not made of silver */
                Sprintf(verbuf,
                        _("%s!  Your silver sheen does not frighten me!"),
                        (gy.youmonst.data == &mons[PM_SILVER_DRAGON])
                            ? _("Fool")
                            : _("Young Fool"));
                verbl_msg = verbuf;
            } else {
                vampindex = rn2(2);
                if (vampindex == 0) {
                    Sprintf(verbuf, _("I vant to suck your %s!"), body_part(BLOOD));
                    verbl_msg = verbuf;
                } else {
                    Sprintf(verbuf, _("I vill come after %s without regret!"),
                            Upolyd ? an(pmname(&mons[u.umonnum],
                                               flags.female ? FEMALE : MALE))
                                   : an(racenoun));
                    verbl_msg = verbuf;
                }
            }
        }
        break;
    }
    case MS_WERE:
        if (flags.moonphase == FULL_MOON && (night() ^ !rn2(13))) {
            pline(_("%s throws back %s head and lets out a blood curdling %s!"),
                  Monnam(mtmp), mhis(mtmp),
                  (ptr == &mons[PM_HUMAN_WERERAT]) ? _("shriek") : _("howl"));
            Soundeffect((ptr == &mons[PM_HUMAN_WERERAT]) ? se_scream
                                                         : se_canine_howl,
                        80);
            wake_nearto(mtmp->mx, mtmp->my, 11 * 11);
        } else {
            pline_msg =
                _("whispers inaudibly.  All you can make out is \"moon\".");
        }
        break;
    case MS_BARK:
        if (flags.moonphase == FULL_MOON && night()) {
            pline_msg = _("howls.");
        } else if (mtmp->mpeaceful) {
            if (mtmp->mtame
                && (mtmp->mconf || mtmp->mflee || mtmp->mtrapped
                    || svm.moves > EDOG(mtmp)->hungrytime || mtmp->mtame < 5))
                pline_msg = _("whines.");
            else if (mtmp->mtame && EDOG(mtmp)->hungrytime > svm.moves + 1000)
                pline_msg = _("yips.");
            else {
                if (ptr != &mons[PM_DINGO]) /* dingos do not actually bark */
                    pline_msg = _("barks.");
            }
        } else {
            pline_msg = _("growls.");
        }
        break;
    case MS_MEW:
        if (mtmp->mtame) {
            if (mtmp->mconf || mtmp->mflee || mtmp->mtrapped
                || mtmp->mtame < 5) {
                Soundeffect(se_feline_yowl, 80);
                pline_msg = _("yowls.");
            } else if (svm.moves > EDOG(mtmp)->hungrytime) {
                Soundeffect(se_feline_meow, 80);
                pline_msg = _("meows.");
            } else if (EDOG(mtmp)->hungrytime > svm.moves + 1000) {
                Soundeffect(se_feline_purr, 40);
                pline_msg = _("purrs.");
            } else {
                Soundeffect(se_feline_mew, 60);
                pline_msg = _("mews.");
            }
            break;
        }
        FALLTHROUGH;
        /*FALLTHRU*/
    case MS_GROWL:
        Soundeffect((mtmp->mpeaceful ? se_snarl : se_growl), 80);
        pline_msg = mtmp->mpeaceful ? _("snarls.") : _("growls!");
        break;
    case MS_ROAR:
        Soundeffect((mtmp->mpeaceful ? se_snarl : se_roar), 80);
        pline_msg = mtmp->mpeaceful ? _("snarls.") : _("roars!");
        break;
    case MS_SQEEK:
        Soundeffect(se_squeak, 80);
        pline_msg = _("squeaks.");
        break;
    case MS_SQAWK:
        if (ptr == &mons[PM_RAVEN] && !mtmp->mpeaceful) {
            verbl_msg = _("Nevermore!");
        } else {
            Soundeffect(se_squawk, 80);
            pline_msg = _("squawks.");
        }
        break;
    case MS_HISS:
        if (!mtmp->mpeaceful) {
            Soundeffect(se_hiss, 80);
            pline_msg = _("hisses!");
        } else {
            return ECMD_OK; /* no sound */
        }
        break;
    case MS_BUZZ:
        Soundeffect((mtmp->mpeaceful ? se_buzz : se_angry_drone), 80);
        pline_msg = mtmp->mpeaceful ? _("drones.") : _("buzzes angrily.");
        break;
    case MS_GRUNT:
        Soundeffect(se_grunt, 60);
        pline_msg = _("grunts.");
        break;
    case MS_NEIGH:
        if (mtmp->mtame < 5) {
            Soundeffect(se_equine_neigh, 60);
            pline_msg = _("neighs.");
        } else if (svm.moves > EDOG(mtmp)->hungrytime) {
            Soundeffect(se_equine_whinny, 60);
            pline_msg = _("whinnies.");
        } else {
            Soundeffect(se_equine_whicker, 60);
            pline_msg = _("whickers.");
        }
        break;
    case MS_MOO:
        Soundeffect(se_bovine_moo, 80);
        pline_msg = _("moos.");
        break;
    case MS_BELLOW:
        Soundeffect((ptr->mlet == S_QUADRUPED) ? se_bovine_bellow
                                               : se_croc_bellow,
                    80);
        pline_msg = _("bellows!");
        break;
    case MS_CHIRP:
        Soundeffect(se_chirp, 60);
        pline_msg = _("chirps.");
        break;
    case MS_WAIL:
        Soundeffect(se_sad_wailing, 60);
        pline_msg = _("wails mournfully.");
        break;
    case MS_GROAN:
        if (!rn2(3)) {
            Soundeffect(se_groan, 60);
            pline_msg = _("groans.");
        }
        break;
    case MS_GURGLE:
        Soundeffect(se_gurgle, 60);
        pline_msg = _("gurgles.");
        break;
    case MS_BURBLE:
        Soundeffect(se_jabberwock_burble, 60);
        pline_msg = _("burbles.");
        break;
    case MS_TRUMPET:
        Soundeffect(se_elephant_trumpet, 60);
        pline_msg = _("trumpets!");
        wake_nearto(mtmp->mx, mtmp->my, 11 * 11);
        break;
    case MS_SHRIEK:
        Soundeffect(se_shriek, 60);
        pline_msg = _("shrieks.");
        aggravate();
        break;
    case MS_IMITATE:
        pline_msg = _("imitates you.");
        break;
    case MS_BONES:
        Soundeffect(se_bone_rattle, 60);
        pline(_("%s rattles noisily."), Monnam(mtmp));
        You(_("freeze for a moment."));
        nomul(-2);
        gm.multi_reason = _("scared by rattling");
        gn.nomovemsg = 0;
        break;
    case MS_LAUGH: {
        int laugh_idx = rn2(4);
        Soundeffect(se_laughter, 60);
        switch (laugh_idx) {
        case 0: pline_msg = _("giggles."); break;
        case 1: pline_msg = _("chuckles."); break;
        case 2: pline_msg = _("snickers."); break;
        default: pline_msg = _("laughs."); break;
        }
        break;
    }
    case MS_MUMBLE:
        pline_msg = _("mumbles incomprehensibly.");
        break;
    case MS_ORC: /* this used to be an alias for grunt, now it is distinct */
        Soundeffect(se_orc_grunt, 60);
        pline_msg = _("grunts.");
        break;
    case MS_DJINNI:
        if (mtmp->mtame) {
            verbl_msg = _("Sorry, I'm all out of wishes.");
        } else if (mtmp->mpeaceful) {
            if (ptr == &mons[PM_WATER_DEMON])
                pline_msg = _("gurgles.");
            else
                verbl_msg = _("I'm free!");
        } else {
            if (ptr != &mons[PM_PRISONER])
                verbl_msg = _("This will teach you not to disturb me!");
            else /* vague because prisoner might already be out of cell */
                verbl_msg = _("Get me out of here.");
        }
        break;
    case MS_BOAST: /* giants */
        if (!mtmp->mpeaceful) {
            switch (rn2(4)) {
            case 0:
                pline(_("%s boasts about %s gem collection."), Monnam(mtmp),
                      mhis(mtmp));
                break;
            case 1:
                pline_msg = _("complains about a diet of mutton.");
                break;
            default:
                pline_msg = _("shouts \"Fee Fie Foe Foo!\" and guffaws.");
                wake_nearto(mtmp->mx, mtmp->my, 7 * 7);
                break;
            }
            break;
        }
        FALLTHROUGH;
        /*FALLTHRU*/
    case MS_HUMANOID:
        if (!mtmp->mpeaceful) {
            if (In_endgame(&u.uz) && is_mplayer(ptr))
                mplayer_talk(mtmp);
            else
                pline_msg = _("threatens you.");
            break;
        }
        /* Generic peaceful humanoid behavior. */
        if (mtmp->mflee)
            pline_msg = _("wants nothing to do with you.");
        else if (mtmp->mhp < mtmp->mhpmax / 4)
            pline_msg = _("moans.");
        else if (mtmp->mconf || mtmp->mstun)
            verbl_msg = !rn2(3) ? _("Huh?") : rn2(2) ? _("What?") : _("Eh?");
        else if (!mtmp->mcansee)
            verbl_msg = _("I can't see!");
        else if (mtmp->mtrapped) {
            struct trap *t = t_at(mtmp->mx, mtmp->my);

            if (t)
                t->tseen = 1;
            verbl_msg = _("I'm trapped!");
        } else if (mtmp->mhp < mtmp->mhpmax / 2)
            pline_msg = _("asks for a potion of healing.");
        else if (mtmp->mtame && !mtmp->isminion
                 && svm.moves > EDOG(mtmp)->hungrytime)
            verbl_msg = _("I'm hungry.");
        /* Specific monsters' interests */
        else if (is_elf(ptr))
            pline_msg = _("curses orcs.");
        else if (is_dwarf(ptr))
            pline_msg = _("talks about mining.");
        else if (likes_magic(ptr))
            pline_msg = _("talks about spellcraft.");
        else if (ptr->mlet == S_CENTAUR)
            pline_msg = _("discusses hunting.");
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
                verbl_msg = (gnomeplan == 1)
                            ? _("Phase one, collect underpants.")
                            : _("Phase three, profit!");
            } else {
                verbl_msg = _("Many enter the dungeon,"
                            " and few return to the sunlit lands.");
            }
        } else
            switch (monsndx(ptr)) {
            case PM_HOBBIT:
                /* 3.7: the 'complains' message used to be given if the
                   hobbit's current hit points were at 10 below max or
                   less, but their max is normally less than 10 so it
                   would almost never occur */
                pline_msg = (mtmp->mhp < mtmp->mhpmax
                             && (mtmp->mhpmax <= 10
                                 || mtmp->mhp <= mtmp->mhpmax - 10))
                            ? _("complains about unpleasant dungeon conditions.")
                            : _("asks you about the One Ring.");
                break;
            case PM_ARCHEOLOGIST:
                pline_msg =
                _("describes a recent article in \"Spelunker Today\" magazine.");
                break;
            case PM_TOURIST:
                verbl_msg = _("Aloha.");
                break;
            default:
                pline_msg = _("discusses dungeon exploration.");
                break;
            }
        break;
    case MS_SEDUCE: {
        int swval;

        if (SYSOPT_SEDUCE) {
            if (ptr->mlet != S_NYMPH
                && (could_seduce(mtmp, &gy.youmonst, (struct attack *) 0)
                    == 1)) {
                (void) doseduce(mtmp);
                break;
            }
            swval = ((poly_gender() != (int) mtmp->female) ? rn2(3) : 0);
        } else
            swval = ((poly_gender() == 0) ? rn2(3) : 0);
        switch (swval) {
        case 2:
            verbl_msg = _("Hello, sailor.");
            break;
        case 1:
            pline_msg = _("comes on to you.");
            break;
        default:
            pline_msg = _("cajoles you.");
        }
    } break;
    case MS_ARREST:
        if (mtmp->mpeaceful) {
            SetVoice(mtmp, 0, 80, 0);
            verbalize(_("Just the facts, %s."), flags.female ? _("Ma'am") : _("Sir"));
        } else {
            int arrest_idx = rn2(3);
            switch (arrest_idx) {
            case 0: verbl_msg = _("Anything you say can be used against you."); break;
            case 1: verbl_msg = _("You're under arrest!"); break;
            default: verbl_msg = _("Stop in the name of the Law!"); break;
            }
        }
        break;
    case MS_BRIBE:
        if (mtmp->mpeaceful && !mtmp->mtame) {
            (void) demon_talk(mtmp);
            break;
        }
        FALLTHROUGH;
        /* FALLTHRU */
    case MS_CUSS:
        if (!mtmp->mpeaceful)
            cuss(mtmp);
        else if (is_lminion(mtmp))
            verbl_msg = _("It's not too late.");
        else
            verbl_msg = _("We're all doomed.");
        break;
    case MS_SPELL:
        /* deliberately vague, since it's not actually casting any spell */
        pline_msg = _("seems to mutter a cantrip.");
        break;
    case MS_NURSE:
        verbl_msg_mcan = _("I hate this job!");
        if (uwep && (uwep->oclass == WEAPON_CLASS || is_weptool(uwep)))
            verbl_msg = _("Put that weapon away before you hurt someone!");
        else if (uarmc || uarm || uarmh || uarms || uarmg || uarmf)
            verbl_msg = Role_if(PM_HEALER)
                            ? _("Doc, I can't help you unless you cooperate.")
                            : _("Please undress so I can examine you.");
        else if (uarmu)
            verbl_msg = _("Take off your shirt, please.");
        else
            verbl_msg = _("Relax, this won't hurt a bit.");
        break;
    case MS_GUARD:
        if (money_cnt(gi.invent))
            verbl_msg = _("Please drop that gold and follow me.");
        else
            verbl_msg = _("Please follow me.");
        break;
    case MS_SOLDIER: {
        int soldier_idx = rn2(3);
        if (mtmp->mpeaceful) {
            switch (soldier_idx) {
            case 0: verbl_msg = _("What lousy pay we're getting here!"); break;
            case 1: verbl_msg = _("The food's not fit for Orcs!"); break;
            default: verbl_msg = _("My feet hurt, I've been on them all day!"); break;
            }
        } else {
            switch (soldier_idx) {
            case 0: verbl_msg = _("Resistance is useless!"); break;
            case 1: verbl_msg = _("You're dog meat!"); break;
            default: verbl_msg = _("Surrender!"); break;
            }
        }
        break;
    }
    case MS_RIDER: {
        const char *tribtitle;
        struct obj *book = 0;
        boolean ms_Death = (ptr == &mons[PM_DEATH]);

        /* 3.6 tribute */
        if (ms_Death && !svc.context.tribute.Deathnotice
            && (book = u_have_novel()) != 0) {
            if ((tribtitle = noveltitle(&book->novelidx)) != 0) {
                Sprintf(verbuf, _("Ah, so you have a copy of /%s/."), tribtitle);
                /* no Death featured in these two, so exclude them */
                if (strcmpi(tribtitle, "Snuff")
                    && strcmpi(tribtitle, "The Wee Free Men"))
                    Strcat(verbuf, _("  I may have been misquoted there."));
                verbl_msg = verbuf;
            }
            svc.context.tribute.Deathnotice = 1;
        } else if (ms_Death && rn2(3) && Death_quote(verbuf, sizeof verbuf)) {
            verbl_msg = verbuf;
        /* end of tribute addition */

        } else if (ms_Death && !rn2(10)) {
            pline_msg = _("is busy reading a copy of Sandman #8.");
        } else
            verbl_msg = _("Who do you think you are, War?");
        break;
    } /* case MS_RIDER */
    } /* switch */

    if (pline_msg) {
        pline(_("%s %s"), Monnam(mtmp), pline_msg);
    } else if (mtmp->mcan && verbl_msg_mcan) {
        SetVoice(mtmp, 0, 80, 0);
        verbalize1(verbl_msg_mcan);
    } else if (verbl_msg) {
        /* more 3.6 tribute */
        if (ptr == &mons[PM_DEATH]) {
            /* Death talks in CAPITAL LETTERS
               and without quotation marks */
            char tmpbuf[BUFSZ];
            pline1(ucase(strcpy(tmpbuf, verbl_msg)));
            SetVoice((struct monst *) 0, 0, 80, voice_death);
            sound_speak(tmpbuf);
        } else {
            SetVoice(mtmp, 0, 80, 0);
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

staticfn int
dochat(void)
{
    struct monst *mtmp;
    int tx, ty;
    struct obj *otmp;

    if (is_silent(gy.youmonst.data)) {
        pline(_("As %s, you cannot speak."),
              an(pmname(gy.youmonst.data, flags.female ? FEMALE : MALE)));
        return ECMD_OK;
    }
    if (Strangled) {
        You_cant(_("speak.  You're choking!"));
        return ECMD_OK;
    }
    if (u.uswallow) {
        pline(_("They won't hear you out there."));
        return ECMD_OK;
    }
    if (Underwater) {
        Your(_("speech is unintelligible underwater."));
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
            pline(_("%s seems not to notice you."), Monnam(u.usteed));
            return ECMD_TIME;
        } else
            return domonnoise(u.usteed);
    }

    if (u.dz) {
        pline(_("They won't hear you %s there."), u.dz < 0 ? _("up") : _("down"));
        return ECMD_OK;
    }

    if (u.dx == 0 && u.dy == 0) {
        /*
         * Let's not include this.
         * It raises all sorts of questions: can you wear
         * 2 helmets, 2 amulets, 3 pairs of gloves or 6 rings as a marilith,
         * etc...  --KAA
        if (u.umonnum == PM_ETTIN) {
            You(_("discover that your other head makes boring conversation."));
            return 1;
        }
         */
        pline(_("Talking to yourself is a bad habit for a dungeoneer."));
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
                pline_The(_("%s seems not to notice you."),
                          /* if hallucinating, you can't tell it's a statue */
                          Hallucination ? rndmonnam((char *) 0) : _("statue"));
            return ECMD_OK;
        }
        if (!Deaf && (IS_WALL(levl[tx][ty].typ)
                      || levl[tx][ty].typ == SDOOR)) {
            /* Talking to a wall; secret door remains hidden by behaving
               like a wall; IS_WALL() test excludes solid rock even when
               that serves as a wall bordering a corridor */
            if (Blind && !IS_WALL(svl.lastseentyp[tx][ty])) {
                /* when blind, you can only talk to a wall if it has
                   already been mapped as a wall */
                ;
            } else if (!Hallucination) {
                pline(_("It's like talking to a wall."));
            } else {
                static const char *const walltalk[] = {
                    N_("gripes about its job."),
                    N_("tells you a funny joke!"),
                    N_("insults your heritage!"),
                    N_("chuckles."),
                    N_("guffaws merrily!"),
                    N_("deprecates your exploration efforts."),
                    N_("suggests a stint of rehab..."),
                    N_("doesn't seem to be interested."),
                };
                int idx = rn2(10);

                if (idx >= SIZE(walltalk))
                    idx = SIZE(walltalk) - 1;
                pline_The(_("wall %s"), _(walltalk[idx]));
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
            pline(_("%s seems not to notice you."), Monnam(mtmp));
        return ECMD_OK;
    }

    /* if this monster is waiting for something, prod it into action */
    mtmp->mstrategy &= ~STRAT_WAITMASK;

    if (!Deaf && mtmp->mtame && mtmp->meating) {
        if (!canspotmon(mtmp))
            map_invisible(mtmp->mx, mtmp->my);
        pline(_("%s is eating noisily."), Monnam(mtmp));
        return ECMD_OK;
    }
    if (Deaf) {
        const char *xresponse = humanoid(gy.youmonst.data)
                    ? _("falls on deaf ears")
                    : _("is inaudible");

        pline(_("Any response%s%s %s."),
              canspotmon(mtmp) ? _(" from ") : "",
              canspotmon(mtmp) ? mon_nam(mtmp) : "",
              xresponse);
        return ECMD_OK;
    }
    return domonnoise(mtmp);
}

/* is there a monster at <x,y> that can see the hero and react? */
staticfn struct monst *
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
    You(_("briefly doff your %s."), helm_simple_name(uarmh));

    if (!u.dx && !u.dy) {
        if (u.usteed && u.dz > 0) {
            if (helpless(u.usteed))
                pline(_("%s doesn't notice."), Monnam(u.usteed));
            else
                (void) domonnoise(u.usteed);
        } else if (u.dz) {
            pline(_("There's no one %s there."), (u.dz < 0) ? _("up") : _("down"));
        } else {
            pline_The(_("lout here doesn't acknowledge you..."));
        }
        return res;
    }

    mtmp = (struct monst *) 0;
    vismon = unseen = statue = 0, glyph = GLYPH_MON_OFF;
    x = u.ux, y = u.uy;
    for (range = 1; range <= BOLT_LIM + 1; ++range) {
        x += u.dx, y += u.dy;
        if (!isok(x, y) || (range > 1 && !couldsee(x, y))) {
            /* switch back to coordinates for previous iteration's 'mtmp' */
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
        pline(_("That %screature is ignoring you!"), unseen ? _("unseen ") : "");
    } else if (!mtmp || !responsive_mon_at(x, y)) {
        if (vismon) /* 'vismon' is only True when 'mtmp' is non-Null */
            pline(_("%s seems not to notice you."), Monnam(mtmp));
        else
            goto nada;
    } else { /* 'mtmp' is guaranteed to be non-Null if we get here */
        /* if this monster is waiting for something, prod it into action */
        mtmp->mstrategy &= ~STRAT_WAITMASK;

        if (vismon && humanoid(mtmp->data) && mtmp->mpeaceful && !Conflict) {
            if ((otmp = which_armor(mtmp, W_ARMH)) == 0) {
                pline(_("%s waves."), Monnam(mtmp));
            } else if (otmp->cursed) {
                pline(_("%s grasps %s %s but can't remove it."), Monnam(mtmp),
                      mhis(mtmp), helm_simple_name(otmp));
                otmp->bknown = 1;
            } else {
                pline(_("%s tips %s %s in response."), Monnam(mtmp),
                      mhis(mtmp), helm_simple_name(otmp));
            }
        } else if (vismon && humanoid(mtmp->data)) {
            static const char *const reaction[3] = {
                N_("curses"), N_("gestures rudely"), N_("gestures offensively"),
            };
            int which = !Deaf ? rn2(3) : rn1(2, 1),
                twice = (Deaf || which > 0 || rn2(3)) ? 0 : rn1(2, 1);

            pline(_("%s %s%s%s at you..."), Monnam(mtmp), _(reaction[which]),
                  twice ? _(" and ") : "", twice ? _(reaction[twice]) : "");
        } else if (next2u(x, y) && !Deaf && domonnoise(mtmp)) {
            if (!vismon)
                map_invisible(x, y);
        } else if (vismon) {
            pline(_("%s doesn't respond."), Monnam(mtmp));
        } else {
 nada:
            pline1(nothing_happens);
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
add_sound_mapping(const char *mapping)
{
    char text[256];
    char filename[256];
    char filespec[256];
    char msgtyp[11];
    int volume, idx = -1;

    msgtyp[0] = '\0';
    filename[sizeof filename - 1] = '\0';
    filespec[sizeof filespec - 1] = '\0';
    text[sizeof text - 1] = '\0';
    if (sscanf(mapping, "MESG \"%255[^\"]\"%*[\t ]\"%255[^\"]\" %d %d",
               text, filename, &volume, &idx) == 4
        || sscanf(mapping,
                  "MESG %10[^\"] \"%255[^\"]\"%*[\t ]\"%255[^\"]\" %d %d",
                  msgtyp, text, filename, &volume, &idx) == 5
        || sscanf(mapping,
                  "MESG %10[^\"] \"%255[^\"]\"%*[\t ]\"%255[^\"]\" %d",
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
                char *re_error_desc
                         = regex_error_desc(new_map->regex, errbuf);

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

staticfn audio_mapping *
sound_matches_message(const char *msg)
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
play_sound_for_message(const char *msg)
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
maybe_play_sound(const char *msg)
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

static struct sound_procs nosound_procs = {
    SOUNDID(nosound),
    0L,
    (void (*)(void)) 0,                           /* init_nhsound   */
    (void (*)(const char *)) 0,                   /* exit_nhsound   */
    (void (*)(schar, schar, int32_t)) 0,          /* achievement    */
    (void (*)(char *, int32_t, int32_t)) 0,       /* sound effect   */
    (void (*)(int32_t, const char *, int32_t)) 0, /* hero_playnotes */
    (void (*)(char *, int32_t, int32_t)) 0,       /* play_usersound */
    (void (*)(int32_t, int32_t, int32_t)) 0,      /* ambience       */
    (void (*)(char *, int32_t, int32_t, int32_t, int32_t)) 0, /* verbal */
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

    if (!IndexOk(idx, soundlib_choices))
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
    if (!IndexOk(idx, soundlib_choices))
        panic("assign_soundlib: invalid soundlib (%d)", idx);

    gc.chosen_soundlib
        = (uint32_t) soundlib_choices[idx].sndprocs->soundlib_id;
}

#if 0
staticfn void
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
    if (!IndexOk(idx, soundlib_choices))
        panic("get_soundlib_name: invalid active_soundlib (%d)", idx);

    src = soundlib_choices[idx].sndprocs->soundname;
    for (count = 1; count < maxlen; count++) {
        if (*src == ',' || *src == '\0')
            break; /*exit on \0 terminator*/
        *dest++ = *src++;
    }
    *dest = '\0';
}

enum soundlib_ids
soundlib_id_from_opt(char *op)
{
    int idx;
    struct sound_procs *defproc = &nosound_procs,
                       *sp = 0;

    for (idx = 0; idx < SIZE(soundlib_choices); ++idx) {
        sp = soundlib_choices[idx].sndprocs;
        if (!strcmp(sp->soundname, op))
            return sp->soundlib_id;
    }
    return defproc->soundlib_id;
}

/*
 * The default sound interface
 *
 * 3rd party sound_procs should be placed in ../sound/x
 * and build procedures should reference them there.
 */

#if 0
staticfn void nosound_init_nhsound(void);
staticfn void nosound_exit_nhsound(const char *);
staticfn void nosound_suspend_nhsound(const char *);
staticfn void nosound_resume_nhsound(void);
staticfn void nosound_achievement(schar, schar, int32_t);
staticfn void nosound_soundeffect(int32_t, int32_t);
staticfn void nosound_play_usersound(char *, int32_t, int32_t);
staticfn void nosound_ambience(int32_t, int32_t, int32_t);
staticfn void nosound_verbal(char *text, int32_t gender, int32_t tone,
                           int32_t vol, int32_t moreinfo);

staticfn void
nosound_init_nhsound(void)
{
}

staticfn void
nosound_exit_nhsound(const char *reason)
{
}

staticfn void
nosound_achievement(schar ach1, schar ach2, int32_t repeat)
{
}

staticfn void
nosound_soundeffect(int32_t seid, int volume)
{
}

staticfn void
nosound_hero_playnotes(int32_t instr, const char *notes, int32_t vol)
{
}

staticfn void
nosound_play_usersound(char *filename, int volume, int idx)
{
}

staticfn void
nosound_ambience(int32_t ambienceid, int32_t ambience_action,
                int32_t hero_proximity)
{
}

staticfn void
nosound_verbal(char *text, int32_t gender, int32_t tone,
               int32_t vol, int32_t moreinfo)
{
}
#endif

#ifdef SND_SOUNDEFFECTS_AUTOMAP

/* prototype in case a build defines staticfn to nothing */
staticfn void initialize_semap_basenames(void);

struct soundeffect_automapping {
    enum sound_effect_entries seid;
    const char *base_filename;
};

#define SEFFECTS_AUTOMAP
static const struct soundeffect_automapping
    se_mappings_init[number_of_se_entries] = {
    { se_zero_invalid,                  "" },
#include "seffects.h"
};
#undef SEFFECTS_AUTOMAP

static const char *semap_basenames[SIZE(se_mappings_init)];
static boolean basenames_initialized = FALSE;

staticfn void
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
    int32_t approach)
{
    static const char prefix[] = "se_", suffix[] = ".wav";
    size_t consumes = 0, baselen = 0, existinglen = 0;
/*    enum sound_effect_entries seid = (enum sound_effect_entries) seidint; */
    char *ourdir = sounddir;       /* sounddir would get set in files.c */
    char *cp = buf;
    boolean needslash = TRUE;

    if (!buf || (!ourdir && approach == sff_default))
        return (char *) 0;

    if (!basenames_initialized) {
        initialize_semap_basenames();
        basenames_initialized = TRUE;
    }

    if (semap_basenames[seidint])
        baselen = strlen(semap_basenames[seidint]);

    consumes = (sizeof prefix - 1) + baselen;
    if (approach == sff_default) {
        consumes += (sizeof suffix - 1) + strlen(ourdir) + 1; /* 1 for '/' */
    } else if (approach == sff_havedir_append_rest) {
        /* consumes += (sizeof suffix - 1); */
        existinglen = strlen(buf);
        if (existinglen > 0) {
            cp = buf + existinglen; /* points at trailing NUL */
            cp--;                   /* points at last character */
            if (*cp == '/' || *cp == '\\')
                needslash = FALSE;
            cp++;                   /* points back at trailing NUL */
        }
        if (needslash)
            consumes++;  /* for '/' */
        consumes += existinglen;
        consumes += (sizeof suffix - 1);
    }
    consumes += 1; /* for trailing NUL */
    /* existinglen could be >= bufsz if caller didn't initialize buf
     * to properly include a trailing NUL */
    if (baselen <= 0 || consumes > bufsz || existinglen >= bufsz)
        return (char *) 0;

#if 0
    if (approach == sff_default) {
        Strcpy(buf, ourdir);
        Strcat(buf, "/");
    } else if (approach == sff_havdir_append_rest) {
        if (needslash)
            Strcat(buf, "/");
    } else if (approach == sff_base_only) {
        buf[0] = '\0';
    } else {
        return (char *) 0;
    }
    Strcat(buf, prefix);
    Strcat(buf, semap_basenames[seidint]);
    if (approach != sff_base_only) {
        Strcat(buf, suffix);
    }
#else
    if (approach == sff_default) {
        Snprintf(buf, bufsz , "%s/%s%s%s", ourdir, prefix,
                 semap_basenames[seidint], suffix);
    } else if (approach == sff_havedir_append_rest) {
        if (needslash) {
            *cp = '/';
            cp++;
            *cp = '\0';
            existinglen++;
        }
        Snprintf(cp, bufsz - (existinglen + 1) , "%s%s%s",
                 prefix, semap_basenames[seidint], suffix);
    } else if (approach == sff_base_only) {
        Snprintf(buf, bufsz, "%s%s", prefix, semap_basenames[seidint]);
    } else {
        return (char *) 0;
    }
#endif
    return buf;
}
#endif  /* SND_SOUNDEFFECTS_AUTOMAP */

char *
base_soundname_to_filename(
    char *basename,
    char *buf,
    size_t bufsz,
    int32_t approach)
{
    static const char suffix[] = ".wav";
    size_t consumes = 0, baselen = 0, existinglen = 0;
    char *cp = buf;
    boolean needslash = TRUE;

    if (!buf)
        return (char *) 0;

    baselen = strlen(basename);
    consumes = baselen;

    if (approach == sff_havedir_append_rest) {
        /* consumes += (sizeof suffix - 1); */
        existinglen = strlen(buf);
        if (existinglen > 0) {
            cp = buf + existinglen; /* points at trailing NUL */
            cp--;                   /* points at last character */
            if (*cp == '/' || *cp == '\\')
                needslash = FALSE;
            cp++;                   /* points back at trailing NUL */
        }
        if (needslash)
            consumes++;  /* for '/' */
        consumes += existinglen;
        consumes += (sizeof suffix - 1);
    }
    consumes += 1; /* for trailing NUL */
    /* existinglen could be >= bufsz if caller didn't initialize buf
     * to properly include a trailing NUL */
    if (!baselen || consumes > bufsz || existinglen >= bufsz)
        return (char *) 0;

#if 0
    if (approach == sff_havedir_append_rest) {
        if (needslash)
            Strcat(buf, "/");
    } else if (approach == sff_base_only) {
        buf[0] = '\0';
    } else {
        return (char *) 0;
    }
    Strcat(buf, basename);
    if (approach != sff_base_only) {
        Strcat(buf, suffix);
    }
#else
    if (approach == sff_havedir_append_rest) {
        if (needslash) {
            *cp = '/';
            cp++;
            *cp = '\0';
            existinglen++;
        }
        Snprintf(cp, bufsz - (existinglen + 1) , "%s%s",
                 basename, suffix);
    } else if (approach == sff_base_only) {
        Snprintf(buf, bufsz, "%s", basename);
    } else {
        return (char *) 0;
    }
#endif
    return buf;
}

#ifdef SND_SPEECH
#define SPEECHONLY
#else
#define SPEECHONLY UNUSED
#endif

void
set_voice(
    struct monst *mtmp SPEECHONLY,
    int32_t tone SPEECHONLY,
    int32_t volume SPEECHONLY,
    int32_t moreinfo SPEECHONLY)
{
#ifdef SND_SPEECH
    int32_t gender = (mtmp && mtmp->female) ? FEMALE : MALE;

    if (gv.voice.nameid)
        free((genericptr_t) gv.voice.nameid);
    gv.voice.gender = gender;
    gv.voice.serialno = mtmp ? mtmp->m_id
                             : ((moreinfo & voice_talking_artifact) != 0)  ? 3
                                 : ((moreinfo & voice_deity) != 0) ? 4 : 2;
    gv.voice.tone = tone;
    gv.voice.volume = volume;
    gv.voice.moreinfo = moreinfo;
    gv.voice.nameid = (const char *) 0;
    gp.pline_flags |= PLINE_SPEECH;
#endif
}

void
sound_speak(const char *text SPEECHONLY)
{
#ifdef SND_SPEECH
    const char *cp1, *cp2;
    char *cpdst;
    char buf[BUFSZ + BUFSZ];

    if (!text || (text && *text == '\0'))
        return;
    if (iflags.voices && soundprocs.sound_verbal
        && (soundprocs.sound_triggers & SOUND_TRIGGER_VERBAL)) {
        cp1 = text;
        cpdst = buf;
        cp2 = c_eos(cp1);
        cp2--;  /* last non-NUL */
        *cpdst = '\0';
        if ((gp.pline_flags & PLINE_VERBALIZE) != 0) {
            if (*cp1 == '"')
                cp1++;
            if (*cp2 == '"')
                cp2--;
        }
        /* cp1 -> 1st, cp2 -> last non-nul) */
        if ((unsigned)(cp2 - cp1) < (sizeof buf - 1U)) {
            while (cp1 <= cp2) {
                *cpdst = *cp1;
                cp1++;
                cpdst++;
            }
            *cpdst = '\0';
        }
        (*soundprocs.sound_verbal)(buf, gv.voice.gender, gv.voice.tone,
                                   gv.voice.volume, gv.voice.moreinfo);
    }
#endif
}

/*sounds.c*/
