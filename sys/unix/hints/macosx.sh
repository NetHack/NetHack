#!/bin/sh
# NetHack 3.6  macosx.sh $NHDT-Date: 1517231957 2018/01/29 13:19:17 $  $NHDT-Branch: githash $:$NHDT-Revision: 1.20 $
# Copyright (c) Kenneth Lorber, Kensington, Maryland, 2007.
# NetHack may be freely redistributed.  See license for details.
#
# hints helper script for macosx
# DO NOT invoke directly.

# Works for 10.4 and 10.5.  (The 10.5 support might work for 10.4 but keep
# both versions in case we need to support earlier versions.)

cmd=$1

case "x$cmd" in
xuser)
		# fail unless user exists (good through 10.4)
	user=$2
	gotuser=`niutil -readval . /users/$user name 0 2>/dev/null`
	[ -z $gotuser ] && (echo "User $user does not exist."
		exit 1;
	)
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
		# fail unless group exists (good through 10.4)
	group=$2
	gotgrp=`niutil -readval . /groups/$group name 0 2>/dev/null`
	[ -z $gotgrp ] && ( echo "Group $group does not exist."
		exit 1
	)
	;;

#niutil -read . /groups/bin name 0
#name: bin
#gid: 7
#passwd: *
#generateduid: ABCDEFAB-CDEF-ABCD-EFAB-CDEF00000007
#smb_sid: S-1-5-21-107
#realname: Binary

xuser2)
		# fail unless user exists (required as of 10.5)
	user=$2
	if( dscl localhost -read /Search/Users/$user 1>/dev/null 2>&1 );
	then
		true;
	else
		echo "User $user does not exist.";
		exit 1;
	fi
	;;

xgroup2)
		# if group does not exist, create it (required as of 10.5)
	group=$2
	[ -z $group ] && ( echo "No group specified."
		exit 1
	)
	if( dscl localhost -read /Search/Groups/$group 1>/dev/null 2>&1 );
	then
		true;
	else
		echo "Group $group does not exist - creating.";
		dseditgroup -o create -r "Games Group" -s 3600 $group
		if( dscl localhost -read /Search/Groups/$group 1>/dev/null 2>&1 );
		then
			true;
		else
			echo "Unable to create group $group."
			exit 1
		fi
	fi
	;;

xeditsysconf)
	src=$2
	dest=$3
	ptg=1
	# We don't need an LLDB module because any MacOSX new enough to
	# have no Apple supported gdb is also new enough to get good
	# stack traces through libc.
	# NB: xcrun will check $PATH
	if [[ -x /usr/bin/xcrun && `/usr/bin/xcrun -f gdb 2>/dev/null` ]] ; then
            gdbpath="GDBPATH="`/usr/bin/xcrun -f gdb`
	elif [ -f /usr/bin/gdb ]; then
	    gdbpath='GDBPATH=/usr/bin/gdb' #traditional location
	elif [ -f /opt/local/bin/ggdb ]; then
	    gdbpath='GDBPATH=/opt/local/bin/ggdb' #macports gdb
	elif [ -f /Developer/usr/bin/gdb ]; then
	    # this one seems to be broken with Xcode 5.1.1 on Mountain Lion
	    gdbpath='GDBPATH=/Developer/usr/bin/gdb' #older Xcode tools
	else
	    gdbpath='#GDBPATH' #none of the above
	    ptg=0
	fi
	if [ -f /bin/grep ]; then
	    greppath='GREPPATH=/bin/grep'
	elif [ -f /usr/bin/grep ]; then
	    greppath='GREPPATH=/usr/bin/grep'
	else
	    greppath='#GREPPATH'
	fi
	# PANICTRACE_GDB value should only be replaced if gdbpath is '#GDBPATH'
	if ! [ -e $dest ]; then
		sed -e "s:^GDBPATH=.*:$gdbpath:" \
		    -e "s:^GREPPATH=.*:$greppath:" \
		    -e "s/^PANICTRACE_GDB=./PANICTRACE_GDB=$ptg/" \
		    -e 's/^#OPTIONS=.*/&\
OPTIONS=!use_darkgray/' \
		    $src > $dest
	fi
	;;

#% dscl localhost -read /Search/Groups/wheel
# AppleMetaNodeLocation: /Local/Default
# GeneratedUID: ABCDEFAB-CDEF-ABCD-EFAB-CDEF00000000
# GroupMembership: root
# Password: *
# PrimaryGroupID: 0
# RealName:
#  System Group
# RecordName: wheel
# RecordType: dsRecTypeStandard:Groups
# SMBSID: S-1-5-21-100

xdescplist)	SVSDOT=`util/makedefs --svs .`
	cat <<E_O_M;
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
        <key>IFPkgDescriptionDeleteWarning</key>
        <string></string>
        <key>IFPkgDescriptionDescription</key>
        <string>NetHack $SVSDOT for the MacOS X Terminal
</string>
        <key>IFPkgDescriptionTitle</key>
        <string>NetHack</string>
        <key>IFPkgDescriptionVersion</key>
        <string>$SVSDOT</string>
</dict>
</plist>
E_O_M
	;;

xinfoplist)	SVSDOT=`util/makedefs --svs .`
	cat <<E_O_M;
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>CFBundleGetInfoString</key>
	<string>NetHack $SVSDOT for the MacOS X Terminal</string>
	<key>CFBundleIdentifier</key>
	<string>org.nethack.term</string>
	<key>CFBundleName</key>
	<string>NetHack</string>
	<key>CFBundleShortVersionString</key>
	<string>$SVSDOT</string>
	<key>IFMajorVersion</key>
	<integer>3</integer>
	<key>IFMinorVersion</key>
	<integer>3</integer>
	<key>IFPkgFlagAllowBackRev</key>
	<true/>
	<key>IFPkgFlagAuthorizationAction</key>
	<string>RootAuthorization</string>
	<key>IFPkgFlagDefaultLocation</key>
	<string>/</string>
	<key>IFPkgFlagInstallFat</key>
	<false/>
	<key>IFPkgFlagIsRequired</key>
	<false/>
	<key>IFPkgFlagOverwritePermissions</key>
	<true/>
	<key>IFPkgFlagRelocatable</key>
	<false/>
	<key>IFPkgFlagRestartAction</key>
	<string>NoRestart</string>
	<key>IFPkgFlagRootVolumeOnly</key>
	<false/>
	<key>IFPkgFlagUpdateInstalledLanguages</key>
	<false/>
	<key>IFPkgFlagUseUserMask</key>
	<false/>
	<key>IFPkgFormatVersion</key>
	<real>0.10000000149011612</real>
</dict>
</plist>
E_O_M
	;;

*)      echo "Unknown command $cmd"
	exit 1
        ;;
esac

# dscl localhost -read /Search/Users/games
# dsAttrTypeNative:_writers_hint: games
# dsAttrTypeNative:_writers_jpegphoto: games
# dsAttrTypeNative:_writers_LinkedIdentity: games
# dsAttrTypeNative:_writers_passwd: games
# dsAttrTypeNative:_writers_picture: games
# dsAttrTypeNative:_writers_realname: games
# dsAttrTypeNative:_writers_UserCertificate: games
# AppleMetaNodeLocation: /Local/Default
# AuthenticationAuthority: ;ShadowHash; ;Kerberosv5;;games@LKDC:SHA1.3F695B215C78511043D9787CA51DE92E6494A021;LKDC:SHA1.3F695B215C78511043D9787CA51DE92E6494A021;
# AuthenticationHint: games
# GeneratedUID: A727EFB1-D6AA-4FE2-8524-0E154890E9A9
# NFSHomeDirectory: /Users/games
# Password: ********
# Picture:
#  /Library/User Pictures/Flowers/Sunflower.tif
# PrimaryGroupID: 20
# RealName: games
# RecordName: games
# RecordType: dsRecTypeStandard:Users
# UniqueID: 505
# UserShell: /bin/bash

# see also: http://developer.apple.com/documentation/Porting/Conceptual/PortingUnix/additionalfeatures/chapter_10_section_9.html

# another mess: 10.4 creates a group for every user, 10.5 dumps you into staff.
# so I think we need to explicitly create group games in both (if it doesn't
# exist) and use owner bin (nope that fails since everything seems to be owned
# by root.  Do we want that?  How about just creating user games as well?)

# [Hermes:sys/unix/hints] keni% dscl localhost -read /Search/Users/games9
# <dscl_cmd> DS Error: -14136 (eDSRecordNotFound)
# [Hermes:sys/unix/hints] keni% dscl localhost -read /Search/Users/games9 >/dev/null
# <dscl_cmd> DS Error: -14136 (eDSRecordNotFound)
# [Hermes:sys/unix/hints] keni% dscl localhost -read /Search/Users/games > /dev/null
# status is good: 0 or 56

# file:///Developer/Documentation/DocSets/com.apple.ADC_Reference_Library.CoreReference.docset/Contents/Resources/Documents/releasenotes/MacOSXServer/RN-DirectoryServices/index.html
