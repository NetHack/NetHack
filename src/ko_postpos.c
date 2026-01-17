/* NetHack 3.7  ko_postpos.c  $NHDT-Date:  $  $NHDT-Branch:  $:$NHDT-Revision:  $ */
/* Copyright (c) HanNetHack Project, 2026. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#ifdef ENABLE_NLS

#include "i18n.h"
#include "ko_postpos.h"
#include <ctype.h>

/*
 * Korean Hangul Unicode ranges:
 *   Hangul Syllables: U+AC00 - U+D7A3
 *   Hangul Jamo: U+1100 - U+11FF
 *   Hangul Compatibility Jamo: U+3130 - U+318F
 *
 * Syllable decomposition formula:
 *   syllable = 0xAC00 + (initial * 588) + (medial * 28) + final
 *   final = (syllable - 0xAC00) % 28
 *   If final == 0: no batchim
 *   If final == 8 (ㄹ): special case for 으로/로
 */

#define HANGUL_SYLLABLE_START  0xAC00
#define HANGUL_SYLLABLE_END    0xD7A3
#define HANGUL_JONGSUNG_COUNT  28
#define HANGUL_JONGSUNG_RIEUL  8  /* Index for ㄹ */

/* Postposition string pairs: [0] = with batchim, [1] = without batchim */
static const char *postposition_pairs[][2] = {
    [KO_PP_NONE]     = { "",     ""     },
    [KO_PP_EUN_NEUN] = { "은",   "는"   },  /* 은/는 topic */
    [KO_PP_I_GA]     = { "이",   "가"   },  /* 이/가 subject */
    [KO_PP_EUL_REUL] = { "을",   "를"   },  /* 을/를 object */
    [KO_PP_GWA_WA]   = { "과",   "와"   },  /* 과/와 and/with */
    [KO_PP_EURO_RO]  = { "으로", "로"   },  /* 으로/로 direction (ㄹ→로) */
    [KO_PP_A_YA]     = { "아",   "야"   },  /* 아/야 vocative */
    [KO_PP_IDA_DA]   = { "이다", "다"   },  /* 이다/다 copula */
    [KO_PP_IEOT_YEOT]= { "이었", "였"   },  /* 이었/였 past copula */
};

/* Pattern strings for parsing: {받침O/받침X} */
static const char *pattern_strings[][2] = {
    [KO_PP_NONE]     = { NULL,     NULL     },
    [KO_PP_EUN_NEUN] = { "은",     "는"     },
    [KO_PP_I_GA]     = { "이",     "가"     },
    [KO_PP_EUL_REUL] = { "을",     "를"     },
    [KO_PP_GWA_WA]   = { "과",     "와"     },
    [KO_PP_EURO_RO]  = { "으로",   "로"     },
    [KO_PP_A_YA]     = { "아",     "야"     },
    [KO_PP_IDA_DA]   = { "이다",   "다"     },
    [KO_PP_IEOT_YEOT]= { "이었",   "였"     },
};

/*
 * English word/number pronunciation endings for Korean postpositions
 *
 * Numbers:
 *   0: 영 (받침X)  1: 일 (받침O)  2: 이 (받침X)  3: 삼 (받침O)
 *   4: 사 (받침X)  5: 오 (받침X)  6: 육 (받침O)  7: 칠 (받침O)
 *   8: 팔 (받침O/ㄹ)  9: 구 (받침X)
 *
 * Common letters (by Korean pronunciation of alphabet names):
 *   A,E,I,O,U: 에이,이,아이,오,유 - all 받침X
 *   B,C,D,G,P,T,V,Z: 비,씨,디,지,피,티,브이,제드 - all 받침X
 *   F,L,M,N,R,S,X: 에프,엘,엠,엔,아르,에스,엑스 - all 받침O
 *   H,K: 에이치,케이 - 받침X
 */

/* Returns TRUE if the number's Korean pronunciation has batchim */
static ko_batchim_type
number_batchim(int digit)
{
    switch (digit) {
    case 1: case 3: case 6: case 7:
        return KO_BATCHIM_OTHER;
    case 8:
        return KO_BATCHIM_RIEUL;  /* 팔 ends with ㄹ */
    default:  /* 0, 2, 4, 5, 9 */
        return KO_BATCHIM_NONE;
    }
}

/* Returns batchim type for single letter by Korean pronunciation */
static ko_batchim_type
letter_batchim(char c)
{
    c = toupper((unsigned char)c);
    switch (c) {
    /* Letters with batchim (consonant ending in Korean) */
    case 'F':  /* 에프 */
    case 'L':  /* 엘 */
    case 'M':  /* 엠 */
    case 'N':  /* 엔 */
    case 'R':  /* 아르 */
    case 'S':  /* 에스 */
    case 'X':  /* 엑스 */
        return KO_BATCHIM_OTHER;
    /* Letters without batchim */
    default:
        return KO_BATCHIM_NONE;
    }
}

/*
 * Get byte length of a UTF-8 character from its first byte
 */
int
utf8_char_len(unsigned char first_byte)
{
    if ((first_byte & 0x80) == 0)
        return 1;  /* ASCII: 0xxxxxxx */
    if ((first_byte & 0xE0) == 0xC0)
        return 2;  /* 110xxxxx */
    if ((first_byte & 0xF0) == 0xE0)
        return 3;  /* 1110xxxx */
    if ((first_byte & 0xF8) == 0xF0)
        return 4;  /* 11110xxx */
    return 1;  /* Invalid, treat as 1 byte */
}

/*
 * Decode a UTF-8 character to Unicode codepoint
 */
unsigned int
utf8_to_codepoint(const char *utf8str, int *bytes_read)
{
    unsigned char c;
    unsigned int cp;
    int len;

    if (!utf8str || !*utf8str) {
        if (bytes_read) *bytes_read = 0;
        return 0;
    }

    c = (unsigned char)*utf8str;
    len = utf8_char_len(c);

    if (bytes_read)
        *bytes_read = len;

    switch (len) {
    case 1:
        return c;
    case 2:
        cp = (c & 0x1F) << 6;
        cp |= ((unsigned char)utf8str[1] & 0x3F);
        return cp;
    case 3:
        cp = (c & 0x0F) << 12;
        cp |= ((unsigned char)utf8str[1] & 0x3F) << 6;
        cp |= ((unsigned char)utf8str[2] & 0x3F);
        return cp;
    case 4:
        cp = (c & 0x07) << 18;
        cp |= ((unsigned char)utf8str[1] & 0x3F) << 12;
        cp |= ((unsigned char)utf8str[2] & 0x3F) << 6;
        cp |= ((unsigned char)utf8str[3] & 0x3F);
        return cp;
    default:
        return c;
    }
}

/*
 * Encode a Unicode codepoint to UTF-8
 */
int
codepoint_to_utf8(unsigned int codepoint, char *outbuf)
{
    if (codepoint < 0x80) {
        outbuf[0] = (char)codepoint;
        outbuf[1] = '\0';
        return 1;
    } else if (codepoint < 0x800) {
        outbuf[0] = (char)(0xC0 | (codepoint >> 6));
        outbuf[1] = (char)(0x80 | (codepoint & 0x3F));
        outbuf[2] = '\0';
        return 2;
    } else if (codepoint < 0x10000) {
        outbuf[0] = (char)(0xE0 | (codepoint >> 12));
        outbuf[1] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        outbuf[2] = (char)(0x80 | (codepoint & 0x3F));
        outbuf[3] = '\0';
        return 3;
    } else if (codepoint < 0x110000) {
        outbuf[0] = (char)(0xF0 | (codepoint >> 18));
        outbuf[1] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
        outbuf[2] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        outbuf[3] = (char)(0x80 | (codepoint & 0x3F));
        outbuf[4] = '\0';
        return 4;
    }
    outbuf[0] = '\0';
    return 0;
}

/*
 * Check batchim for a single Unicode codepoint
 */
ko_batchim_type
ko_check_batchim_codepoint(unsigned int codepoint)
{
    int jongsung;

    /* Check if it's a Hangul syllable */
    if (codepoint >= HANGUL_SYLLABLE_START && codepoint <= HANGUL_SYLLABLE_END) {
        jongsung = (codepoint - HANGUL_SYLLABLE_START) % HANGUL_JONGSUNG_COUNT;

        if (jongsung == 0)
            return KO_BATCHIM_NONE;
        else if (jongsung == HANGUL_JONGSUNG_RIEUL)
            return KO_BATCHIM_RIEUL;
        else
            return KO_BATCHIM_OTHER;
    }

    /* Not a Hangul syllable - check if it's ASCII */
    if (codepoint < 0x80) {
        /* For ASCII, we can't determine from the character alone
         * This will be handled by ko_english_batchim() for full words */
        return KO_BATCHIM_NONE;
    }

    /* Other Unicode characters - default to no batchim */
    return KO_BATCHIM_NONE;
}

/*
 * Check if a Korean syllable has a final consonant (받침)
 */
ko_batchim_type
ko_check_batchim(const char *utf8str)
{
    const char *last_pos;
    int last_len;
    unsigned int cp;
    int bytes;

    if (!utf8str || !*utf8str)
        return KO_BATCHIM_NONE;

    /* Find the last character */
    last_len = ko_find_last_char(utf8str, &last_pos);
    if (last_len <= 0)
        return KO_BATCHIM_NONE;

    /* Decode and check */
    cp = utf8_to_codepoint(last_pos, &bytes);
    return ko_check_batchim_codepoint(cp);
}

/*
 * Get the appropriate postposition form
 */
const char *
ko_get_postposition(ko_batchim_type batchim, ko_postpos_type pp_type)
{
    if (pp_type <= KO_PP_NONE || pp_type >= KO_PP_COUNT)
        return "";

    /* Special case: 으로/로 with ㄹ batchim uses "로" */
    if (pp_type == KO_PP_EURO_RO && batchim == KO_BATCHIM_RIEUL)
        return postposition_pairs[KO_PP_EURO_RO][1];  /* "로" */

    /* Normal case: batchim → [0], no batchim → [1] */
    if (batchim != KO_BATCHIM_NONE)
        return postposition_pairs[pp_type][0];
    else
        return postposition_pairs[pp_type][1];
}

/*
 * Parse a postposition pattern from a string
 */
boolean
ko_parse_postposition_pattern(const char *pattern,
                              ko_postpos_type *pp_type,
                              int *pattern_len)
{
    const char *p;
    const char *slash_pos;
    const char *end_pos;
    int i;
    size_t first_len, second_len;

    if (!pattern || *pattern != KO_PP_START) {
        *pp_type = KO_PP_NONE;
        *pattern_len = 0;
        return FALSE;
    }

    /* Find '/' separator */
    slash_pos = strchr(pattern + 1, KO_PP_SEP);
    if (!slash_pos) {
        *pp_type = KO_PP_NONE;
        *pattern_len = 0;
        return FALSE;
    }

    /* Find '}' end */
    end_pos = strchr(slash_pos + 1, KO_PP_END);
    if (!end_pos) {
        *pp_type = KO_PP_NONE;
        *pattern_len = 0;
        return FALSE;
    }

    first_len = slash_pos - (pattern + 1);
    second_len = end_pos - (slash_pos + 1);

    /* Match against known patterns */
    for (i = 1; i < KO_PP_COUNT; i++) {
        if (!pattern_strings[i][0] || !pattern_strings[i][1])
            continue;

        if (strlen(pattern_strings[i][0]) == first_len &&
            strlen(pattern_strings[i][1]) == second_len &&
            strncmp(pattern + 1, pattern_strings[i][0], first_len) == 0 &&
            strncmp(slash_pos + 1, pattern_strings[i][1], second_len) == 0) {
            *pp_type = (ko_postpos_type)i;
            *pattern_len = (int)(end_pos - pattern + 1);
            return TRUE;
        }
    }

    /* No match found */
    *pp_type = KO_PP_NONE;
    *pattern_len = 0;
    return FALSE;
}

/*
 * Process a string with Korean postpositions
 */
char *
ko_process_string(char *outbuf, size_t outbufsz, const char *input)
{
    char *outp;
    const char *inp;
    ko_postpos_type pp_type;
    int pp_len;
    const char *last_char_pos;
    int last_char_len;
    ko_batchim_type batchim;
    unsigned int cp;
    int bytes;

    if (!outbuf || outbufsz == 0)
        return outbuf;

    if (!input) {
        outbuf[0] = '\0';
        return outbuf;
    }

    outp = outbuf;
    inp = input;

    while (*inp && (size_t)(outp - outbuf) < outbufsz - 10) {
        if (*inp == KO_PP_START) {
            /* Found potential postposition pattern */
            if (ko_parse_postposition_pattern(inp, &pp_type, &pp_len)) {
                /* Find the last character before this pattern */
                *outp = '\0';  /* Temporarily terminate */
                last_char_len = ko_find_last_char(outbuf, &last_char_pos);

                if (last_char_len > 0) {
                    cp = utf8_to_codepoint(last_char_pos, &bytes);
                    batchim = ko_check_batchim_codepoint(cp);
                } else {
                    batchim = KO_BATCHIM_NONE;
                }

                /* Get and append the appropriate postposition */
                const char *pp = ko_get_postposition(batchim, pp_type);
                if (pp) {
                    while (*pp && (size_t)(outp - outbuf) < outbufsz - 1) {
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
        while (charlen-- > 0 && *inp && (size_t)(outp - outbuf) < outbufsz - 1) {
            *outp++ = *inp++;
        }
    }

    *outp = '\0';
    return outbuf;
}

/*
 * Get batchim type for English words/numbers
 */
ko_batchim_type
ko_english_batchim(const char *str)
{
    size_t len;
    char last_char;

    if (!str || !*str)
        return KO_BATCHIM_NONE;

    len = strlen(str);
    last_char = str[len - 1];

    /* Check if it's a number */
    if (isdigit((unsigned char)last_char)) {
        return number_batchim(last_char - '0');
    }

    /* Check if it's a letter */
    if (isalpha((unsigned char)last_char)) {
        return letter_batchim(last_char);
    }

    /* Unknown - default to no batchim */
    return KO_BATCHIM_NONE;
}

/*
 * Find the last meaningful character for batchim check
 */
int
ko_find_last_char(const char *str, const char **lastpos)
{
    const char *p;
    const char *last = NULL;
    int last_len = 0;
    int charlen;

    if (!str || !*str) {
        if (lastpos) *lastpos = NULL;
        return 0;
    }

    /* Scan through the string */
    p = str;
    while (*p) {
        charlen = utf8_char_len((unsigned char)*p);

        /* Skip whitespace and common punctuation */
        unsigned int cp = utf8_to_codepoint(p, NULL);
        if (cp > 0x20 && cp != '.' && cp != ',' && cp != '!' &&
            cp != '?' && cp != ':' && cp != ';' && cp != '"' &&
            cp != '\'' && cp != ')' && cp != ']' && cp != '}') {
            last = p;
            last_len = charlen;
        }

        p += charlen;
    }

    if (lastpos)
        *lastpos = last;
    return last_len;
}

#endif /* ENABLE_NLS */
