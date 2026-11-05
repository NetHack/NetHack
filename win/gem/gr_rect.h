/* $NHDT-Header: NetHack 5.0 gr_rect.h 2026/09/30_14:47:34 NetHack-5.0:1.9 $ */
#include <e_gem.h>
/********** structs **********/
typedef struct {
    GRECT *rects;
    int max, used;
} dirty_rect;
/********* functions ************/
dirty_rect *new_dirty_rect(int size);
int add_dirty_rect(dirty_rect *dr, GRECT *area);
int get_dirty_rect(dirty_rect *dr, GRECT *area);
