python /c/src/pyinstaller-develop/pyinstaller.py -y woo.pyinstaller.spec
REVNO=`bzr revno . 2>/dev/null`
ZIP7="/c/Program Files/7-Zip/7z.exe"
# insane compression on 6 cores
ZIP7FLAGS="-m0=lzma2 -mmt=6"
set -e -x

pushd dist
	DDIR=wwoo-win64 # frozen directory
	#zip wwoo-r$REVNO-win64.zip -9 -i $DDIR/wwoo* $DDIR/woo*
	"$ZIP7" a wwoo-r$REVNO-win64.7z $DDIR/wwoo* $DDIR/woo* $ZIP7FLAGS

	# new attempt: when number is bumped manually, run compression again
	LIBVER=1
	LIBS="wwoo-libs-v$LIBVER-win64.7z"
	[ -f $LIBS ] || "$ZIP7" a $LIBS $DDIR "-x!$DDIR\\wwoo*" "-x!$DDIR\\woo*" $ZIP7FLAGS
		
	# old attempt: compress and binary-compare
	# this fails due to file modifications times
	# being different between installations
	if false; then
		LIBTMP=wwoo-libs-vTMP-win64.7z
		"$ZIP7" a $LIBTMP $DDIR "-x!$DDIR\\wwoo*" "-x!$DDIR\\woo*" $ZIP7FLAGS
		if ! diff -q --binary $LIBS $LIBTMP 2> /dev/null; then
			LIBVER2=$(( $LIBVER + 1 ))
			LIB2="wwoo-libs-v${LIBVER2}-win64.7z"
			echo "Library archives are different. Renaming $LIBTMP to $LIB2."
			mv $LIBTMP $LIB2
			echo "DO NOT FORGET TO SAY LIBVER=$LIBVER2 IN freeze.sh !"
		fi
	fi
popd


