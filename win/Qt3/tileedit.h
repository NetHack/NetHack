/* NetHack 3.6	tileedit.h	$NHDT-Date: 1432512809 2015/05/25 00:13:29 $  $NHDT-Branch: master $:$NHDT-Revision: 1.8 $ */
/* Copyright (c) Warwick Allison, 1999. */
/* NetHack may be freely redistributed.  See license for details. */
#ifndef QNHTILEEDIT_H
#define QNHTILEEDIT_H

#include <qtabwidget.h>
#include <qpixmap.h>
#include <qimage.h>
#include <qvbox.h>

class TilePickerTab : public QWidget
{
    Q_OBJECT
  public:
    TilePickerTab(const char *basename, int id, QWidget *parent);

    bool save();
    int numTiles();

  signals:
    void pick(const QImage &);
    void pickName(const QString &);

  public slots:
    void setCurrent(const QImage &);

  protected:
    void paintEvent(QPaintEvent *);
    QSize sizeHint() const;
    void mousePressEvent(QMouseEvent *);

  private:
    QString filename;
    int id;
    int last_pick;
    int num;
    QPixmap pixmap;
    QImage image;
};

class TilePicker : public QTabWidget
{
    Q_OBJECT
  public:
    TilePicker(QWidget *parent);

    void setTile(int tilenum, const QImage &);

  signals:
    void pick(const QImage &);
    void pickName(const QString &);

  public slots:
    void setCurrent(const QImage &);
    void save();
};

class TrivialTileEditor : public QWidget
{
    Q_OBJECT
  public:
    TrivialTileEditor(QWidget *parent);
    const QImage &image() const;

  signals:
    void edited(const QImage &);
    void pick(QRgb);

  public slots:
    void setColor(QRgb);
    void setImage(const QImage &);

  protected:
    void paintEvent(QPaintEvent *);
    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    QSize sizeHint() const;
    QSize sizeForWidth(int) const;
    QSizePolicy sizePolicy() const;

  private:
    void fill(QPainter &painter, QPoint p, uchar from);
    QImage img;
    QColor pen;
    int penpixel;
    void paintPoint(QPainter &painter, QPoint p);
    QPoint screenPoint(QPoint) const;
    QPoint imagePoint(QPoint) const;
};

class TilePalette : public QWidget
{
    Q_OBJECT
  public:
    TilePalette(QWidget *parent);
    ~TilePalette();
    void setFromImage(const QImage &);

  protected:
    void paintEvent(QPaintEvent *);
    void mousePressEvent(QMouseEvent *);
    QSize sizeHint() const;
    QSizePolicy sizePolicy() const;
  signals:
    void pick(QRgb);
  public slots:
    void setColor(QRgb);

  private:
    int num;
    QRgb *rgb;
};

class TileEditor : public QVBox
{
    Q_OBJECT
  public:
    TileEditor(QWidget *parent);

    const QImage &image() const;

  signals:
    void edited(const QImage &);

  public slots:
    void edit(const QImage &);

  private:
    TrivialTileEditor editor;
    TilePalette palette;
};

#endif
