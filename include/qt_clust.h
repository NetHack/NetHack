/* NetHack 3.5	qt_clust.h	$NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$ */
/* NetHack 3.5	qt_clust.h	$Date: 2009/05/06 10:45:01 $  $Revision: 1.4 $ */
/*	SCCS Id: @(#)qt_clust.h	3.5	1999/11/19	*/
/* Copyright (c) Warwick Allison, 1999. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef clusterizer_H
#define clusterizer_H

#include <qrect.h>

class Clusterizer {
public:
	Clusterizer(int maxclusters);
	~Clusterizer();

	void add(int x, int y); // 1x1 rectangle (point)
	void add(int x, int y, int w, int h);
	void add(const QRect& rect);

	void clear();
	int clusters() { return count; }
	const QRect& operator[](int i);

private:
	QRect* cluster;
	int count;
	const int max;
};

#endif
