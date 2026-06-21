#!/usr/bin/perl
# $NHDT-Date: 1693357449 2023/08/30 01:04:09 $ $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.3 $
# Copyright (c) 2015 by Kenneth Lorber, Kensington, Maryland
# NetHack may be freely redistributed.  See license for details.


use Cwd;
use Getopt::Std;

# Activestate Perl doesn't include File::Spec.  Grr.
BEGIN {
    eval "require File::Spec::Functions";
    if($@){
	die <<E_O_M;
File::Spec not found.  (If you are running ActiveState Perl please run:
	    cpan File::Spec
then re-run this program.
E_O_M
    }
    File::Spec::Functions->import;
}

exit 1 unless(getopts('nvf'));	# TODO: this can probably have better output

BEGIN {
	# OS hackery
    $DS = quotemeta('/');		# Directory Separator (for regex)
    $PDS = '/';				# ... for printing
#   Temporarily disabled; there's something weird about msys
#	msys: POSIXish over a Windows filesystem (so / not \ but \r\n not \n).
#if($^O eq "msys"){
#	$/ = "\r\n";
#	$\ = "\r\n";
#	# NB: We don't need to do anything about File::Spec.  It doesn't know
#	#     about msys but it defaults to Unix, so we'll be ok.
#}
    if($^O eq "MSWin32"){
	$DS = quotemeta('\\');
	$PDS = '\\';
    }

    # Fix @INC so we can 'use NHgithook' before it's installed.
    # Set $is_sourcerepo while we're at it - same logic.
    {
	    # Special case for running nhgitset against a different repo.
	    # Must precede the normal case!
	    # NB: we use $DEVhooksdir later in the program
	$DEVhooksdir = ($0 =~ m!^(.*)$DS!)[0];
	chomp($DEVhooksdir);
	$DEVhooksdir .= $PDS."hooksdir";
	push(@INC, $DEVhooksdir) if(-d $DEVhooksdir);
	    # This one is for the normal case (in NHsource)
	my $topdir = `git rev-parse --show-toplevel`;
	chomp $topdir;
	$topdir .= "${PDS}DEVEL${PDS}hooksdir";
	push(@INC, $topdir) if(-d $topdir);
	$is_sourcerepo = 0;
	$is_sourcerepo = 1 if(-d $topdir);
    }
}

use NHgithook;
	# current (installed) version, if any (0 is no entry ergo new repo)
my $version_old = NHgithook::version_in_git;
	# version this program will install
my($version_new,$message_new) = NHgithook::version_in_devel;

if(0==$version_new and !opt_f){
	# Edge case: this repo has been set up using version >= 4, but now we're running
	# nhgitset after checking out DEVEL supporting version <4.
	# Use -f to recover from broken DEVEL code.
    print STDERR "DEVEL has version <4 code but version >=4 code already installed.  Stopping.\n";
    print STDERR "(If you need to reinstall the old code, rerun with -f\n";
    exit 0;
}


die "Valid version not found in DEVEL/VERSION" unless(0==$version_new or $version_new >= 4);

# make sure we're at the top level of a repo
if(! -d ".git"){
    die "This is not the top level of a git repository.\n";
}

if($version_old >= $version_new and !opt_f){
    print STDERR "Nothing to do.\n";
    exit 0;
}

if($version_old > 0){
    if($version_old != $version_new){
	print STDERR "Migrating from setup version $version_old to $version_new\n";
	if(length $message_new){
	    print STDERR "Additional information:\n$message_new\n";
	}
# This list and the code below will need to be updated if other changes occur
# before this progression is finished.
# 6 announce eventual deprecation of PRE and POST hooks
#  6->7 unset nethack.NoDepWarn, change message, docs in NHgithook.pm
# 7 deprecation of PRE and POST hooks
#  7->8 unset nethack.NoDepWarn, change message, docs in NHgithook.pm and make fail
# 8 PRE and POST throw errors
#  8->9 unset nethack.NoDepWarn, remove code, docs
# 9 removal of PRE and POST code
	system('git','config','--unset','nethack.NoDepWarn') unless($opt_n);
    }
}


# legacy check:
if(length $version_old == 0){
    if(`git config --get merge.NHsubst.name` =~ m/^Net/){
	$version_old = 1;
	print STDERR "Migrating to setup version 1\n" if($opt_v);
    }
}

my $gitadddir = `git config --get nethack.gitadddir`;
chomp($gitadddir);
if(length $gitadddir){
    if(! -d $gitadddir){
	die "nethack.gitadddir has invalid value '$gitadddir'\n";
    }
}
print STDERR "nethack.gitadddir=$gitadddir\n" if($opt_v);

# This is (relatively) safe because we know we're at R in R/DEVEL/nhgitset.pl
my $srcdir = ($0 =~ m!^(.*)$DS!)[0];

#XXX do I really want a full path for srcdir?  how badly?

if(! -f catfile($srcdir, 'nhgitset.pl')){
    die "I can't find myself in '$srcdir'\n";
}

print STDERR "Copying from: $srcdir\n" if($opt_v);

if($opt_f || $version_old==0){
    print STDERR "Configuring line endings\n" if($opt_v);
    system("git reset") unless($opt_n);
    &add_config('core.safecrlf', 'true') unless($opt_n);
    &add_config('core.autocrlf', 'false') unless($opt_n);
} elsif($version_old <2){
    my $xx = `git config --get --local core.safecrlf`;
    if($xx !~ m/true/){
	print STDERR "\nNeed to 'rm .git${PDS}index;git reset'.\n";
	print STDERR " When ready to proceed, re-run with -f flag.\n";
	exit 2;
    }
}

print STDERR "Installing aliases\n" if($opt_v);
$addpath = catfile(curdir(),'.git','hooks','NHadd');
&add_alias('nhadd', "!$addpath add");
    &add_help('nhadd', 'NHadd');
&add_alias('nhcommit', "!$addpath commit");
    &add_help('nhcommit', 'NHadd');
my $nhsub = catfile(curdir(),'.git','hooks','nhsub');
&add_alias('nhsub', "!$nhsub");
    &add_help('nhsub', 'nhsub');
&add_alias('nhhelp', '!'.catfile(curdir(),'.git','hooks','nhhelp'));
    &add_help('nhhelp', 'nhhelp');

&add_help('NHsubst', 'NHsubst');
&add_help('NHgithook', 'NHgithook.pm');
&add_help('nhgitset', 'gitsetdocs', '../../DEVEL/nhgitset.pl');


# removed at version 3
#print STDERR "Installing filter/merge\n" if($opt_v);
#if($^O eq "MSWin32"){
#	$cmd = '.git\\\\hooks\\\\NHtext';
#} else {
#	$cmd = catfile(curdir(),'.git','hooks','NHtext');
#}
#&add_config('filter.NHtext.clean', "$cmd --clean %f");
#&add_config('filter.NHtext.smudge', "$cmd --smudge %f");
if($version_old == 1 or $version_old == 2){
    print STDERR "Removing filter.NHtext\n" if($opt_v);
    system('git','config','--unset','filter.NHtext.clean') unless($opt_n);
    system('git','config','--unset','filter.NHtext.smudge') unless($opt_n);
    system('git','config','--remove-section','filter.NHtext') unless($opt_n);

    print STDERR "Removing NHtext\n" if($opt_v);
    unlink catfile(curdir(),'.git','hooks','NHtext') unless($opt_n);
}

&add_config('nethack.setuppath',$srcdir);
&add_config('nethack.is-sourcerepo',0+$is_sourcerepo);

$cmd = catfile(curdir(),'.git','hooks','NHsubst');
&add_config('merge.NHsubst.name', 'NetHack Keyword Substitution');
&add_config('merge.NHsubst.driver', "$cmd %O %A %B %L");

print STDERR "Running directories\n" if($opt_v);

# copy directories into .git (right now that's just hooks and nhgitset.pl)
my @gitadd = length($gitadddir)?glob("$gitadddir$DS*"):undef;
foreach my $dir ( (glob("$srcdir$DS*"), @gitadd) ){
    next unless(-d $dir);

    my $target = catfile($dir, 'TARGET');
    next unless(-f $target);

    open TARGET, '<', $target or die "$target: $!";
    my $targetpath = <TARGET>;
	    # still have to eat all these line endings under msys, so instead of chomp use this:
    $targetpath =~ s![\r\n]!!g;
    close TARGET;
    print STDERR "Directory $dir -> $targetpath\n" if($opt_v);

    my $enddir = $dir;
    $enddir =~ s!.*$DS!!;
    if(! &process_override($enddir, "INSTEAD")){
	&process_override($enddir, "PRE");
	my $fnname = "do_dir_$enddir";
	if(defined &$fnname){
	    &$fnname($dir, $targetpath);
	}
	&process_override($enddir, "POST");
    }
}
&do_file_nhgitset();

&check_gitvars;	# for variable substitution

if($version_old != $version_new or $opt_f){
    print STDERR "Setting version to $version_new\n" if($opt_v);
    NHgithook::version_set_git($version_new) if(! $opt_n);
}

exit 0;

# @files: [0] is the name under .git/hooks; others are places to
#  check during configuration
sub add_help {
    my($cmd, @files) = @_;

    &add_config("nethack.aliashelp.$cmd", $files[0]);
	# pull out =for nhgitset CMD description...
    my $desc;
    foreach my $file (@files){
	open my $fh, "<", "$DEVhooksdir/$file";
	if($fh){
	    while(<$fh>){
		m/^=for\s+nhgitset\s+\Q$cmd\E\s+(.*)/ && do {
		    $desc = $1;
		    goto found;
		}
	    }
	    close $fh;
	} else {
	    warn "Can't open: '$DEVhooksdir/$file' ($!)\n";
	}
    }
found:

    if($desc){
	&add_config("nethack.aliasdesc.$cmd", $desc);
    } else {
	&add_config("nethack.aliasdesc.$cmd", "(no description available)");
    }
}

sub process_override {
    my($srcdir, $plname) = @_;
    return 0 unless(length $gitadddir);

    my $plpath = catfile($gitadddir, $srcdir, $plname);
    return 0 unless(-x $plpath);

    print STDERR "RunningOverride $plpath\n" if($opt_v);

	# current directory is top of target repo
    unless($opt_n){
	system("$plpath $opt_v") and die "Callout $plpath failed: $?\n";
    }
    return 1;
}

sub add_alias {
    my($name, $def) = @_;
    &add_config("alias.$name",$def);
}

sub add_config {
    my($name, $val) = @_;
    system('git', 'config', '--local', $name, $val) unless($opt_n);
}

sub check_gitvars {
    &check_prefix("substprefix");
    &check_prefix("projectname");
}

sub check_prefix {
    my $which = $_[0];
    my $lcl = `git config --local --get nethack.$which`;
    chomp($lcl);
    if(0==length $lcl){
	my $other = `git config --get nethack.$which`;
	chomp($other);
	if(0==length $other){
	    print STDERR "ERROR: nethack.$which is not set anywhere.  Set it and re-run.\n";
	    exit 2;
	} else {
	    &add_config('nethack.$which', $other);
	    print STDERR "Copying prefix '$other' to local repository.\n" if($opt_v);
	}
	$lcl = $other;	# for display below
    }
    print "Using $which '$lcl' - PLEASE MAKE SURE THIS IS CORRECT\n";
}

sub do_dir_DOTGIT {
    my($srcdir, $targetdir) = @_;
	# not currently in use so just bail
    return;
	# are there other files in .git that we might want to handle?
	# So just in case:
    for my $file ( glob("$srcdir/*") ){
	next if( $file =~ m!.*/TARGET$! );
	next if( $file =~ m!.*/config$! );
	die "ERROR: no handler for $file\n";
    }
}

sub do_dir_hooksdir {
    my($srcdir, $targetdir) = @_;

    unless (-d $targetdir){
		# Older versions of git, when cloning a repo and
		# the expected source templates directory does not
		# exist, does not create .git/hooks.  So do it here.
	mkdir $targetdir;
	print STDERR "WARNING: .git/hooks had to be created.\n";
	print STDERR "  You may want to update git.\n";
    }

    for my $path ( glob("$srcdir$DS*") ){
	next if( $path =~ m!.*${DS}TARGET$! );

	my $file = $path;

	$file =~ s!.*$DS!!;
	$file = catfile($targetdir, $file);

	next if($opt_n);

	open IN, "<", $path or die "Can't open $path: $!";
	open OUT, ">", "$file" or die "Can't open $file: $!";
	while(<IN>){
	    print OUT;
	}
	close OUT;
	close IN;

	if(! -x $file){
	    chmod 0755 ,$file;
	}
    }
}

sub do_file_nhgitset {
    my $infile = "DEVEL/nhgitset.pl";
    $infile = "$DEVhooksdir/../nhgitset.pl" unless(-f $infile);
    my $outfile = ".git/hooks/gitsetdocs";
    open IN, "<", $infile or die "Can't open $infile:$!";
    open OUT, ">", $outfile or die "Can't open $outfile:$!";
    my $started;
    print OUT "=pod\n";
    while(<IN>){
	m/^__END__/ && do {$started =1; next};
	print OUT if($started);
    }
    close OUT;
    close IN;
}

#(can we change the .gitattributes syntax to include a comment character?)
#maybe [comment]  attr.c:parse_attr_line
#grr - looks like # is the comment character
__END__
=head1 NAME

nhgitset.pl - Setup program for NetHack git repositories

=head1 SYNOPSIS

 cd THE_REPO
 [git config nethack.gitadddir GITADDDIR]
 perl SOME_PATH/DEVEL/nhgitset.pl [-v][-n][-f]

=head1 DESCRIPTION

nhgitset.pl installs NetHack-specific setup after a C<git clone> or after
changes to the setup, which are installed by re-running nhgitset.pl.  If
an upgrade is needed, you will be informed during a C<git pull> or similar
operation.

The following options are available:

=over

=item B<-f>

Force.  Do not use this unless the program requests it or the hooks are broken.

=back

=over

=item B<-n>

Dry-run - make no changes.

=back

=over

=item B<-v>

Verbose output.

=back

=head1 CONFIG

nhgitset.pl uses the following non-standard C<git config> variables:

nethack.gitadddir

   DOTGIT/INSTEAD
   DOTGIT/PRE
   DOTGIT/POST
   hooksdir/INSTEAD
   hooksdir/PRE
   hooksdir/POST

nethack.setupversion

   The version for nhgitset.pl and friends; has no relationship
   to NetHack version numbers.

nethack.substprefix

   The prefix this repo uses for variable substitution.

nethack.projectname

   The name of the game being built - see C<Project()>.

nethack.is-sourcerepo

   Does this repo contain NetHack source code? (1 = yes, 0 = no)

nethack.setuppath

   Path to (and including) the DEVEL directory used including the
   copy of nhgitset.pl used to set up this repo.

nethack.aliashelp.*

   The last element of the variable is the name used with C<git nhhelp>
   and the value is the name of a file to display with C<perldoc>.

nethack.aliasdesc.*

   The last element of the variable is the name used with C<git nhhelp>
   and the value is short help displayed with C<git nhhelp -a>.

=head1 EXIT STATUS

0	Success.

1	Fail.

2	Intervention required.

=for nhgitset nhgitset NetHack git helper installer
