/* Copyright (c) NetHack Atari port contributors.  */
/* NetHack may be freely redistributed.  See license for details. */

/* Exception-vector crash dumps and boot checkpoints for the TOS port. */

#ifndef CRASHLOG_H
#define CRASHLOG_H

void crash_install(void);
void crash_checkpoint(const char *tag);
void crash_selftest(void);

#endif /* CRASHLOG_H */
