#
# NetHack 3.7  NHgithook.pm       $NHDT-Date: 1596498406 2020/08/03 23:46:46 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.7 $
# Copyright (c) 2015 by Kenneth Lorber, Kensington, Maryland
# NetHack may be freely redistributed.  See license for details.

# NetHack Git Hook Module

package NHgithook;
use Cwd;

###
### CONFIG
###
my $trace = 0;
my $tracefile = "/tmp/nhgitt.$$";

# OS hackery
my $DS = quotemeta('/');
my $PDS = '/';
if ($^O eq "MSWin32")
{
    $DS = quotemeta('\\');
    $PDS = '\\';
}

our %saved_env;
our @saved_argv;
our $saved_input;

sub saveSTDIN {
    @saved_input = <STDIN>;

    if($trace){
	print TRACE "STDIN:\n";
	print TRACE $saved_input;
	print TRACE "ENDSTDIN\n";
    }

    tie *STDIN, 'NHIO::STDIN', @saved_input;
}

sub resetSTDIN{
    my $x = tied(*STDIN);
    my %x = %$x;
    my $data = @$x{DATA};
    untie *STDIN;
    tie *STDIN, 'NHIO::STDIN', $data;
}

# don't need this now
#sub restore {
#	open STDIN, "<", \$saved_input or die "reopen STDIN: $!";
#	@ARGV = @saved_argv;
#	%ENV = %saved_env;
#}

sub PRE {
    &do_hook("PRE");
}

sub POST {
    &do_hook("POST");
}

###
### versioning for nhgitset and friends
###

# values of nethack.setupversion and DEVEL/VERSION:
# 1 is reserved for repos checked out before versioning was added
# 2 used clean/smudge filter, poorly
# 3 was first production version
# 4 added the version file and version checking; nhhelp, NH_DATESUB support, etc.

sub version_in_devel {
	# (1) check for a non-null nethack.setuppath - this handles
	# any repo that has already been set up (but NOT checking
	# out <v4 over >=v4 since nethack.setuppath will exist but
	# DEVEL/VERSION will not).
	# XXX if the source repo has been removed, we'll fall back to
	#  the third case - hopefully that's ok.
	# XXX there's no way to recover from a missing source repo
	#  without editing .git/config.
    my $path = `git config --local nethack.setuppath`;
    chomp $path;
    $path =~ s/DEVEL$//;    # NOP if config not set

	# (2) else check the local directory; that will be correct for NHsource.
    if(0 == length $path){
	$path = `git rev-parse --show-toplevel`;
	chomp $path;
	$path = '' unless(-d "$path${PDS}DEVEL");
    }
	# (3) If that doesn't exist, check using the invocation path; that will be
	# correct for other repos during nhgitset (but will also fail for
	#  checking out 3 over 4).
    if(0 == length $path){
	    # strip out "DEVEL"
	$path = ($0 =~ m!^(.*)${PDS}DEVEL${PDS}.*?(*nla:DEVEL)!)[0];
    }
	# Uh oh?
    if(0==length($path) or (! -d "$path${PDS}DEVEL")){
	die "Can't locate DEVEL directory in '$path'.";
    }

	# Handle checking out version <4 over version >=4.  If
	# this seems to be the situation, don't revert the code.
    return 0 if(! -f "$path${PDS}DEVEL${PDS}VERSION");

    my $version;
    my $verfile = "$path${PDS}DEVEL${PDS}VERSION";
    open VERFH,"<",$verfile or die "Can't open $verfile: $!";
    $version = 0+<VERFH>;
    my $message = join('',<VERFH>);
    close VERFH;
    die "Valid version not found in $verfile" unless($version >= 4);
    return ($version,$message) if($version > 0);
    return 0;
}

sub version_in_git {
    my $vtemp = `git config --local --get nethack.setupversion`;
    chomp($vtemp);
    return $vtemp if($vtemp > 0);
    return 0;
}

sub version_set_git {
    my $version_new = $_[0];

    system("git config nethack.setupversion $version_new");
    if($?){
	die "Can't set nethack.setupversion $version_new: $?,$!\n";
    }
}

###
### store githash and gitbranch in dat/gitinfo.txt
###
# CAUTION!  This is run not just from git hooks, but also from
#           sys/unix/gitinfo.sh

sub nhversioning {
    use strict;
    use warnings;

	# See if we're (probably) in a "git pull", in which case we need to
	# check for upgrades.
    my $check_upgrade = 1 if($_[0]);

	# Check for pre-v4 source repo.
    my $is_sourcerepo;
    {
	chomp($is_sourcerepo = `git config --int --get nethack.is-sourcerepo`);
	if(0 == length $is_sourcerepo){	# not set - assume old repo
	    $is_sourcerepo = 1;
	}elsif($is_sourcerepo==1){
	    ;
	}elsif($is_sourcerepo==0){
	    ;
	}
    }

	    # Skip the skipping tests if we're being called directly.
	    # NB: post-commit has no args, but that will be caught by
	    #  the next test for non-source repos.
    if($#ARGV != -1){
	    # Skip this if we didn't change branches, but see if we need to warn.
	if(defined($ARGV[2]) and ($ARGV[2] == 0)){
	    # Because we can create an out of sync state, possibly warn.
	    my $ref = $ARGV[1];
	    if($is_sourcerepo and (0 != 0+`git diff --name-only $ref $ref^ |grep ^DEVEL|wc -l`)){
		warn "WARNING: DEVEL directory changed.  Versioning may be inconsistent\n";
	    }
	    return
	}
    }

    if($check_upgrade){
	my $current_version = version_in_git();
	my($new_version,$message) = version_in_devel();
	if($new_version > $current_version){
	    warn "nhgitset.pl and/or related programs have changed.\n";
	    warn "Please re-run nhgitset.pl to update from version $current_version to $new_version.\n";
	    if(length $message){
		warn "Additional information\n$message\n";
	    }
	}
    }

	# Skip versioning if we aren't in a source repo.
    return if(0==$is_sourcerepo);

    my $git_sha = `git rev-parse HEAD`;
    $git_sha =~ s/\s+//g;
    my $git_branch = `git rev-parse --abbrev-ref HEAD`;
    $git_branch =~ s/\s+//g;
    die "git rev-parse failed" unless(length $git_sha and length $git_branch);
    my $exists = 0;

    no strict 'refs';
    no strict 'subs';
    my $file_gitinfo = "dat${PDS}gitinfo.txt";
    my $file_gittemp = "dat${PDS}TMPgitinfo.txt";
    use strict 'subs';
    use strict 'refs';

    if (open my $fh, '<', $file_gitinfo) {
        $exists = 1;
        my $hashok = 0;
        my $branchok = 0;
        while (my $line = <$fh>) {
            if ((index $line, $git_sha) >= 0) {
                $hashok++;
            }
            if ((index $line, $git_branch) >= 0) {
                $branchok++;
            }
        }
        close $fh;
        if ($hashok && $branchok) {
            print "$file_gitinfo unchanged, githash=".$git_sha."\n";
            return;
        }
    } else {
	warn "WARNING: Can't find dat directory\n" unless(-d "dat");
	return;
    }
    if (open my $fh, '>', $file_gittemp) {
        my $how = ($exists ? "updated" : "created");
        print $fh 'githash='.$git_sha."\n";
        print $fh 'gitbranch='.$git_branch."\n";
        print "$file_gitinfo ".$how.", githash=".$git_sha."\n";
	if(close($fh)){
	    if(rename($file_gittemp, $file_gitinfo)){
		;   # all ok
	    } else {
		warn "WARNING: Can't rename $file_gittemp -> $file_gitinfo";
	    }
	} else {
	    warn "WARNING: Can't close temp file: $!";
	}
    } else {
	warn "WARNING: Unable to open $file_gitinfo: $!\n";
    }
}

# PRIVATE
sub do_hook {
    my($p) = @_;
    my $hname = $0;
    $hname =~ s!^((.*$DS)|())(.*)!$1$p-$4!;
    my $vig = version_in_git;
    if(-x $hname){
	if(`git config --bool nethack.NoDepWarn` ne "true\n"){
	    print STDERR <<~E_O_M;
                WARNING: $p hooks will be going away.  See
                         https://www.nethack.org/devel/deprecation.html
                To stop seeing this message, run
                    git config --bool nethack.NoDepWarn true
                E_O_M
	}

	print TRACE "START $p: $hname\n" if($trace);

	open TOHOOK, "|-", $hname or die "open $hname: $!";
	print TOHOOK <STDIN>;
	close TOHOOK or die "close $hname: $! $?";

	print TRACE "END $p\n" if($trace);
    }
}

sub trace_start {
    return unless($trace);
    my $self = shift;
    open TRACE, ">>", $tracefile;
    print TRACE "START CLIENT PID:$$ ARGV:\n";
    print TRACE "CWD: " . cwd() . "\n";
    print TRACE "[0] $0\n";
    my $x1;
    for(my $x=0;$x<scalar @ARGV;$x++){
	$x1 = $x+1;
	print TRACE "[$x1] $ARGV[$x]\n";
    }
    print TRACE "ENV:\n";
    foreach my $k (sort keys %ENV){
	next unless ($k =~ m/(^GIT_)|(^NH)/);
	print TRACE " $k => $ENV{$k}\n";
    }
}

BEGIN {
    %saved_env = %ENV;
    @saved_argv = @ARGV;
    &trace_start;
}

###
### ugly mess so we can re-read STDIN
###
package NHIO::STDIN;
sub TIEHANDLE {
    my $class = shift;
    my %fh;
    if(ref @_[0]){
	$fh{DATA} = @_[0];
    } else {
	$fh{DATA} = \@_;
    }   
    $fh{NEXT} = 0;
    return bless \%fh, $class;
}

sub READLINE {
    my $self = shift;
    return undef if($self->{EOF});
    if(wantarray){
	my $lim = $#{$self->{DATA}};
	my @ary = @{$self->{DATA}}[$self->{NEXT}..$lim];
	my @rv = @ary[$self->{NEXT}..$#ary];
	$self->{EOF} = 1;
	return @rv;
    } else{
	my $rv = $self->{DATA}[$self->{NEXT}];
	if(length $rv){
	    $self->{NEXT}++;
	    return $rv;
	} else {
	    $self->{EOF} = 1;
	    return undef;
	}   
    }   
}

sub EOF {
    $self = shift;
    return $self->{EOF};
}

1;
__END__

=head1 NAME

NHgithook - common code for NetHack git hooks (and other git bits)

=head1 SYNOPSIS

  BEGIN {
        my $DS = quotemeta('/');
	my $PDS = '/';
        if ($^O eq "MSWin32")
        {
            $DS = quotemeta('\\');
	    $PDS = '\\';
        }

        push(@INC, $ENV{GIT_DIR}.$PDS."hooks");    # for most hooks
        push(@INC, ($0 =~ m!^(.*)$DS!)[0]);	  # when the above doesn't work

        $gitdir = `git rev-parse --git-dir`;      # and when the above really doesn't work
        $gitdir =~ s/[\r\n]*$/;
        push(@INC, $gitdir.$PDS."hooks");
  }
  use NHgithook;
  
  &NHgithook::saveSTDIN;
  &NHgithook::PRE;
  (core hook code)
  &NHgithook::POST;

__END__
=for nhgitset NHgithook Infrastructure for NetHack git hooks.

=head1 DESCRIPTION

Perl module for infrastructure of NetHack Git hooks.

Buffers call information so multiple independent actions may be coded for
Git hooks and similar Git callouts.

=over 4

=item ==>

As of Git 2.54, multiple actions may be specified for each hook
event - that makes this redundant for hooks and will be removed at
some future time when Git 2.54 (or later) is better distributed.
This should speed up hook processing.

=back 4

Maintains C<dat/gitinfo.txt>.

Common routines for dealing with nethack.setupversion git config variable.

=head1 SETUP

Changing the C<$trace> and C<$tracefile> variables requires editing the
module source.  Setting C<$trace> enables tracing, logs basic information,
and leaves the C<TRACE> filehandle open for additional output; output to this
filehandle must be guarded by C<$NHgithook::trace>.  Setting
C<$tracefile> specifies the file used for trace output.  Note that C<$$>
may be useful since multiple processes may be live at the same time.

=head1 FUNCTIONS

  NHgithook::saveSTDIN  reads STDIN until EOF and saves it
  NHgithook::PRE 	runs the PRE hook, if it exists
  NHgithook::POST 	runs the POST hook, if it exists

=head1 BUGS

Some features not well tested, especially under Windows.

Not well documented, but almost no one needs to change (or even call)
this code.

=head1 AUTHOR

Kenneth Lorber (keni@his.com)
