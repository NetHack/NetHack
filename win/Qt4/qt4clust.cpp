/*	SCCS Id: @(#)qt_clust.cpp	3.4	1999/11/19	*/
/* Copyright (c) Warwick Allison, 1999. */
/* NetHack may be freely redistributed.  See license for details. */
#include "qt4clust.h"

static void include(QRect& r, const QRect& rect)
{
	if (rect.left()<r.left()) {
			r.setLeft(rect.left());
	}
	if (rect.right()>r.right()) {
			r.setRight(rect.right());
	}
	if (rect.top()<r.top()) {
			r.setTop(rect.top());
	}
	if (rect.bottom()>r.bottom()) {
			r.setBottom(rect.bottom());
	}
}

/*
A Clusterizer groups rectangles (QRects) into non-overlapping rectangles
by a merging heuristic.
*/
Clusterizer::Clusterizer(int maxclusters) :
	cluster(new QRect[maxclusters]),
	count(0),
	max(maxclusters)
{ }

Clusterizer::~Clusterizer()
{
	delete [] cluster;
}

void Clusterizer::clear()
{
	count=0;
}

void Clusterizer::add(int x, int y)
{
	add(QRect(x,y,1,1));
}

void Clusterizer::add(int x, int y, int w, int h)
{
	add(QRect(x,y,w,h));
}

void Clusterizer::add(const QRect& rect)
{
	QRect biggerrect(rect.x()-1,rect.y()-1,rect.width()+2,rect.height()+2);

	//assert(rect.width()>0 && rect.height()>0);

	int cursor;

	for (cursor=0; cursor<count; cursor++) {
		if (cluster[cursor].contains(rect)) {
			// Wholly contained already.
			return;
		}
	}

	int lowestcost=9999999;
	int cheapest=-1;
	for (cursor=0; cursor<count; cursor++) {
		if (cluster[cursor].intersects(biggerrect)) {
			QRect larger=cluster[cursor];
			include(larger,rect);
			int cost=larger.width()*larger.height()
					- cluster[cursor].width()*cluster[cursor].height();

			if (cost < lowestcost) {
				bool bad=false;
				for (int c=0; c<count && !bad; c++) {
					bad=cluster[c].intersects(larger) && c!=cursor;
				}
				if (!bad) {
					cheapest=cursor;
					lowestcost=cost;
				}
			}
		}
	}
	if (cheapest>=0) {
		include(cluster[cheapest],rect);
		return;
	}

	if (count < max) {
		cluster[count++]=rect;
		return;
	}

	// Do cheapest of:
	// 	add to closest cluster
	// 	do cheapest cluster merge, add to new cluster

	lowestcost=9999999;
	cheapest=-1;
	for (cursor=0; cursor<count; cursor++) {
		QRect larger=cluster[cursor];
		include(larger,rect);
		int cost=larger.width()*larger.height()
				- cluster[cursor].width()*cluster[cursor].height();
		if (cost < lowestcost) {
			bool bad=false;
			for (int c=0; c<count && !bad; c++) {
				bad=cluster[c].intersects(larger) && c!=cursor;
			}
			if (!bad) {
				cheapest=cursor;
				lowestcost=cost;
			}
		}
	}

	// XXX could make an heuristic guess as to whether we
	// XXX need to bother looking for a cheap merge.

	int cheapestmerge1=-1;
	int cheapestmerge2=-1;

	for (int merge1=0; merge1<count; merge1++) {
		for (int merge2=0; merge2<count; merge2++) {
			if (merge1!=merge2) {
				QRect larger=cluster[merge1];
				include(larger,cluster[merge2]);
				int cost=larger.width()*larger.height()
					- cluster[merge1].width()*cluster[merge1].height()
					- cluster[merge2].width()*cluster[merge2].height();
				if (cost < lowestcost) {
					bool bad=false;
					for (int c=0; c<count && !bad; c++) {
						bad=cluster[c].intersects(larger) && c!=cursor;
					}
					if (!bad) {
						cheapestmerge1=merge1;
						cheapestmerge2=merge2;
						lowestcost=cost;
					}
				}
			}
		}
	}

	if (cheapestmerge1>=0) {
		include(cluster[cheapestmerge1],cluster[cheapestmerge2]);
		cluster[cheapestmerge2]=cluster[count--];
	} else {
		// if (!cheapest) debugRectangles(rect);
		include(cluster[cheapest],rect);
	}

	// NB: clusters do not intersect (or intersection will
	//	 overwrite).  This is a result of the above algorithm,
	//	 given the assumption that (x,y) are ordered topleft
	//	 to bottomright.
}

const QRect& Clusterizer::operator[](int i)
{
	return cluster[i];
}
