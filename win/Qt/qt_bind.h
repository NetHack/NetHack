// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_bind.h -- bindings between the Qt 4 interface and the main code

#ifndef QT4BIND_H
#define QT4BIND_H

#include "qt_main.h"

namespace nethack_qt_ {

class NetHackQtClickBuffer;

#ifdef KDE
#define NetHackQtBindBase KApplication
#elif defined(QWS)
#define NetHackQtBindBase QPEApplication
#else
#define NetHackQtBindBase QApplication
#endif

class NetHackQtBind : NetHackQtBindBase {
private:
	// Single-instance preservation...
	NetHackQtBind(int& argc, char** argv);

	static NetHackQtBind* instance;

	static NetHackQtKeyBuffer keybuffer;
	static NetHackQtClickBuffer clickbuffer;

	static QFrame* splash;
	static NetHackQtMainWindow* main;

public:
        static void qt_Splash();

        /* window interface */

	static void qt_init_nhwindows(int* argc, char** argv);
	static void qt_player_selection();
	static void qt_askname();
	static void qt_get_nh_event();
	static void qt_exit_nhwindows(const char *);
	static void qt_suspend_nhwindows(const char *);
	static void qt_resume_nhwindows();
	static winid qt_create_nhwindow(int type);
	static void qt_clear_nhwindow(winid wid);
	static void qt_display_nhwindow(winid wid, boolean block);
	static void qt_destroy_nhwindow(winid wid);
	static void qt_curs(winid wid, int x, int y);
	static void qt_putstr(winid wid, int attr, const char *text);
	static void qt_putstr(winid wid, int attr, const std::string& text);
	static void qt_putstr(winid wid, int attr, const QString& text);
	static void qt_display_file(const char *filename, boolean must_exist);
	static void qt_start_menu(winid wid, unsigned long mbehavior);
	static void qt_add_menu(winid wid, const glyph_info *glyphinfo,
		const ANY_P * identifier, char ch, char gch, int attr, int clr,
		const char *str, unsigned int itemflags);
	static void qt_end_menu(winid wid, const char *prompt);
	static int qt_select_menu(winid wid, int how, MENU_ITEM_P **menu_list);
	static void qt_mark_synch();
	static void qt_wait_synch();

	static void qt_cliparound(int x, int y);
	static void qt_cliparound_window(winid wid, int x, int y);
        static void qt_print_glyph(winid wid, coordxy x, coordxy y,
                                   const glyph_info *glyphingo, 
				   const glyph_info *bkglyphinfo);
	static void qt_raw_print(const char *str);
	static void qt_raw_print_bold(const char *str);
	static int qt_nhgetch();
	static int qt_nh_poskey(coordxy *x, coordxy *y, int *mod);
	static void qt_nhbell();
	static int qt_doprev_message();
        static char qt_more();
        static char qt_yn_function(const char *question,
                                   const char *choices, char def);
	static void qt_getlin(const char *prompt, char *line);
	static int qt_get_ext_cmd();
	static void qt_number_pad(int);
	static void qt_delay_output();
	static void qt_start_screen();
	static void qt_end_screen();

        static void qt_preference_update(const char *optname);
        static char *qt_getmsghistory(boolean init);
        static void qt_putmsghistory(const char *msg, boolean is_restoring);

	static void qt_outrip(winid wid, int how, time_t when);
	static int qt_kbhit();
	static void qt_update_inventory(int);
        static win_request_info *qt_ctrl_nhwindow(winid, int, win_request_info *);
	static QWidget *mainWidget() { return main; }
#if defined(SND_LIB_QTSOUND) && !defined(QT_NO_SOUND)
        /* sound interface */
        static void qtsound_init_nhsound(void);
        static void qtsound_exit_nhsound(const char *);
        static void qtsound_achievement(schar, schar, int32_t);
        static void qtsound_soundeffect(char *, int32_t, int32_t);
        static void qtsound_hero_playnotes(int32_t instrument, const char *str, int32_t volume);
        static void qtsound_play_usersound(char *, int32_t, int32_t);
        static void qtsound_ambience(int32_t, int32_t, int32_t);
        static void qtsound_verbal(char *text, int32_t gender, int32_t tone, int32_t vol, int32_t moreinfo);
#endif

private:
	virtual bool notify(QObject *receiver, QEvent *event);

        static QStringList *msgs_strings;
        static boolean msgs_saved;
        static boolean msgs_initd;
};

} // namespace nethack_qt_

#endif
