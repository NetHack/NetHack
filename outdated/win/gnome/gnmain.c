/* NetHack 3.6	gnmain.c	$NHDT-Date: 1432512807 2015/05/25 00:13:27 $  $NHDT-Branch: master $:$NHDT-Revision: 1.15 $ */
/* Copyright (C) 1998 by Erik Andersen <andersee@debian.org> */
/* NetHack may be freely redistributed.  See license for details. */

#include "gnmain.h"
#include "gnsignal.h"
#include "gnbind.h"
#include "gnopts.h"
#include <gnome.h>
#include <getopt.h>
#include <gdk/gdk.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include "hack.h"
#include "date.h"

static GtkWidget *mainWindow = NULL;
static GtkWidget *about = NULL;
static GtkWidget *hBoxFirstRow;
static GtkWidget *vBoxMain;

int restarted = 0;
int os_x = 0, os_y = 0, os_w = 0, os_h = 0;

static GnomeClient *session_id;

static void
ghack_quit_game(GtkWidget *widget, int button)
{
    gtk_widget_hide(widget);
    if (button == 0) {
        gnome_exit_nhwindows(0);
        gtk_object_unref(GTK_OBJECT(session_id));
        clearlocks();
        gtk_exit(0);
    }
}

static void
ghack_quit_game_cb(GtkWidget *widget, gpointer data)
{
    GtkWidget *box;
    box = gnome_message_box_new(
        _("Do you really want to quit?"), GNOME_MESSAGE_BOX_QUESTION,
        GNOME_STOCK_BUTTON_YES, GNOME_STOCK_BUTTON_NO, NULL);
    gnome_dialog_set_default(GNOME_DIALOG(box), 1);
    gnome_dialog_set_parent(GNOME_DIALOG(box),
                            GTK_WINDOW(ghack_get_main_window()));
    gnome_dialog_set_accelerator(GNOME_DIALOG(box), 1, 'n', 0);
    gnome_dialog_set_accelerator(GNOME_DIALOG(box), 0, 'y', 0);

    gtk_window_set_modal(GTK_WINDOW(box), TRUE);
    gtk_signal_connect(GTK_OBJECT(box), "clicked",
                       (GtkSignalFunc) ghack_quit_game, NULL);
    gtk_widget_show(box);
}

static void
ghack_save_game(GtkWidget *widget, int button)
{
    gtk_widget_hide(widget);
    if (button == 0) {
        if (dosave0()) {
            /* make sure they see the Saving message */
            display_nhwindow(WIN_MESSAGE, TRUE);
            gnome_exit_nhwindows("Be seeing you...");
            clearlocks();
            gtk_exit(0);
        } else
            (void) doredraw();
    }
}

void
ghack_save_game_cb(GtkWidget *widget, gpointer data)
{
    GtkWidget *box;
    box = gnome_message_box_new(
        _("Quit and save the current game?"), GNOME_MESSAGE_BOX_QUESTION,
        GNOME_STOCK_BUTTON_YES, GNOME_STOCK_BUTTON_NO, NULL);
    gnome_dialog_set_default(GNOME_DIALOG(box), 1);
    gnome_dialog_set_parent(GNOME_DIALOG(box),
                            GTK_WINDOW(ghack_get_main_window()));
    gnome_dialog_set_accelerator(GNOME_DIALOG(box), 1, 'n', 0);
    gnome_dialog_set_accelerator(GNOME_DIALOG(box), 0, 'y', 0);

    gtk_window_set_modal(GTK_WINDOW(box), TRUE);
    gtk_signal_connect(GTK_OBJECT(box), "clicked",
                       (GtkSignalFunc) ghack_save_game, NULL);
    gtk_widget_show(box);
}

static void
ghack_new_game(GtkWidget *widget, int button)
{
    if (button == 0) {
        g_message("This feature is not yet implemented.  Sorry.");
    }
}

static void
ghack_new_game_cb(GtkWidget *widget, gpointer data)
{
    GtkWidget *box;
    box = gnome_message_box_new(
        _("Start a new game?"), GNOME_MESSAGE_BOX_QUESTION,
        GNOME_STOCK_BUTTON_YES, GNOME_STOCK_BUTTON_NO, NULL);
    gnome_dialog_set_default(GNOME_DIALOG(box), 1);
    gnome_dialog_set_parent(GNOME_DIALOG(box),
                            GTK_WINDOW(ghack_get_main_window()));
    gnome_dialog_set_accelerator(GNOME_DIALOG(box), 1, 'n', 0);
    gnome_dialog_set_accelerator(GNOME_DIALOG(box), 0, 'y', 0);

    gtk_window_set_modal(GTK_WINDOW(box), TRUE);
    gtk_signal_connect(GTK_OBJECT(box), "clicked",
                       (GtkSignalFunc) ghack_new_game, NULL);
    gtk_widget_show(box);
}

static void
about_destroy_callback(void)
{
    about = NULL;
}

static void
ghack_about_cb(GtkWidget *widget, gpointer data)
{
    char buf[BUFSZ] = "\0";
    char buf1[BUFSZ] = "\0";
    const gchar *authors[] = { "Erik Andersen", "Anthony Taylor",
                               "Jeff Garzik", "The Nethack Dev Team", NULL };

    if (about) {
        gdk_window_raise(about->window);
        return;
    }

    getversionstring(buf);
#if 0
/* XXX this needs further re-write to use DEVTEAM_EMAIL, DEVTEAM_URL,
 * sysopt.support, etc.   I'm not doing it now because I can't test
 * it yet. (keni)
 */
/* out of date first cut: */
!     len = strlen(buf);
!     char *str1 = _("\nSend comments and bug reports to:\n");
!     len += strlen(str1);
!     len += sysopt.email;
!     char *str2 = _("\nThis game is free software. See License for details.");
!     len += strlen(str2);
!     str = (char*)alloc(len+1);
!     strcat(buf, str1);
!     strcat(buf, sysopt.email);
!     strcat(buf, str2);
free(str) below
#else
    strcat(buf1, VERSION_STRING);
    strcat(buf,
           _("\nSend comments and bug reports to: nethack-bugs@nethack.org\n"
             "This game is free software. See License for details."));
#endif
    about = gnome_about_new(_("Nethack"), buf1,
                            "Copyright (C) 1985-2002 Mike Stephenson",
                            (const char **) authors, buf, NULL);

    gtk_signal_connect(GTK_OBJECT(about), "destroy",
                       (GtkSignalFunc) about_destroy_callback, NULL);

    gtk_widget_show(about);
}

static void
ghack_settings_cb(GtkWidget *widget, gpointer data)
{
    ghack_settings_dialog();
}

static void
ghack_accelerator_selected(GtkWidget *widget, gpointer data)
{
    GdkEventKey event;
    int key = GPOINTER_TO_INT(data);
    /* g_message("An accelerator for \"%c\" was selected", key); */
    /* stuff a key directly into the keybuffer */
    event.state = 0;
    event.keyval = key;
    ghack_handle_key_press(NULL, &event, NULL);
}

#ifndef M
#ifndef NHSTDC
#define M(c) (0x80 | (c))
#else
#define M(c) ((c) -128)
#endif /* NHSTDC */
#endif
#ifndef C
#define C(c) (0x1f & (c))
#endif

GnomeUIInfo game_tree[] = {
    { GNOME_APP_UI_ITEM, N_("_Change Settings..."),
      N_("Change Game Settings"), ghack_settings_cb, NULL, NULL,
      GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
    GNOMEUIINFO_SEPARATOR,
    { GNOME_APP_UI_ITEM, N_("Version"), NULL, ghack_accelerator_selected,
      GINT_TO_POINTER('v'), NULL, GNOME_APP_PIXMAP_STOCK,
      GNOME_STOCK_MENU_ABOUT, 'v', 0 },
    { GNOME_APP_UI_ITEM, N_("History..."), NULL, ghack_accelerator_selected,
      GINT_TO_POINTER('V'), NULL, GNOME_APP_PIXMAP_STOCK,
      GNOME_STOCK_MENU_ABOUT, 'V', GDK_SHIFT_MASK },
    { GNOME_APP_UI_ITEM, N_("Compilation..."), NULL,
      ghack_accelerator_selected, GINT_TO_POINTER(M('v')), NULL,
      GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ABOUT, 'v', GDK_MOD1_MASK },
    { GNOME_APP_UI_ITEM, N_("Options..."), NULL, ghack_accelerator_selected,
      GINT_TO_POINTER('O'), NULL, GNOME_APP_PIXMAP_STOCK,
      GNOME_STOCK_MENU_PREF, 'O', GDK_SHIFT_MASK },
    { GNOME_APP_UI_ITEM, N_("Explore Mode..."), NULL,
      ghack_accelerator_selected, GINT_TO_POINTER('X'), NULL,
      GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_QUIT, 'X', GDK_SHIFT_MASK },
    GNOMEUIINFO_SEPARATOR,
    GNOMEUIINFO_MENU_NEW_GAME_ITEM(ghack_new_game_cb, NULL),
    GNOMEUIINFO_MENU_SAVE_ITEM(ghack_save_game_cb, NULL),
    { GNOME_APP_UI_ITEM, N_("Exit"), NULL, ghack_quit_game_cb,
      GINT_TO_POINTER(M('Q')), NULL, GNOME_APP_PIXMAP_STOCK,
      GNOME_STOCK_MENU_ABOUT, 'Q', GDK_MOD1_MASK },
    GNOMEUIINFO_END
};

GnomeUIInfo edit_menu[] = {
    { GNOME_APP_UI_ITEM, N_("Inventory"), N_("Edit/View your Inventory"),
      ghack_accelerator_selected, GINT_TO_POINTER('i'), NULL,
      GNOME_APP_PIXMAP_NONE, NULL, 'i', 0 },
    { GNOME_APP_UI_ITEM, N_("Discoveries"), N_("Edit/View your Discoveries"),
      ghack_accelerator_selected, GINT_TO_POINTER('\\'), NULL,
      GNOME_APP_PIXMAP_NONE, NULL, '\\', 0 },
    { GNOME_APP_UI_ITEM, N_("List/reorder your spells"),
      N_("List/reorder your spells"), ghack_accelerator_selected,
      GINT_TO_POINTER('x'), NULL, GNOME_APP_PIXMAP_NONE, NULL, 'x', 0 },
    { GNOME_APP_UI_ITEM, N_("Adjust letters"),
      N_("Adjust letter for items in your Inventory"),
      ghack_accelerator_selected, GINT_TO_POINTER(M('a')), NULL,
      GNOME_APP_PIXMAP_NONE, NULL, 'a', GDK_MOD1_MASK },
    GNOMEUIINFO_SEPARATOR,
    { GNOME_APP_UI_ITEM, N_("Name object"), N_("Assign a name to an object"),
      ghack_accelerator_selected, GINT_TO_POINTER(M('n')), NULL,
      GNOME_APP_PIXMAP_NONE, NULL, 'n', GDK_MOD1_MASK },
    { GNOME_APP_UI_ITEM, N_("Name creature"),
      N_("Assign a name to a creature"), ghack_accelerator_selected,
      GINT_TO_POINTER('C'), NULL, GNOME_APP_PIXMAP_NONE, NULL, 'C',
      GDK_SHIFT_MASK },
    GNOMEUIINFO_SEPARATOR,
    { GNOME_APP_UI_ITEM, N_("Qualifications"), N_("Edit your Qualifications"),
      ghack_accelerator_selected, GINT_TO_POINTER(M('e')), NULL,
      GNOME_APP_PIXMAP_NONE, NULL, 'e', GDK_MOD1_MASK },
    GNOMEUIINFO_END
};

GnomeUIInfo apparel_menu[] = {
    { GNOME_APP_UI_ITEM, N_("Wield Weapon"),
      N_("Select a weapon to fight with"), ghack_accelerator_selected,
      GINT_TO_POINTER('w'), NULL, GNOME_APP_PIXMAP_NONE, NULL, 'w', 0 },
    { GNOME_APP_UI_ITEM, N_("Remove Apparel..."),
      N_("Remove apparel dialog bog"), ghack_accelerator_selected,
      GINT_TO_POINTER('A'), NULL, GNOME_APP_PIXMAP_NONE, NULL, 'A',
      GDK_SHIFT_MASK },
    GNOMEUIINFO_SEPARATOR,
    { GNOME_APP_UI_ITEM, N_("Wear Armor"), N_("Put on armor"),
      ghack_accelerator_selected, GINT_TO_POINTER('W'), NULL,
      GNOME_APP_PIXMAP_NONE, NULL, 'W', GDK_SHIFT_MASK },
    { GNOME_APP_UI_ITEM, N_("Take off Armor"),
      N_("Take off armor you are wearing"), ghack_accelerator_selected,
      GINT_TO_POINTER('T'), NULL, GNOME_APP_PIXMAP_NONE, NULL, 'T',
      GDK_SHIFT_MASK },
    GNOMEUIINFO_SEPARATOR,
    { GNOME_APP_UI_ITEM, N_("Put on non-armor"),
      N_("Put on non-armor apparel"), ghack_accelerator_selected,
      GINT_TO_POINTER('P'), NULL, GNOME_APP_PIXMAP_NONE, NULL, 'P',
      GDK_SHIFT_MASK },
    { GNOME_APP_UI_ITEM, N_("Remove non-armor"),
      N_("Remove non-armor apparel you are wearing"),
      ghack_accelerator_selected, GINT_TO_POINTER('R'), NULL,
      GNOME_APP_PIXMAP_NONE, NULL, 'R', GDK_SHIFT_MASK },
    GNOMEUIINFO_END
};

GnomeUIInfo action_menu[] = {
    { GNOME_APP_UI_ITEM, N_("Get"),
      N_("Pick up things at the current location"),
      ghack_accelerator_selected, GINT_TO_POINTER(','), NULL,
      GNOME_APP_PIXMAP_NONE, NULL, ',', 0 },
    { GNOME_APP_UI_ITEM, N_("Loot"), N_("loot a box on the floor"),
      ghack_accelerator_selected, GINT_TO_POINTER(M('l')), NULL,
      GNOME_APP_PIXMAP_NONE, NULL, 'l', GDK_MOD1_MASK },
    { GNOME_APP_UI_ITEM, N_("Sit"), N_("sit down"),
      ghack_accelerator_selected, GINT_TO_POINTER(M('s')), NULL,
      GNOME_APP_PIXMAP_NONE, NULL, 's', GDK_MOD1_MASK },
    { GNOME_APP_UI_ITEM, N_("Force"), N_("force a lock"),
      ghack_accelerator_selected, GINT_TO_POINTER(M('f')), NULL,
      GNOME_APP_PIXMAP_NONE, NULL, 'f', GDK_MOD1_MASK },
    { GNOME_APP_UI_ITEM, N_("Kick"), N_("kick something (usually a door)"),
      ghack_accelerator_selected, GINT_TO_POINTER(C('d')), NULL,
      GNOME_APP_PIXMAP_NONE, NULL, 'd', GDK_CONTROL_MASK },
    { GNOME_APP_UI_ITEM, N_("Jump"), N_("jump to another location"),
      ghack_accelerator_selected, GINT_TO_POINTER(M('j')), NULL,
      GNOME_APP_PIXMAP_NONE, NULL, 'j', GDK_MOD1_MASK },
    { GNOME_APP_UI_ITEM, N_("Ride"), N_("Ride (or stop riding) a monster"),
      doride, GINT_TO_POINTER(M('r')), NULL, GNOME_APP_PIXMAP_NONE, NULL, 'R',
      GDK_MOD1_MASK },
    { GNOME_APP_UI_ITEM, N_("Wipe face"), N_("wipe off your face"),
      ghack_accelerator_selected, GINT_TO_POINTER(M('w')), NULL,
      GNOME_APP_PIXMAP_NONE, NULL, 'w', GDK_MOD1_MASK },
    { GNOME_APP_UI_ITEM, N_("Throw/Shoot"), N_("throw or shoot a weapon"),
      ghack_accelerator_selected, GINT_TO_POINTER('t'), NULL,
      GNOME_APP_PIXMAP_NONE, NULL, 't', 0 },
    {
        GNOME_APP_UI_ITEM, N_("Quiver/Ready"),
        N_("ready or quiver some ammunition"), ghack_accelerator_selected,
        GINT_TO_POINTER('Q'), NULL, GNOME_APP_PIXMAP_NONE, NULL, 'Q',
        GDK_SHIFT_MASK,
    },
    { GNOME_APP_UI_ITEM, N_("Open Door"), N_("open a door"),
      ghack_accelerator_selected, GINT_TO_POINTER('o'), NULL,
      GNOME_APP_PIXMAP_NONE, NULL, 'o', 0 },
    { GNOME_APP_UI_ITEM, N_("Close Door"), N_("open a door"),
      ghack_accelerator_selected, GINT_TO_POINTER('c'), NULL,
      GNOME_APP_PIXMAP_NONE, NULL, 'c', 0 },
    GNOMEUIINFO_SEPARATOR,
    { GNOME_APP_UI_ITEM, N_("Drop"), N_("drop an object"),
      ghack_accelerator_selected, GINT_TO_POINTER('d'), NULL,
      GNOME_APP_PIXMAP_NONE, NULL, 'd', 0 },
    { GNOME_APP_UI_ITEM, N_("Drop Many"),
      N_("drop selected types of objects"), ghack_accelerator_selected,
      GINT_TO_POINTER('D'), NULL, GNOME_APP_PIXMAP_NONE, NULL, 'D',
      GDK_SHIFT_MASK },
    { GNOME_APP_UI_ITEM, N_("Eat"), N_("eat something"),
      ghack_accelerator_selected, GINT_TO_POINTER('e'), NULL,
      GNOME_APP_PIXMAP_NONE, NULL, 'e', 0 },
    { GNOME_APP_UI_ITEM, N_("Engrave"),
      N_("write a message in the dust on the floor"),
      ghack_accelerator_selected, GINT_TO_POINTER('E'), NULL,
      GNOME_APP_PIXMAP_NONE, NULL, 'E', GDK_SHIFT_MASK },
    { GNOME_APP_UI_ITEM, N_("Apply"),
      N_("apply or use a tool (pick-axe, key, camera, etc.)"),
      ghack_accelerator_selected, GINT_TO_POINTER('a'), NULL,
      GNOME_APP_PIXMAP_NONE, NULL, 'a', 0 },
    GNOMEUIINFO_SEPARATOR,
    { GNOME_APP_UI_ITEM, N_("Up"), N_("go up the stairs"),
      ghack_accelerator_selected, GINT_TO_POINTER('<'), NULL,
      GNOME_APP_PIXMAP_NONE, NULL, '<', 0 },
    { GNOME_APP_UI_ITEM, N_("Down"), N_("go down the stairs"),
      ghack_accelerator_selected, GINT_TO_POINTER('>'), NULL,
      GNOME_APP_PIXMAP_NONE, NULL, '>', 0 },
    { GNOME_APP_UI_ITEM, N_("Rest"), N_("wait for a moment"),
      ghack_accelerator_selected, GINT_TO_POINTER('.'), NULL,
      GNOME_APP_PIXMAP_NONE, NULL, '.', 0 },
    { GNOME_APP_UI_ITEM, N_("Search"),
      N_("search for secret doors, hidden traps and monsters"),
      ghack_accelerator_selected, GINT_TO_POINTER('s'), NULL,
      GNOME_APP_PIXMAP_NONE, NULL, 's', 0 },
    GNOMEUIINFO_SEPARATOR,
    { GNOME_APP_UI_ITEM, N_("Chat"), N_("talk to someone"),
      ghack_accelerator_selected, GINT_TO_POINTER(M('c')), NULL,
      GNOME_APP_PIXMAP_NONE, NULL, 'c', GDK_MOD1_MASK },
    { GNOME_APP_UI_ITEM, N_("Pay"), N_("pay your bill to the shopkeeper"),
      ghack_accelerator_selected, GINT_TO_POINTER('p'), NULL,
      GNOME_APP_PIXMAP_NONE, NULL, 'p', 0 },
    GNOMEUIINFO_END
};

GnomeUIInfo magic_menu[] = {
    { GNOME_APP_UI_ITEM, N_("Quaff potion"), N_("drink a potion"),
      ghack_accelerator_selected, GINT_TO_POINTER('q'), NULL,
      GNOME_APP_PIXMAP_NONE, NULL, 'q', 0 },
    { GNOME_APP_UI_ITEM, N_("Read Book/Scroll"),
      N_("read a spell book or a scroll"), ghack_accelerator_selected,
      GINT_TO_POINTER('r'), NULL, GNOME_APP_PIXMAP_NONE, NULL, 'r', 0 },
    { GNOME_APP_UI_ITEM, N_("Zap Wand"), N_("zap a wand"),
      ghack_accelerator_selected, GINT_TO_POINTER('z'), NULL,
      GNOME_APP_PIXMAP_NONE, NULL, 'z', 0 },
    { GNOME_APP_UI_ITEM, N_("Zap Spell"), N_("cast a spell"),
      ghack_accelerator_selected, GINT_TO_POINTER('Z'), NULL,
      GNOME_APP_PIXMAP_NONE, NULL, 'Z', GDK_SHIFT_MASK },
    GNOMEUIINFO_SEPARATOR,
    { GNOME_APP_UI_ITEM, N_("Dip"), N_("dip an object into something"),
      ghack_accelerator_selected, GINT_TO_POINTER(M('d')), NULL,
      GNOME_APP_PIXMAP_NONE, NULL, 'd', GDK_MOD1_MASK },
    { GNOME_APP_UI_ITEM, N_("Rub"), N_("Rub something (i.e. a lamp)"),
      ghack_accelerator_selected, GINT_TO_POINTER(M('r')), NULL,
      GNOME_APP_PIXMAP_NONE, NULL, 'r', GDK_MOD1_MASK },
    { GNOME_APP_UI_ITEM, N_("Invoke"),
      N_("invoke an object's special powers"), ghack_accelerator_selected,
      GINT_TO_POINTER(M('i')), NULL, GNOME_APP_PIXMAP_NONE, NULL, 'i',
      GDK_MOD1_MASK },
    GNOMEUIINFO_SEPARATOR,
    { GNOME_APP_UI_ITEM, N_("Offer"), N_("offer a sacrifice to the gods"),
      ghack_accelerator_selected, GINT_TO_POINTER(M('o')), NULL,
      GNOME_APP_PIXMAP_NONE, NULL, 'o', GDK_MOD1_MASK },
    { GNOME_APP_UI_ITEM, N_("Pray"), N_("pray to the gods for help"),
      ghack_accelerator_selected, GINT_TO_POINTER(M('p')), NULL,
      GNOME_APP_PIXMAP_NONE, NULL, 'p', GDK_MOD1_MASK },
    GNOMEUIINFO_SEPARATOR,
    { GNOME_APP_UI_ITEM, N_("Teleport"), N_("teleport (if you can)"),
      ghack_accelerator_selected, GINT_TO_POINTER(C('t')), NULL,
      GNOME_APP_PIXMAP_NONE, NULL, 't', GDK_CONTROL_MASK },
    { GNOME_APP_UI_ITEM, N_("Monster Action"),
      N_("use a monster's special ability"), ghack_accelerator_selected,
      GINT_TO_POINTER(M('m')), NULL, GNOME_APP_PIXMAP_NONE, NULL, 'm',
      GDK_MOD1_MASK },
    { GNOME_APP_UI_ITEM, N_("Turn Undead"), N_("turn undead"),
      ghack_accelerator_selected, GINT_TO_POINTER(M('t')), NULL,
      GNOME_APP_PIXMAP_NONE, NULL, 't', GDK_MOD1_MASK },
    GNOMEUIINFO_END
};

GnomeUIInfo help_menu[] = {
    { GNOME_APP_UI_ITEM, N_("About..."), N_("About GnomeHack"),
      ghack_about_cb, NULL, NULL, GNOME_APP_PIXMAP_STOCK,
      GNOME_STOCK_MENU_ABOUT, 0, 0, NULL },
    { GNOME_APP_UI_ITEM, N_("Help"), NULL, ghack_accelerator_selected,
      GINT_TO_POINTER('?'), NULL, GNOME_APP_PIXMAP_STOCK,
      GNOME_STOCK_MENU_ABOUT, '?', 0 },
    GNOMEUIINFO_SEPARATOR,
    { GNOME_APP_UI_ITEM, N_("What is here"),
      N_("Check what items occupy the current location"),
      ghack_accelerator_selected, GINT_TO_POINTER(':'), NULL,
      GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ABOUT, ':', 0 },
    { GNOME_APP_UI_ITEM, N_("What is that"), N_("Identify an object"),
      ghack_accelerator_selected, GINT_TO_POINTER(';'), NULL,
      GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ABOUT, ';', 0 },
    { GNOME_APP_UI_ITEM, N_("Identify a map symbol"),
      N_("Identify a map symbol"), ghack_accelerator_selected,
      GINT_TO_POINTER('/'), NULL, GNOME_APP_PIXMAP_STOCK,
      GNOME_STOCK_MENU_ABOUT, '/', 0 },
    GNOMEUIINFO_END
};

GnomeUIInfo mainmenu[] = { GNOMEUIINFO_MENU_GAME_TREE(game_tree),
                           GNOMEUIINFO_MENU_EDIT_TREE(edit_menu),
                           { GNOME_APP_UI_SUBTREE, N_("Apparel"), NULL,
                             apparel_menu, NULL, NULL, 0, NULL, 0, 0, NULL },
                           { GNOME_APP_UI_SUBTREE, N_("Action"), NULL,
                             action_menu, NULL, NULL, 0, NULL, 0, 0, NULL },
                           { GNOME_APP_UI_SUBTREE, N_("Magic"), NULL,
                             magic_menu, NULL, NULL, 0, NULL, 0, 0, NULL },
                           GNOMEUIINFO_MENU_HELP_TREE(help_menu),
                           GNOMEUIINFO_END };

static void
ghack_main_window_key_press(GtkWidget *widget, GdkEventKey *event,
                            gpointer data)
{
    /* First, turn off the key press propogation.  We've got the
     * key, but we don't wan't the underlying Gtk widgets to get it,
     * since they do the wrong thing with the arrow keys (shift focus)... */
    gtk_signal_emit_stop_by_name(GTK_OBJECT(mainWindow), "key_press_event");

    /* stuff the key event into the keybuffer */
    ghack_handle_key_press(widget, event, data);
}

/* parsing args */
void
parse_args(int argc, char *argv[])
{
    gint ch;

    struct option options[] = { /* Default args */
                                { "help", no_argument, NULL, 'h' },
                                { "version", no_argument, NULL, 'v' },

                                { NULL, 0, NULL, 0 }
    };

    gchar *id = NULL;

    /* initialize getopt */
    optarg = NULL;
    optind = 0;
    optopt = 0;

    while ((ch = getopt_long(argc, argv, "hv", options, NULL)) != EOF) {
        switch (ch) {
        case 'h':
            g_print(
                _("%s: A gnomified 'Hello World' program\n\n"
                  "Usage: %s [--help] [--version]\n\n"
                  "Options:\n"
                  "        --help     display this help and exit\n"
                  "        --version  output version information and exit\n"),
                argv[0], argv[0]);
            exit(0);
            break;
        case 'v':
            g_print(_("NetHack %s.\n"), VERSION_STRING);
            exit(0);
            break;
        case ':':
        case '?':
            g_print(_("Options error\n"));
            exit(0);
            break;
        }
    }

    /* SM stuff */
    session_id = gnome_client_new();
#if 0
  session_id = gnome_client_new (
  	/* callback to save the state and parameter for it */
  	save_state, argv[0], 
  	/* callback to die and parameter for it */
    	NULL, NULL,
	/* id from the previous session if restarted, NULL otherwise */
       	id);
#endif
    /* set the program name */
    gnome_client_set_program(session_id, argv[0]);
    g_free(id);

    return;
}

/*
 * [ALI] Gnome installs its own handler(s) for SIGBUS, SIGFPE and SIGSEGV.
 * These handlers will fork and exec a helper program. When that helper
 * comes to initialize GTK+, it may fail if setuid/setgid. We solve this
 * by dropping privileges before passing the signal along the chain.
 * Note: We don't need to either drop or mask the saved ID since this
 * will be reset when the child process performs the execve() anyway.
 */

static struct {
    int signum;
    void (*handler)(int);
} ghack_chain[] = {
    { SIGBUS },
    { SIGFPE },
    { SIGSEGV },
    { SIGILL } /* Not currently handled by Gnome */
};

static void
ghack_sig_handler(int signum)
{
    int i;
    uid_t uid, euid;
    gid_t gid, egid;
    uid = getuid();
    euid = geteuid();
    gid = getgid();
    egid = getegid();
    if (gid != egid)
        setgid(gid);
    if (uid != euid)
        setuid(uid);
    for (i = SIZE(ghack_chain) - 1; i >= 0; i--)
        if (ghack_chain[i].signum == signum) {
            ghack_chain[i].handler(signum);
            break;
        }
    if (i < 0)
        impossible("Unhandled ghack signal");
    if (uid != euid)
        setuid(euid);
    if (gid != egid)
        setgid(egid);
}

/* initialize gnome and fir up the main window */
void
ghack_init_main_window(int argc, char **argv)
{
    int i;
    struct timeval tv;
    uid_t uid, euid;

    /* It seems that the authors of gnome_score_init() drop group
     * priveledges.  We need group priveledges, so until we change the
     * way we save games to do things the gnome way(???), this stays
     * commented out.  (after hours of frusteration...)
     *  -Erik
     */
    /* gnome_score_init("gnomehack"); */

    gettimeofday(&tv, NULL);
    srand(tv.tv_usec);

    uid = getuid();
    euid = geteuid();
    if (uid != euid)
        setuid(uid);
    hide_privileges(TRUE);
    /* XXX gnome_init must print nethack options for --help, but does not */
    gnome_init("nethack", VERSION_STRING, argc, argv);
    hide_privileges(FALSE);
    parse_args(argc, argv);

/* Initialize the i18n stuff (not that gnomehack supperts it yet...) */
#if 0
    textdomain (PACKAGE);
#endif
    gdk_imlib_init();

    /* Main window */
    mainWindow =
        gnome_app_new((char *) "nethack", (char *) N_("Nethack for Gnome"));
    gtk_widget_realize(mainWindow);
    if (restarted) {
        gtk_widget_set_uposition(mainWindow, os_x, os_y);
        gtk_widget_set_usize(mainWindow, os_w, os_h);
    }
    gtk_window_set_default_size(GTK_WINDOW(mainWindow), 800, 600);
    gtk_window_set_policy(GTK_WINDOW(mainWindow), FALSE, TRUE, TRUE);
    gnome_app_create_menus(GNOME_APP(mainWindow), mainmenu);
    gtk_signal_connect(GTK_OBJECT(mainWindow), "key_press_event",
                       GTK_SIGNAL_FUNC(ghack_main_window_key_press), NULL);
    gtk_signal_connect(GTK_OBJECT(mainWindow), "delete_event",
                       GTK_SIGNAL_FUNC(ghack_quit_game_cb), NULL);

    /* Put some stuff into our main window */
    vBoxMain = gtk_vbox_new(FALSE, 0);
    hBoxFirstRow = gtk_hbox_new(FALSE, 0);

    /* pack Boxes into other boxes to produce the right structure */
    gtk_box_pack_start(GTK_BOX(vBoxMain), hBoxFirstRow, FALSE, TRUE, 0);

    /* pack vBoxMain which contains all our widgets into the main window. */
    gnome_app_set_contents(GNOME_APP(mainWindow), vBoxMain);

    /* DONT show the main window yet, due to a Gtk bug that causes it
     * to not refresh the window when adding widgets after the window
     * has already been shown */
    if (uid != euid)
        setuid(euid);
    for (i = 0; i < SIZE(ghack_chain); i++)
        ghack_chain[i].handler =
            signal(ghack_chain[i].signum, ghack_sig_handler);
}

void
ghack_main_window_add_map_window(GtkWidget *win)
{
    GtkWidget *vBox;

    vBox = gtk_vbox_new(TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vBox), win, TRUE, TRUE, 2);
    gtk_box_pack_start(GTK_BOX(vBoxMain), vBox, TRUE, TRUE, 2);
    gtk_widget_show_all(vBox);
    /* Ok, now show the main window -- now that we have added in
     * all the windows (relys on nethack displaying the map window last
     * (This is an ugly kludge, BTW)
     */
    gtk_widget_show_all(mainWindow);
}

void
ghack_main_window_add_message_window(GtkWidget *win)
{
    gtk_box_pack_start(GTK_BOX(hBoxFirstRow), win, TRUE, TRUE, 2);
    gtk_widget_show_all(win);
}

void
ghack_main_window_add_status_window(GtkWidget *win)
{
    gtk_box_pack_start(GTK_BOX(hBoxFirstRow), win, FALSE, TRUE, 2);
    gtk_widget_show_all(win);
}

void
ghack_main_window_add_worn_window(GtkWidget *win)
{
    gtk_box_pack_end(GTK_BOX(hBoxFirstRow), win, FALSE, TRUE, 2);
    gtk_widget_show_all(win);
}

void
ghack_main_window_add_text_window(GtkWidget *win)
{
    g_warning("Fixme!!! AddTextWindow is not yet implemented");
}

void
ghack_main_window_remove_window(GtkWidget *win)
{
    g_warning("Fixme!!! RemoveWindow is not yet implemented");
}

void
ghack_main_window_update_inventory()
{
    /* For now, do very little.  Eventually we may allow the inv. window
         to stay active.  When we do this, we'll need to implement this...
       g_warning("Fixme!!! updateInventory is not yet implemented");
    */
    gnome_display_nhwindow(WIN_WORN, FALSE);
}

GtkWidget *
ghack_get_main_window()
{
    return (GTK_WIDGET(mainWindow));
}
