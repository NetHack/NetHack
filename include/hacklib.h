/* NetHack 3.7	hacklib.h	$NHDT-Date: 1725653010 2024/09/06 20:03:30 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.0 $ */
/* Copyright (c) Steve Creps, 1988.                               */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef HACKLIB_H
#define HACKLIB_H

/*
 * hacklib true library functions
 */
extern boolean digit(char);
extern boolean letter(char);
extern char highc(char);
extern char lowc(char);
extern char *lcase(char *) NONNULL NONNULLARG1;
extern char *ucase(char *) NONNULL NONNULLARG1;
extern char *upstart(char *); /* ought to be changed to NONNULL NONNULLARG1
                               * and the code changed to not allow NULL arg */
extern char *upwords(char *) NONNULL NONNULLARG1;
extern char *mungspaces(char *) NONNULL NONNULLARG1;
extern char *trimspaces(char *) NONNULL NONNULLARG1;
extern char *strip_newline(char *) NONNULL NONNULLARG1;
extern char *eos(char *) NONNULL NONNULLARG1;
extern const char *c_eos(const char *) NONNULL NONNULLARG1;
extern boolean str_start_is(const char *, const char *, boolean) NONNULLPTRS;
extern boolean str_end_is(const char *, const char *) NONNULLPTRS;
extern int str_lines_maxlen(const char *);
extern char *strkitten(char *, char) NONNULL NONNULLARG1;
extern void copynchars(char *, const char *, int) NONNULLARG12;
extern char chrcasecpy(int, int);
extern char *strcasecpy(char *, const char *) NONNULL NONNULLPTRS;
extern char *s_suffix(const char *) NONNULL NONNULLARG1;
extern char *ing_suffix(const char *) NONNULL NONNULLARG1;
extern char *xcrypt(const char *, char *) NONNULL NONNULLPTRS;
extern boolean onlyspace(const char *) NONNULLARG1;
extern char *tabexpand(char *) NONNULL NONNULLARG1;
extern char *visctrl(char) NONNULL;
extern char *stripchars(char *, const char *,
                                            const char *) NONNULL NONNULLPTRS;
extern char *stripdigits(char *) NONNULL NONNULLARG1;
extern char *strsubst(char *, const char *, const char *) NONNULL NONNULLPTRS;
extern int strNsubst(char *, const char *, const char *, int) NONNULLPTRS;
extern const char *findword(const char *, const char *, int,
                                                         boolean) NONNULLARG2;
extern const char *ordin(int) NONNULL;
extern char *sitoa(int) NONNULL;
extern int sgn(int);
extern int distmin(coordxy, coordxy, coordxy, coordxy);
extern int dist2(coordxy, coordxy, coordxy, coordxy);
extern int isqrt(int);
extern boolean online2(coordxy, coordxy, coordxy, coordxy);
#ifndef STRNCMPI
extern int strncmpi(const char *, const char *, int) NONNULLPTRS;
#endif
#ifndef STRSTRI
extern char *strstri(const char *, const char *) NONNULLPTRS;
#endif
#define FITSint(x) FITSint_(x, __func__, __LINE__)
extern int FITSint_(long long, const char *, int);
#define FITSuint(x) FITSuint_(x, __func__, __LINE__)
extern unsigned FITSuint_(unsigned long long, const char *, int);
extern int case_insensitive_comp(const char *, const char *);
extern boolean fuzzymatch(const char *, const char *,
                          const char *, boolean) NONNULLPTRS;
extern int swapbits(int, int, int);
/* note: the snprintf CPP wrapper includes the "fmt" argument in "..."
   (__VA_ARGS__) to allow for zero arguments after fmt */
extern void nh_snprintf(const char *func, int line, char *str,
                        size_t size, const char *fmt, ...) PRINTF_F(5, 6);
extern void nh_snprintf_w_impossible(const char *func, int line, char *str,
                        size_t size, const char *fmt, ...) PRINTF_F(5, 6);

#define Snprintf(str, size, ...) \
    nh_snprintf(__func__, __LINE__, str, size, __VA_ARGS__)

#if 0
/*#define Strlen(s) Strlen_(s, __func__, __LINE__)*/
extern unsigned Strlen_(const char *, const char *, int) NONNULLPTRS;
#endif
extern int unicodeval_to_utf8str(int, uint8 *, size_t);


#endif /* HACKLIB_H */

