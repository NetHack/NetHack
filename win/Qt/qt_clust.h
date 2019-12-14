/*	SCCS Id: @(#)qt_clust.h	3.4	1999/11/19	*/
/* Copyright (c) Warwick Allison, 1999. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef clusterizer_H
#define clusterizer_H

#include <QtCore/QRect>

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
