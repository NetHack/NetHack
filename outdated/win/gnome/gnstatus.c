/* NetHack 3.6	gnstatus.c	$NHDT-Date: 1432512806 2015/05/25 00:13:26 $  $NHDT-Branch: master $:$NHDT-Revision: 1.15 $ */
/* Copyright (C) 1998 by Erik Andersen <andersee@debian.org> */
/* NetHack may be freely redistributed.  See license for details. */

#include "gnstatus.h"
#include "gnsignal.h"
#include "gn_xpms.h"
#include "gnomeprv.h"

extern const char *hu_stat[];  /* from eat.c */
extern const char *enc_stat[]; /* from botl.c */

void ghack_status_window_update_stats();
void ghack_status_window_clear(GtkWidget *win, gpointer data);
void ghack_status_window_destroy(GtkWidget *win, gpointer data);
void ghack_status_window_display(GtkWidget *win, boolean block,
                                 gpointer data);
void ghack_status_window_cursor_to(GtkWidget *win, int x, int y,
                                   gpointer data);
void ghack_status_window_put_string(GtkWidget *win, int attr,
                                    const char *text, gpointer data);

static void ghack_fade_highlighting();
static void ghack_highlight_widget(GtkWidget *widget, GtkStyle *oldStyle,
                                   GtkStyle *newStyle);

/* some junk to handle when to fade the highlighting */
#define NUM_TURNS_HIGHLIGHTED 3

static GList *s_HighLightList;

typedef struct {
    GtkWidget *widget;
    GtkStyle *oldStyle;
    int nTurnsLeft;
} Highlight;

/* Ok, now for a LONG list of widgets... */
static GtkWidget *statTable = NULL;
static GtkWidget *titleLabel = NULL;
static GtkWidget *dgnLevelLabel = NULL;
static GtkWidget *strPix = NULL;
static GtkWidget *strLabel = NULL;
static GtkWidget *dexPix = NULL;
static GtkWidget *dexLabel = NULL;
static GtkWidget *intPix = NULL;
static GtkWidget *intLabel = NULL;
static GtkWidget *wisPix = NULL;
static GtkWidget *wisLabel = NULL;
static GtkWidget *conPix = NULL;
static GtkWidget *conLabel = NULL;
static GtkWidget *chaPix = NULL;
static GtkWidget *chaLabel = NULL;
static GtkWidget *goldLabel = NULL;
static GtkWidget *hpLabel = NULL;
static GtkWidget *powLabel = NULL;
static GtkWidget *acLabel = NULL;
static GtkWidget *levlLabel = NULL;
static GtkWidget *expLabel = NULL;
static GtkWidget *timeLabel = NULL;
static GtkWidget *scoreLabel = NULL;
static GtkWidget *alignPix = NULL;
static GtkWidget *alignLabel = NULL;
static GtkWidget *hungerPix = NULL;
static GtkWidget *hungerLabel = NULL;
static GtkWidget *sickPix = NULL;
static GtkWidget *sickLabel = NULL;
static GtkWidget *blindPix = NULL;
static GtkWidget *blindLabel = NULL;
static GtkWidget *stunPix = NULL;
static GtkWidget *stunLabel = NULL;
static GtkWidget *halluPix = NULL;
static GtkWidget *halluLabel = NULL;
static GtkWidget *confuPix = NULL;
static GtkWidget *confuLabel = NULL;
static GtkWidget *encumbPix = NULL;
static GtkWidget *encumbLabel = NULL;

static GtkStyle *normalStyle = NULL;
static GtkStyle *bigStyle = NULL;
static GtkStyle *redStyle = NULL;
static GtkStyle *greenStyle = NULL;
static GtkStyle *bigRedStyle = NULL;
static GtkStyle *bigGreenStyle = NULL;

/* Pure red */
static GdkColor color_red = { 0, 0xff00, 0, 0 };
/* ForestGreen (looks better than just pure green) */
static GdkColor color_green = { 0, 0x2200, 0x8b00, 0x2200 };

static int lastDepth;
static int lastStr;
static int lastInt;
static int lastWis;
static int lastDex;
static int lastCon;
static int lastCha;
static long lastAu;
static int lastHP;
static int lastMHP;
static int lastLevel;
static int lastPOW;
static int lastMPOW;
static int lastAC;
static int lastExp;
static aligntyp lastAlignment;
static unsigned lastHungr;
static long lastConf;
static long lastBlind;
static long lastStun;
static long lastHalu;
static long lastSick;
static int lastEncumb;

void
ghack_status_window_clear(GtkWidget *win, gpointer data)
{
    /* Don't think we need this at all */
}

void
ghack_status_window_destroy(GtkWidget *win, gpointer data)
{
    while (s_HighLightList) {
        g_free((Highlight *) s_HighLightList->data);
        s_HighLightList = s_HighLightList->next;
    }
    g_list_free(s_HighLightList);
}

void
ghack_status_window_display(GtkWidget *win, boolean block, gpointer data)
{
    gtk_widget_show_all(GTK_WIDGET(win));
}

void
ghack_status_window_cursor_to(GtkWidget *win, int x, int y, gpointer data)
{
    /* Don't think we need this at all */
}

void
ghack_status_window_put_string(GtkWidget *win, int attr, const char *text,
                               gpointer data)
{
    ghack_status_window_update_stats();
}

GtkWidget *
ghack_init_status_window()
{
    GtkWidget *horizSep0, *horizSep1, *horizSep2, *horizSep3;
    GtkWidget *statsHBox, *strVBox, *dexVBox, *intVBox, *statHBox;
    GtkWidget *wisVBox, *conVBox, *chaVBox;
    GtkWidget *alignVBox, *hungerVBox, *sickVBox, *blindVBox;
    GtkWidget *stunVBox, *halluVBox, *confuVBox, *encumbVBox;

    /* Set up a (ridiculous) initial state */
    lastDepth = 9999;
    lastStr = 9999;
    lastInt = 9999;
    lastWis = 9999;
    lastDex = 9999;
    lastCon = 9999;
    lastCha = 9999;
    lastAu = 9999;
    lastHP = 9999;
    lastMHP = 9999;
    lastLevel = 9999;
    lastPOW = 9999;
    lastMPOW = 9999;
    lastAC = 9999;
    lastExp = 9999;
    lastAlignment = A_NEUTRAL; /* start off guessing neutral */
    lastHungr = 9999;
    lastConf = 9999;
    lastBlind = 9999;
    lastStun = 9999;
    lastHalu = 9999;
    lastSick = 9999;
    lastEncumb = 9999;

    statTable = gtk_table_new(10, 8, FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(statTable), 1);
    gtk_table_set_col_spacings(GTK_TABLE(statTable), 1);

    /* Begin the first row of the table -- the title */
    titleLabel = gtk_label_new(_("GnomeHack!"));
    gtk_table_attach(GTK_TABLE(statTable), titleLabel, 0, 8, 0, 1, GTK_FILL,
                     0, 0, 0);
    if (!normalStyle)
        normalStyle =
            gtk_style_copy(gtk_widget_get_style(GTK_WIDGET(titleLabel)));

    /* Set up some styles to draw stuff with */
    if (!redStyle) {
        g_assert(greenStyle == NULL);
        g_assert(bigStyle == NULL);
        g_assert(bigRedStyle == NULL);
        g_assert(bigGreenStyle == NULL);

        greenStyle = gtk_style_copy(normalStyle);
        redStyle = gtk_style_copy(normalStyle);
        bigRedStyle = gtk_style_copy(normalStyle);
        bigGreenStyle = gtk_style_copy(normalStyle);
        bigStyle = gtk_style_copy(normalStyle);

        greenStyle->fg[GTK_STATE_NORMAL] = color_green;
        redStyle->fg[GTK_STATE_NORMAL] = color_red;
        bigRedStyle->fg[GTK_STATE_NORMAL] = color_red;
        bigGreenStyle->fg[GTK_STATE_NORMAL] = color_green;

        gdk_font_unref(bigRedStyle->font);
        gdk_font_unref(bigGreenStyle->font);
        bigRedStyle->font =
            gdk_font_load("-misc-fixed-*-*-*-*-20-*-*-*-*-*-*-*");
        bigGreenStyle->font =
            gdk_font_load("-misc-fixed-*-*-*-*-20-*-*-*-*-*-*-*");

        gdk_font_unref(bigStyle->font);
        bigStyle->font =
            gdk_font_load("-misc-fixed-*-*-*-*-20-*-*-*-*-*-*-*");
    }
    gtk_widget_set_style(GTK_WIDGET(titleLabel), bigStyle);

    /* Begin the second row */
    dgnLevelLabel = gtk_label_new(_("Nethack for Gnome"));
    gtk_table_attach(GTK_TABLE(statTable), dgnLevelLabel, 0, 8, 1, 2,
                     GTK_FILL, 0, 0, 0);
    gtk_widget_set_style(GTK_WIDGET(dgnLevelLabel), bigStyle);

    /* Begin the third row */
    horizSep0 = gtk_hseparator_new();
    gtk_table_attach(GTK_TABLE(statTable), horizSep0, 0, 8, 2, 3, GTK_FILL,
                     GTK_FILL, 0, 0);

    /* Begin the fourth row */
    statsHBox = gtk_hbox_new(TRUE, 0);

    strVBox = gtk_vbox_new(FALSE, 0);
    strPix = gnome_pixmap_new_from_xpm_d(str_xpm);
    strLabel = gtk_label_new("STR: ");
    gtk_box_pack_start(GTK_BOX(strVBox), strPix, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(strVBox), strLabel, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(statsHBox), GTK_WIDGET(strVBox), TRUE, TRUE,
                       2);

    dexVBox = gtk_vbox_new(FALSE, 0);
    dexPix = gnome_pixmap_new_from_xpm_d(dex_xpm);
    dexLabel = gtk_label_new("DEX: ");
    gtk_box_pack_start(GTK_BOX(dexVBox), dexPix, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(dexVBox), dexLabel, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(statsHBox), GTK_WIDGET(dexVBox), TRUE, TRUE,
                       2);

    conVBox = gtk_vbox_new(FALSE, 0);
    conPix = gnome_pixmap_new_from_xpm_d(cns_xpm);
    conLabel = gtk_label_new("CON: ");
    gtk_box_pack_start(GTK_BOX(conVBox), conPix, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(conVBox), conLabel, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(statsHBox), GTK_WIDGET(conVBox), TRUE, TRUE,
                       2);

    intVBox = gtk_vbox_new(FALSE, 0);
    intPix = gnome_pixmap_new_from_xpm_d(int_xpm);
    intLabel = gtk_label_new("INT: ");
    gtk_box_pack_start(GTK_BOX(intVBox), intPix, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(intVBox), intLabel, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(statsHBox), GTK_WIDGET(intVBox), TRUE, TRUE,
                       2);

    wisVBox = gtk_vbox_new(FALSE, 0);
    wisPix = gnome_pixmap_new_from_xpm_d(wis_xpm);
    wisLabel = gtk_label_new("WIS: ");
    gtk_box_pack_start(GTK_BOX(wisVBox), wisPix, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(wisVBox), wisLabel, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(statsHBox), GTK_WIDGET(wisVBox), TRUE, TRUE,
                       2);

    chaVBox = gtk_vbox_new(FALSE, 0);
    chaPix = gnome_pixmap_new_from_xpm_d(cha_xpm);
    chaLabel = gtk_label_new("CHA: ");
    gtk_box_pack_start(GTK_BOX(chaVBox), chaPix, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(chaVBox), chaLabel, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(statsHBox), GTK_WIDGET(chaVBox), TRUE, TRUE,
                       2);

    gtk_table_attach(GTK_TABLE(statTable), GTK_WIDGET(statsHBox), 0, 8, 3, 4,
                     GTK_FILL, 0, 0, 0);

    /* Begin the fifth row */
    horizSep1 = gtk_hseparator_new();
    gtk_table_attach(GTK_TABLE(statTable), horizSep1, 0, 8, 4, 5, GTK_FILL,
                     GTK_FILL, 0, 0);

    /* Begin the sixth row */
    hpLabel = gtk_label_new("HP: ");
    gtk_table_attach(GTK_TABLE(statTable), hpLabel, 0, 1, 5, 6, GTK_FILL, 0,
                     0, 0);

    acLabel = gtk_label_new("AC: ");
    gtk_table_attach(GTK_TABLE(statTable), acLabel, 2, 3, 5, 6, GTK_FILL, 0,
                     0, 0);

    powLabel = gtk_label_new("Power: ");
    gtk_table_attach(GTK_TABLE(statTable), powLabel, 4, 5, 5, 6, GTK_FILL, 0,
                     0, 0);

    goldLabel = gtk_label_new("Au: ");
    gtk_table_attach(GTK_TABLE(statTable), goldLabel, 6, 7, 5, 6, GTK_FILL, 0,
                     0, 0);

    /* Begin the seventh row */
    horizSep2 = gtk_hseparator_new();
    gtk_table_attach(GTK_TABLE(statTable), horizSep2, 0, 8, 6, 7, GTK_FILL,
                     GTK_FILL, 0, 0);

    /* Begin the eigth row */
    levlLabel = gtk_label_new("Level: ");
    gtk_table_attach(GTK_TABLE(statTable), levlLabel, 0, 1, 7, 8, GTK_FILL, 0,
                     0, 0);

    expLabel = gtk_label_new("Exp: ");
    gtk_table_attach(GTK_TABLE(statTable), expLabel, 2, 3, 7, 8, GTK_FILL, 0,
                     0, 0);

    timeLabel = gtk_label_new("Time: ");
    gtk_table_attach(GTK_TABLE(statTable), timeLabel, 4, 5, 7, 8, GTK_FILL, 0,
                     0, 0);

    scoreLabel = gtk_label_new("Score: ");
    gtk_table_attach(GTK_TABLE(statTable), scoreLabel, 6, 7, 7, 8, GTK_FILL,
                     0, 0, 0);

    /* Begin the ninth row */
    horizSep3 = gtk_hseparator_new();
    gtk_table_attach(GTK_TABLE(statTable), horizSep3, 0, 8, 8, 9, GTK_FILL,
                     GTK_FILL, 0, 0);

    /* Begin the tenth and last row */
    statHBox = gtk_hbox_new(FALSE, 0);

    alignVBox = gtk_vbox_new(FALSE, 0);
    alignPix = gnome_pixmap_new_from_xpm_d(neutral_xpm);
    alignLabel = gtk_label_new("Neutral");
    gtk_box_pack_start(GTK_BOX(alignVBox), alignPix, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(alignVBox), alignLabel, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(statHBox), GTK_WIDGET(alignVBox), TRUE, FALSE,
                       2);

    hungerVBox = gtk_vbox_new(FALSE, 0);
    hungerPix = gnome_pixmap_new_from_xpm_d(hungry_xpm);
    hungerLabel = gtk_label_new("Hungry");
    gtk_box_pack_start(GTK_BOX(hungerVBox), hungerPix, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(hungerVBox), hungerLabel, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(statHBox), GTK_WIDGET(hungerVBox), TRUE, FALSE,
                       2);

    sickVBox = gtk_vbox_new(FALSE, 0);
    sickPix = gnome_pixmap_new_from_xpm_d(sick_fp_xpm);
    sickLabel = gtk_label_new("FoodPois");
    gtk_box_pack_start(GTK_BOX(sickVBox), sickPix, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(sickVBox), sickLabel, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(statHBox), GTK_WIDGET(sickVBox), TRUE, FALSE,
                       2);

    blindVBox = gtk_vbox_new(FALSE, 0);
    blindPix = gnome_pixmap_new_from_xpm_d(blind_xpm);
    blindLabel = gtk_label_new("Blind");
    gtk_box_pack_start(GTK_BOX(blindVBox), blindPix, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(blindVBox), blindLabel, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(statHBox), GTK_WIDGET(blindVBox), TRUE, FALSE,
                       2);

    stunVBox = gtk_vbox_new(FALSE, 0);
    stunPix = gnome_pixmap_new_from_xpm_d(stunned_xpm);
    stunLabel = gtk_label_new("Stun");
    gtk_box_pack_start(GTK_BOX(stunVBox), stunPix, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(stunVBox), stunLabel, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(statHBox), GTK_WIDGET(stunVBox), TRUE, FALSE,
                       2);

    confuVBox = gtk_vbox_new(FALSE, 0);
    confuPix = gnome_pixmap_new_from_xpm_d(confused_xpm);
    confuLabel = gtk_label_new("Confused");
    gtk_box_pack_start(GTK_BOX(confuVBox), confuPix, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(confuVBox), confuLabel, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(statHBox), GTK_WIDGET(confuVBox), TRUE, FALSE,
                       2);

    halluVBox = gtk_vbox_new(FALSE, 0);
    halluPix = gnome_pixmap_new_from_xpm_d(hallu_xpm);
    halluLabel = gtk_label_new("Hallu");
    gtk_box_pack_start(GTK_BOX(halluVBox), halluPix, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(halluVBox), halluLabel, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(statHBox), GTK_WIDGET(halluVBox), TRUE, FALSE,
                       2);

    encumbVBox = gtk_vbox_new(FALSE, 0);
    encumbPix = gnome_pixmap_new_from_xpm_d(slt_enc_xpm);
    encumbLabel = gtk_label_new("Burdened");
    gtk_box_pack_start(GTK_BOX(encumbVBox), encumbPix, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(encumbVBox), encumbLabel, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(statHBox), GTK_WIDGET(encumbVBox), TRUE, FALSE,
                       2);

    gtk_table_attach(GTK_TABLE(statTable), GTK_WIDGET(statHBox), 0, 8, 9, 10,
                     GTK_FILL, GTK_FILL, 0, 0);

    /* Set up the necessary signals */
    gtk_signal_connect(GTK_OBJECT(statTable), "ghack_fade_highlight",
                       GTK_SIGNAL_FUNC(ghack_fade_highlighting), NULL);

    gtk_signal_connect(GTK_OBJECT(statTable), "ghack_putstr",
                       GTK_SIGNAL_FUNC(ghack_status_window_put_string), NULL);

    gtk_signal_connect(GTK_OBJECT(statTable), "ghack_clear",
                       GTK_SIGNAL_FUNC(ghack_status_window_clear), NULL);

    gtk_signal_connect(GTK_OBJECT(statTable), "ghack_curs",
                       GTK_SIGNAL_FUNC(ghack_status_window_cursor_to), NULL);
    gtk_signal_connect(GTK_OBJECT(statTable), "gnome_delay_output",
                       GTK_SIGNAL_FUNC(ghack_delay), NULL);

    /* Lastly, show the status window and everything in it */
    gtk_widget_show_all(statTable);

    return GTK_WIDGET(statTable);
}

void
ghack_status_window_update_stats()
{
    char buf[BUFSZ];
    gchar *buf1;
    const char *hung;
    const char *enc;
    static int firstTime = TRUE;
    long umoney;

    /* First, fill in the player name and the dungeon level */
    strcpy(buf, gp.plname);
    if ('a' <= buf[0] && buf[0] <= 'z')
        buf[0] += 'A' - 'a';
    strcat(buf, " the ");
    if (u.mtimedone) {
        char mname[BUFSZ];
        int k = 0;

        strcpy(mname, mons[u.umonnum].mname);
        while (mname[k] != 0) {
            if ((k == 0 || (k > 0 && mname[k - 1] == ' ')) && 'a' <= mname[k]
                && mname[k] <= 'z') {
                mname[k] += 'A' - 'a';
            }
            k++;
        }
        strcat(buf, mname);
    } else {
        strcat(buf, rank_of(u.ulevel, pl_character[0], flags.female));
    }
    gtk_label_get(GTK_LABEL(titleLabel), &buf1);
    if (strcmp(buf1, buf) != 0 && firstTime == FALSE) {
        /* Ok, this changed so add it to the highlighing list */
        ghack_highlight_widget(titleLabel, bigStyle, bigGreenStyle);
    }
    gtk_label_set(GTK_LABEL(titleLabel), buf);

    if (In_endgame(&u.uz)) {
        strcpy(buf, (Is_astralevel(&u.uz) ? "Astral Plane" : "End Game"));
    } else {
        sprintf(buf, "%s, level %d", dungeons[u.uz.dnum].dname, depth(&u.uz));
    }
    if (lastDepth > depth(&u.uz) && firstTime == FALSE) {
        /* Ok, this changed so add it to the highlighing list */
        ghack_highlight_widget(dgnLevelLabel, bigStyle, bigRedStyle);
    } else if (lastDepth < depth(&u.uz) && firstTime == FALSE) {
        /* Ok, this changed so add it to the highlighing list */
        ghack_highlight_widget(dgnLevelLabel, bigStyle, bigGreenStyle);
    }
    lastDepth = depth(&u.uz);
    gtk_label_set(GTK_LABEL(dgnLevelLabel), buf);

    /* Next, fill in the player's stats */
    if (ACURR(A_STR) > 118) {
        sprintf(buf, "STR:%d", ACURR(A_STR) - 100);
    } else if (ACURR(A_STR) == 118) {
        sprintf(buf, "STR:18/**");
    } else if (ACURR(A_STR) > 18) {
        sprintf(buf, "STR:18/%02d", ACURR(A_STR) - 18);
    } else {
        sprintf(buf, "STR:%d", ACURR(A_STR));
    }
    if (lastStr < ACURR(A_STR) && firstTime == FALSE) {
        /* Ok, this changed so add it to the highlighing list */
        ghack_highlight_widget(strLabel, normalStyle, greenStyle);
    } else if (lastStr > ACURR(A_STR) && firstTime == FALSE) {
        /* Ok, this changed so add it to the highlighing list */
        ghack_highlight_widget(strLabel, normalStyle, redStyle);
    }
    lastStr = ACURR(A_STR);
    gtk_label_set(GTK_LABEL(strLabel), buf);

    sprintf(buf, "INT:%d", ACURR(A_INT));
    if (lastInt < ACURR(A_INT) && firstTime == FALSE) {
        /* Ok, this changed so add it to the highlighing list */
        ghack_highlight_widget(intLabel, normalStyle, greenStyle);
    } else if (lastInt > ACURR(A_INT) && firstTime == FALSE) {
        /* Ok, this changed so add it to the highlighing list */
        ghack_highlight_widget(intLabel, normalStyle, redStyle);
    }
    lastInt = ACURR(A_INT);
    gtk_label_set(GTK_LABEL(intLabel), buf);

    sprintf(buf, "WIS:%d", ACURR(A_WIS));
    if (lastWis < ACURR(A_WIS) && firstTime == FALSE) {
        /* Ok, this changed so add it to the highlighing list */
        ghack_highlight_widget(wisLabel, normalStyle, greenStyle);
    } else if (lastWis > ACURR(A_WIS) && firstTime == FALSE) {
        /* Ok, this changed so add it to the highlighing list */
        ghack_highlight_widget(wisLabel, normalStyle, redStyle);
    }
    lastWis = ACURR(A_WIS);
    gtk_label_set(GTK_LABEL(wisLabel), buf);

    sprintf(buf, "DEX:%d", ACURR(A_DEX));
    if (lastDex < ACURR(A_DEX) && firstTime == FALSE) {
        /* Ok, this changed so add it to the highlighing list */
        ghack_highlight_widget(dexLabel, normalStyle, greenStyle);
    } else if (lastDex > ACURR(A_DEX) && firstTime == FALSE) {
        /* Ok, this changed so add it to the highlighing list */
        ghack_highlight_widget(dexLabel, normalStyle, redStyle);
    }
    lastDex = ACURR(A_DEX);
    gtk_label_set(GTK_LABEL(dexLabel), buf);

    sprintf(buf, "CON:%d", ACURR(A_CON));
    if (lastCon < ACURR(A_CON) && firstTime == FALSE) {
        /* Ok, this changed so add it to the highlighing list */
        ghack_highlight_widget(conLabel, normalStyle, greenStyle);
    } else if (lastCon > ACURR(A_CON) && firstTime == FALSE) {
        /* Ok, this changed so add it to the highlighing list */
        ghack_highlight_widget(conLabel, normalStyle, redStyle);
    }
    lastCon = ACURR(A_CON);
    gtk_label_set(GTK_LABEL(conLabel), buf);

    sprintf(buf, "CHA:%d", ACURR(A_CHA));
    if (lastCha < ACURR(A_CHA) && firstTime == FALSE) {
        /* Ok, this changed so add it to the highlighing list */
        ghack_highlight_widget(chaLabel, normalStyle, greenStyle);
    } else if (lastCha > ACURR(A_CHA) && firstTime == FALSE) {
        /* Ok, this changed so add it to the highlighing list */
        ghack_highlight_widget(chaLabel, normalStyle, redStyle);
    }
    lastCha = ACURR(A_CHA);
    gtk_label_set(GTK_LABEL(chaLabel), buf);

    /* Now do the non-pixmaped stats (gold and such) */
    umoney = money_cnt(gi.invent);
    sprintf(buf, "Au:%ld", umoney);
    if (lastAu < umoney && firstTime == FALSE) {
        /* Ok, this changed so add it to the highlighing list */
        ghack_highlight_widget(goldLabel, normalStyle, greenStyle);
    } else if (lastAu > umoney && firstTime == FALSE) {
        /* Ok, this changed so add it to the highlighing list */
        ghack_highlight_widget(goldLabel, normalStyle, redStyle);
    }
    lastAu = umoney;
    gtk_label_set(GTK_LABEL(goldLabel), buf);

    if (u.mtimedone) {
        /* special case: when polymorphed, show "HD", disable exp */
        sprintf(buf, "HP:%d/%d", ((u.mh > 0) ? u.mh : 0), u.mhmax);
        if ((lastHP < u.mh || lastMHP < u.mhmax) && firstTime == FALSE) {
            /* Ok, this changed so add it to the highlighing list */
            ghack_highlight_widget(hpLabel, normalStyle, greenStyle);
        } else if ((lastHP > u.mh || lastMHP > u.mhmax)
                   && firstTime == FALSE) {
            /* Ok, this changed so add it to the highlighing list */
            ghack_highlight_widget(hpLabel, normalStyle, redStyle);
        }
        lastHP = u.mh;
        lastMHP = u.mhmax;
    } else {
        sprintf(buf, "HP:%d/%d", ((u.uhp > 0) ? u.uhp : 0), u.uhpmax);
        if ((lastHP < u.uhp || lastMHP < u.uhpmax) && firstTime == FALSE) {
            /* Ok, this changed so add it to the highlighing list */
            ghack_highlight_widget(hpLabel, normalStyle, greenStyle);
        } else if ((lastHP > u.uhp || lastMHP > u.uhpmax)
                   && firstTime == FALSE) {
            /* Ok, this changed so add it to the highlighing list */
            ghack_highlight_widget(hpLabel, normalStyle, redStyle);
        }
        lastHP = u.uhp;
        lastMHP = u.uhpmax;
    }
    gtk_label_set(GTK_LABEL(hpLabel), buf);

    if (u.mtimedone) {
        /* special case: when polymorphed, show "HD", disable exp */
        sprintf(buf, "HD:%d", mons[u.umonnum].mlevel);
        if (lastLevel < mons[u.umonnum].mlevel && firstTime == FALSE) {
            /* Ok, this changed so add it to the highlighing list */
            ghack_highlight_widget(levlLabel, normalStyle, greenStyle);
        } else if (lastLevel > mons[u.umonnum].mlevel && firstTime == FALSE) {
            /* Ok, this changed so add it to the highlighing list */
            ghack_highlight_widget(levlLabel, normalStyle, redStyle);
        }
        lastLevel = mons[u.umonnum].mlevel;
    } else {
        sprintf(buf, "Level:%d", u.ulevel);
        if (lastLevel < u.ulevel && firstTime == FALSE) {
            /* Ok, this changed so add it to the highlighing list */
            ghack_highlight_widget(levlLabel, normalStyle, greenStyle);
        } else if (lastLevel > u.ulevel && firstTime == FALSE) {
            /* Ok, this changed so add it to the highlighing list */
            ghack_highlight_widget(levlLabel, normalStyle, redStyle);
        }
        lastLevel = u.ulevel;
    }
    gtk_label_set(GTK_LABEL(levlLabel), buf);

    sprintf(buf, "Power:%d/%d", u.uen, u.uenmax);
    if ((lastPOW < u.uen || lastMPOW < u.uenmax) && firstTime == FALSE) {
        /* Ok, this changed so add it to the highlighing list */
        ghack_highlight_widget(powLabel, normalStyle, greenStyle);
    }
    if ((lastPOW > u.uen || lastMPOW > u.uenmax) && firstTime == FALSE) {
        /* Ok, this changed so add it to the highlighing list */
        ghack_highlight_widget(powLabel, normalStyle, redStyle);
    }
    lastPOW = u.uen;
    lastMPOW = u.uenmax;
    gtk_label_set(GTK_LABEL(powLabel), buf);

    sprintf(buf, "AC:%d", u.uac);
    if (lastAC > u.uac && firstTime == FALSE) {
        /* Ok, this changed so add it to the highlighing list */
        ghack_highlight_widget(acLabel, normalStyle, greenStyle);
    } else if (lastAC < u.uac && firstTime == FALSE) {
        /* Ok, this changed so add it to the highlighing list */
        ghack_highlight_widget(acLabel, normalStyle, redStyle);
    }
    lastAC = u.uac;
    gtk_label_set(GTK_LABEL(acLabel), buf);

    if (flags.showexp) {
        sprintf(buf, "Exp:%ld", u.uexp);
        if (lastExp < u.uexp && firstTime == FALSE) {
            /* Ok, this changed so add it to the highlighing list */
            ghack_highlight_widget(expLabel, normalStyle, greenStyle);
        } else if (lastExp > u.uexp && firstTime == FALSE) {
            /* Ok, this changed so add it to the highlighing list */
            ghack_highlight_widget(expLabel, normalStyle, redStyle);
        }
        lastExp = u.uexp;
        gtk_label_set(GTK_LABEL(expLabel), buf);
    } else {
        gtk_label_set(GTK_LABEL(expLabel), "");
    }

    if (flags.time) {
        sprintf(buf, "Time:%ld", gm.moves);
        gtk_label_set(GTK_LABEL(timeLabel), buf);
    } else
        gtk_label_set(GTK_LABEL(timeLabel), "");
#ifdef SCORE_ON_BOTL
    if (flags.showscore) {
        sprintf(buf, "Score:%ld", botl_score());
        gtk_label_set(GTK_LABEL(scoreLabel), buf);
    } else
        gtk_label_set(GTK_LABEL(scoreLabel), "");
#else
    {
        gtk_label_set(GTK_LABEL(scoreLabel), "");
    }
#endif

    /* See if their alignment has changed */
    if (lastAlignment != u.ualign.type) {
        if (firstTime == FALSE) {
            /* Ok, this changed so add it to the highlighing list */
            ghack_highlight_widget(alignLabel, normalStyle, redStyle);
        }

        lastAlignment = u.ualign.type;
        /* looks like their alignment has changed -- change out the icon */
        if (u.ualign.type == A_CHAOTIC) {
            gtk_label_set(GTK_LABEL(alignLabel), "Chaotic");
            gnome_pixmap_load_xpm_d(GNOME_PIXMAP(alignPix), chaotic_xpm);
        } else if (u.ualign.type == A_NEUTRAL) {
            gtk_label_set(GTK_LABEL(alignLabel), "Neutral");
            gnome_pixmap_load_xpm_d(GNOME_PIXMAP(alignPix), neutral_xpm);
        } else {
            gtk_label_set(GTK_LABEL(alignLabel), "Lawful");
            gnome_pixmap_load_xpm_d(GNOME_PIXMAP(alignPix), lawful_xpm);
        }
    }

    hung = hu_stat[u.uhs];
    if (lastHungr != u.uhs) {
        if (firstTime == FALSE) {
            /* Ok, this changed so add it to the highlighing list */
            ghack_highlight_widget(hungerLabel, normalStyle, redStyle);
        }

        lastHungr = u.uhs;
        if (hung[0] == ' ') {
            gtk_label_set(GTK_LABEL(hungerLabel), "      ");
            gnome_pixmap_load_xpm_d(GNOME_PIXMAP(hungerPix), nothing_xpm);
        } else if (u.uhs == 0 /* SATIATED */) {
            gtk_label_set(GTK_LABEL(hungerLabel), hung);
            gnome_pixmap_load_xpm_d(GNOME_PIXMAP(hungerPix), satiated_xpm);
        } else {
            gtk_label_set(GTK_LABEL(hungerLabel), hung);
            gnome_pixmap_load_xpm_d(GNOME_PIXMAP(hungerPix), hungry_xpm);
        }
    }

    if (lastConf != Confusion) {
        if (firstTime == FALSE) {
            /* Ok, this changed so add it to the highlighing list */
            ghack_highlight_widget(confuLabel, normalStyle, redStyle);
        }

        lastConf = Confusion;
        if (Confusion) {
            gtk_label_set(GTK_LABEL(confuLabel), "Confused");
            gnome_pixmap_load_xpm_d(GNOME_PIXMAP(confuPix), confused_xpm);
        } else {
            gtk_label_set(GTK_LABEL(confuLabel), "        ");
            gnome_pixmap_load_xpm_d(GNOME_PIXMAP(confuPix), nothing_xpm);
        }
    }

    if (lastBlind != Blind) {
        if (firstTime == FALSE) {
            /* Ok, this changed so add it to the highlighing list */
            ghack_highlight_widget(blindLabel, normalStyle, redStyle);
        }

        lastBlind = Blind;
        if (Blind) {
            gtk_label_set(GTK_LABEL(blindLabel), "Blind");
            gnome_pixmap_load_xpm_d(GNOME_PIXMAP(blindPix), blind_xpm);
        } else {
            gtk_label_set(GTK_LABEL(blindLabel), "     ");
            gnome_pixmap_load_xpm_d(GNOME_PIXMAP(blindPix), nothing_xpm);
        }
    }
    if (lastStun != Stunned) {
        if (firstTime == FALSE) {
            /* Ok, this changed so add it to the highlighing list */
            ghack_highlight_widget(stunLabel, normalStyle, redStyle);
        }

        lastStun = Stunned;
        if (Stunned) {
            gtk_label_set(GTK_LABEL(stunLabel), "Stun");
            gnome_pixmap_load_xpm_d(GNOME_PIXMAP(stunPix), stunned_xpm);
        } else {
            gtk_label_set(GTK_LABEL(stunLabel), "    ");
            gnome_pixmap_load_xpm_d(GNOME_PIXMAP(stunPix), nothing_xpm);
        }
    }

    if (lastHalu != Hallucination) {
        if (firstTime == FALSE) {
            /* Ok, this changed so add it to the highlighing list */
            ghack_highlight_widget(halluLabel, normalStyle, redStyle);
        }

        lastHalu = Hallucination;
        if (Hallucination) {
            gtk_label_set(GTK_LABEL(halluLabel), "Hallu");
            gnome_pixmap_load_xpm_d(GNOME_PIXMAP(halluPix), hallu_xpm);
        } else {
            gtk_label_set(GTK_LABEL(halluLabel), "     ");
            gnome_pixmap_load_xpm_d(GNOME_PIXMAP(halluPix), nothing_xpm);
        }
    }

    if (lastSick != Sick) {
        if (firstTime == FALSE) {
            /* Ok, this changed so add it to the highlighing list */
            ghack_highlight_widget(sickLabel, normalStyle, redStyle);
        }

        lastSick = Sick;
        if (Sick) {
            if (u.usick_type & SICK_VOMITABLE) {
                gtk_label_set(GTK_LABEL(sickLabel), "FoodPois");
                gnome_pixmap_load_xpm_d(GNOME_PIXMAP(sickPix), sick_fp_xpm);
            } else if (u.usick_type & SICK_NONVOMITABLE) {
                gtk_label_set(GTK_LABEL(sickLabel), "Ill");
                gnome_pixmap_load_xpm_d(GNOME_PIXMAP(sickPix), sick_il_xpm);
            } else {
                gtk_label_set(GTK_LABEL(sickLabel), "FoodPois");
                gnome_pixmap_load_xpm_d(GNOME_PIXMAP(sickPix), sick_fp_xpm);
            }
        } else {
            gtk_label_set(GTK_LABEL(sickLabel), "        ");
            gnome_pixmap_load_xpm_d(GNOME_PIXMAP(sickPix), nothing_xpm);
        }
    }

    enc = enc_stat[near_capacity()];
    if (lastEncumb != near_capacity()) {
        if (firstTime == FALSE) {
            /* Ok, this changed so add it to the highlighing list */
            ghack_highlight_widget(encumbLabel, normalStyle, redStyle);
        }

        lastEncumb = near_capacity();
        switch (lastEncumb) {
        case 0:
            gtk_label_set(GTK_LABEL(encumbLabel), "        ");
            gnome_pixmap_load_xpm_d(GNOME_PIXMAP(encumbPix), nothing_xpm);
            break;
        case 1:
            gtk_label_set(GTK_LABEL(encumbLabel), enc);
            gnome_pixmap_load_xpm_d(GNOME_PIXMAP(encumbPix), slt_enc_xpm);
            break;
        case 2:
            gtk_label_set(GTK_LABEL(encumbLabel), enc);
            gnome_pixmap_load_xpm_d(GNOME_PIXMAP(encumbPix), mod_enc_xpm);
            break;
        case 3:
            gtk_label_set(GTK_LABEL(encumbLabel), enc);
            gnome_pixmap_load_xpm_d(GNOME_PIXMAP(encumbPix), hvy_enc_xpm);
            break;
        case 4:
            gtk_label_set(GTK_LABEL(encumbLabel), enc);
            gnome_pixmap_load_xpm_d(GNOME_PIXMAP(encumbPix), ext_enc_xpm);
            break;
        case 5:
            gtk_label_set(GTK_LABEL(encumbLabel), enc);
            gnome_pixmap_load_xpm_d(GNOME_PIXMAP(encumbPix), ovr_enc_xpm);
        }
    }
    firstTime = FALSE;
}

static void
ghack_fade_highlighting()
{
    GList *item;
    Highlight *highlt;

    /* Remove any items from the queue if their time is up */
    for (item = g_list_first(s_HighLightList); item;) {
        highlt = (Highlight *) item->data;
        if (highlt) {
            if (highlt->nTurnsLeft <= 0) {
                gtk_widget_set_style(GTK_WIDGET(highlt->widget),
                                     highlt->oldStyle);
                s_HighLightList = g_list_remove_link(s_HighLightList, item);
                g_free(highlt);
                g_list_free_1(item);
                item = g_list_first(s_HighLightList);
                continue;
            } else
                (highlt->nTurnsLeft)--;
        }
        if (item)
            item = item->next;
        else
            break;
    }
}

/* Widget changed, so add it to the highlighing list */
static void
ghack_highlight_widget(GtkWidget *widget, GtkStyle *oldStyle,
                       GtkStyle *newStyle)
{
    Highlight *highlt;
    GList *item;

    /* Check if this widget is already in the queue.  If so then
     * remove it, so we will only have the new entry in the queue  */
    for (item = g_list_first(s_HighLightList); item;) {
        highlt = (Highlight *) item->data;
        if (highlt) {
            if (highlt->widget == widget) {
                s_HighLightList = g_list_remove_link(s_HighLightList, item);
                g_free(highlt);
                g_list_free_1(item);
                break;
            }
        }
        if (item)
            item = item->next;
        else
            break;
    }

    /* Ok, now highlight this widget and add it into the fade
     * highlighting queue  */
    highlt = g_new(Highlight, 1);
    highlt->nTurnsLeft = NUM_TURNS_HIGHLIGHTED;
    highlt->oldStyle = oldStyle;
    highlt->widget = widget;
    s_HighLightList = g_list_prepend(s_HighLightList, highlt);
    gtk_widget_set_style(GTK_WIDGET(widget), newStyle);
}
