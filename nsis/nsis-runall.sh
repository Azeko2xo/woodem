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

REVNO=`bzr revno ..`
DESTDIR=$BINDIR/..

cp *.nsh $BINDIR/
cp *.rtf $BINDIR/
rm -f $BINDIR/*-installer.exe
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

if [ -d /boot ]; then
	# linux
	pushd $BINDIR
		makensis -DVERSION=1.0a nsis-wwoo-libs.nsh 
		makensis -DVERSION=0.99-$REVNO nsis-wwoo-main.nsh 
		# make installers for extra modules
		for EGG in wooExtra.*.egg; do
			COMPONENT=`echo $EGG | cut -f1 -d-`
			VERSION=`echo $EGG | cut -f2 -d-`
			makensis -DCOMPONENT=$COMPONENT -DVERSION=$VERSION nsis-wwoo-extra.nsh
		done
	popd
else
	# windows is so singularly ugly
	MAKENSIS='/c/Program Files (x86)/NSIS/makensis.exe'
	pushd $BINDIR
		# work around msys expanding /D: http://forums.winamp.com/showthread.php?t=253732
		# omg
		echo "!define VERSION 1.0a" > defines.nsh
		"$MAKENSIS" defines.nsh nsis-wwoo-libs.nsh
		echo "!define VERSION 0.99-$REVNO" > defines.nsh
		"$MAKENSIS" defines.nsh nsis-wwoo-main.nsh
		for EGG in wooExtra.*.egg; do
			COMPONENT=`echo $EGG | cut -f1 -d-`
			VERSION=`echo $EGG | cut -f2 -d-`
			echo "!define COMPONENT $COMPONENT" > defines.nsh
			echo "!define VERSION $VERSION" >>defines.nsh
			"$MAKENSIS" defines.nsh nsis-wwoo-extra.nsh
		done
	popd
fi


mv $BINDIR/*-installer.exe $DESTDIR
