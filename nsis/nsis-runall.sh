#!/bin/bash
#
# this script must be run from the woo/nsis directory
#
set -e -x
# must be absolute path where pyinstaller's-generated executable is
BINDIR=/media/eudoxos/WIN7/src/woo/dist/wwoo-win64
REVNO=`bzr revno ..`
DESTDIR=$BINDIR/..

cp *.nsh $BINDIR/
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
pushd $BINDIR
	makensis -DVERSION=1.0 nsis-wwoo-libs.nsh 
	makensis -DVERSION=0.99-$REVNO nsis-wwoo-main.nsh 
	# make installers for extra modules
	for EGG in wooExtra.*.egg; do
		COMPONENT=`echo $EGG | cut -f1 -d-`
		VERSION=`echo $EGG | cut -f2 -d-`
		makensis -DCOMPONENT=$COMPONENT -DVERSION=$VERSION nsis-wwoo-extra.nsh
	done
popd

mv $BINDIR/*-installer.exe $DESTDIR
