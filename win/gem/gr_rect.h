/* $NHDT-Header$ */
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
