/* macuuid.m */
/* Copyright Michael Allison, 2023 */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

void get_macos_uuid(char *);

#ifdef DEBUG
#undef DEBUG
#endif

/* #import <Foundation/Foundation.h> */
#import <AppKit/AppKit.h>

void
get_macos_uuid(char *target)
{
    NSString *uuidString = [[NSUUID UUID] UUIDString];
    const char *str_uuid;
    int i;
    
    /* str_uuid = [uuidString cStringUsingEncoding:NSUTF8StringEncoding]; */
    str_uuid = [uuidString cStringUsingEncoding:NSASCIIStringEncoding];
    
    if (str_uuid && target) {
        for (i = 0; i < (NHUUIDSZ -1 ); ++i) {
            target[i] = str_uuid[i];
        }
        target[(NHUUIDSZ - 1)] = '\0';
    }
    return;
}


/* end of macuuid.m */
