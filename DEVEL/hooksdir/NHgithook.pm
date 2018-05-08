#
# NetHack 3.6  NHgithook.pm       $NHDT-Date: 1524689631 2018/04/25 20:53:51 $  $NHDT-Branch: NetHack-3.6.0 $:$NHDT-Revision: 1.6 $
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
if ($^O eq "MSWin32")
{
    $DS = quotemeta('\\');
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

# XXX this needs a re-write (don't tie and untie, just set NEXT=0)
#  (the sensitive thing is @foo = <STDIN> )
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
### store githash and gitbranch in dat/gitinfo.txt
###

sub nhversioning {
    use strict;
    use warnings;

    my $git_sha = `git rev-parse HEAD`;
    $git_sha =~ s/\s+//g;
    my $git_branch = `git rev-parse --abbrev-ref HEAD`;
    $git_branch =~ s/\s+//g;
    die "git rev-parse failed" unless(length $git_sha and length $git_branch);
    my $exists = 0;

    if (open my $fh, '<', 'dat/gitinfo.txt') {
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
            print "dat/gitinfo.txt unchanged, githash=".$git_sha."\n";
            return;
        }
    } else {
	print "WARNING: Can't find dat directory\n" unless(-d "dat");
    }
    if (open my $fh, '>', 'dat/gitinfo.txt') {
        my $how = ($exists ? "updated" : "created");
        print $fh 'githash='.$git_sha."\n";
        print $fh 'gitbranch='.$git_branch."\n";
        print "dat/gitinfo.txt ".$how.", githash=".$git_sha."\n";
    } else {
	print "WARNING: Unable to open dat/gitinfo.txt: $!\n";
    }
}

# PRIVATE
sub do_hook {
	my($p) = @_;
	my $hname = $0;
	$hname =~ s!^((.*$DS)|())(.*)!$1$p-$4!;
	if(-x $hname){
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
                # XXX yuck
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

=head1 DESCRIPTION

Buffers call information so multiple independent actions may be coded for
Git hooks and similar Git callouts.

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

=head1 AUTHOR

Kenneth Lorber (keni@his.com)
