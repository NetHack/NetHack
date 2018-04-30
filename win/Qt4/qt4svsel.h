// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt4svsel.h -- saved game selector

#ifndef QT4SVSEL_H
#define QT4SVSEL_H

namespace nethack_qt4 {

class NetHackQtSavedGameSelector : public QDialog {
public:
	NetHackQtSavedGameSelector(const char** saved);

	int choose();
};

} // namespace nethack_qt4

#endif
