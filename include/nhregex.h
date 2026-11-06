/* $NHDT-Header: NetHack 5.0 nhregex.h 2026/10/31_22:59:35 NetHack-5.0:1.6 $ */
/* Copyright (c) Michael Allison 2024. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef NHREGEX_H
#define NHREGEX_H

/* ### {cpp,pmatch,posix}regex.c ### */

extern struct nhregex *regex_init(void);
extern boolean regex_compile(const char *, struct nhregex *) NONNULLARG1;
extern char *regex_error_desc(struct nhregex *, char *) NONNULLARG2;
extern boolean regex_match(const char *, struct nhregex *) NO_NNARGS;
extern void regex_free(struct nhregex *) NONNULLARG1;

#endif /* NHREGEX_H */

/*extern.h*/
