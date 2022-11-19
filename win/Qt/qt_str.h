// Copyright (c) Warwick Allison, 1999.
// Qt4 conversion copyright (c) Ray Chason, 2012-2014.
// NetHack may be freely redistributed.  See license for details.

// qt_str.h -- various string functions

#ifndef QT4STR_H
#define QT4STR_H

namespace nethack_qt_ {

// Bounded string copy
extern size_t str_copy(char *dest, const char *src, size_t max);

// Case mappings
extern QString str_titlecase(const QString& str);
extern QString nh_capitalize_words(const QString& str);

// Map symbol conversion
extern int cp437(int ch);

// sprintf
extern QString nh_qsprintf(const char *format, ...)
#if defined(__GNUC__) && (__GNUC__ >= 2)
    __attribute__((format(printf, 1, 2)))
#endif
;

} // namespace nethack_qt_

#endif
