#!/bin/bash
# NetHack Korean Translation Management Tool
# Copyright (c) HanNetHack Project, 2026

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PO_FILE="$SCRIPT_DIR/ko.po"
POT_FILE="$SCRIPT_DIR/nethack.pot"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

usage() {
    echo "NetHack Korean Translation Tool"
    echo ""
    echo "Usage: $0 <command> [options]"
    echo ""
    echo "Commands:"
    echo "  stats           Show translation statistics"
    echo "  search <term>   Search for strings containing <term>"
    echo "  untranslated    List untranslated strings"
    echo "  fuzzy           List fuzzy (needs review) strings"
    echo "  review          Interactive review mode"
    echo "  export-csv      Export translations to CSV for review"
    echo "  import-csv      Import translations from CSV"
    echo "  validate        Validate translation format"
    echo "  postpos-check   Check Korean postposition patterns"
    echo "  backup          Create backup of current translations"
    echo "  help            Show this help"
    echo ""
    echo "Examples:"
    echo "  $0 stats"
    echo "  $0 search 'You hit'"
    echo "  $0 untranslated | head -20"
}

# Show translation statistics
stats() {
    if [ ! -f "$PO_FILE" ]; then
        echo -e "${RED}Error: $PO_FILE not found${NC}"
        exit 1
    fi

    echo -e "${BLUE}=== NetHack Korean Translation Statistics ===${NC}"
    echo ""

    total=$(grep -c '^msgid "' "$PO_FILE" 2>/dev/null || echo 0)
    translated=$(grep -c '^msgstr ".' "$PO_FILE" 2>/dev/null || echo 0)
    fuzzy=$(grep -c '#, fuzzy' "$PO_FILE" 2>/dev/null || echo 0)
    untrans=$((total - translated - 1))  # -1 for header

    if [ "$total" -gt 0 ]; then
        pct=$((translated * 100 / total))
    else
        pct=0
    fi

    echo -e "Total strings:      ${YELLOW}$total${NC}"
    echo -e "Translated:         ${GREEN}$translated${NC} ($pct%)"
    echo -e "Fuzzy (need review):${YELLOW}$fuzzy${NC}"
    echo -e "Untranslated:       ${RED}$untrans${NC}"
    echo ""

    # Progress bar
    bar_width=50
    filled=$((pct * bar_width / 100))
    empty=$((bar_width - filled))
    echo -n "Progress: ["
    printf "%0.s#" $(seq 1 $filled 2>/dev/null) || true
    printf "%0.s-" $(seq 1 $empty 2>/dev/null) || true
    echo "] $pct%"
}

# Search for strings
search() {
    if [ -z "$1" ]; then
        echo "Usage: $0 search <term>"
        exit 1
    fi

    echo -e "${BLUE}=== Searching for: $1 ===${NC}"
    grep -B1 -A1 "$1" "$PO_FILE" | head -100
}

# List untranslated strings
untranslated() {
    echo -e "${BLUE}=== Untranslated Strings ===${NC}"
    # Find msgid followed by empty msgstr
    awk '/^msgid "/ { id=$0; getline; if ($0 == "msgstr \"\"") print id }' "$PO_FILE" | head -50
}

# List fuzzy translations
fuzzy() {
    echo -e "${BLUE}=== Fuzzy Translations (Need Review) ===${NC}"
    grep -B2 '#, fuzzy' "$PO_FILE" | grep 'msgid "' | head -30
}

# Validate postposition patterns
postpos_check() {
    echo -e "${BLUE}=== Checking Korean Postposition Patterns ===${NC}"
    echo ""

    # Valid patterns
    echo "Checking for valid postposition patterns..."
    valid_patterns=('{은/는}' '{이/가}' '{을/를}' '{과/와}' '{으로/로}' '{아/야}' '{이다/다}' '{이었/였}')

    for pattern in "${valid_patterns[@]}"; do
        count=$(grep -c "$pattern" "$PO_FILE" 2>/dev/null || echo 0)
        if [ "$count" -gt 0 ]; then
            echo -e "  $pattern: ${GREEN}$count occurrences${NC}"
        fi
    done

    echo ""
    echo "Checking for potential issues..."

    # Check for malformed patterns
    malformed=$(grep -E '\{[^/}]+/[^}]+\}' "$PO_FILE" | grep -v -E '\{(은/는|이/가|을/를|과/와|으로/로|아/야|이다/다|이었/였)\}' | head -10)
    if [ -n "$malformed" ]; then
        echo -e "${YELLOW}Warning: Potentially malformed postposition patterns:${NC}"
        echo "$malformed"
    else
        echo -e "${GREEN}No malformed patterns found.${NC}"
    fi
}

# Export to CSV for easier review
export_csv() {
    output="$SCRIPT_DIR/translations.csv"
    echo -e "${BLUE}Exporting to $output...${NC}"

    echo "msgid,msgstr,status" > "$output"

    awk '
    BEGIN { FS="\n"; RS=""; OFS="," }
    /^msgid "/ {
        status = "translated"
        if (/fuzzy/) status = "fuzzy"

        for (i=1; i<=NF; i++) {
            if ($i ~ /^msgid "/) {
                gsub(/^msgid "/, "", $i)
                gsub(/"$/, "", $i)
                gsub(/"/, "\"\"", $i)
                msgid = $i
            }
            if ($i ~ /^msgstr "/) {
                gsub(/^msgstr "/, "", $i)
                gsub(/"$/, "", $i)
                gsub(/"/, "\"\"", $i)
                msgstr = $i
                if (msgstr == "") status = "untranslated"
            }
        }
        if (msgid != "") {
            print "\"" msgid "\",\"" msgstr "\",\"" status "\""
        }
    }
    ' "$PO_FILE" >> "$output"

    echo -e "${GREEN}Exported $(wc -l < "$output") entries to $output${NC}"
}

# Create backup
backup() {
    timestamp=$(date +%Y%m%d_%H%M%S)
    backup_file="$SCRIPT_DIR/backups/ko_$timestamp.po"
    mkdir -p "$SCRIPT_DIR/backups"
    cp "$PO_FILE" "$backup_file"
    echo -e "${GREEN}Backup created: $backup_file${NC}"
}

# Validate translation format
validate() {
    echo -e "${BLUE}=== Validating Translations ===${NC}"

    # Check with msgfmt
    if command -v msgfmt &> /dev/null; then
        echo "Running msgfmt validation..."
        msgfmt -c -o /dev/null "$PO_FILE" 2>&1 && echo -e "${GREEN}msgfmt: OK${NC}" || echo -e "${RED}msgfmt: ERRORS${NC}"
    fi

    # Check for format string mismatches
    echo ""
    echo "Checking format string consistency..."
    awk '
    /^msgid "/ {
        msgid = $0
        gsub(/^msgid "/, "", msgid)
        gsub(/"$/, "", msgid)
        # Count format specifiers
        n = gsub(/%[sdcfx]/, "&", msgid)
        id_formats = n
        id_text = msgid
    }
    /^msgstr "/ {
        msgstr = $0
        gsub(/^msgstr "/, "", msgstr)
        gsub(/"$/, "", msgstr)
        if (msgstr != "") {
            n = gsub(/%[sdcfx]/, "&", msgstr)
            if (n != id_formats) {
                print "Format mismatch:"
                print "  msgid:  " id_text " (" id_formats " formats)"
                print "  msgstr: " msgstr " (" n " formats)"
                print ""
            }
        }
    }
    ' "$PO_FILE" | head -30
}

# Interactive review mode
review() {
    echo -e "${BLUE}=== Interactive Review Mode ===${NC}"
    echo "This feature requires an interactive terminal."
    echo "Consider using poedit or another PO file editor."
    echo ""
    echo "Recommended tools:"
    echo "  - poedit (GUI): https://poedit.net/"
    echo "  - lokalize (KDE): https://apps.kde.org/lokalize/"
    echo "  - gtranslator (GNOME): https://wiki.gnome.org/Apps/Gtranslator"
    echo "  - virtaal: https://virtaal.translatehouse.org/"
}

# Main
case "${1:-help}" in
    stats)
        stats
        ;;
    search)
        search "$2"
        ;;
    untranslated)
        untranslated
        ;;
    fuzzy)
        fuzzy
        ;;
    review)
        review
        ;;
    export-csv)
        export_csv
        ;;
    postpos-check)
        postpos_check
        ;;
    backup)
        backup
        ;;
    validate)
        validate
        ;;
    help|--help|-h)
        usage
        ;;
    *)
        echo "Unknown command: $1"
        usage
        exit 1
        ;;
esac
