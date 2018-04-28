#!/usr/bin/perl
# NetHack 3.6  getinfo.pl       $NHDT-Date: 1524689669 2018/04/25 20:54:29 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.2 $
# Copyright (c) 2018 by Michael Allison
# NetHack may be freely redistributed.  See license for details.

#STARTUP-START
BEGIN {
    # OS hackery has to be duplicated in each of the hooks :/
    # first the directory separator
    my $DS = quotemeta('/');
    my $PDS = '/';
    # msys: POSIXish over a Windows filesystem (so / not \ but \r\n not \n).
    # temporarily removed because inconsistent behavior
    # if ($^O eq "msys")
    # {
    #   $/ = "\r\n";
    #   $\ = "\r\n";
    # }
    if($^O eq "MSWin32"){
        $DS = quotemeta('\\');
	$PDS = '\\';
    }
    $gitdir = `git rev-parse --git-dir`;
    chomp $gitdir;
    push(@INC, $gitdir.$PDS."hooks");

	# special case for this script only: allow
	# it to run from DEVEL or $TOP
    if (-f "hooksdir/NHgithook.pm" || -f "DEVEL/hooksdir/NHgithook.pm"){
	push(@INC, "DEVEL/hooksdir");
    }
    chdir("..") if (-f "hooksdir/NHgithook.pm");
}
use NHgithook;

&NHgithook::nhversioning;
