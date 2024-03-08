$ ! vmssetup.com -- place GNU Makefiles in their target directories
$ version_number = "3.7.0"
$ ! $NHDT-Date: 1687541093 2024/03/08 17:24:53 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.39 $
$ ! NetHack may be freely redistributed.  See license for details.
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
