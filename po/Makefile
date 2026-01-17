# NetHack 3.7 Translation Makefile
# Copyright (c) HanNetHack Project, 2026
# NetHack may be freely redistributed. See license for details.

# Tools
XGETTEXT = xgettext
MSGMERGE = msgmerge
MSGFMT = msgfmt
MSGINIT = msginit

# Directories
SRCDIR = ../src
INCDIR = ../include
WINDIR = ../win
DATDIR = ../dat

# Output files
DOMAIN = nethack
POTFILE = $(DOMAIN).pot
LANGUAGES = ko

# Source files to extract strings from
SOURCES = $(wildcard $(SRCDIR)/*.c) \
          $(wildcard $(WINDIR)/tty/*.c) \
          $(wildcard $(WINDIR)/curses/*.c)

# xgettext options
XGETTEXT_OPTS = --keyword=_ --keyword=N_ --keyword=P_:1,2 \
                --keyword=C_:1c,2 \
                --from-code=UTF-8 \
                --add-comments=TRANSLATORS: \
                --package-name=NetHack \
                --package-version=3.7 \
                --msgid-bugs-address=hannethack@example.com \
                --copyright-holder="HanNetHack Project"

.PHONY: all pot update-po compile clean stats help

all: compile

# Generate POT template from source files
pot: $(POTFILE)

$(POTFILE): $(SOURCES)
	@echo "Extracting translatable strings..."
	$(XGETTEXT) $(XGETTEXT_OPTS) -o $@ $(SOURCES)
	@echo "Generated $(POTFILE) with $$(grep -c '^msgid' $@ || echo 0) strings"

# Initialize a new translation file
%.po:
	@if [ ! -f $@ ]; then \
		echo "Creating new translation file: $@"; \
		$(MSGINIT) -i $(POTFILE) -o $@ -l $* --no-translator; \
	fi

# Update existing PO files from POT
update-po: $(POTFILE)
	@for lang in $(LANGUAGES); do \
		if [ -f $$lang.po ]; then \
			echo "Updating $$lang.po..."; \
			$(MSGMERGE) -U $$lang.po $(POTFILE); \
		else \
			echo "Creating $$lang.po..."; \
			$(MSGINIT) -i $(POTFILE) -o $$lang.po -l $$lang --no-translator; \
		fi \
	done

# Compile PO files to MO files
compile: $(LANGUAGES:=.mo)

%.mo: %.po
	@echo "Compiling $<..."
	$(MSGFMT) -c -o $@ $<

# Translation statistics
stats:
	@echo "=== Translation Statistics ==="
	@for lang in $(LANGUAGES); do \
		if [ -f $$lang.po ]; then \
			echo ""; \
			echo "$$lang:"; \
			$(MSGFMT) --statistics $$lang.po 2>&1 || true; \
		fi \
	done

# Check for common translation issues
check:
	@echo "=== Checking translations ==="
	@for lang in $(LANGUAGES); do \
		if [ -f $$lang.po ]; then \
			echo "Checking $$lang.po..."; \
			$(MSGFMT) -c --check-accelerators -o /dev/null $$lang.po 2>&1 || true; \
		fi \
	done

# List untranslated strings
untranslated:
	@for lang in $(LANGUAGES); do \
		if [ -f $$lang.po ]; then \
			echo "=== Untranslated in $$lang.po ==="; \
			grep -A1 'msgstr ""$$' $$lang.po | grep 'msgid' | head -20; \
		fi \
	done

# List fuzzy translations (need review)
fuzzy:
	@for lang in $(LANGUAGES); do \
		if [ -f $$lang.po ]; then \
			echo "=== Fuzzy translations in $$lang.po ==="; \
			grep -B1 '#, fuzzy' $$lang.po | grep 'msgid' | head -20; \
		fi \
	done

# Install compiled translations
install: compile
	@for lang in $(LANGUAGES); do \
		mkdir -p $(DESTDIR)/locale/$$lang/LC_MESSAGES; \
		cp $$lang.mo $(DESTDIR)/locale/$$lang/LC_MESSAGES/$(DOMAIN).mo; \
	done

clean:
	rm -f *.mo *.po~

distclean: clean
	rm -f $(POTFILE)

help:
	@echo "NetHack Translation Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  pot          - Generate POT template from sources"
	@echo "  update-po    - Update PO files from POT template"
	@echo "  compile      - Compile PO files to MO files"
	@echo "  stats        - Show translation statistics"
	@echo "  check        - Check for translation errors"
	@echo "  untranslated - List untranslated strings"
	@echo "  fuzzy        - List fuzzy (needs review) strings"
	@echo "  install      - Install MO files"
	@echo "  clean        - Remove generated files"
	@echo "  help         - Show this help"
	@echo ""
	@echo "Workflow:"
	@echo "  1. make pot        # Extract strings from source"
	@echo "  2. make update-po  # Update/create ko.po"
	@echo "  3. Edit ko.po      # Translate strings"
	@echo "  4. make compile    # Generate ko.mo"
	@echo "  5. make stats      # Check progress"
