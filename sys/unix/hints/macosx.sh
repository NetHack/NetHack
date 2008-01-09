#!/bin/sh
#      SCCS Id: @(#)macosx-mu.sh 3.5     2007/12/12
# Copyright (c) Kenneth Lorber, Kensington, Maryland, 2007.
# NetHack may be freely redistributed.  See license for details.
#
# hints helper script for macosx
# DO NOT invoke directly

cmd=$1

case "x$cmd" in
xuser)
	user=$2
	gotuser=`niutil -readval . /users/$user name 0 2>/dev/null`
	[ -z $gotuser ] && (echo "User $user does not exist."
		exit 1;
	)
	exit 0
	;;
#name: dummy1
#_writers_passwd: dummy1
#_writers_tim_password: dummy1
#_writers_picture: dummy1
#home: /Users/dummy1
#gid: 504
#picture: /Library/User Pictures/Animals/Dragonfly.tif
#uid: 504
#hint: dummy1
#_writers_hint: dummy1
#sharedDir: 
#_shadow_passwd: 
#_writers_realname: dummy1
#shell: /bin/bash
#passwd: ********
#authentication_authority: ;ShadowHash;
#realname: dummyname1
#generateduid: F6D4991C-BDF5-481F-A407-D84C6A2D0E2A
xgroup)
	group=$2
	gotgrp=`niutil -readval . /groups/$group name 0 2>/dev/null`
	[ -z $gotgrp ] && ( echo "Group $group does not exist."
		exit 1
	)
	exit 0
	;;
#niutil -read . /groups/bin name 0
#name: bin
#gid: 7
#passwd: *
#generateduid: ABCDEFAB-CDEF-ABCD-EFAB-CDEF00000007
#smb_sid: S-1-5-21-107
#realname: Binary

*)      echo "Unknown command $cmd"
	exit 1
        ;;
esac
