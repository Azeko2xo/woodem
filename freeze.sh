#python /c/src/pyinstaller-develop/pyinstaller.py -y woo.pyinstaller.spec
REVNO=`bzr revno . 2>/dev/null`
DDIR=dist/wwoo-win64 # frozen directory
zip wwoo-r$REVNO-win64.zip -9 $DDIR/wwoo* $DDIR/woo*

LIBVER=1
LIB=wwoo-libs-v$LIBVER-win64.zip
LIBTMP=wwoo-libs-vTMP-win64.zip
zip $LIBTMP -9 $DDIR/* -x $DDIR/wwoo* $DDIR/woo*
if ! diff -q -- binary $LIB $LIBTMP > /dev/null; then
	echo "Library archives are different. Rename $LIBTMP to wwoo-libs-v$$LIBVER-win64.zip and bump LIBVER in freeze.sh"
fi


