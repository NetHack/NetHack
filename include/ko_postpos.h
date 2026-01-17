/* NetHack 3.7  ko_postpos.h  $NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) HanNetHack Project, 2026. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef KO_POSTPOS_H
#define KO_POSTPOS_H

/*
 * Korean Postposition (조사) Processing
 *
 * Korean grammar requires different postposition forms depending on
 * whether the preceding syllable ends with a consonant (받침, batchim)
 * or not.
 *
 * This module provides functions to:
 * 1. Detect if a Korean syllable has a final consonant
 * 2. Select the appropriate postposition form
 * 3. Process strings with postposition markers
 */

#ifdef ENABLE_NLS

/* Postposition types */
typedef enum {
    KO_PP_NONE = 0,
    KO_PP_EUN_NEUN,    /* 은/는 - topic marker */
    KO_PP_I_GA,        /* 이/가 - subject marker */
    KO_PP_EUL_REUL,    /* 을/를 - object marker */
    KO_PP_GWA_WA,      /* 과/와 - and/with */
    KO_PP_EURO_RO,     /* 으로/로 - direction/means (special: ㄹ uses 로) */
    KO_PP_A_YA,        /* 아/야 - vocative (호격) */
    KO_PP_IDA_DA,      /* 이다/다 - copula */
    KO_PP_IEOT_YEOT,   /* 이었/였 - past copula */
    KO_PP_COUNT
} ko_postpos_type;

/* Batchim (final consonant) detection result */
typedef enum {
    KO_BATCHIM_NONE = 0,    /* No final consonant (모음으로 끝남) */
    KO_BATCHIM_RIEUL,       /* Ends with ㄹ (으로/로 special case) */
    KO_BATCHIM_OTHER        /* Other final consonant */
} ko_batchim_type;

/*
 * Check if a Korean syllable has a final consonant (받침)
 *
 * For Korean Hangul syllables (U+AC00 - U+D7A3):
 *   codepoint = 0xAC00 + (initial * 588) + (medial * 28) + final
 *   final = (codepoint - 0xAC00) % 28
 *   If final == 0, no batchim; otherwise has batchim
 *
 * Parameters:
 *   utf8str - UTF-8 encoded string (checks the last character)
 *
 * Returns:
 *   KO_BATCHIM_NONE  - No final consonant or not Korean
 *   KO_BATCHIM_RIEUL - Ends with ㄹ
 *   KO_BATCHIM_OTHER - Has other final consonant
 */
extern ko_batchim_type ko_check_batchim(const char *utf8str);

/*
 * Check batchim for a single Unicode codepoint
 */
extern ko_batchim_type ko_check_batchim_codepoint(unsigned int codepoint);

/*
 * Get the appropriate postposition form
 *
 * Parameters:
 *   batchim - Batchim type from ko_check_batchim()
 *   pp_type - Postposition type
 *
 * Returns:
 *   UTF-8 encoded postposition string (e.g., "은", "는", "을", "를")
 */
extern const char *ko_get_postposition(ko_batchim_type batchim,
                                       ko_postpos_type pp_type);

/*
 * Parse a postposition pattern from a string
 *
 * Pattern format: {받침있음/받침없음}
 * Examples: {은/는}, {이/가}, {을/를}, {과/와}, {으로/로}
 *
 * Parameters:
 *   pattern - String starting with '{' (e.g., "{은/는} 좋다")
 *   pp_type - Output: detected postposition type
 *   pattern_len - Output: length of the entire pattern including braces
 *
 * Returns:
 *   TRUE if valid pattern found, FALSE otherwise
 */
extern boolean ko_parse_postposition_pattern(const char *pattern,
                                            ko_postpos_type *pp_type,
                                            int *pattern_len);

/*
 * Process a format string with Korean postpositions
 *
 * Scans the input string for postposition patterns {X/Y} and
 * replaces them with the appropriate form based on the preceding
 * character's batchim.
 *
 * Parameters:
 *   outbuf   - Output buffer
 *   outbufsz - Size of output buffer
 *   input    - Input string with postposition patterns
 *
 * Returns:
 *   Pointer to outbuf
 */
extern char *ko_process_string(char *outbuf, size_t outbufsz, const char *input);

/*
 * Get batchim type for common English words/numbers by pronunciation
 *
 * Some English words and numbers have established Korean pronunciations:
 *   - Numbers: 0(영), 1(일), 2(이), 3(삼), etc.
 *   - Letters: A(에이), B(비), C(씨), etc.
 *
 * Parameters:
 *   str - ASCII string (word or number)
 *
 * Returns:
 *   Batchim type based on Korean pronunciation
 */
extern ko_batchim_type ko_english_batchim(const char *str);

/*
 * Extract the last meaningful character for batchim check
 *
 * Skips trailing whitespace and punctuation to find the last
 * character that affects postposition selection.
 *
 * Parameters:
 *   str     - Input UTF-8 string
 *   lastpos - Output: pointer to the start of last character
 *
 * Returns:
 *   Length of the last character in bytes, or 0 if none found
 */
extern int ko_find_last_char(const char *str, const char **lastpos);

/*
 * UTF-8 utility functions
 */

/* Get the byte length of a UTF-8 character from its first byte */
extern int utf8_char_len(unsigned char first_byte);

/* Decode a UTF-8 character to Unicode codepoint */
extern unsigned int utf8_to_codepoint(const char *utf8str, int *bytes_read);

/* Encode a Unicode codepoint to UTF-8 */
extern int codepoint_to_utf8(unsigned int codepoint, char *outbuf);

#else /* !ENABLE_NLS */

/* Stub definitions when NLS is disabled */
#define ko_check_batchim(s)     KO_BATCHIM_NONE
#define ko_process_string(out, sz, in) (strncpy(out, in, sz), out)

#endif /* ENABLE_NLS */

#endif /* KO_POSTPOS_H */
