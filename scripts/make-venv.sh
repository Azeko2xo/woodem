#!/bin/bash
#
# create venv for woo
#
set -e -x
if [ "$#" -ne 3 ]; then
	echo "USAGE: $0 path/to/venv flavor path/to/woo/source"
	exit 1
fi

VENV=$1
NAME=$2
WOOSRC=$3

virtualenv $VENV
source $VENV/bin/activate
pip install --egg scons
pip install cython minieigen ipython numpy matplotlib genshi xlwt xlrd h5py lockfile psutil pillow bzr colour-runner
pip install svn+https://svn.code.sf.net/p/python-xlib/code/trunk/
ln -s /usr/lib/python2.7/dist-packages/{sip*,PyQt4} $VIRTUAL_ENV/lib/python2.7/site-packages
scons -C $WOOSRC features='openmp,opengl,qt4,vtk,log4cxx,gts' jobs=5 CXX='ccache g++' flavor=$2
