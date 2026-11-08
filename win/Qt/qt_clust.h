/* $NHDT-Header: NetHack 5.0 qt_clust.h 2019/12/06_15:36:54 NetHack-5.0:1.1 $ */
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
