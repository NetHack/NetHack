/* gr_rect.h */
/*
 * $NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$
 * $Date: 2002/03/20 03:50:49 $  $Revision: 1.1 $
 */
#include <e_gem.h>
/********** structs **********/
typedef struct {
	GRECT *rects;
	int max,used;
} dirty_rect;
/********* functions ************/
dirty_rect *new_dirty_rect(int size);
void delete_dirty_rect(dirty_rect *this);
int add_dirty_rect(dirty_rect *dr,GRECT *area);
int get_dirty_rect(dirty_rect* dr,GRECT *area);
int clear_dirty_rect(dirty_rect *dr);
int resize_dirty_rect(dirty_rect *dr,int new_size);
