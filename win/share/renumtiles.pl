#!/usr/bin/env perl
#
#  $NHDT-Date: 1524689313 2018/04/25 20:48:33 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.5 $
# Copyright (c) 2015 by Derek S. Ray
# NetHack may be freely redistributed.  See license for details.
#

use strict;
use warnings;

use Getopt::Long;
use Pod::Usage;

$main::VERSION = 1.0;

sub bail($);

my ($infile, $outfile) = ('') x 2;

GetOptions(
    debug   => \my $debug,
    help    => \my $help,
    version => \my $version,
);

pod2usage() if $help;

if ($version) {
    open(my $mem_fh, '>', \my $ver_info);

    pod2usage(
        -verbose  => 99,
        -sections => 'NAME',
        -output   => $mem_fh,
        -exitval  => 'NOEXIT',
    );

    close $mem_fh;

    $ver_info =~ s/\$main::VERSION/"sprintf '%.1f', $&" /ee;
    print $ver_info;
}

my $tilecount = 0;

if ($infile eq '') {
    $infile = $debug ? "objects.txt" : "objects.bak";
}
if ($outfile eq '') {
    $outfile = $debug ? "-" : "objects.txt";
}

unless ($debug) {
    die "something didn't clean up objects.bak from last time; stopping\n" if -e $infile;
    rename($outfile, $infile) or die "couldn't move objects.txt to objects.bak; stopping\n";
}

open(INFILE, "<$infile") or bail("couldn't open $infile; bailing");
open(OUTFILE, ">$outfile") or bail("couldn't open $outfile; bailing");

while (my $line = <INFILE>) {
    if ($line =~ /^# tile \d+ (.*)/) {
        $line = "# tile $tilecount $1\n";
        $tilecount++;
    }

    print OUTFILE $line;
}

close(INFILE);
close(OUTFILE);

unlink $infile unless $debug;

sub bail($) {
    my $message = shift;
    if ($debug) {
        unlink $outfile;
        rename($infile, $outfile);
    }
    die "$message\n";
}
__END__
=head1 NAME

renumtiles.pl $main::VERSION -- tile-renumbering utility for NetHack

=head1 SYNOPSIS

renumtiles.pl [OPTIONS] <textfile>

    -d          debug mode; parse objects.txt to stdout instead of updating
    --help      display this help message and exit
    --version   display version and exit

=cut