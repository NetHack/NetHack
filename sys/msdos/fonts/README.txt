The script makefont.lua converts the Terminus fonts from the BDF sources to
PSF version 2 for use by the MS-DOS NetHack. The directory sys/msdos/fonts
receives the converted fonts.

makefont.lua is specifically meant for use with NetHack; it rearranges the
input glyphs so the first 256 positions conform to IBM437. The fonts can then
support IBMGraphics without using the Unicode table.
