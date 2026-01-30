/* NetHack 3.7  i18n.c  $NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) HanNetHack Project, 2026. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include <wchar.h>
#include <wctype.h>
#include <unistd.h>

/*
 * UTF-8 width functions - always available regardless of ENABLE_NLS
 * These are needed for proper TTY rendering of wide characters.
 */

/*
 * Calculate display width of a UTF-8 string
 *
 * Uses wcwidth() to handle wide characters (CJK, emoji).
 * Returns the number of terminal columns needed to display the string.
 */
int
utf8_display_width(const char *utf8str)
{
    wchar_t wc;
    int width = 0;
    int len;

    if (!utf8str)
        return 0;

    /* Ensure locale is set for mbtowc */
    while (*utf8str) {
        len = mbtowc(&wc, utf8str, MB_CUR_MAX);
        if (len <= 0) {
            /* Invalid or incomplete sequence, count as 1 */
            width++;
            utf8str++;
        } else {
            int w = wcwidth(wc);
            /* wcwidth returns -1 for non-printable, treat as 1 */
            width += (w > 0) ? w : 1;
            utf8str += len;
        }
    }

    return width;
}

/*
 * Get display width of a single UTF-8 character
 *
 * Returns 1 for half-width, 2 for full-width characters.
 */
int
utf8_char_width(const char *utf8str)
{
    wchar_t wc;
    int len;
    int w;

    if (!utf8str || !*utf8str)
        return 0;

    len = mbtowc(&wc, utf8str, MB_CUR_MAX);
    if (len <= 0)
        return 1;

    w = wcwidth(wc);
    return (w > 0) ? w : 1;
}

/*
 * Translate an object name using gettext
 *
 * This function is used to translate item names like "gold piece",
 * "long sword", etc. It simply wraps the name with gettext.
 */
const char *
tr_obj_name(const char *name)
{
    if (!name || !*name)
        return name;
#ifdef ENABLE_NLS
    return gettext(name);
#else
    return name;
#endif
}

/*
 * Get localized filename for help/data files
 *
 * If a non-English locale is active, returns "locale/<lang>/<filename>".
 * The caller should use dlb_fopen to check if the file exists.
 */
const char *
get_localized_filename(const char *fname)
{
    static char buf[BUFSZ];

    if (!fname || !*fname)
        return fname;

#ifdef ENABLE_NLS
    /* For non-English locales, use locale directory */
    const char *lang = get_current_language();
    if (lang && *lang && strcmp(lang, "en") != 0) {
        snprintf(buf, sizeof(buf), "locale/%s/%s", lang, fname);
        return buf;
    }
#endif
    return fname;
}

#ifdef ENABLE_NLS

#include "i18n.h"
#include "ko_postpos.h"

/* Domain name for gettext */
#define TEXTDOMAIN "nethack"

/* Cached language info */
static char current_lang[8] = "";
static boolean korean_locale = FALSE;

/*
 * Map language code to full locale name
 */
static const char *
get_locale_for_lang(const char *lang)
{
    if (!lang || !*lang)
        return "ko_KR.utf8";  /* Default to Korean for HanNetHack */
    if (strcmp(lang, "ko") == 0)
        return "ko_KR.utf8";
    if (strcmp(lang, "en") == 0)
        return "en_US.utf8";
    if (strcmp(lang, "ja") == 0)
        return "ja_JP.utf8";
    if (strcmp(lang, "zh") == 0)
        return "zh_CN.utf8";
    /* For other codes, try to construct a locale name */
    return "en_US.utf8";  /* Fallback */
}

/*
 * Find locale directory by checking multiple paths
 * The lang parameter specifies which language to look for (e.g., "ko", "ja", "en")
 */
static const char *
find_locale_dir(const char *lang)
{
    static char localedir_buf[BUFSZ * 2];
    const char *env_dir;
    char testpath[BUFSZ * 2];

    if (!lang || !*lang)
        lang = "ko";  /* Default to Korean */

    /* 1. Check environment variable first */
    env_dir = getenv("NETHACK_LOCALE_DIR");
    if (env_dir && env_dir[0]) {
        snprintf(testpath, sizeof(testpath), "%s/%s/LC_MESSAGES/nethack.mo", env_dir, lang);
        if (access(testpath, R_OK) == 0)
            return env_dir;
    }

    /* 2. Check relative to executable (for development/portable installs) */
#ifdef __linux__
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
#endif
    {
        char exe_path[BUFSZ * 2];
        ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
        if (len > 0) {
            char *slash;
            exe_path[len] = '\0';
            slash = strrchr(exe_path, '/');
            if (slash) {
                *slash = '\0';
                /* Try ../dat/locale (if exe is in src/) */
                snprintf(localedir_buf, sizeof(localedir_buf), "%s/../dat/locale", exe_path);
                snprintf(testpath, sizeof(testpath), "%s/%s/LC_MESSAGES/nethack.mo", localedir_buf, lang);
                if (access(testpath, R_OK) == 0)
                    return localedir_buf;
                /* Try ./dat/locale (if exe is in root) */
                snprintf(localedir_buf, sizeof(localedir_buf), "%s/dat/locale", exe_path);
                snprintf(testpath, sizeof(testpath), "%s/%s/LC_MESSAGES/nethack.mo", localedir_buf, lang);
                if (access(testpath, R_OK) == 0)
                    return localedir_buf;
            }
        }
    }
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
#endif

    /* 3. Check HACKDIR/locale */
#ifdef HACKDIR
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
#endif
    snprintf(localedir_buf, sizeof(localedir_buf), "%s/locale", HACKDIR);
    snprintf(testpath, sizeof(testpath), "%s/%s/LC_MESSAGES/nethack.mo", localedir_buf, lang);
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
    if (access(testpath, R_OK) == 0)
        return localedir_buf;
#endif

    /* 4. Check compile-time LOCALEDIR */
#ifdef LOCALEDIR
    snprintf(testpath, sizeof(testpath), "%s/%s/LC_MESSAGES/nethack.mo", LOCALEDIR, lang);
    if (access(testpath, R_OK) == 0)
        return LOCALEDIR;
#endif

    /* 5. Fallback to system default */
    return "/usr/share/locale";
}

/*
 * Set the language at runtime
 *
 * This function changes the locale and updates gettext settings.
 * For gettext to work, we need:
 * 1. A non-C locale set (glibc ignores LANGUAGE when LC_ALL=C)
 * 2. LANGUAGE set to the desired language code
 * 3. Correct locale directory for gettext message catalogs
 */
void
set_language(const char *lang)
{
    const char *localedir;
    const char *locale_name;
    char *loc_result;

    if (!lang || !*lang)
        lang = "ko";  /* Default to Korean for HanNetHack */

    /* Automatically find locale directory for the specified language */
    localedir = find_locale_dir(lang);

    /* Set LANGUAGE for gettext message catalog lookup */
    setenv("LANGUAGE", lang, 1);

    /* Get the full locale name for this language */
    locale_name = get_locale_for_lang(lang);

    /* Try to set the locale - this requires locale data to exist in system
     * locale directories. Note: We must NOT set LOCPATH before setlocale
     * because LOCPATH affects where glibc looks for locale data, and our
     * locale directory only contains gettext message catalogs, not locale
     * data (LC_CTYPE, etc). */
    loc_result = setlocale(LC_ALL, locale_name);
    if (!loc_result) {
        /* Locale not available, try empty string (use environment) */
        loc_result = setlocale(LC_ALL, "");
    }
    if (!loc_result) {
        /* Last resort: C.UTF-8 for basic UTF-8 support */
        setlocale(LC_ALL, "C.UTF-8");
    }

    /* Initialize gettext with locale directory */
    bindtextdomain(TEXTDOMAIN, localedir);
    bind_textdomain_codeset(TEXTDOMAIN, "UTF-8");
    textdomain(TEXTDOMAIN);

    /* Update cached language info */
    strncpy(current_lang, lang, sizeof(current_lang) - 1);
    current_lang[sizeof(current_lang) - 1] = '\0';

    /* Check if Korean */
    korean_locale = (strcmp(current_lang, "ko") == 0);
}

/*
 * Initialize internationalization subsystem
 *
 * Should be called early in main() or allmain.c
 * Uses iflags.language if set, otherwise defaults to Korean.
 */
void
init_i18n(void)
{
    const char *lang;

    /* Check if language was set in options */
    if (iflags.language[0]) {
        lang = iflags.language;
    } else {
        /* Default to Korean for HanNetHack */
        lang = "ko";
        strncpy(iflags.language, lang, sizeof(iflags.language) - 1);
        iflags.language[sizeof(iflags.language) - 1] = '\0';
    }

    /* Apply the language setting */
    set_language(lang);
}

/*
 * Get current language code
 */
const char *
get_current_language(void)
{
    return current_lang[0] ? current_lang : "en";
}

/*
 * Check if current language is Korean
 */
boolean
is_korean_locale(void)
{
    return korean_locale;
}

/*
 * Process Korean postpositions in a translated string
 *
 * This function processes format strings containing postposition patterns
 * like {은/는}, {이/가}, {을/를} after variable substitution.
 *
 * Usage:
 *   char buf[BUFSZ];
 *   process_korean_postpositions(buf, "%s{을/를} 때렸다.", mon_nam(mtmp));
 *
 * The function:
 * 1. Performs sprintf-style formatting
 * 2. Scans for postposition patterns {X/Y}
 * 3. Replaces each pattern based on the preceding character's batchim
 */
char *
process_korean_postpositions(char *buf, const char *format, ...)
{
    va_list args;
    char temp[BUFSZ * 2];
    char *outp;
    const char *inp;
    ko_postpos_type pp_type;
    int pp_len;
    const char *last_char_pos;
    int last_char_len;
    ko_batchim_type batchim;

    if (!buf || !format)
        return buf;

    /* First, do standard formatting */
    va_start(args, format);
    vsnprintf(temp, sizeof(temp), format, args);
    va_end(args);

    /* If not Korean locale, just copy and return */
    if (!korean_locale) {
        strncpy(buf, temp, BUFSZ - 1);
        buf[BUFSZ - 1] = '\0';
        return buf;
    }

    /* Process postposition patterns */
    outp = buf;
    inp = temp;

    while (*inp && (outp - buf) < BUFSZ - 10) {
        if (*inp == KO_PP_START) {
            /* Found potential postposition pattern */
            if (ko_parse_postposition_pattern(inp, &pp_type, &pp_len)) {
                /* Find the last character before this pattern */
                *outp = '\0';  /* Temporarily terminate for scanning */
                last_char_len = ko_find_last_char(buf, &last_char_pos);

                if (last_char_len > 0) {
                    /* Determine batchim */
                    unsigned int cp;
                    int bytes;
                    cp = utf8_to_codepoint(last_char_pos, &bytes);
                    batchim = ko_check_batchim_codepoint(cp);

                    /* If ASCII, check English pronunciation rules */
                    if (batchim == KO_BATCHIM_NONE && cp < 0x80) {
                        /* Find start of the ASCII word */
                        const char *word_start = last_char_pos;
                        while (word_start > buf && isalnum((unsigned char)*(word_start-1))) {
                            word_start--;
                        }
                        char word[64];
                        int wlen = last_char_pos + last_char_len - word_start;
                        if (wlen > 0 && wlen < (int)sizeof(word)) {
                            strncpy(word, word_start, wlen);
                            word[wlen] = '\0';
                            batchim = ko_english_batchim(word);
                        }
                    }
                } else {
                    batchim = KO_BATCHIM_NONE;
                }

                /* Get the appropriate postposition */
                const char *pp = ko_get_postposition(batchim, pp_type);
                if (pp) {
                    while (*pp && (outp - buf) < BUFSZ - 1) {
                        *outp++ = *pp++;
                    }
                }

                /* Skip the pattern in input */
                inp += pp_len;
                continue;
            }
        }

        /* Copy regular character */
        int charlen = utf8_char_len((unsigned char)*inp);
        while (charlen-- > 0 && *inp && (outp - buf) < BUFSZ - 1) {
            *outp++ = *inp++;
        }
    }

    *outp = '\0';
    return buf;
}

#endif /* ENABLE_NLS */
