#	NetHack Makefile.
#	SCCS Id: @(#)Makefile.doc	3.4	1996/03/23

# for Atari
# SHELL=E:/GEMINI2/MUPFEL.TTP

GUIDEBOOK = Guidebook		# regular ASCII file
#GUIDEBOOK = Guidebook.ps	# PostScript file
#GUIDEBOOK = Guidebook.dvi	# TeX device-independent file

# Some versions of col need -x to keep them from converting spaces to tabs;
# some versions of col don't do the conversion by default and don't
# recognize the option.  Sigh.
COLCMD = col -bx
#COLCMD = col -b

# The command to use to generate a PostScript file
# PSCMD = ditroff | psdit
PSCMD = groff

# Use the "cat" GUIDECMD if nroff and/or tbl and/or col are not installed
# Not appropriate for creating Guidebook.txt.
# GUIDECMD = cat Guidebook.txt
# The following works better with groff-1.18, eg on Linux
# GUIDECMD = tbl tmac.n Guidebook.mn | nroff -c -Tascii | $(COLCMD)
GUIDECMD = tbl tmac.n Guidebook.mn | nroff | $(COLCMD)

# the basic guidebook
Guidebook:	Guidebook.mn
	$(GUIDECMD) > Guidebook

# Fancier output for those with ditroff, psdit and a PostScript printer.
Guidebook.ps:	Guidebook.mn
	tbl tmac.n Guidebook.mn | $(PSCMD) > Guidebook.ps

# Guidebook.tex is the same as Guidebook.mn but formatted with LaTeX.
# - The invocation command for LaTeX may vary in different installations.
# - To print Guidebook.dvi you need to use a suitable dvi-driver.
Guidebook.dvi:	Guidebook.tex
	latex Guidebook.tex


GAME	= nethack
MANDIR	= /usr/man/man6
MANEXT	= 6

# manual installation for most BSD-style systems
GAMEMANCREATE = cp nethack.6
LEVMANCREATE = cp lev_comp.6
DGNMANCREATE = cp dgn_comp.6
RCVRMANCREATE = cp recover.6
DLBMANCREATE = cp dlb.6
# manual installation for most SYSV-style systems
# GAMEMANCREATE = nroff -man nethack.6 >
# LEVMANCREATE = nroff -man lev_comp.6 >
# DGNMANCREATE = nroff -man dgn_comp.6 >
# RCVRMANCREATE = nroff -man recover.6 >
# DLBMANCREATE = nroff -man dlb.6 >

manpages:
	-$(GAMEMANCREATE) $(MANDIR)/$(GAME).$(MANEXT)
	-$(LEVMANCREATE) $(MANDIR)/lev_comp.$(MANEXT)
	-$(DGNMANCREATE) $(MANDIR)/dgn_comp.$(MANEXT)
	-$(RCVRMANCREATE) $(MANDIR)/recover.$(MANEXT)
	-$(DLBMANCREATE) $(MANDIR)/dlb.$(MANEXT)

# manual creation for distribution
DISTRIB = Guidebook.txt nethack.txt lev_comp.txt dgn_comp.txt recover.txt dlb.txt

distrib: $(DISTRIB)
	@echo "Plain text documentation is up to date."

Guidebook.txt	: Guidebook.mn tmac.n
	$(GUIDECMD) > Guidebook.txt
nethack.txt	: nethack.6
	nroff -man nethack.6 | $(COLCMD) > nethack.txt
lev_comp.txt	: lev_comp.6
	nroff -man lev_comp.6 | $(COLCMD) > lev_comp.txt
dgn_comp.txt	: dgn_comp.6
	nroff -man dgn_comp.6 | $(COLCMD) > dgn_comp.txt
recover.txt	: recover.6
	nroff -man recover.6 | $(COLCMD) > recover.txt
dlb.txt		: dlb.6
	nroff -man dlb.6 | $(COLCMD) > dlb.txt


clean:
	-rm -f Guidebook.aux Guidebook.log

spotless: clean
	-rm -f Guidebook Guidebook.ps Guidebook.dvi

maintainer-clean: spotless
	-rm -f $(DISTRIB)
#	-rm -f Makefile
