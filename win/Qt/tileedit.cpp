/* NetHack 3.6	tileedit.cpp	$NHDT-Date: 1524684508 2018/04/25 19:28:28 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.10 $ */
/* Copyright (c) Warwick Allison, 1999. */
/* NetHack may be freely redistributed.  See license for details. */
/*
Build this little utility program if you want to use it to edit the tile
files.  Move tileedit.cpp and tileedit.h to ../../util, add the
3 lines below to the Makefile there and "make tileedit".

tileedit: tileedit.cpp $(TEXT_IO)
	moc -o tileedit.moc tileedit.h
	$(CC) -o tileedit -I../include -I$(QTDIR)/include -L$(QTDIR)/lib tileedit.cpp $(TEXT_IO) -lqt
*/


#include "tileedit.h"
#include <qapplication.h>
#include <qmainwindow.h>
#include <qkeycode.h>
#include <qpopupmenu.h>
#include <qmenubar.h>
#include <qpainter.h>
#include <qstatusbar.h>
#include <qhbox.h>
#include <qlabel.h>

extern "C" {
#include "config.h"
#include "tile.h"
extern const char *FDECL(tilename, (int, int));
}

#define TILES_ACROSS 20

TilePickerTab::TilePickerTab(const char* basename, int i, QWidget* parent) :
    QWidget(parent)
{
    id = i;
    filename = basename;
    filename += ".txt";
    num = 0;
    int index = 0;
    for (int real=0; real<2; real++) {
	if ( real ) {
	    image.create( TILES_ACROSS*TILE_X,
		((num+TILES_ACROSS-1)/TILES_ACROSS)*TILE_Y, 32 );
	}
	if ( !fopen_text_file(filename.latin1(), RDTMODE) ) {
	    // XXX handle better
	    exit(1);
	}
	pixel p[TILE_Y][TILE_X];
	while ( read_text_tile(p) ) {
	    if ( real ) {
		int ox = (index%TILES_ACROSS)*TILE_X;
		int oy = (index/TILES_ACROSS)*TILE_Y;
		for ( int y=0; y<TILE_Y; y++ ) {
		    QRgb* rgb = ((QRgb*)image.scanLine(oy+y)) + ox;
		    for ( int x=0; x<TILE_X; x++ ) {
			*rgb++ = qRgb(p[y][x].r, p[y][x].g, p[y][x].b);
		    }
		}
		index++;
	    } else {
		// Just count...
		num++;
	    }
	}
	fclose_text_file();
    }
    image = image.convertDepth( 8, AvoidDither );
    pixmap.convertFromImage( image );
}

bool TilePickerTab::save()
{
    if ( !fopen_text_file(filename.latin1(), WRTMODE) ) {
	// XXX handle better
	exit(1);
    }
    pixel p[TILE_Y][TILE_X];
    for ( int index=0; index < num; index++ ) {
	int ox = (index%TILES_ACROSS)*TILE_X;
	int oy = (index/TILES_ACROSS)*TILE_Y;
	for ( int y=0; y<TILE_Y; y++ ) {
	    uchar* c = image.scanLine(oy+y) + ox;
	    for ( int x=0; x<TILE_X; x++ ) {
		QRgb rgb = image.color(*c++);
		p[y][x].r = qRed(rgb);
		p[y][x].g = qGreen(rgb);
		p[y][x].b = qBlue(rgb);
	    }
	}
	write_text_tile(p);
    }
    fclose_text_file();
}

void TilePickerTab::mousePressEvent(QMouseEvent* e)
{
    int ox = e->x()-e->x()%TILE_X;
    int oy = e->y()-e->y()%TILE_Y;
    QImage subimage = image.copy(ox,oy,TILE_X,TILE_Y);
    if ( e->button() == RightButton ) {
	setCurrent(subimage);
    } else {
	last_pick = ox/TILE_X + oy/TILE_Y*TILES_ACROSS;
    }
    emit pick(subimage);
    emit pickName(tilename(id, last_pick));
}

void TilePickerTab::setCurrent(const QImage& i)
{
    int ox = last_pick%TILES_ACROSS * TILE_X;
    int oy = last_pick/TILES_ACROSS * TILE_Y;
    bitBlt( &image, ox, oy, &i );
    bitBlt( &pixmap, ox, oy, &i );
    repaint( ox, oy, TILE_X, TILE_Y, FALSE );
}

QSize TilePickerTab::sizeHint() const
{
    return pixmap.size();
}

void TilePickerTab::paintEvent( QPaintEvent* )
{
    QPainter p(this);
    p.drawPixmap(0,0,pixmap);
}

static struct {
    const char* name;
    TilePickerTab* tab;
} tileset[] = {
    { "monsters", 0 },
    { "objects", 0 },
    { "other", 0 },
    { 0 }
};

TilePicker::TilePicker(QWidget* parent) :
    QTabWidget(parent)
{
    for (int i=0; tileset[i].name; i++) {
	QString tabname = tileset[i].name;
	tabname[0] = tabname[0].upper();
	tileset[i].tab = new TilePickerTab(tileset[i].name,i+1,this);
	addTab( tileset[i].tab, tabname );
	connect( tileset[i].tab, SIGNAL(pick(const QImage&)),
		 this, SIGNAL(pick(const QImage&)) );
	connect( tileset[i].tab, SIGNAL(pickName(const QString&)),
		 this, SIGNAL(pickName(const QString&)) );
    }
}

void TilePicker::setCurrent(const QImage& i)
{
    ((TilePickerTab*)currentPage())->setCurrent(i);
}

void TilePicker::save()
{
    for (int i=0; tileset[i].tab; i++) {
	tileset[i].tab->save();
    }
}

TrivialTileEditor::TrivialTileEditor( QWidget* parent ) :
    QWidget(parent)
{
}

const QImage& TrivialTileEditor::image() const
{
    return img;
}

void TrivialTileEditor::setColor( QRgb rgb )
{
    pen = rgb;
    for (penpixel = 0;
	    penpixel<img.numColors()-1 && (img.color(penpixel)&0xffffff)!=(pen.rgb()&0xffffff);
	    penpixel++)
	continue;
}

void TrivialTileEditor::setImage( const QImage& i )
{
    img = i;
    setColor(pen.rgb()); // update penpixel
    repaint(FALSE);
}

void TrivialTileEditor::paintEvent( QPaintEvent* e )
{
    QRect r = e->rect();
    QPoint tl = imagePoint(r.topLeft());
    QPoint br = imagePoint(r.bottomRight());
    r = QRect(tl,br).intersect(img.rect());
    QPainter painter(this);
    for (int y=r.top(); y<=r.bottom(); y++) {
	for (int x=r.left(); x<=r.right(); x++) {
	    paintPoint(painter,QPoint(x,y));
	}
    }
}

void TrivialTileEditor::paintPoint(QPainter& painter, QPoint p)
{
    QPoint p1 = screenPoint(p);
    QPoint p2 = screenPoint(p+QPoint(1,1));
    QColor c = img.color(img.scanLine(p.y())[p.x()]);
    painter.fillRect(QRect(p1,p2-QPoint(1,1)), c);
}

void TrivialTileEditor::mousePressEvent(QMouseEvent* e)
{
    QPoint p = imagePoint(e->pos());
    if ( !img.rect().contains(p) )
	return;
    uchar& pixel = img.scanLine(p.y())[p.x()];
    if ( e->button() == LeftButton ) {
	pixel = penpixel;
	QPainter painter(this);
	paintPoint(painter,p);
    } else if ( e->button() == RightButton ) {
	emit pick( img.color(pixel) );
    } else if ( e->button() == MidButton ) {
	QPainter painter(this);
	if ( pixel != penpixel )
	    fill(painter,p,pixel);
    }
}

void TrivialTileEditor::fill(QPainter& painter, QPoint p, uchar from)
{
    if ( img.rect().contains(p) ) {
	uchar& pixel = img.scanLine(p.y())[p.x()];
	if ( pixel == from ) {
	    pixel = penpixel;
	    paintPoint(painter,p);
	    fill(painter, p+QPoint(-1,0), from);
	    fill(painter, p+QPoint(+1,0), from);
	    fill(painter, p+QPoint(0,-1), from);
	    fill(painter, p+QPoint(0,+1), from);
	}
    }
}

void TrivialTileEditor::mouseReleaseEvent(QMouseEvent* e)
{
    emit edited(image());
}

void TrivialTileEditor::mouseMoveEvent(QMouseEvent* e)
{
    QPoint p = imagePoint(e->pos());
    if ( !img.rect().contains(p) )
	return;
    uchar& pixel = img.scanLine(p.y())[p.x()];
    pixel = penpixel;
    QPainter painter(this);
    paintPoint(painter,p);
}

QPoint TrivialTileEditor::imagePoint(QPoint p) const
{
    return QPoint(p.x()*TILE_X/width(), p.y()*TILE_Y/height());
}

QPoint TrivialTileEditor::screenPoint(QPoint p) const
{
    return QPoint(p.x()*width()/TILE_X, p.y()*height()/TILE_Y);
}

QSizePolicy TrivialTileEditor::sizePolicy() const
{
    return QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding, TRUE );
}

QSize TrivialTileEditor::sizeHint() const
{
    return sizeForWidth(-1);
}

QSize TrivialTileEditor::sizeForWidth(int w) const
{
    if ( w < 0 )
	return QSize(TILE_X*32,TILE_Y*32);
    else
	return QSize(w,w*TILE_Y/TILE_X);
}


TilePalette::TilePalette( QWidget* parent ) :
    QWidget(parent)
{
    num = 0;
    rgb = 0;
}

TilePalette::~TilePalette()
{
    delete rgb;
}

void TilePalette::setFromImage( const QImage& i )
{
    num = i.numColors();
    rgb = new QRgb[num];
    memcpy(rgb, i.colorTable(), num*sizeof(QRgb));
    repaint(FALSE);
}

void TilePalette::setColor(QRgb c)
{
    for (int i=0; i<num; i++)
	if ( c == rgb[i] ) {
	    emit pick(c);
	    return;
	}
}

QSizePolicy TilePalette::sizePolicy() const
{
    return QSizePolicy( QSizePolicy::Minimum, QSizePolicy::Minimum, FALSE );
}

QSize TilePalette::sizeHint() const
{
    return QSize(num*16,16);
}

void TilePalette::paintEvent( QPaintEvent* )
{
    QPainter p(this);
    for (int i=0; i<num; i++) {
	int x1 = width()*i/num;
	int x2 = width()*(i+1)/num;
	p.fillRect(x1,0,x2-x1,height(),QColor(rgb[i]));
    }
}

void TilePalette::mousePressEvent(QMouseEvent* e)
{
    int c = e->x()*num/width();
    emit pick(rgb[c]);
}

TileEditor::TileEditor(QWidget* parent) :
    QVBox(parent),
    editor(this),
    palette(this)
{
    connect( &palette, SIGNAL(pick(QRgb)),
	     &editor, SLOT(setColor(QRgb)) );
    connect( &editor, SIGNAL(pick(QRgb)),
	     &palette, SLOT(setColor(QRgb)) );
    connect( &editor, SIGNAL(edited(const QImage&)),
	     this, SIGNAL(edited(const QImage&)) );
}

void TileEditor::edit(const QImage& i)
{
    editor.setImage(i);
    palette.setFromImage(i);
}

const QImage& TileEditor::image() const
{
    return editor.image();
}

class Main : public QMainWindow {
public:
    Main() :
	central(this),
	editor(&central),
	picker(&central)
    {
	QPopupMenu* file = new QPopupMenu(menuBar());
	file->insertItem("&Save", &picker, SLOT(save()), CTRL+Key_S);
	file->insertSeparator();
	file->insertItem("&Exit", qApp, SLOT(quit()), CTRL+Key_Q);
	menuBar()->insertItem("&File", file);

	connect( &picker, SIGNAL(pick(const QImage&)),
		 &editor, SLOT(edit(const QImage&)) );
	connect( &picker, SIGNAL(pickName(const QString&)),
		 statusBar(), SLOT(message(const QString&)) );
	connect( &editor, SIGNAL(edited(const QImage&)),
		 &picker, SLOT(setCurrent(const QImage&)) );

	setCentralWidget(&central);
    }

private:
    QHBox central; 
    TileEditor editor;
    TilePicker picker;
};

main(int argc, char** argv)
{
    QApplication app(argc,argv);
    Main m;
    app.setMainWidget(&m);
    m.show();
    return app.exec();
}

#include "tileedit.moc"
