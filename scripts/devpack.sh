#!/bin/sh
# wrap up "development" files using the same method as for the official
# distribution files

douu=0

COMPRESS=compress
TZINT=taz
TZEXT=tar.Z
TZUEXT=tzu

while [ $# -gt 0 ] ; do
    case X$1 in
	X-x)	echo "usage $0 [-u|-gz|-x]"
		echo "-u    uuencode output."
		echo "-gz   use gzip as compress utility (default, compress)."
		echo "-x    display this text."
		exit ;;
	X-u)	douu=1 ;;
	X-gz)	COMPRESS=gzip
		TZINT=tgz
		TZEXT=tar.gz
		TZUEXT=tgu ;;
	X*)	echo "usage $0 [-u|-gz|-x]" ; exit ;;
    esac
    shift
done
 
PACKDIR=packages
 
if [ ! -d ${PACKDIR} ]
then
        mkdir ${PACKDIR}
fi
 
# wrap up files from top directory
DEV1="doc/archives doc/direct.tre doc/fixes31.1 doc/fixes31.2 doc/fixes31.3 doc/fixes32.0 doc/fixes32.1 doc/fixes32.2 doc/fixes33.0 doc/lists"
DEV2="doc/mythos.doc doc/style.doc util/heaputil.c sys/msdos/viewtib.c sys/msdos/def2mak.c"
DEV3="sys/msdos/compwarn.lst sys/msdos/genschem.l sys/msdos/prebuild.mak sys/msdos/schema1 sys/msdos/schema2 sys/msdos/schema3 sys/msdos/template.mak"
DEV4="doc/buglist sys/share/flexhack.skl sys/amiga/amibug sys/amiga/ship/README.shp sys/amiga/ship/cmove sys/amiga/ship/makescript sys/amiga/ship/metareadme sys/amiga/ship/mkdz.awk sys/amiga/ship/mkfd.awk sys/amiga/ship/shiplist sys/amiga/ship/strip"

( tar cvf dev1.tar $DEV1 ; tar cvf dev2.tar $DEV2 )
( tar cvf dev3.tar $DEV3 ; tar cvf dev4.tar $DEV4 )
for i in dev1 dev2 dev3 dev4
do
        $COMPRESS $i.tar
        if [ $douu -gt 0 ]
        then
                uuencode $i.${TZEXT} $i.${TZINT} >${PACKDIR}/$i.${TZUEXT}
                rm $i.${TZEXT}
        else
                mv $i.${TZEXT} ${PACKDIR}/$i.${TZINT}
        fi
done

cp scripts/pack.sh scripts/unpack.sh ${PACKDIR}
cp scripts/msunpack.bat scripts/vmsunpack.com ${PACKDIR}
