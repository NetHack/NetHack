// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_bind.cpp -- bindings between the Qt 4 interface and the main code

extern "C" {
#include "hack.h"
}

#include "qt_pre.h"
#include <QtGui/QtGui>
#include <QtCore/QStringList>
#if QT_VERSION < 0x050000
#include <QtGui/QSoundEffect>
#elif QT_VERSION < 0x060000
#include <QtWidgets/QtWidgets>
#include <QtMultimedia/QSoundEffect>
#else
/* Qt6 or above */
#include <QtWidgets/QtWidgets>
#include <QSoundEffect>
#endif  /* QT_VERSION */
#include "qt_post.h"
#include "qt_bind.h"
#include "qt_click.h"
#ifdef TIMED_DELAY
#include "qt_delay.h"
#endif
#include "qt_xcmd.h"
#include "qt_key.h"
#include "qt_map.h"
#include "qt_menu.h"
#include "qt_msg.h"
#include "qt_plsel.h"
#include "qt_svsel.h"
#include "qt_set.h"
#include "qt_stat.h"
#include "qt_streq.h"
#include "qt_yndlg.h"
#include "qt_str.h"

extern "C" {
#include "dlb.h"
}

// temporary
extern int qt_compact_mode;
// end temporary

namespace nethack_qt_ {

// XXX Should be from Options [or from Qt Settings (aka Preferences)].
//
// XXX Hmm.  Tricky part is that perhaps some macros should only be active
// XXX       when a key is about to be gotten.  For example, the user could
// XXX       define "-" to do "E-yyyyyyyy\r", but would still need "-" for
// XXX       other purposes.  Maybe just too bad.
//
static struct key_macro_rec {
    int key;
    uint state;
    const char *macro, *numpad_macro;
} key_macro[]={
    { Qt::Key_F1,  0U, "100.", "n100." }, // Rest (x100)
    { Qt::Key_F2,  0U, "20s",  "n20s"  }, // Search (x20)
    { Qt::Key_Tab, 0U, "\001", "\001"  }, // ^A (Do-again)
    { 0, 0U, (const char *) 0, (const char *) 0 }
};

NetHackQtBind::NetHackQtBind(int& argc, char** argv) :
#ifdef KDE
    KApplication(argc,argv)
#elif defined(QWS) // not quite the right condition
    QPEApplication(argc,argv)
#else
    QApplication(argc,argv)
#endif
{
    splash = 0;
    if (iflags.wc_splash_screen)
        NetHackQtBind::qt_Splash(); // show something while starting up

    // these used to be in MainWindow but we want them before QtSettings
    // which we want before MainWindow...
    QCoreApplication::setOrganizationName("The NetHack DevTeam");
    QCoreApplication::setOrganizationDomain("nethack.org");
    QCoreApplication::setApplicationName("NetHack-Qt"); // Qt NetHack
    {
        char cvers[BUFSZ];
        QString qvers = QString(::version_string(cvers, sizeof cvers));
        QCoreApplication::setApplicationVersion(qvers);
    }
#ifdef MACOS
    /* without this, neither control+x nor option+x do anything;
       with it, control+x is ^X and option+x still does nothing */
    QCoreApplication::setAttribute(Qt::AA_MacDontSwapCtrlAndMeta);
#endif

    qt_settings = new NetHackQtSettings(); /*(main->width(),main->height());*/

    main = new NetHackQtMainWindow(keybuffer);
    connect(qApp, SIGNAL(lastWindowClosed()), qApp, SLOT(quit()));
    msgs_strings = new QStringList();
    msgs_initd = false;
    msgs_saved = false;
}

// before the game windows have been rendered, display a small, centered
// window showing a flying red dragon ridden by someone wielding a lance,
// with caption "Loading..."
void
NetHackQtBind::qt_Splash()
{
    QPixmap pm("nhsplash.xpm"); // load splash image from a file in HACKDIR
    if (!pm.isNull()) {
        splash = new QFrame(NULL, (Qt::FramelessWindowHint
                                   | Qt::X11BypassWindowManagerHint
                                   | Qt::WindowStaysOnTopHint));
        QVBoxLayout *vb = new QVBoxLayout(splash);
        QLabel *lsplash = new QLabel(splash);
        vb->addWidget(lsplash);
        lsplash->setAlignment(Qt::AlignCenter);
        lsplash->setPixmap(pm);
        lsplash->setFixedSize(pm.size());
        //lsplash->setMask(pm.mask());
        QLabel *capt = new QLabel("Loading...", splash);
        vb->addWidget(capt);
        capt->setAlignment(Qt::AlignCenter);

#if QT_VERSION < 0x060000
        QSize screensize = QApplication::desktop()->size();
#else
        QSize screensize = splash->screen()->size();
#endif
        splash->move((screensize.width() - pm.width()) / 2,
                     (screensize.height() - pm.height()) / 2);
        //splash->setGeometry(0,0,100,100);
        if (qt_compact_mode) {
            splash->showMaximized();
        } else {
#if __cplusplus >= 202002L
            splash->setFrameStyle(static_cast<int>(QFrame::WinPanel)
                                     | static_cast<int>(QFrame::Raised));
#else
            splash->setFrameStyle(QFrame::WinPanel | QFrame::Raised);
#endif
            splash->setLineWidth(10);
            splash->adjustSize();
            splash->show();
        }

        // force content refresh outside event loop
        splash->repaint();
        lsplash->repaint();
        capt->repaint();
        qApp->processEvents();
    } else {
        splash = 0; // caller has alrady done this...
    }
}

void NetHackQtBind::qt_init_nhwindows(int *argc, char **argv)
{
    // menu entries use embedded <tab> to align fields;
    // it could be toggled off via 'O', but only when in wizard mode
    ::iflags.menu_tab_sep = true;

    // force high scores display to be shown in a window, and don't allow
    // that to be toggled off via 'O' (note: 'nethack -s' won't reach here;
    // its output goes to stdout so can potentially be redirected into a file)
    ::iflags.toptenwin = true;
    ::set_option_mod_status("toptenwin", ::set_in_config);

#ifdef UNIX
// Userid control
//
// Michael Hohmuth <hohmuth@inf.tu-dresden.de>...
//
// As the game runs setuid games, it must seteuid(getuid()) before
// calling XOpenDisplay(), and reset the euid afterwards.
// Otherwise, it can't read the $HOME/.Xauthority file and whines about
// not being able to open the X display (if a magic-cookie
// authorization mechanism is being used). 

    uid_t gamesuid=geteuid();
    seteuid(getuid());
#endif

    instance=new NetHackQtBind(*argc,argv);

#ifdef UNIX
    seteuid(gamesuid);
#endif

#ifdef _WS_WIN_
    // This nethack engine feature should be moved into windowport API
    nt_kbhit = NetHackQtBind::qt_kbhit;
#endif

#ifndef DYNAMIC_STATUSLINES
    // 'statuslines' option can be set in config file but not via 'O'
    set_wc2_option_mod_status(WC2_STATUSLINES, set_gameview);
#endif
}

int NetHackQtBind::qt_kbhit()
{
    return !keybuffer.Empty();
}


static bool have_asked = false;

void NetHackQtBind::qt_player_selection()
{
    if ( !have_asked )
	qt_askname();
}

void NetHackQtBind::qt_askname()
{
    char default_plname[PL_NSIZ];
    int ch = -1; // -1 => new game

    have_asked = true;
    str_copy(default_plname, g.plname, PL_NSIZ);

    // We do it all here (plus qt_plsel.cpp and qt_svsel.cpp),
    // nothing in player_selection().

#ifdef SELECTSAVED
    char **saved = 0;
    if (::iflags.wc2_selectsaved)
        saved = get_saved_games();
    if (saved && *saved) {
        if (splash)
            splash->hide();
        NetHackQtSavedGameSelector sgsel((const char **) saved);
        ch = sgsel.choose();
        if (ch >= 0)
            str_copy(g.plname, saved[ch], SIZE(g.plname));
        // caller needs new lock name even if plname[] hasn't changed
        // because successful get_saved_games() clobbers g.SAVEF[]
        ::iflags.renameinprogress = TRUE;
    }
    free_saved_games(saved);
#endif

    switch (ch) {
    case -1:
        // New Game
        if (splash)
            splash->hide();
        if (NetHackQtPlayerSelector(keybuffer).Choose()) {
            // success; handle plname[] verification below prior to returning
            break;
        }
        /*FALLTHRU*/
    case -2:
        // Quit
        clearlocks();
        qt_exit_nhwindows(0);
        nh_terminate(0);
        /*NOTREACHED*/
        break;
    default:
        // picked a character from the saved games list
        break;
    }

    if (!*g.plname)
        // in case Choose() returns with plname[] empty
        Strcpy(g.plname, default_plname);
    else if (strcmp(g.plname, default_plname) != 0)
        // caller needs to set new lock file name
        ::iflags.renameinprogress = TRUE;
    return;
}

void NetHackQtBind::qt_get_nh_event()
{
}

#if defined(QWS)
// Kludge to access lastWindowClosed() signal.
class TApp : public QApplication {
public:
    TApp(int& c, char**v) : QApplication(c,v) {}
    void lwc() { emit lastWindowClosed(); }
};
#endif
 
void NetHackQtBind::qt_exit_nhwindows(const char *)
{
#if defined(QWS)
    // Avoids bug in SHARP SL5500
    ((TApp*)qApp)->lwc();
    qApp->quit();
#endif
 
    delete instance; // ie. qApp
}

void NetHackQtBind::qt_suspend_nhwindows(const char *)
{
}

void NetHackQtBind::qt_resume_nhwindows()
{
}

static QVector<NetHackQtWindow*> id_to_window;

winid NetHackQtBind::qt_create_nhwindow(int type)
{
    winid id;
    for (id = 0; id < (winid) id_to_window.size(); id++) {
	if ( !id_to_window[(int)id] )
	    break;
    }
    if ( id == (winid) id_to_window.size() )
	id_to_window.resize(id+1);

    NetHackQtWindow* window=0;

    switch (type) {
    case NHW_MAP: {
	NetHackQtMapWindow2* w=new NetHackQtMapWindow2(clickbuffer);
	main->AddMapWindow(w);
	window=w;
        break;
    }
    case NHW_MESSAGE: {
	NetHackQtMessageWindow* w=new NetHackQtMessageWindow;
	main->AddMessageWindow(w);
	window=w;
        break;
    }
    case NHW_STATUS: {
	NetHackQtStatusWindow* w=new NetHackQtStatusWindow;
	main->AddStatusWindow(w);
	window=w;
        break;
    }
    case NHW_MENU:
	window=new NetHackQtMenuOrTextWindow(mainWidget());
        break;
    case NHW_TEXT:
	window=new NetHackQtTextWindow(mainWidget());
        break;
    }

    window->nhid = id;

    // Note: use of isHidden does not work with Qt 2.1
    if ( splash 
#if QT_VERSION >= 300
        && !main->isHidden()
#else
	&& main->isVisible()
#endif
	) {
	delete splash;
	splash = 0;
    }

    id_to_window[(int)id] = window;
    return id;
}

void NetHackQtBind::qt_clear_nhwindow(winid wid)
{
    NetHackQtWindow* window=id_to_window[(int)wid];
    if (window)
        window->Clear();
}

void NetHackQtBind::qt_display_nhwindow(winid wid, boolean block)
{
    NetHackQtWindow* window=id_to_window[(int)wid];
    if (window)
        window->Display(block);
}

void NetHackQtBind::qt_destroy_nhwindow(winid wid)
{
    NetHackQtWindow* window=id_to_window[(int)wid];
    if (window) {
        main->RemoveWindow(window);
        if (window->Destroy())
            delete window;
        id_to_window[(int) wid] = 0;
    }
}

void NetHackQtBind::qt_curs(winid wid, int x, int y)
{
    NetHackQtWindow* window=id_to_window[(int)wid];
    window->CursorTo(x,y);
}

void NetHackQtBind::qt_putstr(winid wid, int attr, const char *text)
{
    NetHackQtWindow* window=id_to_window[(int)wid];
    window->PutStr(attr,QString::fromLatin1(text));
}

void NetHackQtBind::qt_putstr(winid wid, int attr, const std::string& text)
{
    NetHackQtWindow* window=id_to_window[(int)wid];
    window->PutStr(attr,QString::fromLatin1(text.c_str(), text.size()));
}

void NetHackQtBind::qt_putstr(winid wid, int attr, const QString& text)
{
    NetHackQtWindow* window=id_to_window[(int)wid];
    window->PutStr(attr,text);
}

void NetHackQtBind::qt_display_file(const char *filename, boolean must_exist)
{
    NetHackQtTextWindow* window=new NetHackQtTextWindow(mainWidget());
    bool complain = false;

    {
	dlb *f;
	char buf[BUFSZ];
	char *cr;

	window->Clear();
	f = dlb_fopen(filename, "r");
	if (!f) {
	    complain = must_exist;
	} else {
	    while (dlb_fgets(buf, BUFSZ, f)) {
		if ((cr = strchr(buf, '\n')) != 0) *cr = 0;
#ifdef MSDOS
		if ((cr = strchr(buf, '\r')) != 0) *cr = 0;
#endif
		window->PutStr(ATR_NONE, tabexpand(buf));
	    }
	    window->Display(false);
	    (void) dlb_fclose(f);
	}
    }

    if (complain) {
	QString message = QString::asprintf("File not found: %s\n",filename);
	QMessageBox::warning(NULL, "File Error", message, QMessageBox::Ignore);
    }
}

void NetHackQtBind::qt_start_menu(winid wid, unsigned long mbehavior UNUSED)
{
    NetHackQtWindow* window=id_to_window[(int)wid];
    window->StartMenu(wid == WIN_INVEN);
}

void NetHackQtBind::qt_add_menu(winid wid, const glyph_info *glyphinfo,
    const ANY_P * identifier, char ch, char gch, int attr, int clr UNUSED,
    const char *str, unsigned itemflags)
{
    NetHackQtWindow* window=id_to_window[(int)wid];
    window->AddMenu(glyphinfo->glyph, identifier, ch, gch, attr,
            QString::fromLatin1(str),
            itemflags);
}

void NetHackQtBind::qt_end_menu(winid wid, const char *prompt)
{
    NetHackQtWindow* window=id_to_window[(int)wid];
    window->EndMenu(prompt);
}

int NetHackQtBind::qt_select_menu(winid wid, int how, MENU_ITEM_P **menu_list)
{
    NetHackQtWindow* window=id_to_window[(int)wid];
    return window->SelectMenu(how,menu_list);
}

void NetHackQtBind::qt_update_inventory(int arg UNUSED)
{
    if (main)
	main->updateInventory(); // update the paper doll inventory subset

    /* doesn't work yet
    if (g.program_state.something_worth_saving && iflags.perm_invent)
        display_inventory(NULL, false);
    */
}

win_request_info *NetHackQtBind::qt_ctrl_nhwindow(
    winid wid UNUSED,
    int request UNUSED,
    win_request_info *wri UNUSED)
{
    NetHackQtWindow* window UNUSED =id_to_window[(int)wid];
    return (win_request_info *) 0;
}

void NetHackQtBind::qt_mark_synch()
{
}

void NetHackQtBind::qt_wait_synch()
{
}

void NetHackQtBind::qt_cliparound(int x, int y)
{
    // XXXNH - winid should be a parameter!
    qt_cliparound_window(WIN_MAP,x,y);
}

void NetHackQtBind::qt_cliparound_window(winid wid, int x, int y)
{
    NetHackQtWindow* window=id_to_window[(int)wid];
    window->ClipAround(x,y);
}

void NetHackQtBind::qt_print_glyph(
    winid wid, coordxy x, coordxy y,
    const glyph_info *glyphinfo,
    const glyph_info *bkglyphinfo UNUSED)
{
    /* TODO: bkglyph */
    NetHackQtWindow *window = id_to_window[(int) wid];
    window->PrintGlyph(x, y, glyphinfo);
}

#if 0
void NetHackQtBind::qt_print_glyph_compose(
    winid wid, coordxy x, coordxy y, int glyph1, int glyph2)
{
    NetHackQtWindow *window = id_to_window[(int) wid];
    window->PrintGlyphCompose(x, y, glyph1, glyph2);
}
#endif /*0*/

//
// FIXME: sending output to stdout can mean that the player never sees it.
//
void NetHackQtBind::qt_raw_print(const char *str)
{
    puts(str);
}

void NetHackQtBind::qt_raw_print_bold(const char *str)
{
    qt_raw_print(str);
}

int NetHackQtBind::qt_nhgetch()
{
    if (main)
	main->fadeHighlighting(true);

    // Process events until a key arrives.
    //
    while (keybuffer.Empty()) {
        int exc = qApp->exec();
        /*
         * On OSX (possibly elsewhere), this prevents an infinite
         * loop repeatedly issuing the complaint:
QCoreApplication::exec: The event loop is already running
         * to stderr if you syncronously start nethack from a terminal
         * then switch focus back to that terminal and type ^C.
         *  SIGINT -> done1() -> done2() -> yn_function("Really quit?")
         * in the core asks for another keystroke.
         *
         * However, it still issues one such complaint, and whatever
         * prompt wanted a response ("Really quit?") is shown in the
         * message window but is auto-answered with ESC.
         */
        if (exc == -1)
            keybuffer.Put('\033');
    }

    // after getting a key rather than before
    if (main)
        main->fadeHighlighting(false);

    return keybuffer.GetAscii();
}

int NetHackQtBind::qt_nh_poskey(coordxy *x, coordxy *y, int *mod)
{
    if (main)
	main->fadeHighlighting(true);

    // Process events until a key or map-click arrives.
    //
    while (keybuffer.Empty() && clickbuffer.Empty()) {
        int exc = qApp->exec();
        // [see comment above in qt_nhgetch()]
        if (exc == -1)
            keybuffer.Put('\033');
    }

    // after getting a key or click rather than before
    if (main)
        main->fadeHighlighting(false);

    if (!keybuffer.Empty()) {
	return keybuffer.GetAscii();
    } else {
	*x=clickbuffer.NextX();
	*y=clickbuffer.NextY();
	*mod=clickbuffer.NextMod();
	clickbuffer.Get();
	return 0;
    }
}

void NetHackQtBind::qt_nhbell()
{
    QApplication::beep();
}

int NetHackQtBind::qt_doprev_message()
{
    // Don't need it - uses scrollbar
    // XXX but could make this a shortcut
    return 0;
}

// display "--More--" as a prompt and wait for a response from the user
//
// Used by qt_display_nhwindow(WIN_MESSAGE, TRUE) where second argument
// True requests blocking.  We need it to support MSGTYPE=stop but the
// core also uses that in various other situations.
char NetHackQtBind::qt_more()
{
    char ch = '\033';

    // without this gameover hack, quitting via menu or window close
    // button ends up provoking a complaint from qt_nhgetch() [see the
    // ^C comment in that routine] when the core triggers --More-- via
    //  done2() -> really_done() -> display_nhwindow(WIN_MESSAGE, TRUE)
    // (get rid of this if the exec() loop issue gets properly fixed)
    if (::g.program_state.gameover)
        return ch; // bypass --More-- and just continue with program exit

    NetHackQtMessageWindow *mesgwin = main ? main->GetMessageWindow() : NULL;

    // kill any typeahead; for '!popup_dialog' this forces qt_nhgetch()
    keybuffer.Drain();

    if (mesgwin && !::iflags.wc_popup_dialog && WIN_MESSAGE != WIN_ERR) {

        mesgwin->AddToStr("--More--");
        bool retry = false;
        int complain = 0;
        do {
            ch = NetHackQtBind::qt_nhgetch();
            switch (ch) {
            case '\0': // hypothetical
                ch = '\033';
                /*FALLTHRU*/
            case ' ':
            case '\n':
            case '\r':
            case '\033':
                retry = false;
                break;
            default:
                if (++complain > 1)
                    NetHackQtBind::qt_nhbell();
                // typing anything caused the most recent message line
                // (which happens to our prompt) from having highlighting
                // be removed; put that back
                if (mesgwin)
                    mesgwin->RehighlightPrompt();
                retry = true;
                break;
            }
        } while (retry);
        // unhighlight the line with the prompt; does not erase the window
        NetHackQtBind::qt_clear_nhwindow(WIN_MESSAGE);

    } else {
        // use a popup dialog box; unlike yn_function(), we don't show
        // the prompt+response in the message window
        NetHackQtYnDialog dialog(main, "--More--", " \033\n\r", ' ');
        ch = dialog.Exec();
        if (ch == '\0') {
            ch = '\033';
        }
        // discard any input that YnDialog() might have left pending
        keybuffer.Drain();
    }

    return ch;
}

char NetHackQtBind::qt_yn_function(const char *question_,
                                   const char *choices, char def)
{
    QString question(QString::fromLatin1(question_));
    QString message;
    char yn_esc_map='\033';
    int result = -1;

    if (choices) {
        QString choicebuf;
        for (const char *p = choices; *p; ++p) {
            if (*p == '\033') // <esc> and anything beyond is hidden
                break;
            choicebuf += visctrl(*p);
        }
        choicebuf.truncate(QBUFSZ - 1); // no effect if already shorter
        message = QString("%1 [%2] ").arg(question, choicebuf);
        if (def)
            message += QString("(%1) ").arg(QChar(def));
        // escape maps to 'q' or 'n' or default, in that order
        yn_esc_map = strchr(choices, 'q') ? 'q'
                     : strchr(choices, 'n') ? 'n'
                       : def;
    } else {
        message = question;
    }

    if (
        /*
         * The 'Settings' dialog doesn't present prompting-in-message-window
         * as a candidate for customization but core supports 'popup_dialog'
         * option so let player use that instead.
         */
#if 0
        qt_settings->ynInMessages()
#else
        !::iflags.wc_popup_dialog
#endif
        && WIN_MESSAGE != WIN_ERR) {
	// Similar to X11 windowport `slow' feature.

        char cbuf[20];
        cbuf[0] = '\0';

        // add the prompt to the messsage window
	NetHackQtBind::qt_putstr(WIN_MESSAGE, ATR_BOLD, message);

	while (result < 0) {
            cbuf[0] = '\0';
	    char ch=NetHackQtBind::qt_nhgetch();
	    if (ch=='\033') {
		result=yn_esc_map;
                Strcpy(cbuf, "ESC");
	    } else if (choices && !strchr(choices,ch)) {
		if (def && (ch==' ' || ch=='\r' || ch=='\n')) {
		    result=def;
                    Strcpy(cbuf, visctrl(def));
		} else {
		    NetHackQtBind::qt_nhbell();
                    // typing anything caused the most recent message line
                    // (which happens to our prompt) from having highlighting
                    // be removed; put that back
                    NetHackQtMessageWindow
                            *mesgwin = main ? main->GetMessageWindow() : NULL;
                    if (mesgwin)
                        mesgwin->RehighlightPrompt();
		    // and try again...
		}
	    } else {
		result=ch;
                Strcpy(cbuf, visctrl(ch));
	    }
	}

        // update the prompt message line to include the response
        if (cbuf[0]) {
            if (!strcmp(cbuf, " "))
                Strcpy(cbuf, "SPC");

            NetHackQtMessageWindow *mesgwin = main->GetMessageWindow();
            if (mesgwin)
                mesgwin->AddToStr(cbuf);
        }

    } else {
        // use a popup dialog box
        NetHackQtYnDialog dialog(main, question, choices, def);
        char ret = dialog.Exec();
        if (ret == 0) {
            ret = '\033';
        }
        // discard any input that YnDialog() might have left pending
        keybuffer.Drain();

        // combine the prompt and result
        char cbuf[40];
        Strcpy(cbuf, (ret == '\033') ? "ESC"
                     : (ret == ' ') ? "SPC"
                       : visctrl(ret));
        if (ret == '#' && choices && !strncmp(choices, "yn#", (size_t) 3))
            Sprintf(eos(cbuf), " %ld", ::yn_number);
        message += QString(" %1").arg(cbuf);

        // add the prompt with appended response to the message window
        if (WIN_MESSAGE != WIN_ERR)
            NetHackQtBind::qt_putstr(WIN_MESSAGE, ATR_BOLD, message);

        result = ret;
    }

    // unhighlight the prompt; does not erase the multi-line message window
    if (WIN_MESSAGE != WIN_ERR)
        NetHackQtBind::qt_clear_nhwindow(WIN_MESSAGE);

    return (char) result;
}

void NetHackQtBind::qt_getlin(const char *prompt, char *line)
{
    NetHackQtStringRequestor requestor(mainWidget(),prompt);
    if (!requestor.Get(line, BUFSZ, 40)) {
        Strcpy(line, "\033");
        // discard any input that Get() might have left pending
        keybuffer.Drain();
    }

    // add the prompt with appended response to the messsage window
    char buf[BUFSZ + 20], *q; /* +20: plenty of extra room for visctrl() */
    copynchars(buf, prompt, BUFSZ - 1);
    q = eos(buf);
    *q++ = ' '; /* guaranteed to fit; temporary lack of terminator is ok */

    if (line[0] == '\033') {
        Strcpy(q, "ESC");
    } else if (line[0] == ' ' && !line[1]) {
        Strcpy(q, "SPC");
    } else {
        /* buf[] has more than enough room to hold one extra visctrl()
           in case q is at the last viable slot and *p yields "M-^c" */
        for (char *p = line; *p && q < &buf[BUFSZ - 1]; ++p, q = eos(q))
            Strcpy(q, visctrl(*p));
    }
    if (q > &buf[BUFSZ - 1])
        q = &buf[BUFSZ - 1];
    *q = '\0';

    NetHackQtBind::qt_putstr(WIN_MESSAGE, ATR_BOLD, buf);
    // unhighlight the prompt; does not erase the multi-line message window
    NetHackQtBind::qt_clear_nhwindow(WIN_MESSAGE);
}

// User has typed '#' to begin entering an extended command; core calls us.
int NetHackQtBind::qt_get_ext_cmd()
{
    NetHackQtExtCmdRequestor *xcmd;
    int result;
    do {
        xcmd = new NetHackQtExtCmdRequestor(mainWidget());
        result = xcmd->get();
        delete xcmd;
    } while (result == xcmdNoMatch);
    // refresh message window after extended command dialog is dismissed
    NetHackQtBind::qt_clear_nhwindow(WIN_MESSAGE);
    return result;
}

void NetHackQtBind::qt_number_pad(int)
{
    // Ignore.
}

void NetHackQtBind::qt_delay_output()
{
#ifdef TIMED_DELAY
    NetHackQtDelay delay(50);
    delay.wait();
#endif
}

void NetHackQtBind::qt_start_screen()
{
    // Ignore.
}

void NetHackQtBind::qt_end_screen()
{
    // Ignore.
}

void NetHackQtBind::qt_outrip(winid wid, int how, time_t when)
{
    NetHackQtWindow* window=id_to_window[(int)wid];
    window->UseRIP(how, when);
}

void NetHackQtBind::qt_preference_update(const char *optname)
{
#ifdef DYNAMIC_STATUSLINES  // defined in qt_main.h
    if (!strcmp(optname, "statuslines")) {
        // delete and recreate status window
        // to toggle statuslines from 2 to 3 or vice versa
        id_to_window[WIN_STATUS] = main->redoStatus();
    }
#else
    nhUse(optname);
#endif
}

char *NetHackQtBind::qt_getmsghistory(boolean init)
{
    NetHackQtMessageWindow *window = main->GetMessageWindow();
    if (window)
        return (char *) window->GetStr((bool) init);
    return NULL;
}

void NetHackQtBind::qt_putmsghistory(const char *msg, boolean is_restoring)
{
    NetHackQtMessageWindow *window = main->GetMessageWindow();
    if (!window)
        return;

    if (is_restoring && !msgs_initd) {
        /* we're restoring history from the previous session, but new
           messages have already been issued this session */
        int i = 0;
        const char *str;

        while ((str = window->GetStr((bool) (i == 0))) != 0) {
            msgs_strings->append(str);
            i++;
        }
        msgs_initd = true;
        msgs_saved = (i > 0);
        window->ClearMessages();
    }

    if (msg) {
        //raw_printf("msg='%s'", msg);
        window->PutStr(ATR_NONE, QString::fromLatin1(msg));
#ifdef DUMPLOG
        dumplogmsg(msg);
#endif
    } else if (msgs_saved) {
        /* restore strings */
        for (int i = 0; i < msgs_strings->size(); ++i) {
            const QString &nxtmsg = msgs_strings->at(i);
            window->PutStr(ATR_NONE, nxtmsg);
#ifdef DUMPLOG
            dumplogmsg(nxtmsg.toLatin1().constData());
#endif
        }
        delete msgs_strings;
        msgs_initd = false;
    }
}

// event loop callback
bool NetHackQtBind::notify(QObject *receiver, QEvent *event)
{
    // Ignore Alt-key navigation to menubar, it's annoying when you
    // use Alt-Direction to move around.
    if (main && receiver == main && event->type() == QEvent::KeyRelease
        && ((QKeyEvent *) event)->key() == Qt::Key_Alt)
        return true;

    bool result = QApplication::notify(receiver, event);
    int evtyp = event->type();

    if (evtyp == QEvent::KeyPress) {
        QKeyEvent *key_event = (QKeyEvent *) event;

        if (!key_event->isAccepted()) {
            Qt::KeyboardModifiers mod = key_event->modifiers();
            const int k = key_event->key();
            for (int i = 0; key_macro[i].key; i++) {
                if (key_macro[i].key == k
                    && ((key_macro[i].state & mod) == key_macro[i].state)) {
                    // matched macro; put its expansion into the input buffer
                    keybuffer.Put(!::iflags.num_pad ? key_macro[i].macro
                                  : key_macro[i].numpad_macro);
                    key_event->accept();
                    qApp->exit();
                    return true;
                }
            }
            QString key = key_event->text();
            QChar ch = !key.isEmpty() ? key.at(0) : QChar(0);
            if (ch > QChar(128))
                ch = QChar(0);
            // on OSX, ascii control codes are not sent, force them
            if (ch == 0 && (mod & Qt::ControlModifier) != 0) {
                if (k >= Qt::Key_A && k <= Qt::Key_Underscore)
                    ch = (QChar) (k - (Qt::Key_A - 1));
            }
            //raw_printf("notify()=%d \"%s\"", k, visctrl(ch.cell()));
            // if we have a valid character, queue it up
            if (ch != 0) {
                bool alt = ((mod & Qt::AltModifier) != 0
                            || (k >= Qt::Key_0 && k <= Qt::Key_9
                                && (mod & Qt::ControlModifier) != 0));
                keybuffer.Put(k, ch.cell() + (alt ? 128 : 0), (uint) mod);
                key_event->accept();
                qApp->exit();
                result = true;
            }

#if 0   /* this was a failed attempt to prevent qt_more() from looping
         * after command+q (on OSX) is used to bring up the quit dialog;
         * now qt_more() uses an early return if program_state.gameover
         * is set */
        } else if (evtyp == QEvent::FocusOut
                   || evtyp == QEvent::ShortcutOverride
                   || evtyp == QEvent::PlatformSurface) {
            // leave qt_nhgetch()'s event loop if focus switches somewhere else
            qApp->exit();
            result = false;
#endif
        }
    }
    return result;
}

NetHackQtBind* NetHackQtBind::instance=0;
NetHackQtKeyBuffer NetHackQtBind::keybuffer;
NetHackQtClickBuffer NetHackQtBind::clickbuffer;
NetHackQtMainWindow* NetHackQtBind::main=0;
QFrame* NetHackQtBind::splash=0;
QStringList *NetHackQtBind::msgs_strings;
boolean NetHackQtBind::msgs_saved = false;
boolean NetHackQtBind::msgs_initd = false;
#if 0
static void Qt_positionbar(char *) {}
#endif
} // namespace nethack_qt_

struct window_procs Qt_procs = {
    WPID(Qt),
    (WC_COLOR | WC_HILITE_PET
     | WC_ASCII_MAP | WC_TILED_MAP
     | WC_FONT_MAP | WC_TILE_FILE | WC_TILE_WIDTH | WC_TILE_HEIGHT
     | WC_POPUP_DIALOG | WC_PLAYER_SELECTION | WC_SPLASH_SCREEN),
    (WC2_HITPOINTBAR
#ifdef SELECTSAVED
     | WC2_SELECTSAVED
#endif
#ifdef ENHANCED_SYMBOLS
     | WC2_U_UTF8STR | WC2_U_24BITCOLOR
#endif
     | WC2_STATUSLINES),
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}, /* color availability */
    nethack_qt_::NetHackQtBind::qt_init_nhwindows,
    nethack_qt_::NetHackQtBind::qt_player_selection,
    nethack_qt_::NetHackQtBind::qt_askname,
    nethack_qt_::NetHackQtBind::qt_get_nh_event,
    nethack_qt_::NetHackQtBind::qt_exit_nhwindows,
    nethack_qt_::NetHackQtBind::qt_suspend_nhwindows,
    nethack_qt_::NetHackQtBind::qt_resume_nhwindows,
    nethack_qt_::NetHackQtBind::qt_create_nhwindow,
    nethack_qt_::NetHackQtBind::qt_clear_nhwindow,
    nethack_qt_::NetHackQtBind::qt_display_nhwindow,
    nethack_qt_::NetHackQtBind::qt_destroy_nhwindow,
    nethack_qt_::NetHackQtBind::qt_curs,
    nethack_qt_::NetHackQtBind::qt_putstr,
    genl_putmixed,
    nethack_qt_::NetHackQtBind::qt_display_file,
    nethack_qt_::NetHackQtBind::qt_start_menu,
    nethack_qt_::NetHackQtBind::qt_add_menu,
    nethack_qt_::NetHackQtBind::qt_end_menu,
    nethack_qt_::NetHackQtBind::qt_select_menu,
    genl_message_menu,      /* no need for Qt-specific handling */
    nethack_qt_::NetHackQtBind::qt_mark_synch,
    nethack_qt_::NetHackQtBind::qt_wait_synch,
#ifdef CLIPPING
    nethack_qt_::NetHackQtBind::qt_cliparound,
#endif
#ifdef POSITIONBAR
    nethack_qt_::Qt_positionbar,
#endif
    nethack_qt_::NetHackQtBind::qt_print_glyph,
    //NetHackQtBind::qt_print_glyph_compose,
    nethack_qt_::NetHackQtBind::qt_raw_print,
    nethack_qt_::NetHackQtBind::qt_raw_print_bold,
    nethack_qt_::NetHackQtBind::qt_nhgetch,
    nethack_qt_::NetHackQtBind::qt_nh_poskey,
    nethack_qt_::NetHackQtBind::qt_nhbell,
    nethack_qt_::NetHackQtBind::qt_doprev_message,
    nethack_qt_::NetHackQtBind::qt_yn_function,
    nethack_qt_::NetHackQtBind::qt_getlin,
    nethack_qt_::NetHackQtBind::qt_get_ext_cmd,
    nethack_qt_::NetHackQtBind::qt_number_pad,
    nethack_qt_::NetHackQtBind::qt_delay_output,
#ifdef CHANGE_COLOR     /* only a Mac option currently */
    donull,
    donull,
    donull,
    donull,
#endif
    /* other defs that really should go away (they're tty specific) */
    nethack_qt_::NetHackQtBind::qt_start_screen,
    nethack_qt_::NetHackQtBind::qt_end_screen,
#ifdef GRAPHIC_TOMBSTONE
    nethack_qt_::NetHackQtBind::qt_outrip,
#else
    genl_outrip,
#endif
    nethack_qt_::NetHackQtBind::qt_preference_update,
    nethack_qt_::NetHackQtBind::qt_getmsghistory,
    nethack_qt_::NetHackQtBind::qt_putmsghistory,
    genl_status_init,
    genl_status_finish, genl_status_enablefield,
#ifdef STATUS_HILITES
    genl_status_update,
#else
    genl_status_update,
#endif
    genl_can_suspend_yes,
    nethack_qt_::NetHackQtBind::qt_update_inventory,
    nethack_qt_::NetHackQtBind::qt_ctrl_nhwindow,
};

#ifndef WIN32
extern "C" void play_usersound(const char *, int);

QSoundEffect *effect = NULL;

/* called from core, sounds.c */
void
play_usersound(const char *filename, int volume)
{
#if defined(USER_SOUNDS) && !defined(QT_NO_SOUND)
    if (!effect)
        effect = new QSoundEffect(nethack_qt_::NetHackQtBind::mainWidget());
    if (effect) {
        effect->setLoopCount(1);
        effect->setVolume((1.00f * volume) / 100.0f);
        effect->setSource(QUrl::fromLocalFile(filename));
        effect->play();
    }
#else
    nhUse(filename);
    nhUse(volume);
#endif
}
#endif /*!WIN32*/

//qt_bind.cpp
