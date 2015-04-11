#!/bin/perl
#
#  $NHDT-Date$  $NHDT-Branch$:$NHDT-Revision$
#  $Date: 2002/01/05 21:06:02 $  $Revision: 1.1 $
#

sub bail($);
sub setfile($);
sub helpmessage();

use Getopt::Long qw(:config bundling auto_version permute);

$main::VERSION = 1.1;

our %opts = ();
our %commands = (
    'output|o:s' => "-o, --output\tspecify an alternate output file",
    'debug|d!' => "-d, --debug\tforce output to STDOUT",
    'help|h' => "-h, --help\tdisplay this message and exit",
);

my $helpformat = "%20s   %s\n";
my $tilecount = 0;

GetOptions(\%opts, '<>' => \&setfile, keys(%commands));

if ($opts{'help'}) { helpmessage(); exit; }
if (!defined($opts{'infile'})) { helpmessage(); die "no file specified for processing; stopping\n"; }

if ($opts{'debug'}) {
    $opts{'output'} = '-';
}

if (defined($opts{'output'})) {
    if (-e $opts{'output'}) { die "can't write to $opts{'output'}, it already exists; stopping\n"; }
    open(INFILE, "<$opts{'infile'}") or bail("couldn't open $opts{'infile'}; bailing");
} else {
    $opts{'output'} = $opts{'infile'};
    if (-e "$opts{'infile'}.bak") { die "something didn't clean up $opts{'infile'}.bak from last time; stopping\n"; }
    rename($opts{'infile'},"$opts{'infile'}.bak") or die "couldn't move $opts{'infile'} to $opts{'infile'}.bak; stopping\n";
    open(INFILE, "<$opts{'infile'}.bak") or bail("couldn't open $opts{'infile'}.bak; bailing");
}

open(OUTFILE, ">$opts{'output'}") or bail("couldn't open $opts{'output'}; bailing");

while (my $line = <INFILE>)
{
    $line =~ s/cmap \d+/cmap $tilecount/;
    if ($line =~ s/^# tile \d+/# tile $tilecount/) { $tilecount++; }
    print OUTFILE $line;
}

close(INFILE);
close(OUTFILE);

unless ($opts{'debug'}) { unlink "$opts{'infile'}.bak"; }

exit;

sub helpmessage()
{
    print("Usage: renumtiles.pl [OPTIONS] <textfile>\n\n");
    foreach $opt (keys(%commands)) {
        my ($vname, $desc) = split("\t", $commands{$opt});
        $desc =~ s/\t//g;
        printf($helpformat, $vname, $desc);
    }
    printf($helpformat, "--version", "display version and exit\n");
}

sub bail($)
{
    unless ($opts{'debug'}) {
        unlink $opts{'outfile'};
        rename ("$opts{'infile'}.bak",$opts{'outfile'});
    }
    shift;
    die "$_\n";
}

sub setfile($)
{
    if (defined($opts{'infile'})) {
        return;
    }
    $opts{'infile'} = shift;
}

