/* NetHack 3.6	winstat.c	$NHDT-Date: 1452920162 2016/01/16 04:56:02 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.16 $ */
/* Copyright (c) Dean Luick, 1992				  */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * Status window routines.  This file supports both the "traditional"
 * tty status display and a "fancy" status display.  A tty status is
 * made if a popup window is requested, otherewise a fancy status is
 * made.  This code assumes that only one fancy status will ever be made.
 * Currently, only one status window (of any type) is _ever_ made.
 */

#ifndef SYSV
#define PRESERVE_NO_SYSV /* X11 include files may define SYSV */
#endif

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xaw/AsciiText.h>
#include <X11/Xaw/Cardinals.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Paned.h>
#include <X11/Xaw/Label.h>
#include <X11/Xatom.h>

#ifdef PRESERVE_NO_SYSV
#ifdef SYSV
#undef SYSV
#endif
#undef PRESERVE_NO_SYSV
#endif

#include "hack.h"
#include "winX.h"

extern const char *hu_stat[];  /* from eat.c */
extern const char *enc_stat[]; /* from botl.c */

static void FDECL(update_fancy_status, (struct xwindow *));
static Widget FDECL(create_fancy_status, (Widget, Widget));
static void FDECL(destroy_fancy_status, (struct xwindow *));

void
create_status_window(wp, create_popup, parent)
struct xwindow *wp; /* window pointer */
boolean create_popup;
Widget parent;
{
    XFontStruct *fs;
    Arg args[8];
    Cardinal num_args;
    Position top_margin, bottom_margin, left_margin, right_margin;

    wp->type = NHW_STATUS;

    if (!create_popup) {
        /*
         * If we are not creating a popup, then we must be the "main" status
         * window.
         */
        if (!parent)
            panic("create_status_window: no parent for fancy status");
        wp->status_information = 0;
        wp->w = create_fancy_status(parent, (Widget) 0);
        return;
    }

    wp->status_information =
        (struct status_info_t *) alloc(sizeof(struct status_info_t));

    init_text_buffer(&wp->status_information->text);

    num_args = 0;
    XtSetArg(args[num_args], XtNallowShellResize, False);
    num_args++;
    XtSetArg(args[num_args], XtNinput, False);
    num_args++;

    wp->popup = parent = XtCreatePopupShell(
        "status_popup", topLevelShellWidgetClass, toplevel, args, num_args);
    /*
     * If we're here, then this is an auxiliary status window.  If we're
     * cancelled via a delete window message, we should just pop down.
     */

    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNdisplayCaret), False);
    num_args++;
    XtSetArg(args[num_args], nhStr(XtNscrollHorizontal),
             XawtextScrollWhenNeeded);
    num_args++;
    XtSetArg(args[num_args], nhStr(XtNscrollVertical),
             XawtextScrollWhenNeeded);
    num_args++;

    wp->w = XtCreateManagedWidget("status", /* name */
                                  asciiTextWidgetClass,
                                  parent,    /* parent widget */
                                  args,      /* set some values */
                                  num_args); /* number of values to set */

    /*
     * Adjust the height and width of the message window so that it
     * is two lines high and COLNO of the widest characters wide.
     */

    /* Get the font and margin information. */
    num_args = 0;
    XtSetArg(args[num_args], XtNfont, &fs);
    num_args++;
    XtSetArg(args[num_args], nhStr(XtNtopMargin), &top_margin);
    num_args++;
    XtSetArg(args[num_args], nhStr(XtNbottomMargin), &bottom_margin);
    num_args++;
    XtSetArg(args[num_args], nhStr(XtNleftMargin), &left_margin);
    num_args++;
    XtSetArg(args[num_args], nhStr(XtNrightMargin), &right_margin);
    num_args++;
    XtGetValues(wp->w, args, num_args);

    wp->pixel_height = 2 * nhFontHeight(wp->w) + top_margin + bottom_margin;
    wp->pixel_width =
        COLNO * fs->max_bounds.width + left_margin + right_margin;

    /* Set the new width and height. */
    num_args = 0;
    XtSetArg(args[num_args], XtNwidth, wp->pixel_width);
    num_args++;
    XtSetArg(args[num_args], XtNheight, wp->pixel_height);
    num_args++;
    XtSetValues(wp->w, args, num_args);
}

void
destroy_status_window(wp)
struct xwindow *wp;
{
    /* If status_information is defined, then it a "text" status window. */
    if (wp->status_information) {
        if (wp->popup) {
            nh_XtPopdown(wp->popup);
            if (!wp->keep_window)
                XtDestroyWidget(wp->popup), wp->popup = (Widget) 0;
        }
        free((genericptr_t) wp->status_information);
        wp->status_information = 0;
    } else {
        destroy_fancy_status(wp);
    }
    if (!wp->keep_window)
        wp->type = NHW_NONE;
}

/*
 * This assumes several things:
 *	+ Status has only 2 lines
 *	+ That both lines are updated in succession in line order.
 *	+ We didn't set stringInPlace on the widget.
 */
void
adjust_status(wp, str)
struct xwindow *wp;
const char *str;
{
    Arg args[2];
    Cardinal num_args;

    if (!wp->status_information) {
        update_fancy_status(wp);
        return;
    }

    if (wp->cursy == 0) {
        clear_text_buffer(&wp->status_information->text);
        append_text_buffer(&wp->status_information->text, str, FALSE);
        return;
    }
    append_text_buffer(&wp->status_information->text, str, FALSE);

    /* Set new buffer as text. */
    num_args = 0;
    XtSetArg(args[num_args], XtNstring, wp->status_information->text.text);
    num_args++;
    XtSetValues(wp->w, args, num_args);
}

/* Fancy Status
 * -------------------------------------------------------------*/
static int hilight_time = 1; /* number of turns to hilight a changed value */

struct X_status_value {
    /* we have to cast away 'const' when assigning new names */
    const char *name;   /* text name */
    int type;           /* status type */
    Widget w;           /* widget of name/value pair */
    long last_value;    /* value displayed */
    int turn_count;     /* last time the value changed */
    boolean set;        /* if highlighted */
    boolean after_init; /* don't highlight on first change (init) */
};

/* valid type values */
#define SV_VALUE 0 /* displays a label:value pair */
#define SV_LABEL 1 /* displays a changable label */
#define SV_NAME 2  /* displays an unchangeable name */

static void FDECL(hilight_label, (Widget));
static void FDECL(update_val, (struct X_status_value *, long));
static const char *FDECL(width_string, (int));
static void FDECL(create_widget, (Widget, struct X_status_value *, int));
static void FDECL(get_widths, (struct X_status_value *, int *, int *));
static void FDECL(set_widths, (struct X_status_value *, int, int));
static Widget FDECL(init_column, (const char *, Widget, Widget, Widget,
                                  int *));
static void NDECL(fixup_cond_widths);
static Widget FDECL(init_info_form, (Widget, Widget, Widget));

/*
 * Form entry storage indices.
 */
#define F_DUMMY     0
#define F_STR       1
#define F_DEX       2
#define F_CON       3
#define F_INT       4
#define F_WIS       5
#define F_CHA       6

#define F_NAME      7
#define F_DLEVEL    8
#define F_GOLD      9
#define F_HP       10
#define F_MAXHP    11
#define F_POWER    12
#define F_MAXPOWER 13
#define F_AC       14
#define F_LEVEL    15
#define F_EXP      16
#define F_ALIGN    17
#define F_TIME     18
#define F_SCORE    19

/* status conditions grouped by columns; tty orders these differently */
#define F_STONE    20
#define F_SLIME    21
#define F_STRNGL   22
#define F_FOODPOIS 23
#define F_TERMILL  24

#define F_HUNGER   25
#define F_ENCUMBER 26
#define F_LEV      27
#define F_FLY      28
#define F_RIDE     29

#define F_BLIND    30
#define F_DEAF     31
#define F_STUN     32
#define F_CONF     33
#define F_HALLU    34

#define NUM_STATS 35

/*
 * Notes:
 * + Alignment needs a different init value, because -1 is an alignment.
 * + Armor Class is an schar, so 256 is out of range.
 * + Blank value is 0 and should never change.
 */
static struct X_status_value shown_stats[NUM_STATS] = {
    { "", SV_NAME, (Widget) 0, -1, 0, FALSE, FALSE }, /* 0*/

    { "Strength", SV_VALUE, (Widget) 0, -1, 0, FALSE, FALSE },
    { "Dexterity", SV_VALUE, (Widget) 0, -1, 0, FALSE, FALSE },
    { "Constitution", SV_VALUE, (Widget) 0, -1, 0, FALSE, FALSE },
    { "Intelligence", SV_VALUE, (Widget) 0, -1, 0, FALSE, FALSE },
    { "Wisdom", SV_VALUE, (Widget) 0, -1, 0, FALSE, FALSE }, /* 5*/
    { "Charisma", SV_VALUE, (Widget) 0, -1, 0, FALSE, FALSE },

    { "", SV_LABEL, (Widget) 0, -1, 0, FALSE, FALSE }, /* name */
    { "", SV_LABEL, (Widget) 0, -1, 0, FALSE, FALSE }, /* dlvl */
    { "Gold", SV_VALUE, (Widget) 0, -1, 0, FALSE, FALSE },
    { "Hit Points", SV_VALUE, (Widget) 0, -1, 0, FALSE, FALSE }, /*10*/
    { "Max HP", SV_VALUE, (Widget) 0, -1, 0, FALSE, FALSE },
    { "Power", SV_VALUE, (Widget) 0, -1, 0, FALSE, FALSE },
    { "Max Power", SV_VALUE, (Widget) 0, -1, 0, FALSE, FALSE },
    { "Armor Class", SV_VALUE, (Widget) 0, 256, 0, FALSE, FALSE },
    { "Level", SV_VALUE, (Widget) 0, -1, 0, FALSE, FALSE }, /*15*/
    { "Experience", SV_VALUE, (Widget) 0, -1, 0, FALSE, FALSE },
    { "Alignment", SV_VALUE, (Widget) 0, -2, 0, FALSE, FALSE },
    { "Time", SV_VALUE, (Widget) 0, -1, 0, FALSE, FALSE },
    { "Score", SV_VALUE, (Widget) 0, -1, 0, FALSE, FALSE },

    { "Petrifying", SV_NAME, (Widget) 0, 0, 0, FALSE, TRUE }, /*20*/
    { "Slimed", SV_NAME, (Widget) 0, 0, 0, FALSE, TRUE },
    { "Strangled", SV_NAME, (Widget) 0, 0, 0, FALSE, TRUE },
    { "Food Pois", SV_NAME, (Widget) 0, 0, 0, FALSE, TRUE },
    { "Term Ill", SV_NAME, (Widget) 0, 0, 0, FALSE, TRUE },

    { "", SV_NAME, (Widget) 0, -1, 0, FALSE, TRUE }, /*25*/     /* hunger */
    { "", SV_NAME, (Widget) 0, 0, 0, FALSE, TRUE },             /*encumbr */
    { "Levitating", SV_NAME, (Widget) 0, 0, 0, FALSE, TRUE },
    { "Flying", SV_NAME, (Widget) 0, 0, 0, FALSE, TRUE },
    { "Riding", SV_NAME, (Widget) 0, 0, 0, FALSE, TRUE },

    { "Blind", SV_NAME, (Widget) 0, 0, 0, FALSE, TRUE }, /*30*/
    { "Deaf", SV_NAME, (Widget) 0, 0, 0, FALSE, TRUE },
    { "Stunned", SV_NAME, (Widget) 0, 0, 0, FALSE, TRUE },
    { "Confused", SV_NAME, (Widget) 0, 0, 0, FALSE, TRUE },
    { "Hallucinating", SV_NAME, (Widget) 0, 0, 0, FALSE, TRUE },
};

/*
 * Set all widget values to a null string.  This is used after all spacings
 * have been calculated so that when the window is popped up we don't get all
 * kinds of funny values being displayed.
 */
void
null_out_status()
{
    int i;
    struct X_status_value *sv;
    Arg args[1];

    for (i = 0, sv = shown_stats; i < NUM_STATS; i++, sv++) {
        switch (sv->type) {
        case SV_VALUE:
            set_value(sv->w, "");
            break;

        case SV_LABEL:
        case SV_NAME:
            XtSetArg(args[0], XtNlabel, "");
            XtSetValues(sv->w, args, ONE);
            break;

        default:
            impossible("null_out_status: unknown type %d\n", sv->type);
            break;
        }
    }
}

/* This is almost an exact duplicate of hilight_value() */
static void
hilight_label(w)
Widget w; /* label widget */
{
    Arg args[2];
    Pixel fg, bg;

    XtSetArg(args[0], XtNforeground, &fg);
    XtSetArg(args[1], XtNbackground, &bg);
    XtGetValues(w, args, TWO);

    XtSetArg(args[0], XtNforeground, bg);
    XtSetArg(args[1], XtNbackground, fg);
    XtSetValues(w, args, TWO);
}

static void
update_val(attr_rec, new_value)
struct X_status_value *attr_rec;
long new_value;
{
    char buf[BUFSZ];
    Arg args[4];

    if (attr_rec->type == SV_LABEL) {
        if (attr_rec == &shown_stats[F_NAME]) {
            Strcpy(buf, plname);
            buf[0] = highc(buf[0]);
            Strcat(buf, " the ");
            if (Upolyd) {
                char mname[BUFSZ];
                int k;

                Strcpy(mname, mons[u.umonnum].mname);
                for (k = 0; mname[k] != '\0'; k++) {
                    if (k == 0 || mname[k - 1] == ' ')
                        mname[k] = highc(mname[k]);
                }
                Strcat(buf, mname);
            } else
                Strcat(buf, rank_of(u.ulevel, pl_character[0], flags.female));

        } else if (attr_rec == &shown_stats[F_DLEVEL]) {
            if (!describe_level(buf)) {
                Strcpy(buf, dungeons[u.uz.dnum].dname);
                Sprintf(eos(buf), ", level %d", depth(&u.uz));
            }
        } else {
            impossible("update_val: unknown label type \"%s\"",
                       attr_rec->name);
            return;
        }

        if (strcmp(buf, attr_rec->name) == 0)
            return; /* same */

        /* Set the label.  'name' field is const for most entries;
           we need to cast away that const for this assignment */
        Strcpy((char *) attr_rec->name, buf);
        XtSetArg(args[0], XtNlabel, buf);
        XtSetValues(attr_rec->w, args, ONE);

    } else if (attr_rec->type == SV_NAME) {
        if (attr_rec->last_value == new_value)
            return; /* no change */

        attr_rec->last_value = new_value;

        /* special cases: hunger, encumbrance, sickness */
        if (attr_rec == &shown_stats[F_HUNGER]) {
            XtSetArg(args[0], XtNlabel, hu_stat[new_value]);
        } else if (attr_rec == &shown_stats[F_ENCUMBER]) {
            XtSetArg(args[0], XtNlabel, enc_stat[new_value]);
        } else if (new_value) {
            XtSetArg(args[0], XtNlabel, attr_rec->name);
        } else {
            XtSetArg(args[0], XtNlabel, "");
        }
        XtSetValues(attr_rec->w, args, ONE);

    } else { /* a value pair */
        boolean force_update = FALSE;

        /* special case: time can be enabled & disabled */
        if (attr_rec == &shown_stats[F_TIME]) {
            static boolean flagtime = TRUE;

            if (flags.time && !flagtime) {
                set_name(attr_rec->w, shown_stats[F_TIME].name);
                force_update = TRUE;
                flagtime = flags.time;
            } else if (!flags.time && flagtime) {
                set_name(attr_rec->w, "");
                set_value(attr_rec->w, "");
                flagtime = flags.time;
            }
            if (!flagtime)
                return;
        }

        /* special case: exp can be enabled & disabled */
        else if (attr_rec == &shown_stats[F_EXP]) {
            static boolean flagexp = TRUE;

            if (flags.showexp && !flagexp) {
                set_name(attr_rec->w, shown_stats[F_EXP].name);
                force_update = TRUE;
                flagexp = flags.showexp;
            } else if (!flags.showexp && flagexp) {
                set_name(attr_rec->w, "");
                set_value(attr_rec->w, "");
                flagexp = flags.showexp;
            }
            if (!flagexp)
                return;
        }

        /* special case: score can be enabled & disabled */
        else if (attr_rec == &shown_stats[F_SCORE]) {
            static boolean flagscore = TRUE;
#ifdef SCORE_ON_BOTL

            if (flags.showscore && !flagscore) {
                set_name(attr_rec->w, shown_stats[F_SCORE].name);
                force_update = TRUE;
                flagscore = flags.showscore;
            } else if (!flags.showscore && flagscore) {
                set_name(attr_rec->w, "");
                set_value(attr_rec->w, "");
                flagscore = flags.showscore;
            }
            if (!flagscore)
                return;
#else
            if (flagscore) {
                set_name(attr_rec->w, "");
                set_value(attr_rec->w, "");
                flagscore = FALSE;
            }
            return;
#endif
        }

        /* special case: when polymorphed, show "HD", disable exp */
        else if (attr_rec == &shown_stats[F_LEVEL]) {
            static boolean lev_was_poly = FALSE;

            if (Upolyd && !lev_was_poly) {
                force_update = TRUE;
                set_name(attr_rec->w, "HD");
                lev_was_poly = TRUE;
            } else if (Upolyd && lev_was_poly) {
                force_update = TRUE;
                set_name(attr_rec->w, shown_stats[F_LEVEL].name);
                lev_was_poly = FALSE;
            }
        } else if (attr_rec == &shown_stats[F_EXP]) {
            static boolean exp_was_poly = FALSE;

            if (Upolyd && !exp_was_poly) {
                force_update = TRUE;
                set_name(attr_rec->w, "");
                set_value(attr_rec->w, "");
                exp_was_poly = TRUE;
            } else if (Upolyd && exp_was_poly) {
                force_update = TRUE;
                set_name(attr_rec->w, shown_stats[F_EXP].name);
                exp_was_poly = FALSE;
            }
            if (Upolyd)
                return; /* no display for exp when poly */
        }

        if (attr_rec->last_value == new_value && !force_update) /* same */
            return;

        attr_rec->last_value = new_value;

        /* Special cases: strength, alignment and "clear". */
        if (attr_rec == &shown_stats[F_STR]) {
            if (new_value > 18) {
                if (new_value > 118)
                    Sprintf(buf, "%ld", new_value - 100);
                else if (new_value < 118)
                    Sprintf(buf, "18/%02ld", new_value - 18);
                else
                    Strcpy(buf, "18/**");
            } else {
                Sprintf(buf, "%ld", new_value);
            }
        } else if (attr_rec == &shown_stats[F_ALIGN]) {
            Strcpy(buf,
                   (new_value == A_CHAOTIC)
                       ? "Chaotic"
                       : (new_value == A_NEUTRAL) ? "Neutral" : "Lawful");
        } else {
            Sprintf(buf, "%ld", new_value);
        }
        set_value(attr_rec->w, buf);
    }

    /*
     * Now hilight the changed information.  Names, time and score don't
     * hilight.  If first time, don't hilight.  If already lit, don't do
     * it again.
     */
    if (attr_rec->type != SV_NAME && attr_rec != &shown_stats[F_TIME]) {
        if (attr_rec->after_init) {
            if (!attr_rec->set) {
                if (attr_rec->type == SV_LABEL)
                    hilight_label(attr_rec->w);
                else
                    hilight_value(attr_rec->w);
                attr_rec->set = TRUE;
            }
            attr_rec->turn_count = 0;
        } else {
            attr_rec->after_init = TRUE;
        }
    }
}

/*
 * Update the displayed status.  The current code in botl.c updates
 * two lines of information.  Both lines are always updated one after
 * the other.  So only do our update when we update the second line.
 *
 * Information on the first line:
 *	name, attributes, alignment, score
 *
 * Information on the second line:
 *	dlvl, gold, hp, power, ac, {level & exp or HD **}, time,
 *	status * (stone, slime, strngl, foodpois, termill,
 *                hunger, encumbrance, lev, fly, ride,
 *                blind, deaf, stun, conf, hallu)
 *
 *  [*] order of status fields is different on tty.
 * [**] HD is shown instead of level and exp if Upolyd.
 */
static void
update_fancy_status(wp)
struct xwindow *wp;
{
    struct X_status_value *sv;
    long val;
    int i;

    if (wp->cursy != 0)
        return; /* do a complete update when line 0 is done */

    for (i = 0, sv = shown_stats; i < NUM_STATS; i++, sv++) {
        switch (i) {
        case F_DUMMY:
            val = 0L;
            break;
        case F_STR:
            val = (long) ACURR(A_STR);
            break;
        case F_DEX:
            val = (long) ACURR(A_DEX);
            break;
        case F_CON:
            val = (long) ACURR(A_CON);
            break;
        case F_INT:
            val = (long) ACURR(A_INT);
            break;
        case F_WIS:
            val = (long) ACURR(A_WIS);
            break;
        case F_CHA:
            val = (long) ACURR(A_CHA);
            break;
        /*
         * Label stats.  With the exceptions of hunger, encumbrance, sick
         * these are either on or off.  Pleae leave the ternary operators
         * the way they are.  I want to specify 0 or 1, not a boolean.
         */
        case F_HUNGER:
            val = (long) u.uhs;
            break;
        case F_ENCUMBER:
            val = (long) near_capacity();
            break;
        case F_LEV:
            val = Levitation ? 1L : 0L;
            break;
        case F_FLY:
            val = Flying ? 1L : 0L;
            break;
        case F_RIDE:
            val = u.usteed ? 1L : 0L;
            break;
        /* fatal status conditions */
        case F_STONE:
            val = Stoned ? 1L : 0L;
            break;
        case F_SLIME:
            val = Slimed ? 1L : 0L;
            break;
        case F_STRNGL:
            val = Strangled ? 1L : 0L;
            break;
        case F_FOODPOIS:
            val = (Sick && (u.usick_type & SICK_VOMITABLE)) ? 1L : 0L;
            break;
        case F_TERMILL:
            val = (Sick && (u.usick_type & SICK_NONVOMITABLE)) ? 1L : 0L;
            break;
        /* non-fatal status conditions */
        case F_BLIND:
            val = Blind ? 1L : 0L;
            break;
        case F_DEAF:
            val = Deaf ? 1L : 0L;
            break;
        case F_STUN:
            val = Stunned ? 1L : 0L;
            break;
        case F_CONF:
            val = Confusion ? 1L : 0L;
            break;
        case F_HALLU:
            val = Hallucination ? 1L : 0L;
            break;

        case F_NAME:
            val = (long) 0L;
            break; /* special */
        case F_DLEVEL:
            val = (long) 0L;
            break; /* special */
        case F_GOLD:
            val = money_cnt(invent);
            if (val < 0L)
                val = 0L; /* ought to issue impossible() and discard gold */
            break;
        case F_HP:
            val = (long) (Upolyd ? (u.mh > 0 ? u.mh : 0)
                                 : (u.uhp > 0 ? u.uhp : 0));
            break;
        case F_MAXHP:
            val = (long) (Upolyd ? u.mhmax : u.uhpmax);
            break;
        case F_POWER:
            val = (long) u.uen;
            break;
        case F_MAXPOWER:
            val = (long) u.uenmax;
            break;
        case F_AC:
            val = (long) u.uac;
            break;
        case F_LEVEL:
            val = (long) (Upolyd ? mons[u.umonnum].mlevel : u.ulevel);
            break;
        case F_EXP:
            val = flags.showexp ? u.uexp : 0L;
            break;
        case F_ALIGN:
            val = (long) u.ualign.type;
            break;
        case F_TIME:
            val = flags.time ? (long) moves : 0L;
            break;
        case F_SCORE:
#ifdef SCORE_ON_BOTL
            val = flags.showscore ? botl_score() : 0L;
#else
            val = 0L;
#endif
            break;
        default: {
            /*
             * There is a possible infinite loop that occurs with:
             *
             * 	impossible->pline->flush_screen->bot->bot{1,2}->
             * 	putstr->adjust_status->update_other->impossible
             *
             * Break out with this.
             */
            static boolean active = FALSE;

            if (!active) {
                active = TRUE;
                impossible("update_other: unknown shown value");
                active = FALSE;
            }
            val = 0L;
            break;
        } /* default */
        } /* switch */
        update_val(sv, val);
    }
}

/*
 * Turn off hilighted status values after a certain amount of turns.
 */
void
check_turn_events()
{
    int i;
    struct X_status_value *sv;

    for (sv = shown_stats, i = 0; i < NUM_STATS; i++, sv++) {
        if (!sv->set)
            continue;

        if (sv->turn_count++ >= hilight_time) {
            if (sv->type == SV_LABEL)
                hilight_label(sv->w);
            else
                hilight_value(sv->w);
            sv->set = FALSE;
        }
    }
}

/* Initialize alternate status =============================================
 */

/* Return a string for the initial width. */
static const char *
width_string(sv_index)
int sv_index;
{
    switch (sv_index) {
    case F_DUMMY:
        return " ";

    case F_STR:
        return "018/**";
    case F_DEX:
    case F_CON:
    case F_INT:
    case F_WIS:
    case F_CHA:
        return "088"; /* all but str never get bigger */

    case F_HUNGER:
        return "Satiated";
    case F_ENCUMBER:
        return "Overloaded";

    case F_LEV:
    case F_FLY:
    case F_RIDE:
    case F_STONE:
    case F_SLIME:
    case F_STRNGL:
    case F_FOODPOIS:
    case F_TERMILL:
    case F_BLIND:
    case F_DEAF:
    case F_STUN:
    case F_CONF:
    case F_HALLU:
        return shown_stats[sv_index].name;

    case F_NAME:
    case F_DLEVEL:
        return "";
    case F_HP:
    case F_MAXHP:
        return "9999";
    case F_POWER:
    case F_MAXPOWER:
        return "9999";
    case F_AC:
        return "-127";
    case F_LEVEL:
        return "99";
    case F_GOLD:
        /* strongest hero can pick up roughly 30% of this much */
        return "999999"; /* same limit as tty */
    case F_EXP:
    case F_TIME:
    case F_SCORE:
        return "123456789"; /* a tenth digit will still fit legibly */
    case F_ALIGN:
        return "Neutral";
    }
    impossible("width_string: unknown index %d\n", sv_index);
    return "";
}

static void
create_widget(parent, sv, sv_index)
Widget parent;
struct X_status_value *sv;
int sv_index;
{
    Arg args[4];
    Cardinal num_args;

    switch (sv->type) {
    case SV_VALUE:
        sv->w = create_value(parent, sv->name);
        set_value(sv->w, width_string(sv_index));
        break;
    case SV_LABEL:
        /* Labels get their own buffer. */
        sv->name = (char *) alloc(BUFSZ);
        /* we need to cast away 'const' when assigning a value */
        *(char *) (sv->name) = '\0';

        num_args = 0;
        XtSetArg(args[num_args], XtNborderWidth, 0);
        num_args++;
        XtSetArg(args[num_args], XtNinternalHeight, 0);
        num_args++;
        sv->w = XtCreateManagedWidget((sv_index == F_NAME)
                                         ? "name"
                                         : "dlevel",
                                      labelWidgetClass, parent,
                                      args, num_args);
        break;
    case SV_NAME:
        num_args = 0;
        XtSetArg(args[0], XtNlabel, width_string(sv_index)); num_args++;
        XtSetArg(args[num_args], XtNborderWidth, 0); num_args++;
        XtSetArg(args[num_args], XtNinternalHeight, 0); num_args++;
        sv->w = XtCreateManagedWidget(sv->name, labelWidgetClass, parent,
                                      args, num_args);
        break;
    default:
        panic("create_widget: unknown type %d", sv->type);
    }
}

/*
 * Get current width of value.  width2p is only valid for SV_VALUE types.
 */
static void
get_widths(sv, width1p, width2p)
struct X_status_value *sv;
int *width1p, *width2p;
{
    Arg args[1];
    Dimension width;

    switch (sv->type) {
    case SV_VALUE:
        *width1p = get_name_width(sv->w);
        *width2p = get_value_width(sv->w);
        break;
    case SV_LABEL:
    case SV_NAME:
        XtSetArg(args[0], XtNwidth, &width);
        XtGetValues(sv->w, args, ONE);
        *width1p = width;
        *width2p = 0;
        break;
    default:
        panic("get_widths: unknown type %d", sv->type);
    }
}

static void
set_widths(sv, width1, width2)
struct X_status_value *sv;
int width1, width2;
{
    Arg args[1];

    switch (sv->type) {
    case SV_VALUE:
        set_name_width(sv->w, width1);
        set_value_width(sv->w, width2);
        break;
    case SV_LABEL:
    case SV_NAME:
        XtSetArg(args[0], XtNwidth, (width1 + width2));
        XtSetValues(sv->w, args, ONE);
        break;
    default:
        panic("set_widths: unknown type %d", sv->type);
    }
}

static Widget
init_column(name, parent, top, left, col_indices)
const char *name;
Widget parent, top, left;
int *col_indices;
{
    Widget form;
    Arg args[4];
    Cardinal num_args;
    int max_width1, width1, max_width2, width2;
    int *ip;
    struct X_status_value *sv;

    num_args = 0;
    if (top != (Widget) 0) {
        XtSetArg(args[num_args], nhStr(XtNfromVert), top);
        num_args++;
    }
    if (left != (Widget) 0) {
        XtSetArg(args[num_args], nhStr(XtNfromHoriz), left);
        num_args++;
    }
    XtSetArg(args[num_args], nhStr(XtNdefaultDistance), 0);
    num_args++;
    form = XtCreateManagedWidget(name, formWidgetClass, parent,
                                 args, num_args);

    max_width1 = max_width2 = 0;
    for (ip = col_indices; *ip >= 0; ip++) {
        sv = &shown_stats[*ip];
        create_widget(form, sv, *ip); /* will set init width */
        if (ip != col_indices) {      /* not first */
            num_args = 0;
            XtSetArg(args[num_args], nhStr(XtNfromVert),
                     shown_stats[*(ip - 1)].w);
            num_args++;
            XtSetValues(sv->w, args, num_args);
        }
        get_widths(sv, &width1, &width2);
        if (width1 > max_width1)
            max_width1 = width1;
        if (width2 > max_width2)
            max_width2 = width2;
    }
    for (ip = col_indices; *ip >= 0; ip++) {
        set_widths(&shown_stats[*ip], max_width1, max_width2);
    }

    /* There is room behind the end marker for the two widths. */
    *++ip = max_width1;
    *++ip = max_width2;

    return form;
}

/*
 * These are the orders of the displayed columns.  Change to suit.  The -1
 * indicates the end of the column.  The two numbers after that are used
 * to store widths that are calculated at run-time.
 */
static int attrib_indices[] = { F_STR, F_DEX, F_CON, F_INT, F_WIS, F_CHA,
                                -1, 0, 0 };
/* including F_DUMMY makes the three status condition columns evenly
   spaced with regard to the adjacent characteristics (Str,Dex,&c) column;
   we lose track of the Widget pointer for them, each use clobbering the
   one before, leaving the one from leftover_indices[]; since they're never
   updated, that shouldn't matter */
static int status_indices[3][9] = { { F_STONE, F_SLIME, F_STRNGL,
                                      F_FOODPOIS, F_TERMILL, F_DUMMY,
                                      -1, 0, 0 },
                                    { F_HUNGER, F_ENCUMBER,
                                      F_LEV, F_FLY, F_RIDE, F_DUMMY,
                                      -1, 0, 0 },
                                    { F_BLIND, F_DEAF, F_STUN,
                                      F_CONF, F_HALLU, F_DUMMY,
                                      -1, 0, 0 } };
/* used to fill up the empty space to right of 3rd status condition column */
static int leftover_indices[] = { F_DUMMY, -1, 0, 0 };

static int col1_indices[] = { F_HP,    F_POWER,    F_AC,    F_LEVEL, F_GOLD,
                              F_SCORE, -1, 0, 0 };
static int col2_indices[] = { F_MAXHP, F_MAXPOWER, F_ALIGN, F_EXP,   F_TIME,
                              -1, 0, 0 };
/*
 * Produce a form that looks like the following:
 *
 *		   name
 *		  dlevel
 * col1_indices[0]	col2_indices[0]
 * col1_indices[1]	col2_indices[1]
 *    .		    .
 *    .		    .
 * col1_indices[n]	col2_indices[n]
 *
 * TODO:  increase the space between the two columns.
 */
static Widget
init_info_form(parent, top, left)
Widget parent, top, left;
{
    Widget form, col1;
    struct X_status_value *sv_name, *sv_dlevel;
    Arg args[6];
    Cardinal num_args;
    int total_width, *ip;

    num_args = 0;
    if (top != (Widget) 0) {
        XtSetArg(args[num_args], nhStr(XtNfromVert), top);
        num_args++;
    }
    if (left != (Widget) 0) {
        XtSetArg(args[num_args], nhStr(XtNfromHoriz), left);
        num_args++;
    }
    XtSetArg(args[num_args], nhStr(XtNdefaultDistance), 0);
    num_args++;
    form = XtCreateManagedWidget("status_info", formWidgetClass, parent, args,
                                 num_args);

    /* top of form */
    sv_name = &shown_stats[F_NAME];
    create_widget(form, sv_name, F_NAME);

    /* second */
    sv_dlevel = &shown_stats[F_DLEVEL];
    create_widget(form, sv_dlevel, F_DLEVEL);

    num_args = 0;
    XtSetArg(args[num_args], nhStr(XtNfromVert), sv_name->w);
    num_args++;
    XtSetValues(sv_dlevel->w, args, num_args);

    /* two columns beneath */
    col1 = init_column("name_col1", form, sv_dlevel->w, (Widget) 0,
                       col1_indices);
    (void) init_column("name_col2", form, sv_dlevel->w, col1, col2_indices);

    /* Add calculated widths. */
    for (ip = col1_indices; *ip >= 0; ip++)
        ; /* skip to end */
    total_width = *++ip;
    total_width += *++ip;
    for (ip = col2_indices; *ip >= 0; ip++)
        ; /* skip to end */
    total_width += *++ip;
    total_width += *++ip;

    XtSetArg(args[0], XtNwidth, total_width);
    XtSetValues(sv_name->w, args, ONE);
    XtSetArg(args[0], XtNwidth, total_width);
    XtSetValues(sv_dlevel->w, args, ONE);

    return form;
}

/* give the three status condition columns the same width */
static void
fixup_cond_widths()
{
    int pass, i, *ip, w1, w2;

    w1 = w2 = 0;
    for (pass = 1; pass <= 2; ++pass) { /* two passes... */
        for (i = 0; i < 3; i++) { /* three columns */
            for (ip = status_indices[i]; *ip != -1; ++ip) { /* X fields */
                /* pass 1: find -1;  pass 2: update field widths, find -1 */
                if (pass == 2)
                    set_widths(&shown_stats[*ip], w1, w2);
            }
            /* found -1; the two slots beyond it contain column widths */
            if (pass == 1) { /* pass 1: collect maxima */
                if (ip[1] > w1)
                    w1 = ip[1];
                if (ip[2] > w2)
                    w2 = ip[2];
            } else { /* pass 2: update column widths with maxima */
                ip[1] = w1;
                ip[2] = w2;
            }
        }
        /* ascetics:  expand the maximum width to make cond columns wider */
        if (pass == 1) {
            w1 += 20;
            if (w2 > 0)
                w2 += 20;
        }
    }
}

/*
 * Create the layout for the fancy status.  Return a form widget that
 * contains everything.
 */
static Widget
create_fancy_status(parent, top)
Widget parent, top;
{
    Widget form; /* The form that surrounds everything. */
    Widget w;
    Arg args[8];
    Cardinal num_args;
    char buf[32];
    int i;

    num_args = 0;
    if (top != (Widget) 0) {
        XtSetArg(args[num_args], nhStr(XtNfromVert), top);
        num_args++;
    }
    XtSetArg(args[num_args], nhStr(XtNdefaultDistance), 0);
    num_args++;
    XtSetArg(args[num_args], XtNborderWidth, 0);
    num_args++;
    XtSetArg(args[num_args], XtNorientation, XtorientHorizontal);
    num_args++;
    form = XtCreateManagedWidget("fancy_status", panedWidgetClass, parent,
                                 args, num_args);

    w = init_info_form(form, (Widget) 0, (Widget) 0);
    w = init_column("status_attributes", form, (Widget) 0, w, attrib_indices);
    for (i = 0; i < 3; i++) {
        Sprintf(buf, "status_condition%d", i + 1);
        w = init_column(buf, form, (Widget) 0, w, status_indices[i]);
    }
    fixup_cond_widths(); /* make all 3 status_conditionN columns same width */
    w = init_column("status_leftover", form, (Widget) 0, w, leftover_indices);
    nhUse(w);
    return form;
}

static void
destroy_fancy_status(wp)
struct xwindow *wp;
{
    int i;
    struct X_status_value *sv;

    if (!wp->keep_window)
        XtDestroyWidget(wp->w), wp->w = (Widget) 0;

    for (i = 0, sv = shown_stats; i < NUM_STATS; i++, sv++)
        if (sv->type == SV_LABEL) {
            free((genericptr_t) sv->name);
            sv->name = 0;
        }
}

/*winstat.c*/
