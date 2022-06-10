/* NetHack 3.7	quest.c	$NHDT-Date: 1596498200 2020/08/03 23:43:20 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.29 $ */
/*      Copyright 1991, M. Stephenson             */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

/*  quest dungeon branch routines. */

#include "quest.h"

#define Not_firsttime (on_level(&u.uz0, &u.uz))
#define Qstat(x) (g.quest_status.x)

static void on_start(void);
static void on_locate(void);
static void on_goal(void);
static boolean not_capable(void);
static int is_pure(boolean);
static void expulsion(boolean);
static void chat_with_leader(struct monst *);
static void chat_with_nemesis(void);
static void chat_with_guardian(void);
static void prisoner_speaks(struct monst *);

static void
on_start(void)
{
    if (!Qstat(first_start)) {
        qt_pager("firsttime");
        Qstat(first_start) = TRUE;
    } else if ((u.uz0.dnum != u.uz.dnum) || (u.uz0.dlevel < u.uz.dlevel)) {
        if (Qstat(not_ready) <= 2)
            qt_pager("nexttime");
        else
            qt_pager("othertime");
    }
}

static void
on_locate(void)
{
    /* the locate messages are phrased in a manner such that they only
       make sense when arriving on the level from above */
    boolean from_above = (u.uz0.dlevel < u.uz.dlevel);

    if (Qstat(killed_nemesis)) {
        return;
    } else if (!Qstat(first_locate)) {
        if (from_above)
            qt_pager("locate_first");
        /* if we've arrived from below this will be a lie, but there won't
           be any point in delivering the message upon a return visit from
           above later since the level has now been seen */
        Qstat(first_locate) = TRUE;
    } else {
        if (from_above)
            qt_pager("locate_next");
    }
}

static void
on_goal(void)
{
    if (Qstat(killed_nemesis)) {
        return;
    } else if (!Qstat(made_goal)) {
        qt_pager("goal_first");
        Qstat(made_goal) = 1;
    } else {
        /*
         * Some QT_NEXTGOAL messages reference the quest artifact;
         * find out if it is still present.  If not, request an
         * alternate message (qt_pager() will revert to delivery
         * of QT_NEXTGOAL if current role doesn't have QT_ALTGOAL).
         * Note: if hero is already carrying it, it is treated as
         * being absent from the level for quest message purposes.
         */
        unsigned whichobjchains = ((1 << OBJ_FLOOR)
                                   | (1 << OBJ_MINVENT)
                                   | (1 << OBJ_BURIED));
        struct obj *qarti = find_quest_artifact(whichobjchains);

        qt_pager(qarti ? "goal_next" : "goal_alt");
        if (Qstat(made_goal) < 7)
            Qstat(made_goal)++;
    }
}

void
onquest(void)
{
    if (u.uevent.qcompleted || Not_firsttime)
        return;
    if (!Is_special(&u.uz))
        return;

    if (Is_qstart(&u.uz))
        on_start();
    else if (Is_qlocate(&u.uz))
        on_locate();
    else if (Is_nemesis(&u.uz))
        on_goal();
    return;
}

void
nemdead(void)
{
    if (!Qstat(killed_nemesis)) {
        Qstat(killed_nemesis) = TRUE;
        qt_pager("killed_nemesis");
    }
}

void
leaddead(void)
{
    if (!Qstat(killed_leader)) {
        Qstat(killed_leader) = TRUE;
        /* TODO: qt_pager("killed_leader"); ? */
    }
}

void
artitouch(struct obj *obj)
{
    if (!Qstat(touched_artifact)) {
        /* in case we haven't seen the item yet (ie, currently blinded),
           this quest message describes it by name so mark it as seen */
        obj->dknown = 1;
        /* only give this message once */
        Qstat(touched_artifact) = TRUE;
        qt_pager("gotit");
        exercise(A_WIS, TRUE);
    }
}

/* external hook for do.c (level change check) */
boolean
ok_to_quest(void)
{
    return (boolean) (((Qstat(got_quest) || Qstat(got_thanks))
                       && is_pure(FALSE) > 0) || Qstat(killed_leader));
}

static boolean
not_capable(void)
{
    return (boolean) (u.ulevel < MIN_QUEST_LEVEL);
}

static int
is_pure(boolean talk)
{
    int purity;
    aligntyp original_alignment = u.ualignbase[A_ORIGINAL];

    if (wizard && talk) {
        if (u.ualign.type != original_alignment) {
            You("are currently %s instead of %s.", align_str(u.ualign.type),
                align_str(original_alignment));
        } else if (u.ualignbase[A_CURRENT] != original_alignment) {
            You("have converted.");
        } else if (u.ualign.record < MIN_QUEST_ALIGN) {
            You("are currently %d and require %d.", u.ualign.record,
                MIN_QUEST_ALIGN);
            if (yn_function("adjust?", (char *) 0, 'y') == 'y')
                u.ualign.record = MIN_QUEST_ALIGN;
        }
    }
    purity = (u.ualign.record >= MIN_QUEST_ALIGN
              && u.ualign.type == original_alignment
              && u.ualignbase[A_CURRENT] == original_alignment)
                 ? 1
                 : (u.ualignbase[A_CURRENT] != original_alignment) ? -1 : 0;
    return purity;
}

/*
 * Expel the player to the stairs on the parent of the quest dungeon.
 *
 * This assumes that the hero is currently _in_ the quest dungeon and that
 * there is a single branch to and from it.
 */
static void
expulsion(boolean seal)
{
    branch *br;
    d_level *dest;
    struct trap *t;
    int portal_flag = u.uevent.qexpelled ? UTOTYPE_NONE : UTOTYPE_PORTAL;

    br = dungeon_branch("The Quest");
    dest = (br->end1.dnum == u.uz.dnum) ? &br->end2 : &br->end1;
    if (seal)
        portal_flag |= UTOTYPE_RMPORTAL;
    nomul(0); /* stop running */
    schedule_goto(dest, portal_flag, (char *) 0, (char *) 0);
    if (seal) { /* remove the portal to the quest - sealing it off */
        int reexpelled = u.uevent.qexpelled;

        u.uevent.qexpelled = 1;
        remdun_mapseen(quest_dnum);
        /* Delete the near portal now; the far (main dungeon side)
           portal will be deleted as part of arrival on that level.
           If monster movement is in progress, any who haven't moved
           yet will now miss out on a chance to wander through it... */
        for (t = g.ftrap; t; t = t->ntrap)
            if (t->ttyp == MAGIC_PORTAL)
                break;
        if (t)
            deltrap(t); /* (display might be briefly out of sync) */
        else if (!reexpelled)
            impossible("quest portal already gone?");
    }
}

/* Either you've returned to quest leader while carrying the quest
   artifact or you've just thrown it to/at him or her.  If quest
   completion text hasn't been given yet, give it now.  Otherwise
   give another message about the character keeping the artifact
   and using the magic portal to return to the dungeon. */
void
finish_quest(struct obj *obj) /* quest artifact; possibly null if carrying
                                 Amulet */
{
    struct obj *otmp;

    if (u.uhave.amulet) { /* unlikely but not impossible */
        qt_pager("hasamulet");
        /* leader IDs the real amulet but ignores any fakes */
        if ((otmp = carrying(AMULET_OF_YENDOR)) != 0)
            fully_identify_obj(otmp);
    } else {
        qt_pager(!Qstat(got_thanks) ? "offeredit" : "offeredit2");
        /* should have obtained bell during quest;
           if not, suggest returning for it now */
        if ((otmp = carrying(BELL_OF_OPENING)) == 0)
            com_pager("quest_complete_no_bell");
    }
    Qstat(got_thanks) = TRUE;

    if (obj) {
        u.uevent.qcompleted = 1; /* you did it! */
        /* behave as if leader imparts sufficient info about the
           quest artifact */
        fully_identify_obj(obj);
        update_inventory();
    }
}

static void
chat_with_leader(struct monst *mtmp)
{
    if (!mtmp->mpeaceful || Qstat(pissed_off))
        return;

    /*  Rule 0: Cheater checks. */
    if (u.uhave.questart && !Qstat(met_nemesis))
        Qstat(cheater) = TRUE;

    /*  It is possible for you to get the amulet without completing
     *  the quest.  If so, try to induce the player to quest.
     */
    if (Qstat(got_thanks)) {
        /* Rule 1: You've gone back with/without the amulet. */
        if (u.uhave.amulet)
            finish_quest((struct obj *) 0);

        /* Rule 2: You've gone back before going for the amulet. */
        else
            qt_pager("posthanks");

    /* Rule 3: You've got the artifact and are back to return it. */
    } else if (u.uhave.questart) {
        struct obj *otmp;

        for (otmp = g.invent; otmp; otmp = otmp->nobj)
            if (is_quest_artifact(otmp))
                break;

        finish_quest(otmp);

    /* Rule 4: You haven't got the artifact yet. */
    } else if (Qstat(got_quest)) {
        qt_pager("encourage");

    /* Rule 5: You aren't yet acceptable - or are you? */
    } else {
        int purity = 0;

        if (!Qstat(met_leader)) {
            qt_pager("leader_first");
            Qstat(met_leader) = TRUE;
            Qstat(not_ready) = 0;
        } else
            qt_pager("leader_next");

        /* the quest leader might have passed through the portal into
           the regular dungeon; none of the remaining make sense there */
        if (!on_level(&u.uz, &qstart_level))
            return;

        if (not_capable()) {
            qt_pager("badlevel");
            exercise(A_WIS, TRUE);
            expulsion(FALSE);
        } else if ((purity = is_pure(TRUE)) < 0) {
            if (!Qstat(pissed_off)) {
                com_pager("banished");
                Qstat(pissed_off) = TRUE;
                expulsion(FALSE);
            }
        } else if (purity == 0) {
            qt_pager("badalign");
            Qstat(not_ready) = 1;
            exercise(A_WIS, TRUE);
            expulsion(FALSE);
        } else { /* You are worthy! */
            qt_pager("assignquest");
            exercise(A_WIS, TRUE);
            Qstat(got_quest) = TRUE;
        }
    }
}

void
leader_speaks(struct monst *mtmp)
{
    /* maybe you attacked leader? */
    if (!mtmp->mpeaceful) {
        if (!Qstat(pissed_off)) {
            /* again, don't end it permanently if the leader gets angry
             * since you're going to have to kill him to go questing... :)
             * ...but do only show this crap once. */
            qt_pager("leader_last");
        }
        Qstat(pissed_off) = TRUE;
        mtmp->mstrategy &= ~STRAT_WAITMASK; /* end the inaction */
    }
    /* the quest leader might have passed through the portal into the
       regular dungeon; if so, mustn't perform "backwards expulsion" */
    if (!on_level(&u.uz, &qstart_level))
        return;

    if (!Qstat(pissed_off))
        chat_with_leader(mtmp);
}

static void
chat_with_nemesis(void)
{
    /*  The nemesis will do most of the talking, but... */
    qt_pager("discourage");
    if (!Qstat(met_nemesis))
        Qstat(met_nemesis++);
}

void
nemesis_speaks(void)
{
    if (!Qstat(in_battle)) {
        if (u.uhave.questart)
            qt_pager("nemesis_wantsit");
        else if (Qstat(made_goal) == 1 || !Qstat(met_nemesis))
            qt_pager("nemesis_first");
        else if (Qstat(made_goal) < 4)
            qt_pager("nemesis_next");
        else if (Qstat(made_goal) < 7)
            qt_pager("nemesis_other");
        else if (!rn2(5))
            qt_pager("discourage");
        if (Qstat(made_goal) < 7)
            Qstat(made_goal)++;
        Qstat(met_nemesis) = TRUE;
    } else /* he will spit out random maledictions */
        if (!rn2(5))
        qt_pager("discourage");
}

static void
chat_with_guardian(void)
{
    /*  These guys/gals really don't have much to say... */
    if (u.uhave.questart && Qstat(killed_nemesis))
        qt_pager("guardtalk_after");
    else
        qt_pager("guardtalk_before");
}

static void
prisoner_speaks(struct monst *mtmp)
{
    if (mtmp->data == &mons[PM_PRISONER]
        && (mtmp->mstrategy & STRAT_WAITMASK)) {
        /* Awaken the prisoner */
        if (canseemon(mtmp))
            pline("%s speaks:", Monnam(mtmp));
        verbalize("I'm finally free!");
        mtmp->mstrategy &= ~STRAT_WAITMASK;
        mtmp->mpeaceful = 1;

        /* Your god is happy... */
        adjalign(3);

        /* ...But the guards are not */
        (void) angry_guards(FALSE);
    }
    return;
}

void
quest_chat(struct monst *mtmp)
{
    if (mtmp->m_id == Qstat(leader_m_id)) {
        chat_with_leader(mtmp);
        /* leader might have become pissed during the chat */
        if (Qstat(pissed_off))
            setmangry(mtmp, FALSE);
        return;
    }
    switch (mtmp->data->msound) {
    case MS_NEMESIS:
        chat_with_nemesis();
        break;
    case MS_GUARDIAN:
        chat_with_guardian();
        break;
    default:
        impossible("quest_chat: Unknown quest character %s.", mon_nam(mtmp));
    }
}

void
quest_talk(struct monst *mtmp)
{
    if (mtmp->m_id == Qstat(leader_m_id)) {
        leader_speaks(mtmp);
        return;
    }
    switch (mtmp->data->msound) {
    case MS_NEMESIS:
        nemesis_speaks();
        break;
    case MS_DJINNI:
        prisoner_speaks(mtmp);
        break;
    default:
        break;
    }
}

void
quest_stat_check(struct monst *mtmp)
{
    if (mtmp->data->msound == MS_NEMESIS)
        Qstat(in_battle) = (!helpless(mtmp)
                            && monnear(mtmp, u.ux, u.uy));
}

/*quest.c*/
