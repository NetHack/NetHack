 These are patches for MOVEINIT.C, supplied    
 with the MSVC compiler in the compiler's SOURCE\MOVE  
 subdirectory. (Copy that and the MOVEAPI.H    
 file into your NetHack src directory and apply this patch) 


*** ../linc/src/moveinit.c	Tue Nov 23 08:01:00 1993
--- src/moveinit.c	Sun Mar 13 10:13:10 1994
***************
*** 13,18 ****
--- 13,19 ----
  *******************************************************************************/
  
  #include "moveapi.h"
+ extern unsigned memavail(unsigned);
  
  #ifndef MOVE_ENV
  
***************
*** 125,132 ****
      /* attempt to allocate the overlay heap. ignore return value (heap size).
       * note that MOVE will abort if not enough memory to alloc minimum size.
       */
! 
!     _movesetheap ($$COVL, cparaLarge, cparaMax);
  
      /* get available cache ressource amount */
  
--- 126,133 ----
      /* attempt to allocate the overlay heap. ignore return value (heap size).
       * note that MOVE will abort if not enough memory to alloc minimum size.
       */
!     cparaMax = memavail(cparaMin);
!     _movesetheap ($$COVL, cparaMin, cparaMax);
  
      /* get available cache ressource amount */
  
