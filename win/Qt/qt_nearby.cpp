// Copyright (c) Warwick Allison, 1999
// Inventory menu code (c) Kestrel Gregorich-Trevor, 2021.
// NetHack may be freely redistributed. See license for details.

// qt_nearby.cpp -- a nearby characters window

extern "C" {
#include "hack.h"
}

#include "qt_pre.h"
#include <QtGui/QtGui>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QtWidgets>
#endif
#include "qt_post.h"
#include "qt_nearby.h"
#include "qt_nearby.moc"
#include "qt_map.h"
#include "qt_set.h"
#include "qt_str.h"


namespace nethack_qt_ {

NetHackQtNearbyWindow::NetHackQtNearbyWindow() :
    list(new QListWidget()),
    scrollarea(new QScrollArea())
{
    list->setFocusPolicy(Qt::NoFocus);
    scrollarea->setFocusPolicy(Qt::NoFocus);
    scrollarea->takeWidget();
    ::iflags.window_inited = 1;
    map = 0;
    connect(qt_settings,SIGNAL(fontChanged()),this,SLOT(updateFont()));
    updateFont();
}

NetHackQtNearbyWindow::~NetHackQtNearbyWindow()
{
    ::iflags.window_inited = 0;
    delete list;
}

QWidget* NetHackQtNearbyWindow::Widget() {
    return list;
}

void NetHackQtNearbyWindow::setMap(NetHackQtMapWindow2* m)
{
    map = m;
    updateFont();
}

void NetHackQtNearbyWindow::updateFont()
{
    list->setFont(qt_settings->normalFont());
    if ( map )
	map->setFont(qt_settings->normalFont());
}

void NetHackQtNearbyWindow::Scroll(int dx UNUSED, int dy UNUSED)
{
    //RLC Is this necessary?
    //RLC list->Scroll(dx,dy);
}

void NetHackQtNearbyWindow::Update(int toggle)
{

    int cx, cy, lo_x, lo_y, hi_x, hi_y, glyph;
    int count = 0;
    char lookbuf[BUFSZ], outbuf[BUFSZ], glyphbuf[40];
    boolean peaceful = false;
    glyph_info glyphinfo;

    // Clear the list of nearby characters
    list->clear();

    // Toggle display
    if (toggle) monsters = !monsters;

    // Add the list items
    lo_y = TRUE ? std::max(u.uy - BOLT_LIM, 0) : 0;
    lo_x = TRUE ? std::max(u.ux - BOLT_LIM, 1) : 1;
    hi_y = TRUE ? std::min(u.uy + BOLT_LIM, ROWNO - 1) : ROWNO - 1;
    hi_x = TRUE ? std::min(u.ux + BOLT_LIM, COLNO - 1) : COLNO - 1;
    for (cy = lo_y; cy <= hi_y; cy++) {
        for (cx = lo_x; cx <= hi_x; cx++) {
            lookbuf[0] = '\0';
            glyph = glyph_at(cx, cy);
            map_glyphinfo(0, 0, glyph, 0, &glyphinfo);
            peaceful = FALSE;
            if (monsters) {
                if (glyph_is_monster(glyph)) {
                    struct monst *mtmp;

                    if ((mtmp = m_at(cx, cy)) != 0) {
                        look_at_monster(lookbuf, (char *) 0, mtmp, cx, cy);
                        ++count;
                        if (mtmp->mpeaceful) peaceful = TRUE;
                    }
                } else if (glyph_is_invisible(glyph)) {
                    Strcpy(lookbuf, "remembered, unseen, creature");
                    ++count;
                } else if (glyph_is_warning(glyph)) {
                    int warnindx = glyph_to_warning(glyph);

                    Strcpy(lookbuf, def_warnsyms[warnindx].explanation);
                    ++count;
                }
            } else { /* !monsters */
                if (glyph_is_object(glyph)) {
                    look_at_object(lookbuf, cx, cy, glyph);
                    ++count;
                }
            }
            if (*lookbuf) {
                char coordbuf[20], which[12], cmode;
                if (count == 1) {
                    Strcpy(which, monsters ? "monsters" : "objects");
                    if (TRUE)
                        Sprintf(outbuf, "Nearby %s (Toggle: ])",
                                upstart(which));
                    else
                        Sprintf(outbuf, "Shown %s",
                                which);
                    QListWidgetItem *item = new QListWidgetItem(outbuf);
                    list->addItem(outbuf);
                }
                Sprintf(outbuf, " %s %s ", 
                    decode_mixed(glyphbuf, encglyph(glyph)),
                    peaceful ? "+" : "-");
                /* guard against potential overflow */
                lookbuf[sizeof lookbuf - 1 - strlen(outbuf)] = '\0';
                Strcat(outbuf, lookbuf);
                QListWidgetItem *item = new QListWidgetItem(outbuf);
                list->addItem(outbuf);
            }
        }
    }

    // Scroll the scrollbar all the way back to the right.
    QScrollBar *sb = list->horizontalScrollBar();
    if (sb && sb->value() > 0) {
        sb->setValue(0);
        this->viewport()->update();
    }
}

}