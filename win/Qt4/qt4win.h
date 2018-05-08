// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// Qt Binding for NetHack 3.4
//
// Unfortunately, this doesn't use Qt as well as I would like,
// primarily because NetHack is fundamentally a getkey-type
// program rather than being event driven (hence the ugly key
// and click buffer rather), but also because this is my first
// major application of Qt.
//

#ifndef qt4win_h
#define qt4win_h

namespace nethack_qt4 {

class NetHackQtWindow {
public:
	NetHackQtWindow();
	virtual ~NetHackQtWindow();

	virtual QWidget* Widget() =0;

	virtual void Clear();
	virtual void Display(bool block);
	virtual bool Destroy();
	virtual void CursorTo(int x,int y);
	virtual void PutStr(int attr, const QString& text);
        void PutStr(int attr, const char *text)
        {
            PutStr(attr, QString::fromUtf8(text).replace(QChar(0x200B), ""));
        }
	virtual void StartMenu();
	virtual void AddMenu(int glyph, const ANY_P* identifier, char ch, char gch, int attr,
			const QString& str, bool presel);
	virtual void EndMenu(const QString& prompt);
	virtual int SelectMenu(int how, MENU_ITEM_P **menu_list);
	virtual void ClipAround(int x,int y);
	virtual void PrintGlyph(int x,int y,int glyph);
	virtual void UseRIP(int how, time_t when);

	int nhid;
};

} // namespace nethack_qt4

#endif
