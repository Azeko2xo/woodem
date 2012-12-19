#!/bin/bash
set -e -x
echo "*** This script sets up environment with Woo. Read the script before continuing. ***"
echo
echo "    1. Required packages will be installed"
echo "    2. /usr/local is chown'ed to the current user so that we can install there without sudo"
echo "    3. ~/woo is created/updated to the latest upstream source"
echo "    4. ~/woo/wooExtra is populated with extra modules, which are given as arguments to the script"
echo "    5. Woo is compiled and installed."
echo
echo "*** You will be asked for root password several times. That is normal. ***"

sudo apt-get install software-properties-common python-software-properties # for add-apt-repository
sudo add-apt-repository ppa:eudoxos/minieigen
sudo apt-get update

sudo chown -R $USER: /usr/local

WOODEPS="python-setuptools python-all-dev pyqt4-dev-tools libqt4-dev qt4-dev-tools libgle3-dev libqglviewer-qt4-dev libvtk5-dev libgts-dev libeigen3-dev freeglut3-dev python-xlrd python-xlwt python-numpy python-matplotlib python-qt4 python-xlib python-genshi python-psutil bzr ccache scons ipython"

# extra packages are not in 12.04, but are in later releases
if [[ "`cat /etc/issue`" != *12.04* ]]; then
	sudo apt-get install $WOODEPS libboost-all-dev python-colorama libqt4-dev-bin
else
	# force boost 1.48, not the default 1.46
	sudo apt-get install $WOODEPS libboost-date-time1.48-dev libboost-filesystem1.48-dev libboost-iostreams1.48-dev libboost-python1.48-dev libboost-regex1.48-dev libboost-chrono1.48-dev libboost-serialization1.48-dev libboost-system1.48-dev libboost-thread1.48-dev 
	# easy_install is in python-setuptools, which we just installed
	easy_install colorama
fi


if [ ! -d ~/woo/.bzr ]; then
	rm -rf ~/woo # in case it contains some garbage
	bzr co http://bazaar.launchpad.net/~eudoxos/woo/trunk ~/woo
else
	bzr up ~/woo
fi

# grab extra modules
for KEY in "$@"; do
	DEST=~/woo/wooExtra/$KEY
	if [ ! -d $DEST/.bzr ]; then
		rm -rf $DEST
		bzr co http://woodem.eu/private/$KEY/branch $DEST
	else
		bzr up $DEST
	fi
done

# compile
mkdir -p ~/woo-build
ccache -M50G -F10000 # adjust maxima for ccache
scons -C ~/woo flavor= features=gts,opengl,openmp,qt4,vtk jobs=4 buildPrefix=~/woo-build CPPPATH=`ls -d /usr/include/vtk-5.*`:/usr/include/eigen3 CXX='ccache g++' brief=0 debug=0
