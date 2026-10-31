$ ! $NHDT-Header: NetHack 5.0 vmssetup.com 2026/09/25_13:07:53 NetHack-5.0:1.3 $
$ ! NetHack may be freely redistributed.  See license for details.
$ ! Copyright (c) Michael Allison, 2024
$ ! vmssetup.com -- place GNU Makefiles in their target directories
$ version_number = "5.0.1"
$ !
$ relsrc = ""
$ reldst = "-.-"
$ if (f$search("[]dat.dir") .nes. "") .AND. -
     (f$search("[]src.dir") .nes. "") .AND. -
     (f$search("[]include.dir") .nes. "") .AND. -
     (f$search("[]sys.dir") .nes. "") then -
    relsrc = ".sys.vms"
$ if (f$search("[]dat.dir") .nes. "") .AND. -
     (f$search("[]src.dir") .nes. "") .AND. -
     (f$search("[]include.dir") .nes. "") .AND. -
     (f$search("[]sys.dir") .nes. "") then -
    reldst = ""
$ copy ['relsrc']Makefile_top.vms ['reldst']Makefile.vms
$ copy ['relsrc']Makefile_dat.vms ['reldst'.dat]Makefile.vms
$ copy ['relsrc']Makefile_doc.vms ['reldst'.doc]Makefile.vms
$ copy ['relsrc']Makefile_src.vms ['reldst'.src]Makefile.vms
$ copy ['relsrc']Makefile_utl.vms ['reldst'.util]Makefile.vms
$!
$done:
$ exit
