#!/usr/bin/env perl
#
#  $NHDT-Date: 1524689313 2018/04/25 20:48:33 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.5 $
# Copyright (c) 2015 by Derek S. Ray
# NetHack may be freely redistributed.  See license for details.
#

use Getopt::Long;

my $ver = "1.0";

my %commands_descriptions = (
    'd' => 'debug mode; parse objects.txt to stdout instead of updating',
);

my $debug = '';
my $help = '';
my $version = '';
my $outfile = '';
my $infile = '';

GetOptions(
    "debug"   => \$debug,
    "help"    => \$help,
    "version" => \$version
);

if ($help ne '') {
    printHelpMessage();
}

if ($version ne '') {
    printVersionMessage();
}

my $tilecount = 0;

if ($infile eq '') {
    $infile = $debug ? "objects.txt" : "objects.bak";
}
if ($outfile eq '') {
    $outfile = $debug ? "-" : "objects.txt";
}

if ($debug eq '') {
    die "something didn't clean up $infile from last time; stopping\n" if -e "$infile";
    rename($outfile, $infile) or die "couldn't move $outfile to $infile; stopping\n";
}

open(INFILE, "<$infile") or bail("couldn't open $infile; bailing");
open(OUTFILE, ">$outfile") or bail("couldn't open $outfile; bailing");

while (my $line = <INFILE>) {
    if (my $tiletext = $line =~ /^# tile \d+ (.*)/) {
        $line = "# tile $tilecount $tiletext\n";
        $tilecount++;
    }

    print OUTFILE $line;
}

close(INFILE);
close(OUTFILE);

if ($debug eq '') {unlink $infile;}

sub printHelpMessage() {
    print <<"STARTHELP";
Usage: renumtiles.pl [OPTIONS] <textfile>

STARTHELP
    foreach my $cmd (keys(%commands_descriptions)) {
        printf("%10s          %s\n", '-' . $cmd, $commands_descriptions{$cmd});
    }
    print <<"ENDHELP";

\t--help      display this help message and exit
\t--version   display version and exit
ENDHELP
    exit;
}

sub printVersionMessage() {
    print <<"STARTHELP";
renumtiles $ver -- tile-renumbering utility for NetHack
STARTHELP
}

sub bail {
    my $message = shift;
    if ($debug eq '') {
        unlink $outfile;
        rename($infile, $outfile);
    }
    die "$message\n";
}

