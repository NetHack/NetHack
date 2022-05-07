// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_map.cpp -- the map window

extern "C" {
#include "hack.h"

extern glyph_map glyphmap[MAX_GLYPH]; /* from tile.c */
}

#include "qt_pre.h"
#include <QtGui/QtGui>
#if QT_VERSION >= 0x050000
#include <QtWidgets/QtWidgets>
#endif
#include "qt_post.h"
#include "qt_map.h"
#include "qt_map.moc"
#include "qt_click.h"
#include "qt_glyph.h"
#include "qt_set.h"
#include "qt_str.h"

// pet- and pile-mark xpm arrays moved out of qt_xpms.h so that we don't
// include it here anymore; including that header in two files resulted in
// two copies of all the static xpm data and all the rest is for qt_stat.cpp
//
/* XPM */
static const char *pet_mark_xpm[] = {
/* width height ncolors chars_per_pixel */
"8 7 2 1",
/* colors */
". c None",
"  c #FF0000",
/* pixels */
"........",
"..  .  .",
".       ",
".       ",
"..     .",
"...   ..",
".... ..."
};
/* XPM */
static const char *pet_mark_small_xpm[] = {
/* width height ncolors chars_per_pixel */
"5 5 2 1",
/* colors */
". c None",
"X c #FF0000",
/* pixels */
".X.X.",
"XXXXX",
".XXX.",
"..X.."
};
/* XPM */
static const char *pile_mark_xpm[] = {
/* width height ncolors chars_per_pixel */
"5 5 2 1",
/* colors */
". c None",
"X c #00FF00",
/* pixels */
"..X..",
"..X..",
"XXXXX",
"..X..",
"..X.."
};

// temporary
extern int qt_compact_mode;
// end temporary

namespace nethack_qt_ {

#ifdef TEXTCOLOR
static const QPen& nhcolor_to_pen(int c)
{
    static QPen *pen = (QPen *) 0;
    if (!pen) {
        pen = new QPen[17];
        //
        // FIXME:  these are duplicated in qt_menu.cpp
        //
        pen[ 0] = QColor(64, 64, 64);    // black
        pen[ 1] = QColor(Qt::red);
        pen[ 2] = QColor(0, 191, 0);     // green
        pen[ 3] = QColor(127, 127, 0);   // brownish
        pen[ 4] = QColor(Qt::blue);
        pen[ 5] = QColor(Qt::magenta);
        pen[ 6] = QColor(Qt::cyan);
        pen[ 7] = QColor(Qt::gray);
        // on tty, "light" variations are "bright" instead; here they're paler
        pen[ 8] = QColor(Qt::white);     // no color
        pen[ 9] = QColor(255, 127, 0);   // orange
        pen[10] = QColor(127, 255, 127); // light green
        pen[11] = QColor(Qt::yellow);
        pen[12] = QColor(127, 127, 255); // light blue
        pen[13] = QColor(255, 127, 255); // light magenta
        pen[14] = QColor(127, 255, 255); // light cyan
        pen[15] = QColor(Qt::white);
        // ? out of range for 0..15
        pen[16] = QColor(Qt::black);
    }

    return pen[c];
}
#endif

NetHackQtMapViewport::NetHackQtMapViewport(NetHackQtClickBuffer& click_sink) :
    QWidget(NULL),
    rogue_font(NULL),
    clicksink(click_sink),
    change(10)
{
    pet_annotation = QPixmap(qt_compact_mode ? pet_mark_small_xpm
                                             : pet_mark_xpm);
    pile_annotation = QPixmap(pile_mark_xpm);

    Clear(); // initializes glyph[][], glyphttychar, glyphcolor, glyphflags
    cursor.setX(0);
    cursor.setY(0);
}

NetHackQtMapViewport::~NetHackQtMapViewport(void)
{
    delete rogue_font; //rogue_font = NULL;
}

// pick a font to use for text map; rogue level is always displayed as text
void NetHackQtMapViewport::SetupTextmapFont(QPainter &painter)
{
    QString fontfamily = iflags.wc_font_map ? iflags.wc_font_map
                                            : "Monospace";
    int maybebold = QFont::Normal;
    if (fontfamily.right(5).toLower() == "-bold") {
        fontfamily.truncate(fontfamily.length() - 5);
        maybebold = QFont::Bold;
    }
    // Find font...
    int pts = 5;
    while (pts < 32) {
        QFont f(fontfamily, pts, maybebold);
        painter.setFont(QFont(fontfamily, pts));
        QFontMetrics fm = painter.fontMetrics();
        if (fm.QFM_WIDTH("M") > qt_settings->glyphs().width())
            break;
        if (fm.height() > qt_settings->glyphs().height())
            break;
        pts++;
    }
    rogue_font = new QFont(fontfamily, pts - 1);
}

void NetHackQtMapViewport::paintEvent(QPaintEvent* event)
{
    NetHackQtGlyphs &glyphs = qt_settings->glyphs();
    int gW = glyphs.width(),
        gH = glyphs.height();
    QRect area = event->rect();
    QRect garea;
    garea.setCoords(std::max(0, area.left() / gW),
                    std::max(0, area.top() / gH),
                    std::min(COLNO - 1, area.right() / gW),
                    std::min(ROWNO - 1, area.bottom() / gH));

    QPainter painter;
    painter.begin(this);

    if (Is_rogue_level(&u.uz) || iflags.wc_ascii_map) {
	// You enter a VERY primitive world!

	painter.setClipRect( event->rect() ); // (normally we don't clip)
	painter.fillRect( event->rect(), Qt::black );

	if (!rogue_font)
            SetupTextmapFont(painter);
	painter.setFont(*rogue_font);

        for (int j = garea.top(); j <= garea.bottom(); j++) {
            for (int i = garea.left(); i <= garea.right(); i++) {
		unsigned short color = Glyphcolor(i, j);
		unsigned short ch = Glyphttychar(i, j);
		unsigned special = Glyphflags(i, j);
/*		unsigned short tileidx = Glyphtileidx(i, j); */
		ch = cp437(ch);
#ifdef TEXTCOLOR
		painter.setPen( nhcolor_to_pen(color) );
#else
		painter.setPen( Qt::green );
#endif
		if (!DrawWalls(painter, i * gW, j * gH, gW, gH, ch)) {
		    painter.drawText(i * gW, j * gH, gW, gH, Qt::AlignCenter,
                                     QString(QChar(ch)).left(1));
		}
#ifdef TEXTCOLOR
                if ((special & MG_PET) != 0 && ::iflags.hilite_pet) {
                    painter.drawPixmap(QPoint(i * gW, j * gH),
                                       pet_annotation);
                } else if ((special & MG_OBJPILE) != 0
                           && ::iflags.hilite_pile) {
                    painter.drawPixmap(QPoint(i * gW, j * gH),
                                       pile_annotation);
                }
#endif
            }
        }

	painter.setFont(font());
    } else { // tiles
        for (int j = garea.top(); j <= garea.bottom(); j++) {
            for (int i = garea.left(); i <= garea.right(); i++) {
                unsigned short g = Glyph(i,j);
		unsigned special = Glyphflags(i, j);
		unsigned tileidx = Glyphtileidx(i, j);

                glyphs.drawCell(painter, g, tileidx, i, j);

                if ((special & MG_PET) != 0 && ::iflags.hilite_pet) {
                    painter.drawPixmap(QPoint(i * gW, j * gH),
                                       pet_annotation);
                } else if ((special & MG_OBJPILE) != 0
                           && ::iflags.hilite_pile) {
                    painter.drawPixmap(QPoint(i * gW, j * gH),
                                       pile_annotation);
                }
	    }
	}
    }

    if (garea.contains(cursor)) {
	if (Is_rogue_level(&u.uz)) {
#ifdef TEXTCOLOR
	    painter.setPen( Qt::white );
#else
	    painter.setPen( Qt::green ); // REALLY primitive
#endif
	} else {
	    int hp = !Upolyd ? u.uhp : u.mh,
                hpmax = !Upolyd ? u.uhpmax :  u.mhmax,
                hp100 = hpmax ? (hp * 100 / hpmax) : 100;

            // this uses a different color scheme from hitpoint bar but has
            // the same cutoff thresholds (except for lack of separate 100%)
            if (hp100 < 10 || hp < 5)
                painter.setPen(Qt::magenta);
            else if (hp100 < 25 || hp < 10 )
                painter.setPen(Qt::red);
            else if (hp100 < 50)
                painter.setPen(QColor(0xff, 0xbf, 0x00)); // orange
            else if (hp100 < 75)
                painter.setPen(Qt::yellow);
            else
                painter.setPen(Qt::white);
	}

	painter.drawRect(cursor.x() * gW, cursor.y() * gH, gW - 1, gH - 1);
    }

#if 0
    if (area.intersects(messages_rect)) {
	painter.setPen(Qt::black);
        painter.drawText(viewport.contentsX() + 1, viewport.contentsY() + 1,
                         viewport.width(), 0,
                         (Qt::TextWordWrap | Qt::AlignTop
                          | Qt::AlignLeft | Qt::TextDontClip), messages);
	painter.setPen(Qt::white);
        painter.drawText(viewport.contentsX(), viewport.contentsY(),
                         viewport.width(), 0,
                         (Qt::TextWordWrap | Qt::AlignTop
                          | Qt::AlignLeft | Qt::TextDontClip), messages);
    }
#endif

    painter.end();
}

bool NetHackQtMapViewport::DrawWalls(QPainter& painter, int x, int y,
                                     int w, int h, unsigned short ch)
{
    enum wallbits {
	w_left      = 0x01,
	w_right     = 0x02,
	w_up        = 0x04,
	w_down      = 0x08,
	w_sq_top    = 0x10,
	w_sq_bottom = 0x20,
	w_sq_left   = 0x40,
	w_sq_right  = 0x80
    };
    unsigned linewidth;
    unsigned walls;
    int x1, y1, x2, y2, x3, y3;
 
    linewidth = ((w < h) ? w : h) / 8;
    if (linewidth == 0)
        linewidth = 1;

    // Single walls
    walls = 0;
    switch (ch) {
    case 0x2500: // box drawings light horizontal
	walls = w_left | w_right;
	break;
    case 0x2502: // box drawings light vertical
	walls = w_up | w_down;
	break;
    case 0x250C: // box drawings light down and right
	walls = w_down | w_right;
	break;
    case 0x2510: // box drawings light down and left
	walls = w_down | w_left;
	break;
    case 0x2514: // box drawings light up and right
	walls = w_up | w_right;
	break;
    case 0x2518: // box drawings light up and left
	walls = w_up | w_left;
	break;
    case 0x251C: // box drawings light vertical and right
	walls = w_up | w_down | w_right;
	break;
    case 0x2524: // box drawings light vertical and left
	walls = w_up | w_down | w_left;
	break;
    case 0x252C: // box drawings light down and horizontal
	walls = w_down | w_left | w_right;
	break;
    case 0x2534: // box drawings light up and horizontal
	walls = w_up | w_left | w_right;
	break;
    case 0x253C: // box drawings light vertical and horizontal
	walls = w_up | w_down | w_left | w_right;
	break;
    }

    if (walls != 0) {
	x1 = x + w / 2;
	switch (walls & (w_up | w_down)) {
	case w_up:
	    painter.drawLine(x1, y, x1, y + h / 2);
	    break;
	case w_down:
	    painter.drawLine(x1, y + h / 2, x1, y + h - 1);
	    break;
	case w_up | w_down:
	    painter.drawLine(x1, y, x1, y + h - 1);
	    break;
	}

	y1 = y + h / 2;
	switch (walls & (w_left | w_right)) {
	case w_left:
	    painter.drawLine(x, y1, x + w / 2, y1);
	    break;
	case w_right:
	    painter.drawLine(x + w / 2, y1, x + w - 1, y1);
	    break;
	case w_left | w_right:
	    painter.drawLine(x, y1, x + w - 1, y1);
	    break;
	}

	return true;
    }

    // Double walls
    walls = 0;
    switch (ch) {
    case 0x2550: // box drawings double horizontal
	walls = w_left | w_right | w_sq_top | w_sq_bottom;
	break;
    case 0x2551: // box drawings double vertical
	walls = w_up | w_down | w_sq_left | w_sq_right;
	break;
    case 0x2554: // box drawings double down and right
	walls = w_down | w_right | w_sq_top | w_sq_left;
	break;
    case 0x2557: // box drawings double down and left
	walls = w_down | w_left | w_sq_top | w_sq_right;
	break;
    case 0x255A: // box drawings double up and right
	walls = w_up | w_right | w_sq_bottom | w_sq_left;
	break;
    case 0x255D: // box drawings double up and left
	walls = w_up | w_left | w_sq_bottom | w_sq_right;
	break;
    case 0x2560: // box drawings double vertical and right
	walls = w_up | w_down | w_right | w_sq_left;
	break;
    case 0x2563: // box drawings double vertical and left
	walls = w_up | w_down | w_left | w_sq_right;
	break;
    case 0x2566: // box drawings double down and horizontal
	walls = w_down | w_left | w_right | w_sq_top;
	break;
    case 0x2569: // box drawings double up and horizontal
	walls = w_up | w_left | w_right | w_sq_bottom;
	break;
    case 0x256C: // box drawings double vertical and horizontal
	walls = w_up | w_down | w_left | w_right;
	break;
    }

    if (walls != 0) {
	x1 = x + w / 2 - linewidth;
	x2 = x + w / 2 + linewidth;
	x3 = x + w - 1;
	y1 = y + h / 2 - linewidth;
	y2 = y + h / 2 + linewidth;
	y3 = y + h - 1;
	if (walls & w_up) {
	    painter.drawLine(x1, y, x1, y1);
	    painter.drawLine(x2, y, x2, y1);
	}
	if (walls & w_down) {
	    painter.drawLine(x1, y2, x1, y3);
	    painter.drawLine(x2, y2, x2, y3);
	}
	if (walls & w_left) {
	    painter.drawLine(x, y1, x1, y1);
	    painter.drawLine(x, y2, x1, y2);
	}
	if (walls & w_right) {
	    painter.drawLine(x2, y1, x3, y1);
	    painter.drawLine(x2, y2, x3, y2);
	}
	if (walls & w_sq_top) {
	    painter.drawLine(x1, y1, x2, y1);
	}
	if (walls & w_sq_bottom) {
	    painter.drawLine(x1, y2, x2, y2);
	}
	if (walls & w_sq_left) {
	    painter.drawLine(x1, y1, x1, y2);
	}
	if (walls & w_sq_right) {
	    painter.drawLine(x2, y1, x2, y2);
	}

	return true;
    }

    // Solid blocks
    if (0x2591 <= ch && ch <= 0x2593) {
	unsigned shade = ch - 0x2590;
	QColor rgb(painter.pen().color());
        QColor rgb2(rgb.red() * shade / 4,
                    rgb.green() * shade / 4,
                    rgb.blue() * shade / 4);
	painter.fillRect(x, y, w, h, rgb2);
	return true;
    }

    return false;
}

void NetHackQtMapViewport::mousePressEvent(QMouseEvent* event)
{
    clicksink.Put(event->pos().x() / qt_settings->glyphs().width(),
                  event->pos().y() / qt_settings->glyphs().height(),
                  (event->button() == Qt::LeftButton) ? CLICK_1 : CLICK_2);
    qApp->exit();
}

void NetHackQtMapViewport::updateTiles()
{
    change.clear();
    change.add(0, 0, COLNO, ROWNO);
    delete rogue_font; rogue_font = NULL;
}

QSize NetHackQtMapViewport::sizeHint() const
{
    return QSize(qt_settings->glyphs().width() * COLNO,
                 qt_settings->glyphs().height() * ROWNO);
}

QSize NetHackQtMapViewport::minimumSizeHint() const
{
    return sizeHint();
}

void NetHackQtMapViewport::clickCursor()
{
    clicksink.Put(cursor.x(), cursor.y(), CLICK_1);
    qApp->exit();
}

// [re-]init map display to unexplored with no changed cells
void NetHackQtMapViewport::Clear()
{
    for (int j = 0; j < ROWNO; ++j) {
        //
        // FIXME:  map column 0 should be surpressed from being displayed
        //
        Glyph(0, j) = GLYPH_NOTHING;
        Glyphttychar(0, j) = ' ';
        Glyphcolor(0, j) = NO_COLOR;
        Glyphflags(0, j) = 0U;
        Glyphtileidx(0, j) = ::glyphmap[GLYPH_NOTHING].tileidx;

        for (int i = 1; i < COLNO; ++i) {
            Glyph(i, j) = GLYPH_UNEXPLORED;
            Glyphttychar(i, j) = ' ';
            Glyphcolor(i, j) = NO_COLOR;
            Glyphflags(i, j) = 0U;
            Glyphtileidx(i, j) = ::glyphmap[GLYPH_UNEXPLORED].tileidx;
        }
    }

    change.clear();
    change.add(0, 0, COLNO, ROWNO);
}

void NetHackQtMapViewport::Display(bool block)
{
    int gW = qt_settings->glyphs().width(),
        gH = qt_settings->glyphs().height();

    for (int i = 0; i < change.clusters(); i++) {
	const QRect& chg = change[i];
	repaint(chg.x() * gW, chg.y() * gH,
                chg.width() * gW, chg.height() * gH);
    }
    change.clear();

    if (block) {
	yn_function("Press a key when done viewing", NULL, '\0');
    }
}

void NetHackQtMapViewport::CursorTo(int x,int y)
{
    Changed(cursor.x(), cursor.y());
    cursor.setX(x);
    cursor.setY(y);
    Changed(cursor.x(), cursor.y());
}

void NetHackQtMapViewport::PrintGlyph(int x, int y,
                                      const glyph_info *glyphinfo)
{
    Glyph(x, y) = (unsigned short) glyphinfo->glyph;
    Glyphttychar(x, y) = (unsigned short) glyphinfo->ttychar;
    Glyphcolor(x, y) = (unsigned short) glyphinfo->gm.sym.color;
    Glyphflags(x, y) = glyphinfo->gm.glyphflags;
    Glyphtileidx(x, y) = (unsigned short) glyphinfo->gm.tileidx;
    Changed(x, y);
}

void NetHackQtMapViewport::Changed(int x, int y)
{
    change.add(x, y);
}

NetHackQtMapWindow2::NetHackQtMapWindow2(NetHackQtClickBuffer& click_sink) :
    QScrollArea(NULL),
    m_viewport(new NetHackQtMapViewport(click_sink))
{
    QPalette palette;
    palette.setColor(backgroundRole(), Qt::black);
    setPalette(palette);

    setWidget(m_viewport);

    connect(qt_settings, SIGNAL(tilesChanged()), this, SLOT(updateTiles()));
    updateTiles();
}

void NetHackQtMapWindow2::updateTiles()
{
    NetHackQtGlyphs& glyphs = qt_settings->glyphs();
    int gW = glyphs.width();
    int gH = glyphs.height();
    // Be exactly the size we want to be - full map...
    m_viewport->resize(COLNO * gW, ROWNO * gH);

    verticalScrollBar()->setSingleStep(gH);
    verticalScrollBar()->setPageStep(gH);
    horizontalScrollBar()->setSingleStep(gW);
    horizontalScrollBar()->setPageStep(gW);

    m_viewport->updateTiles();
    Display(false);

    emit resized();
}

void NetHackQtMapWindow2::clearMessages()
{
    messages = "";
    update(messages_rect);
    messages_rect = QRect();
}

void NetHackQtMapWindow2::putMessage(int attr UNUSED, const QString& text)
{
    if (!messages.isEmpty())
	messages += "\n";
    messages += QString(text).replace(QChar(0x200B), "");
#if 0
    QFontMetrics fm = fontMetrics();
    messages_rect = fm.boundingRect(viewport.contentsX(), viewport.contentsY(),
                                    viewport.width(), 0,
                                    (Qt::TextWordWrap | Qt::AlignTop
                                     | Qt::AlignLeft | Qt::TextDontClip),
                                    messages);
    update(messages_rect);
#endif
}

void NetHackQtMapWindow2::clickCursor()
{
    m_viewport->clickCursor();
}

QWidget *NetHackQtMapWindow2::Widget()
{
    return this;
}

void NetHackQtMapWindow2::Clear()
{
    m_viewport->Clear();
}

void NetHackQtMapWindow2::Display(bool block)
{
    m_viewport->Display(block);
}

void NetHackQtMapWindow2::CursorTo(int x,int y)
{
    m_viewport->CursorTo(x, y);
}

void NetHackQtMapWindow2::PutStr(int attr UNUSED, const QString& text UNUSED)
{
    puts("unexpected PutStr in MapWindow");
}

void NetHackQtMapWindow2::ClipAround(int x,int y)
{
    // Convert to pixel of center of tile
    x=x*qt_settings->glyphs().width()+qt_settings->glyphs().width()/2;
    y=y*qt_settings->glyphs().height()+qt_settings->glyphs().height()/2;

    // Then ensure that pixel is visible
    ensureVisible(x,y,width()*0.45,height()*0.45);
}

void NetHackQtMapWindow2::PrintGlyph(int x,int y,
                                     const glyph_info *glyphinfo)
{
    m_viewport->PrintGlyph(x, y, glyphinfo);
}

#if 0 //RLC
// XXX Hmmm... crash after saving bones file if Map window is
// XXX deleted.  Strange bug somewhere.
bool NetHackQtMapWindow::Destroy() { return false; }

NetHackQtMapWindow::NetHackQtMapWindow(NetHackQtClickBuffer& click_sink) :
    clicksink(click_sink),
    change(10),
    rogue_font(0)
{
    viewport.addChild(this);

    QPalette palette;
    palette.setColor(backgroundRole(), Qt::black);
    setPalette(palette);
    palette.setColor(viewport.backgroundRole(), Qt::black);
    viewport.setPalette(palette);

    pet_annotation = QPixmap(qt_compact_mode ? pet_mark_small_xpm
                                             : pet_mark_xpm);
    pile_annotation = QPixmap(pile_mark_xpm);

    cursor.setX(0);
    cursor.setY(0);
    Clear();

    connect(qt_settings,SIGNAL(tilesChanged()),this,SLOT(updateTiles()));
    connect(&viewport, SIGNAL(contentsMoving(int,int)), this,
		SLOT(moveMessages(int,int)));

    updateTiles();
    //setFocusPolicy(Qt::StrongFocus);
}

void NetHackQtMapWindow::moveMessages(int x, int y)
{
    QRect u = messages_rect;
    messages_rect.moveTopLeft(QPoint(x,y));
    u |= messages_rect;
    update(u);
}

void NetHackQtMapWindow::clearMessages()
{
    messages = "";
    update(messages_rect);
    messages_rect = QRect();
}

void NetHackQtMapWindow::putMessage(int attr, const QString& text)
{
    if ( !messages.isEmpty() )
	messages += "\n";
    messages += QString(text).replace(QChar(0x200B), "");
    QFontMetrics fm = fontMetrics();
    messages_rect = fm.boundingRect(viewport.contentsX(), viewport.contentsY(),
                                    viewport.width(), 0,
                                    (Qt::TextWordWrap | Qt::AlignTop
                                     | Qt::AlignLeft | Qt::TextDontClip),
                                    messages);
    update(messages_rect);
}

void NetHackQtMapWindow::updateTiles()
{
    NetHackQtGlyphs& glyphs = qt_settings->glyphs();
    int gw = glyphs.width();
    int gh = glyphs.height();
    // Be exactly the size we want to be - full map...
    resize(COLNO*gw,ROWNO*gh);

    viewport.verticalScrollBar()->setSingleStep(gh);
    viewport.verticalScrollBar()->setPageStep(gh);
    viewport.horizontalScrollBar()->setSingleStep(gw);
    viewport.horizontalScrollBar()->setPageStep(gw);
    /*
    viewport.setMaximumSize(
	gw*COLNO + viewport.verticalScrollBar()->width(),
	gh*ROWNO + viewport.horizontalScrollBar()->height()
    );
    */
    viewport.updateScrollBars();

    change.clear();
    change.add(0,0,COLNO,ROWNO);
    delete rogue_font; rogue_font = 0;
    Display(false);

    emit resized();
}

NetHackQtMapWindow::~NetHackQtMapWindow()
{
    // Remove from viewport porthole, since that is a destructible member.
    viewport.removeChild(this);
    setParent(0,0);
}

QWidget* NetHackQtMapWindow::Widget()
{
    return &viewport;
}

void NetHackQtMapWindow::Scroll(int dx, int dy)
{
    if (viewport.horizontalScrollBar()->isVisible()) {
	while (dx<0) { viewport.horizontalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepSub); dx++; }
	while (dx>0) { viewport.horizontalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepAdd); dx--; }
    }
    if (viewport.verticalScrollBar()->isVisible()) {
	while (dy<0) { viewport.verticalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepSub); dy++; }
	while (dy>0) { viewport.verticalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepAdd); dy--; }
    }
}

void NetHackQtMapWindow::Clear()
{
    for (int j = 0; j < ROWNO; ++j) {
        Glyph(0, j) = GLYPH_NOTHING;
        Glyphcolor(0, j) = NO_COLOR;
        Glyphttychar(0, j) = ' ';
        Glyphflags(0, j) = 0;
        Glyphtileidx(0, j) = ::glyphmap[GLYPH_NOTHING].tileidx;

        for (int i = 1; i < COLNO; ++i) {
            Glyph(i, j) = GLYPH_UNEXPLORED;
            Glyphcolor(i, j) = NO_COLOR;
            Glyphttychar(i, j) = ' ';
            Glyphflags(i, j) = 0;
            Glyphtileidx(i, j) = ::glyphmap[GLYPH_UNEXPLORED].tileidx;
        }
    }

    change.clear();
    change.add(0,0,COLNO,ROWNO);
}

void NetHackQtMapWindow::clickCursor()
{
    clicksink.Put(cursor.x(),cursor.y(),CLICK_1);
    qApp->exit();
}

void NetHackQtMapWindow::mousePressEvent(QMouseEvent* event)
{
    clicksink.Put(
	event->pos().x()/qt_settings->glyphs().width(),
	event->pos().y()/qt_settings->glyphs().height(),
	event->button()==Qt::LeftButton ? CLICK_1 : CLICK_2
    );
    qApp->exit();
}

void NetHackQtMapWindow::paintEvent(QPaintEvent* event)
{
    QRect area=event->rect();
    QRect garea;
    garea.setCoords(
	std::max(0,area.left()/qt_settings->glyphs().width()),
	std::max(0,area.top()/qt_settings->glyphs().height()),
	std::min(COLNO-1,area.right()/qt_settings->glyphs().width()),
	std::min(ROWNO-1,area.bottom()/qt_settings->glyphs().height())
    );

    QPainter painter;

    painter.begin(this);

    if (is_rogue_level(&u.uz) || iflags.wc_ascii_map) {
	// You enter a VERY primitive world!

	painter.setClipRect( event->rect() ); // (normally we don't clip)
	painter.fillRect( event->rect(), Qt::black );

	if ( !rogue_font ) {
	    // Find font...
	    int pts = 5;
	    QString fontfamily = iflags.wc_font_map
		? iflags.wc_font_map : "Courier";
	    bool bold = false;
	    if ( fontfamily.right(5).toLower() == "-bold" ) {
		fontfamily.truncate(fontfamily.length()-5);
		bold = true;
	    }
	    while ( pts < 32 ) {
		QFont f(fontfamily, pts, bold ? QFont::Bold : QFont::Normal);
		painter.setFont(QFont(fontfamily, pts));
		QFontMetrics fm = painter.fontMetrics();
		if ( fm.width("M") > qt_settings->glyphs().width() )
		    break;
		if ( fm.height() > qt_settings->glyphs().height() )
		    break;
		pts++;
	    }
	    rogue_font = new QFont(fontfamily,pts-1);
	}
	painter.setFont(*rogue_font);

	for (int j=garea.top(); j<=garea.bottom(); j++) {
	    for (int i=garea.left(); i<=garea.right(); i++) {
		unsigned short g=Glyph(i,j);
#if 0
		int color;
		char32_t ch;
		unsigned special;
#else
		int color = Glyphcolor(i,j);
		char32_t ch = Glyphttychar(i,j);
		unsigned special = Glyphflags(i,j);
#endif
		painter.setPen( Qt::green );
#ifdef TEXTCOLOR
		painter.setPen( nhcolor_to_pen(color) );
#endif
		painter.drawText(
		    i*qt_settings->glyphs().width(),
		    j*qt_settings->glyphs().height(),
		    qt_settings->glyphs().width(),
		    qt_settings->glyphs().height(),
		    Qt::AlignCenter,
		    QString(QChar(ch)).left(1)
		);
#ifdef TEXTCOLOR
                if ((special & MG_PET) != 0 && ::iflags.hilite_pet) {
                    painter.drawPixmap(QPoint(i*qt_settings->glyphs().width(),
                                             j*qt_settings->glyphs().height()),
                                       pet_annotation);
                } else if ((special & MG_OBJPILE) != 0
                           && ::iflags.hilite_pile) {
                    painter.drawPixmap(QPoint(i*qt_settings->glyphs().width(),
                                             j*qt_settings->glyphs().height()),
                                       pile_annotation);
                }
#endif
	    }
	}

	painter.setFont(font());
    } else {
	for (int j=garea.top(); j<=garea.bottom(); j++) {
	    for (int i=garea.left(); i<=garea.right(); i++) {
		unsigned short g=Glyph(i,j);
		int color = Glyphcolor(i,j);
		int ch = Glyphttychar(i,j);
		unsigned special = Glyphflags(i,j);
		unsigned short tileidx = Glyphtileidx(i,j);

                qt_settings->glyphs().drawCell(painter, g, tileidx, i, j);
#ifdef TEXTCOLOR
                if ((special & MG_PET) != 0 && ::iflags.hilite_pet) {
                    painter.drawPixmap(QPoint(i*qt_settings->glyphs().width(),
                                             j*qt_settings->glyphs().height()),
                                       pet_annotation);
                } else if ((special & MG_OBJPILE) != 0
                           && ::iflags.hilite_pile) {
                    painter.drawPixmap(QPoint(i*qt_settings->glyphs().width(),
                                             j*qt_settings->glyphs().height()),
                                       pile_annotation);
                }
#endif
	    }
	}
    }

    if (garea.contains(cursor)) {
	if (Is_rogue_level(&u.uz)) {
#ifdef TEXTCOLOR
	    painter.setPen( Qt::white );
#else
	    painter.setPen( Qt::green ); // REALLY primitive
#endif
	} else
	{
	    int hp100;
	    if (Upolyd) {
		hp100=u.mhmax ? u.mh*100/u.mhmax : 100;
	    } else {
		hp100=u.uhpmax ? u.uhp*100/u.uhpmax : 100;
	    }

            if (hp100 > 75)
                painter.setPen(Qt::white);
            else if (hp100 > 50)
                painter.setPen(Qt::yellow);
            else if (hp100 > 25)
                painter.setPen(QColor(0xff, 0xbf, 0x00)); // orange
            else if (hp100 > 10)
                painter.setPen(Qt::red);
            else
                painter.setPen(Qt::magenta);
	}

        painter.drawRect(cursor.x() * qt_settings->glyphs().width(),
                         cursor.y() * qt_settings->glyphs().height(),
                         qt_settings->glyphs().width() - 1,
                         qt_settings->glyphs().height() - 1);
    }

    if (area.intersects(messages_rect)) {
	painter.setPen(Qt::black);
        painter.drawText(viewport.contentsX() + 1, viewport.contentsY() + 1,
                         viewport.width(), 0,
                         (Qt::TextWordWrap | Qt::AlignTop
                          | Qt::AlignLeft | Qt::TextDontClip), messages);
	painter.setPen(Qt::white);
        painter.drawText(viewport.contentsX(), viewport.contentsY(),
                         viewport.width(), 0,
                         (Qt::TextWordWrap | Qt::AlignTop
                          | Qt::AlignLeft | Qt::TextDontClip), messages);
    }

    painter.end();
}

void NetHackQtMapWindow::Display(bool block)
{
    for (int i=0; i<change.clusters(); i++) {
	const QRect& ch=change[i];
	repaint(
	    ch.x()*qt_settings->glyphs().width(),
	    ch.y()*qt_settings->glyphs().height(),
	    ch.width()*qt_settings->glyphs().width(),
	    ch.height()*qt_settings->glyphs().height()
	);
    }

    change.clear();

    if (block) {
	yn_function("Press a key when done viewing",0,'\0');
    }
}

void NetHackQtMapWindow::CursorTo(int x,int y)
{
    Changed(cursor.x(),cursor.y());
    cursor.setX(x);
    cursor.setY(y);
    Changed(cursor.x(),cursor.y());
}

void NetHackQtMapWindow::PutStr(int attr, const QString& text)
{
    puts("unexpected PutStr in MapWindow");
}

void NetHackQtMapWindow::ClipAround(int x,int y)
{
    // Convert to pixel of center of tile
    x=x*qt_settings->glyphs().width()+qt_settings->glyphs().width()/2;
    y=y*qt_settings->glyphs().height()+qt_settings->glyphs().height()/2;

    // Then ensure that pixel is visible
    viewport.center(x,y,0.45,0.45);
}

void NetHackQtMapWindow::PrintGlyph(int x,int y, const glyph_info *glyphinfo)
{
    Glyph(x,y)=glyphinfo->glyph;
    Glyphttychar(x,y)=glyphinfo->ttychar;
    Glyphcolor(x,y)=glyphinfo->color;
    Glyphflags(x,y)=glyphinfo->glyphflags;
    Glyphtileidx(x,y)=glyphinfo->tileidx;
    Changed(x,y);
}

//void NetHackQtMapWindow::PrintGlyphCompose(int x,int y,int glyph1, int glyph2)
//{
    // TODO: composed graphics
//}

void NetHackQtMapWindow::Changed(int x, int y)
{
    change.add(x,y);
}
#endif /* 0 //RLC */

} // namespace nethack_qt_
