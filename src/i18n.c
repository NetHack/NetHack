/* NetHack 3.7  i18n.c  $NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) HanNetHack Project, 2026. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include <wchar.h>
#include <wctype.h>

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

#ifdef ENABLE_NLS

#include "i18n.h"
#include "ko_postpos.h"

/* Domain name for gettext */
#define TEXTDOMAIN "nethack"

/* Cached language info */
static char current_lang[8] = "";
static boolean korean_locale = FALSE;

/*
 * Initialize internationalization subsystem
 *
 * Should be called early in main() or allmain.c
 */
void
init_i18n(void)
{
    const char *lang;
    const char *localedir;

    /* Set locale from environment */
    setlocale(LC_ALL, "");

    /* Determine locale directory */
    /* Try environment variable first, then standard locations */
    localedir = getenv("NETHACK_LOCALE_DIR");
    if (!localedir) {
#ifdef LOCALEDIR
        localedir = LOCALEDIR;  /* Defined in Makefile */
#else
        localedir = "/usr/share/locale";  /* Default fallback */
#endif
    }

    /* Initialize gettext */
    bindtextdomain(TEXTDOMAIN, localedir);
    bind_textdomain_codeset(TEXTDOMAIN, "UTF-8");
    textdomain(TEXTDOMAIN);

    /* Cache current language */
    lang = getenv("LANGUAGE");
    if (!lang)
        lang = getenv("LC_ALL");
    if (!lang)
        lang = getenv("LC_MESSAGES");
    if (!lang)
        lang = getenv("LANG");

    if (lang) {
        /* Extract language code (e.g., "ko" from "ko_KR.UTF-8") */
        strncpy(current_lang, lang, sizeof(current_lang) - 1);
        current_lang[sizeof(current_lang) - 1] = '\0';

        /* Truncate at underscore or dot */
        char *p = strchr(current_lang, '_');
        if (p) *p = '\0';
        p = strchr(current_lang, '.');
        if (p) *p = '\0';

        /* Check if Korean */
        korean_locale = (strcmp(current_lang, "ko") == 0);
    } else {
        strcpy(current_lang, "en");
        korean_locale = FALSE;
    }
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
    const char *pp_start;
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
