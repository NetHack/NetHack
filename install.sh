#!/bin/bash
sh sys/unix/setup.sh sys/unix/hints/linux
make all
make install
mkdir -p ~/.local/bin
ln -s ~/nh/install/games/lib/nethackdir/nethack ~/.local/bin/nethack
