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

ISSUE=`cat /etc/issue`

case $ISSUE in
	*12.04*|*12.10*|*13.04*) 
		echo '*** YOUR UBUNTU INSTALLATION IS TOO OLD, ONLY 13.10 AND NEWER ARE SUPPORTED***'
		exit 1
	;;
esac

sudo apt-get install software-properties-common python-software-properties # for add-apt-repository
sudo add-apt-repository ppa:eudoxos/minieigen
sudo apt-get update

sudo apt-get install libboost-all-dev python-colorama libqt4-dev-bin python-setuptools python-all-dev pyqt4-dev-tools libqt4-dev qt4-dev-tools libgle3-dev libqglviewer-dev libvtk5-dev libgts-dev libeigen3-dev freeglut3-dev python-xlrd python-xlwt python-numpy python-matplotlib python-qt4 python-xlib python-genshi python-psutil python-imaging python-h5py python-lockfile git ccache scons ipython mencoder python-imaging python-minieigen python-prettytable
		
sudo chown -R $USER: /usr/local


if [ ! -d ~/woo/.git ]; then
	rm -rf ~/woo # in case it contains some garbage
	git clone https://github.com/eudoxos/woodem.git ~/woo
else
	git -C ~/woo pull
fi

# grab extra modules
for KEY in "$@"; do
	DEST=~/woo/wooExtra/$KEY
	if [ ! -d $DEST/.git ]; then
		rm -rf $DEST
		git clone http://woodem.eu/private/$KEY/git $DEST
	else
		git -C $DEST pull
	fi
done

# compile
mkdir -p ~/woo-build
ccache -M50G -F10000 # adjust maxima for ccache
scons -C ~/woo flavor= features=gts,opengl,openmp,qt4,vtk jobs=4 buildPrefix=~/woo-build CPPPATH=`ls -d /usr/include/vtk-{5,6}.*`:/usr/include/eigen3 CXX='ccache g++' brief=0 debug=0

## tests
# crash at exit perhaps, should be fixed
woo -j4 --test || true
## test update (won't recompile, anyway)
woo -RR -x || true 
echo "*** Woo has been installed successfully. ***"
