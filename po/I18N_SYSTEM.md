# HanNetHack Internationalization (i18n) System

## Overview

HanNetHack extends NetHack 3.7 with a comprehensive internationalization system that supports:

1. **GNU gettext-based translation** - Standard i18n framework for message translation
2. **Korean postposition system** - Automatic grammar handling for Korean language
3. **Full-width character support** - Proper display of CJK characters in terminal
4. **Runtime language switching** - Change language without restarting the game
5. **Localized data files** - Translated help files, rumors, and game content

---

## Table of Contents

1. [Architecture](#1-architecture)
2. [Core Components](#2-core-components)
3. [Translation Macros](#3-translation-macros)
4. [Korean Postposition System](#4-korean-postposition-system)
5. [UTF-8 and Wide Character Support](#5-utf-8-and-wide-character-support)
6. [Language Configuration](#6-language-configuration)
7. [Locale Data Structure](#7-locale-data-structure)
8. [Window System Integration](#8-window-system-integration)
9. [Adding a New Language](#9-adding-a-new-language)
10. [Build System](#10-build-system)
11. [API Reference](#11-api-reference)

---

## 1. Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        Game Code                                 │
│   pline(_("You hit %s{을/를}."), mon_nam(mtmp));                │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    i18n.c / i18n.h                               │
│   • _() macro → gettext()                                        │
│   • Translation lookup from .mo file                             │
│   • Korean postposition processing                               │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                  ko_postpos.c / ko_postpos.h                     │
│   • Batchim (final consonant) detection                          │
│   • Postposition pattern replacement                             │
│   • UTF-8 codepoint handling                                     │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                    Window System (TTY/Curses)                    │
│   • utf8_display_width() for column calculation                  │
│   • utf8_char_width() for character rendering                    │
│   • Proper line wrapping for wide characters                     │
└─────────────────────────────────────────────────────────────────┘
```

---

## 2. Core Components

### File Structure

```
HanNetHack/
├── include/
│   ├── i18n.h              # Main i18n header (macros, declarations)
│   └── ko_postpos.h        # Korean postposition header
├── src/
│   ├── i18n.c              # i18n implementation
│   ├── ko_postpos.c        # Korean grammar processing
│   └── obj_descr_i18n.c    # Object description markers
├── po/
│   ├── nethack.pot         # Translation template
│   ├── ko.po               # Korean translations (source)
│   └── messages.mo         # Compiled translations
├── dat/locale/
│   └── ko/                 # Korean locale data
│       ├── LC_MESSAGES/
│       │   └── nethack.mo  # Installed message catalog
│       ├── help            # Localized help files
│       ├── rumors.tru      # Localized rumors
│       └── *.lua           # Localized Lua scripts
└── win/
    └── tty/
        ├── topl.c          # Top line with UTF-8 support
        └── wintty.c        # TTY output with wide char support
```

### Key Source Files

| File | Lines | Purpose |
|------|-------|---------|
| `i18n.h` | 123 | Translation macros and function declarations |
| `i18n.c` | 420 | Locale management, UTF-8 functions |
| `ko_postpos.h` | 166 | Korean grammar data structures |
| `ko_postpos.c` | 477 | Batchim detection and postposition processing |

---

## 3. Translation Macros

### Basic Macros

```c
#include "i18n.h"

// Standard translation - use for most strings
_("text")

// No-operation marker - for static arrays (translated at use time)
N_("text")

// Plural forms (rarely needed for Korean/Japanese)
P_("singular", "plural", count)

// Context-aware translation (for ambiguous strings)
C_("context", "text")
```

### Usage Examples

```c
// Simple message
pline(_("You feel hungry."));

// With format specifier
pline(_("You hit %s."), mon_nam(mtmp));

// Static array (deferred translation)
static const char *msgs[] = {
    N_("option 1"),
    N_("option 2"),
};
pline("%s", _(msgs[i]));  // Translate at use time

// Conditional strings
const char *msg = cond ? _("yes") : _("no");
```

### What NOT to Wrap

```c
// Already translated strings
pline("%s", translated_variable);

// Debug messages
impossible("debug: unexpected state");

// Empty strings
enl_msg(You_, verb, verb, suffix, "");  // Last "" not wrapped

// Format specifiers alone
Sprintf(buf, "%s", name);  // Don't wrap "%s"
```

---

## 4. Korean Postposition System

### Overview

Korean grammar requires different postpositions based on whether the preceding syllable ends with a consonant (받침/batchim). This system automatically selects the correct form.

### Postposition Patterns

| Pattern | With Batchim | Without Batchim | Usage |
|---------|-------------|-----------------|-------|
| `{은/는}` | 은 | 는 | Topic marker |
| `{이/가}` | 이 | 가 | Subject marker |
| `{을/를}` | 을 | 를 | Object marker |
| `{과/와}` | 과 | 와 | "and/with" |
| `{으로/로}` | 으로 | 로 | Direction/means* |
| `{아/야}` | 아 | 야 | Vocative |
| `{이다/다}` | 이다 | 다 | Copula |
| `{이/}` | 이 | (empty) | Optional subject |

*Special case: Words ending with ㄹ use "로" instead of "으로"

### How It Works

```
Input:  "%s{을/를} 때렸다."  with argument "고블린"

Step 1: Format substitution
        "고블린{을/를} 때렸다."

Step 2: Find last character before pattern
        "린" (Unicode U+B9B0)

Step 3: Check batchim (final consonant)
        린 = 리 + ㄴ → has batchim (ㄴ)

Step 4: Select postposition form
        {을/를} with batchim → "을"

Result: "고블린을 때렸다."
```

### Hangul Decomposition Algorithm

Korean syllables (U+AC00 - U+D7A3) are composed of:
- Choseong (initial consonant): 19 types
- Jungseong (vowel): 21 types
- Jongseong (final consonant): 28 types (including none)

```c
// For codepoint in range [0xAC00, 0xD7A3]:
int jongseong = (codepoint - 0xAC00) % 28;

if (jongseong == 0)  → KO_BATCHIM_NONE   // No final consonant
if (jongseong == 8)  → KO_BATCHIM_RIEUL  // ㄹ (special case)
else                 → KO_BATCHIM_OTHER  // Other consonants
```

### English Word Support

The system also handles English words and numbers by using their Korean pronunciation:

**Numbers:**
| Digit | Korean | Batchim |
|-------|--------|---------|
| 0 | 영/공 | None |
| 1 | 일 | ㄹ (Rieul) |
| 2 | 이 | None |
| 3 | 삼 | ㅁ (Other) |
| 6 | 육 | ㄱ (Other) |
| 7 | 칠 | ㄹ (Rieul) |
| 8 | 팔 | ㄹ (Rieul) |

**Letters:**
| Letter | Korean | Batchim |
|--------|--------|---------|
| F | 에프 | ㅍ (Other) |
| L | 엘 | ㄹ (Rieul) |
| M | 엠 | ㅁ (Other) |
| N | 엔 | ㄴ (Other) |
| R | 아르 | ㄹ (Rieul) |

### API Functions

```c
// Check batchim type of last character in string
ko_batchim_type ko_check_batchim(const char *utf8str);

// Get correct postposition form
const char *ko_get_postposition(ko_batchim_type batchim,
                                 ko_postpos_type type);

// Process all postposition patterns in a string
void ko_process_string(char *outbuf, size_t size, const char *input);

// High-level processing with printf-style formatting
char *process_korean_postpositions(char *buf, const char *format, ...);
```

---

## 5. UTF-8 and Wide Character Support

### Display Width Functions

CJK characters typically occupy 2 terminal columns while ASCII uses 1. These functions handle proper width calculation:

```c
// Get total display width of a UTF-8 string
int utf8_display_width(const char *utf8str);
// Example: utf8_display_width("한글AB") returns 6 (2+2+1+1)

// Get display width of a single UTF-8 character
int utf8_char_width(const char *utf8str);
// Example: utf8_char_width("한") returns 2
```

### UTF-8 Utility Functions

```c
// Get byte length of UTF-8 character from first byte
int utf8_char_len(unsigned char first_byte);

// Decode UTF-8 to Unicode codepoint
unsigned int utf8_to_codepoint(const char *utf8str, int *bytes_read);

// Encode Unicode codepoint to UTF-8
int codepoint_to_utf8(unsigned int codepoint, char *outbuf);
```

### Integration Points

**Terminal Output (win/tty/wintty.c):**
```c
// Track actual display columns, not bytes
for (cp = &cw->data[i][1]; *cp; cp++) {
    if (((unsigned char) *cp & 0xC0) != 0x80) {
        int charwidth = utf8_char_width(cp);
        if (ttyDisplay->curx + charwidth > ttyDisplay->cols)
            break;  // Don't split wide characters
        ttyDisplay->curx += charwidth;
    }
    putchar(*cp);
}
```

**Message Line (win/tty/topl.c):**
```c
// Proper line length calculation for wrapping
int n0 = utf8_display_width(bp);
if (n0 + utf8_display_width(gt.toplines) + 3 < CO - 8)
    // Message fits on current line
```

---

## 6. Language Configuration

### Setting Language

**Via nethackrc:**
```
OPTIONS=language:ko    # Korean
OPTIONS=language:en    # English (default)
```

**At Runtime:**
```c
// In options.c
set_language("ko");  // Switch to Korean
set_language("en");  // Switch to English
```

### Language Initialization

Called early in `main()` from `allmain.c`:

```c
void init_i18n(void)
{
    const char *lang = iflags.language[0] ? iflags.language : "ko";
    set_language(lang);
}
```

### Locale Directory Search Order

1. `NETHACK_LOCALE_DIR` environment variable
2. Relative to executable: `../dat/locale`
3. `HACKDIR/locale`
4. Compile-time `LOCALEDIR`
5. `/usr/share/locale` (fallback)

### Supported Languages

| Code | Locale | Status |
|------|--------|--------|
| `ko` | ko_KR.UTF-8 | Complete |
| `en` | en_US.UTF-8 | Default |

---

## 7. Locale Data Structure

```
dat/locale/ko/
├── LC_MESSAGES/
│   └── nethack.mo          # Compiled message catalog (638 KB)
├── help                    # General help
├── hh                      # Short help
├── cmdhelp                 # Command help
├── keyhelp                 # Keyboard help
├── opthelp                 # Options help
├── optmenu                 # Options menu text
├── wizhelp                 # Wizard mode help
├── usagehlp                # Usage help
├── history                 # Game history
├── rumors.tru              # True rumors
├── rumors.fal              # False rumors
├── epitaph.txt             # Tombstone epitaphs
├── engrave.txt             # Engravings
├── bogusmon.txt            # Fake monster names
├── oracles.txt             # Oracle messages
└── *.lua                   # Lua quest dialogues
```

### Help File Loading

```c
// Get localized filename
const char *get_localized_filename(const char *fname);
// "help" → "locale/ko/help" (if Korean)
// "help" → "help" (if English)
```

---

## 8. Window System Integration

### TTY Interface

The TTY window system has been modified to handle UTF-8 properly:

**Column Tracking:**
- All `strlen()` calls for display purposes replaced with `utf8_display_width()`
- Cursor position tracked in display columns, not bytes

**Character Rendering:**
- Wide characters (CJK) render in 2 columns
- Line wrapping prevents splitting multi-byte characters
- Proper handling of combining characters

**Key Files Modified:**
- `win/tty/wintty.c` - Main TTY window code
- `win/tty/topl.c` - Top message line
- `win/tty/getline.c` - Input handling

### Curses Interface

Similar modifications apply to the curses interface for proper wide character support.

---

## 9. Adding a New Language

### Step 1: Create PO File

```bash
cd po/
msginit -i nethack.pot -o ja.po -l ja_JP.UTF-8
```

### Step 2: Translate Messages

Edit `ja.po` and translate all `msgstr` entries:

```
#: ../src/eat.c:123
msgid "You feel hungry."
msgstr "お腹が空いた。"
```

### Step 3: Compile MO File

```bash
msgfmt -c -v -o ja.mo ja.po
```

### Step 4: Create Locale Directory

```bash
mkdir -p dat/locale/ja/LC_MESSAGES
cp ja.mo dat/locale/ja/LC_MESSAGES/nethack.mo
```

### Step 5: Add Language Support

In `src/i18n.c`, add to `set_language()`:

```c
} else if (strcmp(lang, "ja") == 0) {
    strncpy(locale_name, "ja_JP.UTF-8", sizeof(locale_name) - 1);
```

### Step 6: Translate Data Files

Copy and translate files in `dat/locale/ja/`:
- help, cmdhelp, rumors.tru, etc.

### Step 7: Add Language-Specific Processing (Optional)

If the language has special grammar rules (like Korean postpositions), create dedicated processing functions.

---

## 10. Build System

### Makefile Targets

```bash
# Generate POT template from source
make pot

# Update PO file with new strings
msgmerge -U ko.po nethack.pot

# Compile MO file
msgfmt -c -v -o messages.mo ko.po

# Install to locale directory
cp messages.mo ../dat/locale/ko/LC_MESSAGES/nethack.mo
```

### Compilation Flags

Required defines:
- `ENABLE_NLS` - Enable Native Language Support
- `LOCALEDIR` - Default locale directory path

Required libraries:
- `libintl` - GNU gettext runtime
- `libiconv` - Character encoding conversion (if needed)

### CMake Configuration

```cmake
find_package(Intl REQUIRED)
target_link_libraries(nethack ${Intl_LIBRARIES})
target_compile_definitions(nethack PRIVATE ENABLE_NLS)
```

---

## 11. API Reference

### i18n.h

```c
// Initialize i18n subsystem (call early in main())
void init_i18n(void);

// Set/change current language
void set_language(const char *lang);

// Get current language code
const char *get_current_language(void);

// Check if Korean locale is active
boolean is_korean_locale(void);

// Get display width of UTF-8 string
int utf8_display_width(const char *utf8str);

// Get display width of single UTF-8 character
int utf8_char_width(const char *utf8str);

// Translate object name
const char *tr_obj_name(const char *name);

// Get localized filename
const char *get_localized_filename(const char *fname);

// Process Korean postpositions with printf-style formatting
char *process_korean_postpositions(char *buf, const char *format, ...);
```

### ko_postpos.h

```c
// Batchim types
typedef enum {
    KO_BATCHIM_NONE,   // No final consonant
    KO_BATCHIM_RIEUL,  // ㄹ final consonant
    KO_BATCHIM_OTHER   // Other final consonants
} ko_batchim_type;

// Postposition types
typedef enum {
    KO_PP_EUN_NEUN,    // {은/는}
    KO_PP_I_GA,        // {이/가}
    KO_PP_EUL_REUL,    // {을/를}
    KO_PP_GWA_WA,      // {과/와}
    KO_PP_EURO_RO,     // {으로/로}
    KO_PP_A_YA,        // {아/야}
    KO_PP_IDA_DA,      // {이다/다}
    KO_PP_I_EMPTY,     // {이/}
    KO_PP_IEOT_YEOT    // {이었/였}
} ko_postpos_type;

// Check batchim of last character
ko_batchim_type ko_check_batchim(const char *utf8str);

// Get postposition form
const char *ko_get_postposition(ko_batchim_type bt, ko_postpos_type pt);

// Process postposition patterns in string
void ko_process_string(char *outbuf, size_t size, const char *input);

// UTF-8 utilities
int utf8_char_len(unsigned char first_byte);
unsigned int utf8_to_codepoint(const char *utf8str, int *bytes_read);
int codepoint_to_utf8(unsigned int codepoint, char *outbuf);
```

---

## Appendix A: Unicode Ranges

| Range | Description |
|-------|-------------|
| U+AC00 - U+D7A3 | Hangul Syllables (가-힣) |
| U+1100 - U+11FF | Hangul Jamo |
| U+3130 - U+318F | Hangul Compatibility Jamo |
| U+4E00 - U+9FFF | CJK Unified Ideographs |
| U+3040 - U+309F | Hiragana |
| U+30A0 - U+30FF | Katakana |

---

## Appendix B: Troubleshooting

### Messages Not Translated

1. Check MO file is compiled: `msgfmt -c ko.po`
2. Verify locale directory: `ls dat/locale/ko/LC_MESSAGES/`
3. Check language setting: `OPTIONS=language:ko`
4. Rebuild nhdat: `make`

### Wide Characters Display Wrong

1. Verify terminal supports UTF-8
2. Check `LANG` environment variable
3. Ensure font has CJK glyphs
4. Test with: `echo "한글테스트"`

### Postpositions Not Working

1. Verify `is_korean_locale()` returns TRUE
2. Check pattern syntax: `{은/는}` not `(은/는)`
3. Ensure string is processed through `ko_process_string()`

### Build Errors

```
undefined reference to `gettext'
```
→ Link with `-lintl`

```
i18n.h: No such file or directory
```
→ Add `include/` to include path

---

*This document describes the HanNetHack i18n system as of NetHack 3.7.*
*For Korean-specific translation guidelines, see TRANSLATION_GUIDE_KO.md.*
