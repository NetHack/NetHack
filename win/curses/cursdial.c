/* vim:set cin ft=c sw=4 sts=4 ts=8 et ai cino=Ls\:0t0(0 : -*- mode:c;fill-column:80;tab-width:8;c-basic-offset:4;indent-tabs-mode:nil;c-file-style:"k&r" -*-*/
/* NetHack 3.7 cursdial.c */
/* Copyright (c) Karl Garrison, 2010. */
/* NetHack may be freely redistributed.  See license for details. */

#include "curses.h"
#include "hack.h"
#include "wincurs.h"
#include "cursdial.h"
#include "func_tab.h"
#include <ctype.h>

#if defined(FILENAME_CMP) && !defined(strcasecmp)
#define strcasecmp FILENAME_CMP
#endif
#if defined(STRNCMPI) && !defined(strncasecmp)
#define strncasecmp strncmpi
#endif

/* defined in sys/<foo>/<foo>tty.c or cursmain.c as last resort;
   set up by curses_init_nhwindows() */
extern char erase_char, kill_char;

/* Dialog windows for curses interface */

WINDOW *activemenu = NULL; /* for count_window refresh handling */

/* Private declarations */

typedef struct nhmi {
    winid wid;                  /* NetHack window id */
    glyph_info glyphinfo;       /* holds menu glyph and additional info */
    anything identifier;        /* Value returned if item selected */
    char accelerator;           /* Character used to select item from menu */
    char group_accel;           /* Group accelerator for menu item, if any */
    int attr;                   /* Text attributes for item */
    const char *str;            /* Text of menu item */
    boolean presel;             /* Whether menu item should be preselected */
    boolean selected;           /* Whether item is currently selected */
    unsigned itemflags;
    int page_num;               /* Display page number for entry */
    int line_num;               /* Line number on page where entry begins */
    int num_lines;              /* Number of lines entry uses on page */
    int count;                  /* Count for selected item */
    struct nhmi *prev_item;     /* Pointer to previous entry */
    struct nhmi *next_item;     /* Pointer to next entry */
} nhmenu_item;

typedef struct nhm {
    winid wid;                  /* NetHack window id */
    const char *prompt;         /* Menu prompt text */
    nhmenu_item *entries;       /* Menu entries */
    int num_entries;            /* Number of menu entries */
    int num_pages;              /* Number of display pages for menu */
    int height;                 /* Window height of menu */
    int width;                  /* Window width of menu */
    unsigned long mbehavior;    /* menu flags */
    boolean reuse_accels;       /* Non-unique accelerators per page */
    boolean bottom_heavy;       /* display multi-page menu starting at end */
    struct nhm *prev_menu;      /* Pointer to previous menu */
    struct nhm *next_menu;      /* Pointer to next menu */
} nhmenu;

typedef enum menu_op_type {
    SELECT,
    DESELECT,
    INVERT
} menu_op;

static nhmenu_item *curs_new_menu_item(winid, const char *);
static void curs_pad_menu(nhmenu *, boolean);
static nhmenu *get_menu(winid wid);
static char menu_get_accel(boolean first);
static void menu_determine_pages(nhmenu *menu);
static boolean menu_is_multipage(nhmenu *menu, int width, int height);
static void menu_win_size(nhmenu *menu);
#ifdef NCURSES_MOUSE_VERSION
static nhmenu_item *get_menuitem_y(nhmenu *menu, WINDOW *win UNUSED,
                                   int page_num, int liney);
#endif /*NCURSES_MOUSE_VERSION*/
static void menu_display_page(nhmenu *menu, WINDOW *win, int page_num,
                              char *, char *);
static int menu_get_selections(WINDOW *win, nhmenu *menu, int how);
static void menu_select_deselect(WINDOW *win, nhmenu_item *item,
                                 menu_op operation, int);
static int menu_operation(WINDOW *win, nhmenu *menu, menu_op operation,
                          int page_num);
static void menu_clear_selections(nhmenu *menu);
static int menu_max_height(void);

static nhmenu *nhmenus = NULL;  /* NetHack menu array */

/* maximum number of selector entries per page; if '$' is present
   it isn't counted toward maximum, so actual max per page is 53 */
#define MAX_ACCEL_PER_PAGE 52 /* 'a'..'z' + 'A'..'Z' */
/* TODO?  limit per page should be ignored for perm_invent, which might
   have some '#' overflow entries and isn't used to select items */


/* Get a line of text from the player, such as asking for a character name
   or a wish.  Note: EDIT_GETLIN not supported for popup prompting. */

void
curses_line_input_dialog(
    const char *prompt,
    char *answer,
    int buffer)
{
    int map_height, map_width, maxwidth, remaining_buf, winx, winy, count;
    WINDOW *askwin, *bwin;
    char *tmpstr;
    int prompt_width, prompt_height = 1, height = prompt_height,
        answerx = 0, answery = 0, trylim;
    char input[BUFSZ];

    /* if messages were being suppressed for the remainder of the turn,
       re-activate them now that input is being requested */
    curses_got_input();

    if (buffer > (int) sizeof input)
        buffer = (int) sizeof input;
    /* +1: space between prompt and answer; buffer already accounts for \0 */
    prompt_width = (int) strlen(prompt) + 1 + buffer;
    maxwidth = term_cols - 2;

    if (iflags.window_inited) {
        if (!iflags.wc_popup_dialog) {
            curses_message_win_getline(prompt, answer, buffer);
            return;
        }
        curses_get_window_size(MAP_WIN, &map_height, &map_width);
        if ((prompt_width + 2) > map_width)
            maxwidth = map_width - 2;
    }

    if (prompt_width > maxwidth) {
        prompt_height = curses_num_lines(prompt, maxwidth);
        height = prompt_height;
        prompt_width = maxwidth;
        tmpstr = curses_break_str(prompt, maxwidth, prompt_height);
        remaining_buf = buffer - ((int) strlen(tmpstr) - 1);
        if (remaining_buf > 0) {
            height += (remaining_buf / prompt_width);
            if ((remaining_buf % prompt_width) > 0) {
                height++;
            }
        }
        free(tmpstr);
    }

    bwin = curses_create_window(prompt_width, height,
                                iflags.window_inited ? UP : CENTER);
    wrefresh(bwin);
    getbegyx(bwin, winy, winx);
    askwin = newwin(height, prompt_width, winy + 1, winx + 1);

    for (count = 0; count < prompt_height; count++) {
        tmpstr = curses_break_str(prompt, maxwidth, count + 1);
        mvwaddstr(askwin, count, 0, tmpstr);
        if (count == prompt_height - 1) { /* Last line */
            if ((int) strlen(tmpstr) < maxwidth)
                waddch(askwin, ' ');
            else
                wmove(askwin, count + 1, 0);
        }
        free(tmpstr);
        /* remember where user's typed response will start, in case we
           need to re-prompt */
        getyx(askwin, answery, answerx);
    }

    echo();
    curs_set(1);
    trylim = 10;
    do {
        /* move and clear are only needed for 2nd and subsequent passes */
        wmove(askwin, answery, answerx);
        wclrtoeol(askwin);

        wgetnstr(askwin, input, buffer - 1);
        /* ESC after some input kills that input and tries again;
           ESC at the start cancels, leaving ESC in the result buffer.
           [Note: wgetnstr() treats <escape> as an ordinary character
           so user has to type <escape><return> for it to behave the
           way we want it to.] */
        if (input[0] != '\033' && strchr(input, '\033') != 0)
             input[0] = '\0';
    } while (--trylim > 0 && !input[0]);
    curs_set(0);
    Strcpy(answer, input);
    werase(bwin);
    delwin(bwin);
    curses_destroy_win(askwin);
    noecho();
}


/* Get a single character response from the player, such as a y/n prompt */

int
curses_character_input_dialog(
    const char *prompt,
    const char *choices,
    char def)
{
    WINDOW *askwin = NULL;
#ifdef PDCURSES
    WINDOW *message_window;
#endif
    int answer, count, maxwidth, map_height, map_width;
    char *linestr;
    char askstr[BUFSZ + QBUFSZ];
    char choicestr[QBUFSZ];
    int prompt_width;
    int prompt_height = 1;
    boolean any_choice = FALSE;
    boolean accept_count = FALSE;

    /* if messages were being suppressed for the remainder of the turn,
       re-activate them now that input is being requested */
    curses_got_input();

    if (gi.invent || (gm.moves > 1)) {
        curses_get_window_size(MAP_WIN, &map_height, &map_width);
    } else {
        map_height = term_rows;
        map_width = term_cols;
    }

#ifdef PDCURSES
    message_window = curses_get_nhwin(MESSAGE_WIN);
#endif
    maxwidth = map_width - 2;

    if (choices != NULL) {
        for (count = 0; choices[count] != '\0'; count++) {
            if (choices[count] == '#') { /* Accept a count */
                accept_count = TRUE;
            }
        }
        choicestr[0] = ' ';
        choicestr[1] = '[';
        for (count = 0; choices[count] != '\0'; count++) {
            if (choices[count] == '\033') { /* Escape */
                break;
            }
            choicestr[count + 2] = choices[count];
        }
        choicestr[count + 2] = ']';
        if (((def >= 'A') && (def <= 'Z')) || ((def >= 'a') && (def <= 'z'))) {
            choicestr[count + 3] = ' ';
            choicestr[count + 4] = '(';
            choicestr[count + 5] = def;
            choicestr[count + 6] = ')';
            choicestr[count + 7] = '\0';
        } else {                /* No usable default choice */

            choicestr[count + 3] = '\0';
            def = '\0';         /* Mark as no default */
        }
        strcpy(askstr, prompt);
        strcat(askstr, choicestr);
    } else {
        strcpy(askstr, prompt);
        any_choice = TRUE;
    }

    /* +1: room for a trailing space where the cursor will rest */
    prompt_width = (int) strlen(askstr) + 1;

    if ((prompt_width + 2) > maxwidth) {
        prompt_height = curses_num_lines(askstr, maxwidth);
        prompt_width = map_width - 2;
    }

    if (iflags.wc_popup_dialog /*|| curses_stupid_hack*/) {
        askwin = curses_create_window(prompt_width, prompt_height, UP);
        activemenu = askwin;

        for (count = 0; count < prompt_height; count++) {
            linestr = curses_break_str(askstr, maxwidth, count + 1);
            mvwaddstr(askwin, count + 1, 1, linestr);
            free(linestr);
        }

        wrefresh(askwin);
    } else {
        /* TODO: add SUPPRESS_HISTORY flag, then after getting a response,
           append it and use put_msghistory() on combined prompt+answer */
        custompline(OVERRIDE_MSGTYPE, "%s", askstr);
    }

    /*curses_stupid_hack = 0; */

    curs_set(1);
    while (1) {
#ifdef PDCURSES
        answer = wgetch(message_window);
#else
        answer = curses_read_char();
#endif
        if (answer == ERR) {
            answer = def;
            break;
        }

        answer = curses_convert_keys(answer);

        if (answer == KEY_ESC) {
            if (choices == NULL) {
                break;
            }
            answer = def;
            for (count = 0; choices[count] != '\0'; count++) {
                if (choices[count] == 'q') {    /* q is preferred over n */
                    answer = 'q';
                } else if ((choices[count] == 'n') && answer != 'q') {
                    answer = 'n';
                }
            }
            break;
        } else if ((answer == '\n') || (answer == '\r') || (answer == ' ')) {
            if ((choices != NULL) && (def != '\0')) {
                answer = def;
            }
            break;
        }

        if (digit(answer) && accept_count) {
            if (answer != '0') {
                yn_number = curses_get_count(answer);

                if (iflags.wc_popup_dialog) {
                    curses_count_window(NULL);
                    touchwin(askwin);
                    wrefresh(askwin);
                }
            }
            answer = '#';
            break;
        }

        if (any_choice) {
            break;
        }

        if (choices != NULL && answer != '\0' && strchr(choices, answer))
            break;
    }
    curs_set(0);

    if (iflags.wc_popup_dialog) {
        /* Kludge to make prompt visible after window is dismissed
           when inputting a number */
        if (digit(answer)) {
            custompline(OVERRIDE_MSGTYPE, "%s", askstr);
            curs_set(1);
        }

        curses_destroy_win(askwin);
    } else {
        curses_clear_unhighlight_message_window();
    }

    return answer;
}


/* Return an extended command from the user */

int
curses_ext_cmd(void)
{
    int letter, prompt_width, startx, starty, winx, winy;
    int messageh, messagew, maxlen = BUFSZ - 1;
    int ret = -1;
    char cur_choice[BUFSZ];
    int matches = 0;
    int *ecmatches = NULL;
    WINDOW *extwin = NULL, *extwin2 = NULL;

    if (iflags.extmenu) {
        return extcmd_via_menu();
    }

    startx = 0;
    starty = 0;
    if (iflags.wc_popup_dialog) { /* Prompt in popup window */
        int x0, y0, w, h; /* bounding coords of popup */

        extwin2 = curses_create_window(25, 1, UP);
        wrefresh(extwin2);
        /* create window inside window to prevent overwriting of border */
        getbegyx(extwin2, y0, x0);
        getmaxyx(extwin2, h, w);
        extwin = newwin(1, w - 2, y0 + 1, x0 + 1);
        if (w - 4 < maxlen)
            maxlen = w - 4;
        nhUse(h); /* needed only to give getmaxyx three arguments */
    } else {
        curses_get_window_xy(MESSAGE_WIN, &winx, &winy);
        curses_get_window_size(MESSAGE_WIN, &messageh, &messagew);

        if (curses_window_has_border(MESSAGE_WIN)) {
            winx++;
            winy++;
        }

        winy += messageh - 1;
        extwin = newwin(1, messagew - 2, winy, winx);
        if (messagew - 4 < maxlen)
            maxlen = messagew - 4;
        custompline(OVERRIDE_MSGTYPE, "#");
    }

    cur_choice[0] = '\0';

    while (1) {
        wmove(extwin, starty, startx);
        waddstr(extwin, "# ");
        wmove(extwin, starty, startx + 2);
        waddstr(extwin, cur_choice);
        wmove(extwin, starty, (int) strlen(cur_choice) + startx + 2);
        wclrtoeol(extwin);

        /* if we have an autocomplete command, AND it matches uniquely */
        if (matches == 1 && ecmatches) {
            struct ext_func_tab *ec = extcmds_getentry(ecmatches[0]);

            if (ec) {
                curses_toggle_color_attr(extwin, NONE, A_UNDERLINE, ON);
                wmove(extwin, starty, (int) strlen(cur_choice) + startx + 2);
                wprintw(extwin, "%s", ec->ef_txt + (int) strlen(cur_choice));
                curses_toggle_color_attr(extwin, NONE, A_UNDERLINE, OFF);
            }
        }

        curs_set(1);
        wrefresh(extwin);
        letter = pgetchar(); /* pgetchar(cmd.c) implements do-again */
        curs_set(0);
        prompt_width = (int) strlen(cur_choice);
        matches = 0;

        if (letter == '\033' || letter == ERR) {
            ret = -1;
            break;
        }

        if (letter == '\r' || letter == '\n') {
            (void) mungspaces(cur_choice);
            if (ret == -1) {
                matches = extcmds_match(cur_choice,
                                        (ECM_IGNOREAC | ECM_EXACTMATCH),
                                        &ecmatches);
                if (matches == 1 && ecmatches)
                    ret = ecmatches[0];
            }
            break;
        }

        if (letter == '\177') /* DEL/Rubout */
            letter = '\b';
        if (letter == '\b' || letter == KEY_BACKSPACE
            || (erase_char && letter == (int) (uchar) erase_char)) {
            if (prompt_width == 0) {
                ret = -1;
                break;
            } else {
                cur_choice[prompt_width - 1] = '\0';
                letter = '*';
                prompt_width--;
                ret = -1;
            }

        /* honor kill_char if it's ^U or similar, but not if it's '@' */
        } else if (kill_char && letter == (int) (uchar) kill_char
                   && (letter < ' ' || letter >= '\177')) { /*ASCII*/
            cur_choice[0] = '\0';
            letter = '*';
            prompt_width = 0;
        }

        if (letter != '*' && prompt_width < maxlen) {
            cur_choice[prompt_width] = letter;
            cur_choice[prompt_width + 1] = '\0';
            ret = -1;
        }
        matches = extcmds_match(cur_choice, ECM_NOFLAGS, &ecmatches);
        if (matches == 1 && ecmatches)
            ret = ecmatches[0];
    }

    curses_destroy_win(extwin);
    if (extwin2)
        curses_destroy_win(extwin2);

    if (ret != -1) {
    } else {
        char extcmd_char = extcmd_initiator();

        if (*cur_choice)
            pline("%s%s: unknown extended command.",
                  visctrl(extcmd_char), cur_choice);
    }
    return ret;
}


/* Initialize a menu from given NetHack winid */

void
curses_create_nhmenu(winid wid, unsigned long mbehavior)
{
    nhmenu *new_menu = NULL;
    nhmenu *menuptr = nhmenus;
    nhmenu_item *menu_item_ptr = NULL;
    nhmenu_item *tmp_menu_item = NULL;

    new_menu = get_menu(wid);

    if (new_menu != NULL) {
        /* Reuse existing menu, clearing out current entries */
        menu_item_ptr = new_menu->entries;

        if (menu_item_ptr != NULL) {
            while (menu_item_ptr->next_item != NULL) {
                tmp_menu_item = menu_item_ptr->next_item;
                free((genericptr_t) menu_item_ptr->str);
                free((genericptr_t) menu_item_ptr);
                menu_item_ptr = tmp_menu_item;
            }
            free((genericptr_t) menu_item_ptr->str);
            free((genericptr_t) menu_item_ptr); /* Last entry */
            new_menu->entries = NULL;
        }
        if (new_menu->prompt != NULL) { /* Reusing existing menu */
            free((genericptr_t) new_menu->prompt);
            new_menu->prompt = NULL;
        }
        new_menu->num_pages = 0;
        new_menu->height = 0;
        new_menu->width = 0;
        new_menu->mbehavior = mbehavior;
        new_menu->reuse_accels = FALSE;
        new_menu->bottom_heavy = FALSE;
        return;
    }

    new_menu = (nhmenu *) alloc((signed) sizeof (nhmenu));
    new_menu->wid = wid;
    new_menu->prompt = NULL;
    new_menu->entries = NULL;
    new_menu->num_pages = 0;
    new_menu->height = 0;
    new_menu->width = 0;
    new_menu->mbehavior = mbehavior;
    new_menu->reuse_accels = FALSE;
    new_menu->bottom_heavy = FALSE;
    new_menu->next_menu = NULL;

    if (nhmenus == NULL) {      /* no menus in memory yet */
        new_menu->prev_menu = NULL;
        nhmenus = new_menu;
    } else {
        while (menuptr->next_menu != NULL) {
            menuptr = menuptr->next_menu;
        }
        new_menu->prev_menu = menuptr;
        menuptr->next_menu = new_menu;
    }
}

static nhmenu_item *
curs_new_menu_item(winid wid, const char *str)
{
    char *new_str;
    nhmenu_item *new_item;

    new_str = curses_copy_of(str);
    curses_rtrim(new_str);
    new_item = (nhmenu_item *) alloc((unsigned) sizeof (nhmenu_item));
    new_item->wid = wid;
    new_item->glyphinfo = nul_glyphinfo;
    new_item->identifier = cg.zeroany;
    new_item->accelerator = '\0';
    new_item->group_accel = '\0';
    new_item->attr = 0;
    new_item->str = new_str;
    new_item->presel = FALSE;
    new_item->selected = FALSE;
    new_item->itemflags = MENU_ITEMFLAGS_NONE;
    new_item->page_num = 0;
    new_item->line_num = 0;
    new_item->num_lines = 0;
    new_item->count = -1;
    new_item->next_item = NULL;
    return new_item;
}
/* Add a menu item to the given menu window */

void
curses_add_nhmenu_item(
    winid wid,
    const glyph_info *glyphinfo,
    const ANY_P *identifier,
    char accelerator,
    char group_accel,
    int attr,
    const char *str,
    unsigned itemflags)
{
    nhmenu_item *new_item, *current_items, *menu_item_ptr;
    nhmenu *current_menu = get_menu(wid);
    boolean presel = (itemflags & MENU_ITEMFLAGS_SELECTED) != 0;

    if (current_menu == NULL) {
        impossible(
           "curses_add_nhmenu_item: attempt to add item to nonexistent menu");
        return;
    }

    if (str == NULL) {
        return;
    }

    new_item = curs_new_menu_item(wid, str);
    new_item->glyphinfo = *glyphinfo;
    new_item->identifier = *identifier;
    new_item->accelerator = accelerator;
    new_item->group_accel = group_accel;
    new_item->attr = attr;
    new_item->presel = presel;
    new_item->itemflags = itemflags;
    current_items = current_menu->entries;
    menu_item_ptr = current_items;

    if (current_items == NULL) {
        new_item->prev_item = NULL;
        current_menu->entries = new_item;
    } else {
        while (menu_item_ptr->next_item != NULL) {
            menu_item_ptr = menu_item_ptr->next_item;
        }
        new_item->prev_item = menu_item_ptr;
        menu_item_ptr->next_item = new_item;
    }
}

/* for menu->bottom_heavy -- insert enough blank lines at top of
   first page to make the last page become a full one */
static void
curs_pad_menu(
    nhmenu *current_menu,
    boolean do_pad UNUSED)
{
    nhmenu_item *menu_item_ptr;
    int numpages = current_menu->num_pages;

    /* caller has already called menu_win_size() */
    menu_determine_pages(current_menu); /* sets 'menu->num_pages' */
    numpages = current_menu->num_pages;
    /* pad beginning of menu so that partial last page becomes full;
       might be slightly less than full if any entries take multiple
       lines and the padding would force those to span page boundary
       and that gets prevented; so we re-count the number of pages
       with every insertion instead of trying to calculate the number
       of them to add */
    do {
        menu_item_ptr = curs_new_menu_item(current_menu->wid, "");
        menu_item_ptr->next_item = current_menu->entries;
        current_menu->entries->prev_item = menu_item_ptr;
        current_menu->entries = menu_item_ptr;
        current_menu->num_entries += 1;

        menu_determine_pages(current_menu);
    } while (current_menu->num_pages == numpages);

    /* we inserted blank lines at beginning until final entry spilled
       over to another page; take the most recent blank one back out */
    current_menu->num_entries -= 1;
    current_menu->entries = menu_item_ptr->next_item;
    current_menu->entries->prev_item = (nhmenu_item *) 0;
    free((genericptr_t) menu_item_ptr->str);
    free((genericptr_t) menu_item_ptr);

    /* reset page count; shouldn't need to re-count */
    current_menu->num_pages = numpages;
    return;
}

/* mark ^P message recall menu, for msg_window:full (FIFO), where we'll
   start viewing on the last page so be able to see most recent immediately */
void
curs_menu_set_bottom_heavy(winid wid)
{
    nhmenu *menu = get_menu(wid);

    /*
     * Called after end_menu + finalize_nhmenu,
     * before select_menu + display_nhmenu.
     */
    menu_win_size(menu); /* (normally not done until display_nhmenu) */
    if (menu_is_multipage(menu, menu->width, menu->height)) {
        menu->bottom_heavy = TRUE;

        /* insert enough blank lines at top of first page to make the
           last page become a full one */
        curs_pad_menu(menu, TRUE);
    }
    return;
}

/* No more entries are to be added to menu, so details of the menu can be
 finalized in memory */

void
curses_finalize_nhmenu(winid wid, const char *prompt)
{
    int count = 0;
    nhmenu_item *menu_item_ptr;
    nhmenu *current_menu = get_menu(wid);

    if (current_menu == NULL) {
        impossible(
              "curses_finalize_nhmenu: attempt to finalize nonexistent menu");
        return;
    }
    for (menu_item_ptr = current_menu->entries;
         menu_item_ptr != NULL; menu_item_ptr = menu_item_ptr->next_item)
        count++;

    current_menu->num_entries = count;
    current_menu->prompt = curses_copy_of(prompt);
}


/* Display a nethack menu, and return a selection, if applicable */

int
curses_display_nhmenu(
    winid wid,
    int how,
    MENU_ITEM_P **_selected)
{
    nhmenu *current_menu = get_menu(wid);
    nhmenu_item *menu_item_ptr;
    int num_chosen, count;
    WINDOW *win;
    MENU_ITEM_P *selected = NULL;

    *_selected = NULL;

    if (current_menu == NULL) {
        impossible(
                "curses_display_nhmenu: attempt to display nonexistent menu");
        return -1; /* not ESC which falsely claims 27 items were selected */
    }

    menu_item_ptr = current_menu->entries;

    if (menu_item_ptr == NULL) {
        impossible("curses_display_nhmenu: attempt to display empty menu");
        return -1;
    }

    /* Reset items to unselected to clear out selections from previous
       invocations of this menu, and preselect appropriate items */
    while (menu_item_ptr != NULL) {
        menu_item_ptr->selected = menu_item_ptr->presel;
        menu_item_ptr = menu_item_ptr->next_item;
    }

    menu_win_size(current_menu);
    menu_determine_pages(current_menu);

    /* Display pre and post-game menus centered */
    if ((gm.moves <= 1 && !gi.invent) || gp.program_state.gameover) {
        win = curses_create_window(current_menu->width,
                                   current_menu->height, CENTER);
    } else { /* Display during-game menus on the right out of the way */
        win = curses_create_window(current_menu->width,
                                   current_menu->height, RIGHT);
    }

    num_chosen = menu_get_selections(win, current_menu, how);
    curses_destroy_win(win);

    if (num_chosen > 0) {
        selected = (MENU_ITEM_P *) alloc((unsigned)
                                         (num_chosen * sizeof (MENU_ITEM_P)));
        count = 0;
        menu_item_ptr = current_menu->entries;

        while (menu_item_ptr != NULL) {
            if (menu_item_ptr->selected) {
                if (count == num_chosen) {
                    impossible("curses_display_nhmenu: Selected items "
                               "exceeds expected number");
                    break;
                }
                selected[count].item = menu_item_ptr->identifier;
                selected[count].count = menu_item_ptr->count;
                count++;
            }
            menu_item_ptr = menu_item_ptr->next_item;
        }

        if (count != num_chosen) {
            impossible(
           "curses_display_nhmenu: Selected items less than expected number");
            num_chosen = min(count, num_chosen);
        }
    }

    *_selected = selected;

    return num_chosen;
}


boolean
curses_menu_exists(winid wid)
{
    if (get_menu(wid) != NULL) {
        return TRUE;
    } else {
        return FALSE;
    }
}

/* Delete the menu associated with the given NetHack winid from memory */

void
curses_del_menu(winid wid, boolean del_wid_too)
{
    nhmenu_item *tmp_menu_item;
    nhmenu_item *menu_item_ptr;
    nhmenu *tmpmenu;
    nhmenu *current_menu = get_menu(wid);

    if (current_menu == NULL) {
        return;
    }

    menu_item_ptr = current_menu->entries;

    /* First free entries associated with this menu from memory */
    if (menu_item_ptr != NULL) {
        while (menu_item_ptr->next_item != NULL) {
            tmp_menu_item = menu_item_ptr->next_item;
            free((genericptr_t) menu_item_ptr->str);
            free(menu_item_ptr);
            menu_item_ptr = tmp_menu_item;
        }
        free((genericptr_t) menu_item_ptr->str);
        free(menu_item_ptr);    /* Last entry */
        current_menu->entries = NULL;
    }

    /* Now unlink the menu from the list and free it as well */
    if ((tmpmenu = current_menu->prev_menu) != NULL) {
        tmpmenu->next_menu = current_menu->next_menu;
    } else {
        nhmenus = current_menu->next_menu;      /* New head node or NULL */
    }
    if ((tmpmenu = current_menu->next_menu) != NULL) {
        tmpmenu->prev_menu = current_menu->prev_menu;
    }

    if (current_menu->prompt)
        free((genericptr_t) current_menu->prompt);
    free((genericptr_t) current_menu);

    if (del_wid_too)
        curses_del_wid(wid);
}


/* return a pointer to the menu associated with the given NetHack winid */

static nhmenu *
get_menu(winid wid)
{
    nhmenu *menuptr = nhmenus;

    while (menuptr != NULL) {
        if (menuptr->wid == wid) {
            return menuptr;
        }
        menuptr = menuptr->next_menu;
    }

    return NULL;                /* Not found */
}

static char
menu_get_accel(boolean first)
{
    static char next_letter;
    char ret;

    if (first) {
        next_letter = 'a';
    }

    ret = next_letter;

    if ((next_letter < 'z' && next_letter >= 'a')
        || (next_letter < 'Z' && next_letter >= 'A')) {
        next_letter++;
    } else if (next_letter == 'z') {
        next_letter = 'A';
    } else if (next_letter == 'Z') {
        next_letter = 'a'; /* wrap to beginning */
    }

    return ret;
}


/* Determine if menu will require multiple pages to display */

static boolean
menu_is_multipage(nhmenu *menu, int width, int height)
{
    int num_lines;
    int curline = 0, accel_per_page = 0;
    nhmenu_item *menu_item_ptr = menu->entries;

    if (*menu->prompt) {
        curline += curses_num_lines(menu->prompt, width) + 1;
    }

    while (menu_item_ptr != NULL) {
        menu_item_ptr->line_num = curline;
        if (menu_item_ptr->identifier.a_void == NULL) {
            num_lines = curses_num_lines(menu_item_ptr->str, width);
        } else {
            if (menu_item_ptr->accelerator != GOLD_SYM) {
                if (++accel_per_page > MAX_ACCEL_PER_PAGE)
                    break;
            }
            /* Add space for accelerator */
            num_lines = curses_num_lines(menu_item_ptr->str, width - 4);
        }
        menu_item_ptr->num_lines = num_lines;
        curline += num_lines;
        menu_item_ptr = menu_item_ptr->next_item;
        if (curline > height
            || (curline > height - 2 && height == menu_max_height())) {
            break;
        }
    }
    return (menu_item_ptr != NULL) ? TRUE : FALSE;
}


/* Determine which entries go on which page, and total number of pages */

static void
menu_determine_pages(nhmenu *menu)
{
    int tmpline, num_lines, accel_per_page;
    int curline = 0;
    int page_num = 1;
    nhmenu_item *menu_item_ptr = menu->entries;
    int width = menu->width;
    int height = menu->height;
    int page_end = height;


    if (*menu->prompt) {
        curline += curses_num_lines(menu->prompt, width) + 1;
    }
    tmpline = curline;

    if (menu_is_multipage(menu, width, height)) {
        page_end -= 2;          /* Room to display current page number */
    }
    accel_per_page = 0;

    /* Determine what entries belong on which page */
    for (menu_item_ptr = menu->entries; menu_item_ptr != NULL;
         menu_item_ptr = menu_item_ptr->next_item) {
        menu_item_ptr->page_num = page_num;
        menu_item_ptr->line_num = curline;
        if (menu_item_ptr->identifier.a_void == NULL) {
            num_lines = curses_num_lines(menu_item_ptr->str, width);
        } else {
            if (menu_item_ptr->accelerator != GOLD_SYM)
                ++accel_per_page;
            /* Add space for accelerator */
            num_lines = curses_num_lines(menu_item_ptr->str, width - 4);
        }
        menu_item_ptr->num_lines = num_lines;
        curline += num_lines;
        if (curline > page_end || accel_per_page > MAX_ACCEL_PER_PAGE) {
            ++page_num;
            accel_per_page = 0; /* reset */
            curline = tmpline;
            /* Move ptr back so entry will be reprocessed on new page */
            menu_item_ptr = menu_item_ptr->prev_item;
        }
    }

    menu->num_pages = page_num;
}


/* Determine dimensions of menu window based on term size and entries */

static void
menu_win_size(nhmenu *menu)
{
    int maxwidth, maxheight, curentrywidth, lastline;
    int maxentrywidth = 0;
    int maxheaderwidth = menu->prompt ? (int) strlen(menu->prompt) : 0;
    nhmenu_item *menu_item_ptr;

    if (gp.program_state.gameover) {
        /* for final inventory disclosure, use full width */
        maxwidth = term_cols - 2; /* +2: borders assumed */
    } else {
        /* this used to be 38, which is 80/2 - 2 (half a 'normal' sized
           screen minus room for a border box), but some data files
           have been manually formatted for 80 columns (usually limited
           to 78 but sometimes 79, rarely 80 itself) and using a value
           less that 40 meant that a full line would wrap twice:
           1..38, 39..76, and 77..80 */
        maxwidth = 40; /* Reasonable minimum usable width */
        if ((term_cols / 2) > maxwidth)
            maxwidth = (term_cols / 2); /* Half the screen */
    }
    maxheight = menu_max_height();

    /* First, determine the width of the longest menu entry */
    for (menu_item_ptr = menu->entries; menu_item_ptr != NULL;
         menu_item_ptr = menu_item_ptr->next_item) {
        curentrywidth = (int) strlen(menu_item_ptr->str);
        if (menu_item_ptr->identifier.a_void == NULL) {
            if (curentrywidth > maxheaderwidth) {
                maxheaderwidth = curentrywidth;
            }
        } else {
            /* Add space for accelerator (selector letter) */
            curentrywidth += 4;
#if 0 /* FIXME: menu glyphs */
            if (menu_item_ptr->glyphinfo.glyph != NO_GLYPH
                && iflags.use_menu_glyphs)
                curentrywidth += 2;
#endif
        }
        if (curentrywidth > maxentrywidth) {
            maxentrywidth = curentrywidth;
        }
    }

    /*
     * 3.6.3: This used to set maxwidth to maxheaderwidth when that was
     * bigger but only set it to maxentrywidth if the latter was smaller,
     * so entries wider than the default would always wrap unless at
     * least one header or separator line was long, even when there was
     * lots of space available to display them without wrapping.  The
     * reason to force narrow menus isn't known.  It may have been to
     * reduce the amount of map rewriting when menu is dismissed, but if
     * so, that was an issue due to excessive screen writing for the map
     * (output was flushed for each character) which was fixed long ago.
     */
    maxwidth = max(maxentrywidth, maxheaderwidth);
    if (maxwidth > term_cols - 2) { /* -2: space for left and right borders */
        maxwidth = term_cols - 2;
    }

    /* Possibly reduce height if only 1 page */
    if (!menu_is_multipage(menu, maxwidth, maxheight)) {
        menu_item_ptr = menu->entries;

        while (menu_item_ptr->next_item != NULL) {
            menu_item_ptr = menu_item_ptr->next_item;
        }

        lastline = (menu_item_ptr->line_num) + menu_item_ptr->num_lines;

        if (lastline < maxheight) {
            maxheight = lastline;
        }
    }

    /* avoid a tiny popup window; when it's shown over the endings of
       old messsages rather than over the map, it is fairly easy for
       the player to overlook it, particularly when walking around and
       stepping on a pile of 2 items; also, multi-page menus need enough
       room for "(Page M of N) => " even if all entries are narrower
       than that; we specify same minimum width even when single page */
    menu->width = max(maxwidth, 25);
    menu->height = max(maxheight, 5);
}

#ifdef NCURSES_MOUSE_VERSION
static nhmenu_item *
get_menuitem_y(
    nhmenu *menu,
    WINDOW *win UNUSED,
    int page_num,
    int liney)
{
    nhmenu_item *menu_item_ptr;
    int count, num_lines, entry_cols = menu->width;
    char *tmpstr;

    menu_item_ptr = menu->entries;

    while (menu_item_ptr != NULL) {
        if (menu_item_ptr->page_num == page_num) {
            break;
        }
        menu_item_ptr = menu_item_ptr->next_item;
    }

    if (menu_item_ptr == NULL) {        /* Page not found */
        impossible("get_menuitem_y: attempt to display nonexistent page");
        return NULL;
    }

    if (menu->prompt && *menu->prompt) {
        num_lines = curses_num_lines(menu->prompt, menu->width);

        for (count = 0; count < num_lines; count++) {
            tmpstr = curses_break_str(menu->prompt, menu->width, count + 1);
            free(tmpstr);
        }
    }

    while (menu_item_ptr != NULL) {
        if (menu_item_ptr->page_num != page_num) {
            break;
        }
        if (menu_item_ptr->identifier.a_void != NULL) {
            if (menu_item_ptr->line_num + 1 == liney)
                return menu_item_ptr;
        }

        num_lines = curses_num_lines(menu_item_ptr->str, entry_cols);
        for (count = 0; count < num_lines; count++) {
            if (menu_item_ptr->str && *menu_item_ptr->str) {
                tmpstr = curses_break_str(menu_item_ptr->str,
                                          entry_cols, count + 1);
                free(tmpstr);
            }
        }

        menu_item_ptr = menu_item_ptr->next_item;
    }

    return NULL;

}
#endif /*NCURSES_MOUSE_VERSION*/

/* Displays menu selections in the given window */

static void
menu_display_page(
    nhmenu *menu, WINDOW *win,
    int page_num,      /* page number, 1..n rather than 0..n-1 */
    char *selectors,   /* selection letters on current page */
    char *groupaccels) /* group accelerator characters on any page */
{
    nhmenu_item *menu_item_ptr;
    int count, curletter, entry_cols, start_col, num_lines;
    char *tmpstr;
    boolean first_accel = TRUE;
    int color = NO_COLOR, attr = A_NORMAL;
    boolean menu_color = FALSE;

    /* letters assigned to entries on current page */
    if (selectors)
        (void) memset((genericptr_t) selectors, 0, 256);
    /* characters assigned to group accelerators on any page */
    if (groupaccels)
        (void) memset((genericptr_t) groupaccels, 0, 256);

    /* Cycle through entries until we are on the correct page */

    menu_item_ptr = menu->entries;

    while (menu_item_ptr != NULL) {
        if (menu_item_ptr->page_num == page_num) {
            break;
        }
        menu_item_ptr = menu_item_ptr->next_item;
    }

    if (menu_item_ptr == NULL) {        /* Page not found */
        impossible("menu_display_page: attempt to display nonexistent page");
        return;
    }

    werase(win);

    if (menu->prompt && *menu->prompt) {
        num_lines = curses_num_lines(menu->prompt, menu->width);

        for (count = 0; count < num_lines; count++) {
            tmpstr = curses_break_str(menu->prompt, menu->width, count + 1);
            mvwprintw(win, count + 1, 1, "%s", tmpstr);
            free(tmpstr);
        }
    }

    /* Display items for current page */

    while (menu_item_ptr != NULL) {
        /* collect group accelerators for every page */
        if (groupaccels && menu_item_ptr->identifier.a_void != NULL) {
            unsigned gch = (unsigned) menu_item_ptr->group_accel;

            if (gch > 0 && gch <= 255) /* note: skip it if 0 */
                groupaccels[gch] = 1;
        }
        /* remaining processing applies to current page only */
        if (menu_item_ptr->page_num != page_num) {
            break;
        }
        if (menu_item_ptr->identifier.a_void != NULL) {
            if (menu_item_ptr->accelerator != 0) {
                curletter = menu_item_ptr->accelerator;
            } else {
                if (first_accel) {
                    curletter = menu_get_accel(TRUE);
                    first_accel = FALSE;
                    if (!menu->reuse_accels && (menu->num_pages > 1)) {
                        menu->reuse_accels = TRUE;
                    }
                } else {
                    curletter = menu_get_accel(FALSE);
                }
                menu_item_ptr->accelerator = curletter;
            }
            /* we have a selector letter; tell caller about it */
            if (selectors)
                selectors[(unsigned) (curletter & 0xFF)]++;

            if (menu_item_ptr->selected) {
                curses_toggle_color_attr(win, HIGHLIGHT_COLOR, A_REVERSE, ON);
                mvwaddch(win, menu_item_ptr->line_num + 1, 1, '<');
                mvwaddch(win, menu_item_ptr->line_num + 1, 2, curletter);
                mvwaddch(win, menu_item_ptr->line_num + 1, 3, '>');
                curses_toggle_color_attr(win, HIGHLIGHT_COLOR, A_REVERSE, OFF);
            } else {
                curses_toggle_color_attr(win, HIGHLIGHT_COLOR, NONE, ON);
                mvwaddch(win, menu_item_ptr->line_num + 1, 2, curletter);
                curses_toggle_color_attr(win, HIGHLIGHT_COLOR, NONE, OFF);
                mvwprintw(win, menu_item_ptr->line_num + 1, 3, ") ");
            }
        }
        entry_cols = menu->width;
        start_col = 1;

        if (menu_item_ptr->identifier.a_void != NULL) {
            entry_cols -= 4;
            start_col += 4;
        }
#if 0
        /* FIXME: menuglyphs not implemented yet */
        if (menu_item_ptr->glyphinfo.glyph != NO_GLYPH
            && iflags.use_menu_glyphs) {
            color = (int) menu_item_ptr->glyphinfo.color;
            curses_toggle_color_attr(win, color, NONE, ON);
            mvwaddch(win, menu_item_ptr->line_num + 1, start_col, curletter);
            curses_toggle_color_attr(win, color, NONE, OFF);
            mvwaddch(win, menu_item_ptr->line_num + 1, start_col + 1, ' ');
            entry_cols -= 2;
            start_col += 2;
        }
#endif
        color = NONE;
        menu_color = iflags.use_menu_color
                     && get_menu_coloring(menu_item_ptr->str, &color, &attr);
        if (menu_color) {
            attr = curses_convert_attr(attr);
            if (color != NONE || attr != A_NORMAL)
                curses_menu_color_attr(win, color, attr, ON);
        } else {
            attr = menu_item_ptr->attr;
            if (color != NONE || attr != A_NORMAL)
                curses_toggle_color_attr(win, color, attr, ON);
        }

        num_lines = curses_num_lines(menu_item_ptr->str, entry_cols);
        for (count = 0; count < num_lines; count++) {
            if (menu_item_ptr->str && *menu_item_ptr->str) {
                tmpstr = curses_break_str(menu_item_ptr->str,
                                          entry_cols, count + 1);
                mvwprintw(win, menu_item_ptr->line_num + count + 1, start_col,
                          "%s", tmpstr);
                free(tmpstr);
            }
        }
        if (color != NONE || attr != A_NORMAL) {
            if (menu_color)
                curses_menu_color_attr(win, color, attr, OFF);
            else
                curses_toggle_color_attr(win, color, attr, OFF);
        }

        menu_item_ptr = menu_item_ptr->next_item;
    }

    if (menu->num_pages > 1) {
        int footer_x, footwidth, shoesize = menu->num_pages;

        footwidth = (int) (sizeof "<- (Page X of Y) ->" - sizeof "");
        while (shoesize >= 10) { /* possible for pickup from big piles... */
             /* room for wider feet; extra digit for both X and Y */
            footwidth += 2;
            shoesize /= 10;
        }
        footer_x = !menu->bottom_heavy ? (menu->width - footwidth) : 2;
        if (page_num != 1) {
            curses_toggle_color_attr(win, HIGHLIGHT_COLOR, NONE, ON);
            mvwaddstr(win, menu->height, footer_x, "<=");
            curses_toggle_color_attr(win, HIGHLIGHT_COLOR, NONE, OFF);
        }
        mvwprintw(win, menu->height, footer_x + 2, " (Page %d of %d) ",
                  page_num, menu->num_pages);
        if (page_num != menu->num_pages) {
            curses_toggle_color_attr(win, HIGHLIGHT_COLOR, NONE, ON);
            mvwaddstr(win, menu->height, footer_x + footwidth - 2, "=>");
            curses_toggle_color_attr(win, HIGHLIGHT_COLOR, NONE, OFF);
        }
    }
    curses_toggle_color_attr(win, DIALOG_BORDER_COLOR, NONE, ON);
    box(win, 0, 0);
    curses_toggle_color_attr(win, DIALOG_BORDER_COLOR, NONE, OFF);
    wrefresh(win);
}

/* split out from menu_get_selections() so that perm_invent scrolling
   can be controlled from outside the normal menu activity */
boolean
curs_nonselect_menu_action(
    WINDOW *win, void *menu_v,
    int how,
    int curletter,
    int *curpage_p,
    char selectors[256],
    int *num_selected_p)
{
    nhmenu_item *menu_item_ptr;
    nhmenu *menu = (nhmenu *) menu_v;
    boolean dismiss = FALSE;
    int menucmd = (curletter <= 0 || curletter >= 255) ? curletter
                  : (int) (uchar) map_menu_cmd(curletter);

    switch (menucmd) {
    case KEY_ESC:
        *num_selected_p = -1;
        dismiss = TRUE;
        break;
    case '\n':
    case '\r':
        dismiss = TRUE;
        break;
#ifdef NCURSES_MOUSE_VERSION
    case KEY_MOUSE: {
        MEVENT mev;

        if (getmouse(&mev) == OK && how != PICK_NONE) {
            if (wmouse_trafo(win, &mev.y, &mev.x, FALSE)) {
                int y = mev.y;

                menu_item_ptr = get_menuitem_y(menu, win, *curpage_p, y);

                if (menu_item_ptr) {
                    if (how == PICK_ONE) {
                        menu_clear_selections(menu);
                        menu_select_deselect(win, menu_item_ptr,
                                             SELECT, *curpage_p);
                        *num_selected_p = 1;
                        dismiss = TRUE;
                    } else {
                        menu_select_deselect(win, menu_item_ptr,
                                             INVERT, *curpage_p);
                    }
                }
            }
        }
        break;
    } /* case KEY_MOUSE */
#endif /*NCURSES_MOUSE_VERSION*/
    case KEY_RIGHT:
    case KEY_NPAGE:
    case MENU_NEXT_PAGE:
    case ' ':
        if (*curpage_p < menu->num_pages) {
            ++(*curpage_p);
            menu_display_page(menu, win, *curpage_p, selectors, (char *) 0);
        } else if (menucmd == ' ') {
            dismiss = TRUE;
            break;
        }
        break;
    case KEY_LEFT:
    case KEY_PPAGE:
    case MENU_PREVIOUS_PAGE:
        if (*curpage_p > 1) {
            --(*curpage_p);
            menu_display_page(menu, win, *curpage_p, selectors, (char *) 0);
        }
        break;
    case KEY_END:
    case MENU_LAST_PAGE:
        if (*curpage_p != menu->num_pages) {
            *curpage_p = menu->num_pages;
            menu_display_page(menu, win, *curpage_p, selectors, (char *) 0);
        }
        break;
    case KEY_HOME:
    case MENU_FIRST_PAGE:
        if (*curpage_p != 1) {
            *curpage_p = 1;
            menu_display_page(menu, win, *curpage_p, selectors, (char *) 0);
        }
        break;
    case MENU_SEARCH: {
        char search_key[BUFSZ];

        search_key[0] = '\0';
        curses_line_input_dialog("Search for:", search_key, BUFSZ);

        refresh();
        touchwin(win);
        wrefresh(win);

        if (!*search_key)
            break;

        menu_item_ptr = menu->entries;

        while (menu_item_ptr != NULL) {
            if (menu_item_ptr->identifier.a_void != NULL
                && strstri(menu_item_ptr->str, search_key)) {
                if (how == PICK_ONE) {
                    menu_clear_selections(menu);
                    menu_select_deselect(win, menu_item_ptr,
                                         SELECT, *curpage_p);
                    *num_selected_p = 1;
                    dismiss = TRUE;
                    break;
                } else {
                    menu_select_deselect(win, menu_item_ptr,
                                         INVERT, *curpage_p);
                }
            }
            menu_item_ptr = menu_item_ptr->next_item;
        }

        menu_item_ptr = menu->entries;
        break;
    } /* case MENU_SEARCH */
    default:
        if (how == PICK_NONE) {
            *num_selected_p = 0;
            dismiss = TRUE;
            break;
        }
    }

    return dismiss;
}

static int
menu_get_selections(WINDOW *win, nhmenu *menu, int how)
{
    int curletter, menucmd;
    long count = -1L;
    int count_letter = '\0';
    int curpage = !menu->bottom_heavy ? 1 : menu->num_pages;
    int num_selected = 0;
    boolean dismiss = FALSE;
    char selectors[256], groupaccels[256];
    nhmenu_item *menu_item_ptr = menu->entries;

    activemenu = win;
    menu_display_page(menu, win, curpage, selectors, groupaccels);

    while (!dismiss) {
        curletter = curses_getch();

        if (curletter == ERR) {
            num_selected = -1;
            dismiss = TRUE;
        }

        if (curletter == '\033') {
            curletter = curses_convert_keys(curletter);
        }

        switch (how) {
        case PICK_NONE:
            if (menu->num_pages == 1) {
                if (curletter == KEY_ESC) {
                    num_selected = -1;
                } else {
                    num_selected = 0;
                }
                dismiss = TRUE;
                break;
            }
            break;
        case PICK_ANY:
            if (curletter <= 0 || curletter >= 256
                || (!selectors[curletter] && !groupaccels[curletter])) {
                menucmd = (curletter <= 0 || curletter >= 255) ? curletter
                          : (int) (uchar) map_menu_cmd(curletter);
                switch (menucmd) {
                case MENU_SELECT_PAGE:
                    (void) menu_operation(win, menu, SELECT, curpage);
                    break;
                case MENU_SELECT_ALL:
                    curpage = menu_operation(win, menu, SELECT, 0);
                    break;
                case MENU_UNSELECT_PAGE:
                    (void) menu_operation(win, menu, DESELECT, curpage);
                    break;
                case MENU_UNSELECT_ALL:
                    curpage = menu_operation(win, menu, DESELECT, 0);
                    break;
                case MENU_INVERT_PAGE:
                    (void) menu_operation(win, menu, INVERT, curpage);
                    break;
                case MENU_INVERT_ALL:
                    curpage = menu_operation(win, menu, INVERT, 0);
                    break;
                }
            }
            /*FALLTHRU*/
        default:
            if (isdigit(curletter) && !groupaccels[curletter]) {
                count = curses_get_count(curletter);
                /* after count, we know some non-digit is already pending */
                curletter = curses_getch();
                count_letter = (count > 0L) ? curletter : '\0';

                /* remove the count wind (erases last line of message wind) */
                curses_count_window(NULL);
                /* force redraw of the menu that is receiving the count */
                touchwin(win);
                wrefresh(win);
            }
        }

        if (curletter <= 0 || curletter >= 256
            || (!selectors[curletter] && !groupaccels[curletter])) {
            dismiss = curs_nonselect_menu_action(win, (void *) menu, how,
                                                 curletter, &curpage,
                                                 selectors, &num_selected);
            if (num_selected == -1) {
                activemenu = NULL;
                return -1;
            }
        }

        menu_item_ptr = menu->entries;
        while (menu_item_ptr != NULL) {
            if (menu_item_ptr->identifier.a_void != NULL) {
                if ((curletter == menu_item_ptr->accelerator
                     && (curpage == menu_item_ptr->page_num
                         || !menu->reuse_accels))
                    || (menu_item_ptr->group_accel
                        && curletter == menu_item_ptr->group_accel)) {
                    if (curpage != menu_item_ptr->page_num) {
                        curpage = menu_item_ptr->page_num;
                        menu_display_page(menu, win, curpage, selectors,
                                          (char *) 0);
                    }

                    if (how == PICK_ONE) {
                        menu_clear_selections(menu);
                        menu_select_deselect(win, menu_item_ptr,
                                             SELECT, curpage);
                        if (count)
                            menu_item_ptr->count = count;
                        num_selected = 1;
                        dismiss = TRUE;
                        break;
                    } else if (how == PICK_ANY && curletter == count_letter) {
                        menu_select_deselect(win, menu_item_ptr,
                                             SELECT, curpage);
                        menu_item_ptr->count = count;
                        count = 0;
                        count_letter = '\0';
                    } else {
                        menu_select_deselect(win, menu_item_ptr,
                                             INVERT, curpage);
                    }
                }
            }
            menu_item_ptr = menu_item_ptr->next_item;
        }
    }

    if (how == PICK_ANY && num_selected != -1) {
        num_selected = 0;
        menu_item_ptr = menu->entries;

        while (menu_item_ptr != NULL) {
            if (menu_item_ptr->identifier.a_void != NULL) {
                if (menu_item_ptr->selected) {
                    num_selected++;
                }
            }
            menu_item_ptr = menu_item_ptr->next_item;
        }
    }

    activemenu = NULL;
    return num_selected;
}

/* Select, deselect, or toggle selected for the given menu entry.
   For search operations, the toggled entry might be on a different
   page than the one currently shown. */

static void
menu_select_deselect(
    WINDOW *win,
    nhmenu_item *item,
    menu_op operation,
    int current_page)
{
    int curletter = item->accelerator;
    boolean visible = (item->page_num == current_page);

    if (operation == DESELECT || (item->selected && operation == INVERT)) {
        item->selected = FALSE;
        item->count = -1L;
        if (visible) {
            mvwaddch(win, item->line_num + 1, 1, ' ');
            curses_toggle_color_attr(win, HIGHLIGHT_COLOR, NONE, ON);
            mvwaddch(win, item->line_num + 1, 2, curletter);
            curses_toggle_color_attr(win, HIGHLIGHT_COLOR, NONE, OFF);
            mvwaddch(win, item->line_num + 1, 3, ')');
        }
    } else {
        item->selected = TRUE;
        if (visible) {
            curses_toggle_color_attr(win, HIGHLIGHT_COLOR, A_REVERSE, ON);
            mvwaddch(win, item->line_num + 1, 1, '<');
            mvwaddch(win, item->line_num + 1, 2, curletter);
            mvwaddch(win, item->line_num + 1, 3, '>');
            curses_toggle_color_attr(win, HIGHLIGHT_COLOR, A_REVERSE, OFF);
        }
    }
    if (visible)
        wrefresh(win);
}


/* Perform the selected operation (select, unselect, invert selection)
on the given menu page.  If menu_page is 0, then perform opetation on
all pages in menu.  Returns last page displayed.  */

static int
menu_operation(
    WINDOW *win,
    nhmenu *menu,
    menu_op operation,
    int page_num)
{
    int first_page, last_page, current_page;
    nhmenu_item *menu_item_ptr = menu->entries;

    if (page_num == 0) {        /* Operation to occur on all pages */
        first_page = 1;
        last_page = menu->num_pages;
    } else {
        first_page = page_num;
        last_page = page_num;
    }

    /* Cycle through entries until we are on the correct page */
    while (menu_item_ptr != NULL) {
        if (menu_item_ptr->page_num == first_page) {
            break;
        }
        menu_item_ptr = menu_item_ptr->next_item;
    }

    current_page = first_page;

    if (page_num == 0) {
        menu_display_page(menu, win, current_page, (char *) 0, (char *) 0);
    }

    if (menu_item_ptr == NULL) {        /* Page not found */
        impossible("menu_display_page: attempt to display nonexistent page");
        return 0;
    }

    while (menu_item_ptr != NULL) {
        if (menu_item_ptr->page_num != current_page) {
            if (menu_item_ptr->page_num > last_page) {
                break;
            }

            current_page = menu_item_ptr->page_num;
            menu_display_page(menu, win, current_page, (char *) 0, (char *) 0);
        }

        if (menu_item_ptr->identifier.a_void != NULL) {
            if  (menuitem_invert_test(((operation == INVERT) ? 0
                                       : (operation == SELECT) ? 1 : 2),
                                      menu_item_ptr->itemflags,
                                      menu_item_ptr->selected))
                menu_select_deselect(win, menu_item_ptr,
                                     operation, current_page);
        }

        menu_item_ptr = menu_item_ptr->next_item;
    }

    return current_page;
}


/* Set all menu items to unselected in menu */

static void
menu_clear_selections(nhmenu *menu)
{
    nhmenu_item *menu_item_ptr = menu->entries;

    while (menu_item_ptr != NULL) {
        menu_item_ptr->selected = FALSE;
        menu_item_ptr = menu_item_ptr->next_item;
    }
}


/* Get the maximum height for a menu */

static int
menu_max_height(void)
{
    return term_rows - 2;
}

/*cursdial.c*/
