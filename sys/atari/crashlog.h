/* $NHDT-Header: NetHack 5.0 bmp2img_host.c 2026/09/30_10:47:34 NetHack-5.0:1.2 $ */
/* Copyright (c) Ingo Paschke, 2026 */
/* Copyright (c) NetHack Atari port contributors.  */
/* NetHack may be freely redistributed.  See license for details. */

/* Exception-vector crash dumps and boot checkpoints for the TOS port. */

#ifndef CRASHLOG_H
#define CRASHLOG_H

void crash_install(void);
void crash_checkpoint(const char *tag);
void crash_selftest(void);

#endif /* CRASHLOG_H */
