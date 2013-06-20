#!/bin/bash
#
# this script must be run from the woo/nsis directory
#
set -e -x
# must be absolute path where pyinstaller's-generated executable is
if [ -d /boot ]; then
	# linux
	BINDIR=/media/eudoxos/WIN7/src/woo/dist/wwoo-win64
else
	# windows
	BINDIR=/c/src/woo/dist/wwoo-win64
fi

DO_LIBS=false
DO_WOO=false
DO_EXTRA=false
DO_UPLOAD=false

while getopts ":lweu" opt; do
	case $opt in
		l) DO_LIBS=true ;;
		w) DO_WOO=true ;;
		e) DO_EXTRA=true ;;
		u) DO_UPLOAD=true ;;
   esac
done


REVNO=`bzr revno ..`
DESTDIR=$BINDIR/..

cp *.nsh $BINDIR/
cp *.rtf $BINDIR/
rm -f $BINDIR/*-installer.exe

if $DO_EXTRA; then
	# copy eggs to $BINDIR; they are not installed with other installers, so it is safe to have them there
	rm -f $BINDIR/wooExtra*.egg
	for d in ../wooExtra/*; do
		echo $d
		[ -f $d/setup.py ] || continue
		pushd $d
			rm -rf dist/*.egg
			python setup.py bdist_egg
			cp dist/*.egg $BINDIR
		popd
	done
fi

echo 'HERE'

if [ -d /boot ]; then
	# linux
	pushd $BINDIR
		if $DO_LIBS; then makensis -DVERSION=1.0c nsis-wwoo-libs.nsh; fi
		if $DO_WOO; then makensis -DVERSION=0.99-r$REVNO nsis-wwoo-main.nsh; fi
		# make installers for extra modules
		if $DO_EXTRA; then
			for EGG in wooExtra.*.egg; do
				COMPONENT=`echo $EGG | cut -f1 -d-`
				VERSION=`echo $EGG | cut -f2 -d-`
				makensis -DCOMPONENT=$COMPONENT -DVERSION=$VERSION nsis-wwoo-extra.nsh
				if $DO_UPLOAD; then
					scp $EGG Woo-$COMPONENT-*-installer.exe bbeta:host/woodem/private/`python -c "import $COMPONENT; print $COMPONENT.KEY"`/inst/
				fi
			done
		fi
	popd
else
	# windows is so singularly ugly
	MAKENSIS='/c/Program Files (x86)/NSIS/makensis.exe'
	pushd $BINDIR
		# work around msys expanding /D: http://forums.winamp.com/showthread.php?t=253732
		# omg
		if $DO_LIBS; then
			echo "!define VERSION 1.0c" > defines.nsh
			"$MAKENSIS" defines.nsh nsis-wwoo-libs.nsh
		fi
		if $DO_WOO; then
			echo "!define VERSION 0.99-r$REVNO" > defines.nsh
			"$MAKENSIS" defines.nsh nsis-wwoo-main.nsh
		fi
		if $DO_EXTRA; then
			for EGG in wooExtra.*.egg; do
				COMPONENT=`echo $EGG | cut -f1 -d-`
				VERSION=`echo $EGG | cut -f2 -d-`
				echo "!define COMPONENT $COMPONENT" > defines.nsh
				echo "!define VERSION $VERSION" >>defines.nsh
				"$MAKENSIS" defines.nsh nsis-wwoo-extra.nsh
			done
		fi
	popd
fi

mv $BINDIR/*-installer.exe $DESTDIR
