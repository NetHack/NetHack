# $NHDT-Header: NetHack 5.0 unx2atar.sed 2026/09/30_14:47:34 NetHack-5.0:1.3 $
# Copyright (c) Michael Allison 2020.
# NetHack may be freely redistributed.  See license for details.

: loop
/\\$/N
/\\$/b loop
# for each line, append any continuation lines before trying to classify it
/^	/ {
# if it starts with a tab, it's meant for the shell, and we should think
#  about reversing the slashes
s;cd ../util;cd ..\\util;
s;cd ../src;cd ..\\src;
/librarian/ s;dat/options;dat\\options;
/$(MAKE)/b
/$(CC)/b
s;/;\\;g
}
# unfortunately, we do not want to reverse *all* the slashes, as even the
#  Atari make and gcc like forward ones, and it's messy to avoid the ones in
#  sed addresses
# so, flip the first one in e.g.
# 	@( cd ../util ; $(MAKE) ../include/onames.h )
# flip the librarian-related ones in dat/options
# ignore other lines related to make and gcc
# and flip any slashes left over, which include a number of UNIX-only things
#  that we didn't need to do but don't hurt
