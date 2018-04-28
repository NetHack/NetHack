#!/usr/bin/perl
# $NHDT-Date: 1524689669 2018/04/25 20:54:29 $ $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.2 $
# Copyright (c) 2015 by Kenneth Lorber, Kensington, Maryland
# NetHack may be freely redistributed.  See license for details.

# value of nethack.setupversion we will end up with when this is done
# version 1 is reserved for repos checked out before versioning was added
my $version_new = 3;
my $version_old = 0;	# current version, if any (0 is no entry ergo new repo)

use Cwd;
use Getopt::Std;

# Activestate Perl doesn't include File::Spec.  Grr.
BEGIN {
        eval "require File::Spec::Functions";
        if($@){
                die <<E_O_M;
File::Spec not found.  (If you are running ActiveState Perl please run:
                cpan File::Spec
and re-run this program.
E_O_M
        }
        File::Spec::Functions->import;
}

exit 1 unless(getopts('nvf'));	# TODO: this can probably have better output

# OS hackery
my $DS = quotemeta('/');		# Directory Separator (for regex)
my $DSP = '/';				# ... for printing
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
	$DSP = '\\';
}

# make sure we're at the top level of a repo
if(! -d ".git"){
	die "This is not the top level of a git repository.\n";
}

my $vtemp = `git config --local --get nethack.setupversion`;
chomp($vtemp);
if($vtemp > 0){
	$version_old = 0+$vtemp;
	if($version_old != $version_new){
		print STDERR "Migrating from setup version $version_old to $version_new\n" if($opt_v);
	}
}
# legacy check:
if(length $vtemp == 0){
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

if(! -f catfile($srcdir, 'nhgitset.pl')){
	die "I can't find myself in '$srcdir'\n";
}

print STDERR "Copying from: $srcdir\n" if($opt_v);

if($opt_f || $version_old==0){
	print STDERR "Configuring line endings\n" if($opt_v);
	unlink catfile('.git','index') unless($opt_n);
	system("git reset") unless($opt_n);
	system("git config --local core.safecrlf true") unless($opt_n);
	system("git config --local core.autocrlf false") unless($opt_n);
} elsif($version_old <2){
	my $xx = `git config --get --local core.safecrlf`;
	if($xx !~ m/true/){
		print STDERR "\nNeed to 'rm .git${DSP}index;git reset'.\n";
		print STDERR " When ready to proceed, re-run with -f flag.\n";
		exit 2;
	}
}



print STDERR "Installing aliases\n" if($opt_v);
$addpath = catfile(curdir(),'.git','hooks','NHadd');
&add_alias('nhadd', "!$addpath add");
&add_alias('nhcommit', "!$addpath commit");
my $nhsub = catfile(curdir(),'.git','hooks','nhsub');
&add_alias('nhsub', "!$nhsub");

print STDERR "Installing filter/merge\n" if($opt_v);

# XXXX need it in NHadd to find nhsub???
# removed at version 3
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

$cmd = catfile(curdir(),'.git','hooks','NHsubst');
&add_config('merge.NHsubst.name', 'NetHack Keyword Substitution');
&add_config('merge.NHsubst.driver', "$cmd %O %A %B %L");

print STDERR "Running directories\n" if($opt_v);

foreach my $dir ( glob("$srcdir$DS*") ){
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

&check_prefix;	# for variable substitution

if($version_old != $version_new){
	print STDERR "Setting version to $version_new\n" if($opt_v);
	if(! $opt_n){
		system("git config nethack.setupversion $version_new");
		if($?){
			die "Can't set nethack.setupversion $version_new: $?,$!\n";
		}
	}
}

exit 0;

sub process_override {
	my($srcdir, $plname) = @_;
	return 0 unless(length $gitadddir);

	my $plpath = catfile($gitadddir, $srcdir, $plname);
#print STDERR "   ",catfile($srcdir, $plname),"\n"; # save this for updating docs - list of overrides
	return 0 unless(-x $plpath);

	print STDERR "Running $plpath\n" if($opt_v);
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

sub check_prefix {
	my $lcl = `git config --local --get nethack.substprefix`;
	chomp($lcl);
	if(0==length $lcl){
		my $other = `git config --get nethack.substprefix`;
		chomp($other);
		if(0==length $other){
			print STDERR "ERROR: nethack.substprefix is not set anywhere.  Set it and re-run.\n";
			exit 2;
		} else {
			&add_config('nethack.substprefix', $other);
			print STDERR "Copying prefix '$other' to local repository.\n" if($opt_v);
		}
		$lcl = $other;	# for display below
	}
	print "\n\nUsing prefix '$lcl' - PLEASE MAKE SURE THIS IS CORRECT\n\n";
}

sub do_dir_DOTGIT {
if(1){
	# We are NOT going to mess with config now.
	return;
} else {
	my($srcdir, $targetdir) = @_;
#warn "do_dir_DOTGIT($srcdir, $targetdir)\n";
	my $cname = "$srcdir/config";
	if(-e $cname){
		print STDERR "Appending to .git/config\n" if($opt_v);
		open CONFIG, ">>.git/config" or die "open .git/config: $!";
		open IN, "<", $cname or die "open $cname: $!";
		my @data = <IN>;
		print CONFIG @data;
		close IN;
		close CONFIG;
	} else {
		print STDERR " Nothing to add to .git/config\n" if($opt_v);
	}
# XXX are there other files in .git that we might want to handle?
# So just in case:
	for my $file ( glob("$srcdir/*") ){
		next if( $file =~ m!.*/TARGET$! );
		next if( $file =~ m!.*/config$! );
		die "ERROR: no handler for $file\n";
	}
}
}

sub do_dir_hooksdir {
	my($srcdir, $targetdir) = @_;

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

__END__
(can we change the .gitattributes syntax to include a comment character?)
maybe [comment]  attr.c:parse_attr_line
grr - looks like # is the comment character



=head1 NAME

nhgitset.pl - Setup program for NetHack git repositories

=head1 SYNOPSIS

 cd THE_REPO
 [git config nethack.gitadddir GITADDDIR]
 perl SOME_PATH/DEVEL/nhgitset.pl [-v][-n][-f]

=head1 DESCRIPTION

nhgitset.pl installs NetHack-specific setup after a C<git clone> (or after
changes to the desired configuration, which are installed by re-running
nhgitset.pl).

The follwing options are available:

B<-f>	Force.  Do not use this unless the program requests it.

B<-n>	Make no changes.

B<-v>	Verbose output.

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

nethack.substprefix


=head1 EXIT STATUS

0	Success.

1	Fail.

2	Intervention required.
