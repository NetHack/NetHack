#!/bin/perl
#
#  $NHDT-Date: 1524689313 2018/04/25 20:48:33 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.5 $
# Copyright (c) 2015 by Derek S. Ray
# NetHack may be freely redistributed.  See license for details.
#

sub bail($);

use Getopt::Std;

# TODO: switch to Getopt::Long so we can parse normal arguments too
$Getopt::Std::STANDARD_HELP_VERSION = TRUE;
$main::VERSION = 1.0;

my %commands = (
    'd' => 'debug mode; parse objects.txt to stdout instead of updating',
);

getopts(join('', keys(%commands)));

my $debug = (defined($opt_d) && $opt_d == 1);
my $tilecount = 0;
my $outfile = $debug ? "-" : "objects.txt";
my $infile = $debug ? "objects.txt" : "objects.bak";


unless ($debug) {
    if (-e "$infile") { die "something didn't clean up objects.bak from last time; stopping\n"; }
    rename($outfile,$infile) or die "couldn't move objects.txt to objects.bak; stopping\n";
}

open(INFILE, "<$infile") or bail("couldn't open $infile; bailing");
open(OUTFILE, ">$outfile") or bail("couldn't open $outfile; bailing");

while (my $line = <INFILE>)
{
    if (my ($tiletext) = $line =~ /^# tile \d+ (.*)/)
    {
        $line = "# tile $tilecount $tiletext\n";
        $tilecount++;
    }

    print OUTFILE $line;
}

close(INFILE);
close(OUTFILE);

unless ($debug) { unlink $infile; }

exit;

sub main::HELP_MESSAGE()
{
    print <<"STARTHELP";
Usage: renumtiles.pl [OPTIONS] <textfile>

STARTHELP
    foreach $cmd (keys(%commands)) {
        printf("%10s          %s\n", '-'.$cmd, $commands{$cmd});
    }
    print <<"ENDHELP";

\t--help      display this help message and exit
\t--version   display version and exit
ENDHELP
    exit;
}

sub main::VERSION_MESSAGE()
{
    my ($objglob, $optpackage, $ver, $switches) = @_;
    print <<"STARTHELP";
renumtiles $ver -- tile-renumbering utility for NetHack
STARTHELP
}

sub bail($)
{
    unless ($debug) {
        unlink $outfile;
        rename ($infile,$outfile);
    }
    shift;
    die "$_\n";
}

