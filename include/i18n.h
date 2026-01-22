/* NetHack 3.7  i18n.h  $NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) HanNetHack Project, 2026. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef I18N_H
#define I18N_H

/*
 * Internationalization (i18n) support for NetHack
 *
 * This header provides gettext macros and Korean postposition processing
 * for the Korean localization of NetHack.
 */

#ifdef ENABLE_NLS

#include <libintl.h>
#include <locale.h>

/* Standard gettext macros */
#define _(String)       gettext(String)
#define N_(String)      gettext_noop(String)
#define gettext_noop(String) (String)

/* Plural forms */
#define P_(Singular, Plural, N) ngettext(Singular, Plural, N)

/* Context-aware translation (pgettext) */
#define C_(Context, String) pgettext(Context, String)

/* Initialize internationalization subsystem */
extern void init_i18n(void);

/* Set the language at runtime (e.g., "ko", "en") */
extern void set_language(const char *lang);

/* Get current language code (e.g., "ko", "en") */
extern const char *get_current_language(void);

/* Check if current language is Korean */
extern boolean is_korean_locale(void);

/* Process Korean postpositions in a translated string
 * Handles patterns like {은/는}, {이/가}, {을/를}, {과/와}, {으로/로}
 * based on the last character of the preceding word.
 *
 * Example:
 *   Input:  "고블린{을/를} 때렸다."
 *   Output: "고블린을 때렸다."
 *
 *   Input:  "오크{을/를} 때렸다."
 *   Output: "오크를 때렸다."
 */
extern char *process_korean_postpositions(char *buf, const char *format, ...);

#else /* !ENABLE_NLS */

/* Fallback macros when NLS is disabled */
#define _(String)       (String)
#define N_(String)      (String)
#define P_(Singular, Plural, N) ((N) == 1 ? (Singular) : (Plural))
#define C_(Context, String) (String)

#define init_i18n()     ((void)0)
#define set_language(x) ((void)0)
#define get_current_language() "en"
#define is_korean_locale() FALSE
#define process_korean_postpositions(buf, fmt, ...) (buf)

#endif /* ENABLE_NLS */

/*
 * UTF-8 width functions are always available (defined in i18n.c)
 * These are needed for proper TTY rendering regardless of NLS setting.
 */
extern int utf8_display_width(const char *utf8str);
extern int utf8_char_width(const char *utf8str);

/*
 * Object name translation function (defined in i18n.c)
 * Translates item names like "gold piece", "long sword" via gettext.
 * Returns the translated name, or the original if NLS is disabled.
 */
extern const char *tr_obj_name(const char *name);

/*
 * Get localized filename for help/data files
 * If a non-English locale is active, returns "locale/<lang>/<filename>".
 *
 * Example:
 *   get_localized_filename("help") returns "locale/ko/help" (Korean)
 *   get_localized_filename("help") returns "help" (English)
 *
 * Note: The returned pointer points to a static buffer that is
 * overwritten on each call. Copy the result if you need to keep it.
 */
extern const char *get_localized_filename(const char *fname);

/*
 * Korean postposition markers for translation strings
 *
 * Usage in .po files:
 *   msgid "You hit %s."
 *   msgstr "%s{을/를} 때렸다."
 *
 * The postposition processor will automatically select the correct
 * form based on whether the preceding Korean syllable has a final
 * consonant (받침) or not.
 *
 * Postposition types:
 *   {은/는} - Topic marker (eun/neun)
 *   {이/가} - Subject marker (i/ga)
 *   {을/를} - Object marker (eul/reul)
 *   {과/와} - "and/with" (gwa/wa)
 *   {으로/로} - Direction/means (euro/ro) - special: ㄹ받침 uses "로"
 */

/* Postposition pattern delimiters */
#define KO_PP_START     '{'
#define KO_PP_END       '}'
#define KO_PP_SEP       '/'

#endif /* I18N_H */
