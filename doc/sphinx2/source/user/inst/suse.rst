OpenSUSE 13.1
==============


zypper install git
 
* go to https://build.opensuse.org/package/show/KDE:Qt/libQGLViewer
* select repository matching your install (OpenSUSE_13.1, for example)
* click "Go to download repository", copy URL of the KDE:Qt.repo file
sudo zypper ar -f http://download.opensuse.org/repositories/KDE:/Qt/openSUSE_13.1/KDE:Qt.repo
sudo zypper update    # type "a" ("trust always") for the new repo key
zypper install scons ccache gcc-c++ binutils-gold libeigen3-devel boost-devel ipython python-matplotlib python-numpy python-genshi python-xlwt python-xlrd python-h5py python-lockfile python-xlib vtk-devel ipython hdf5-devel python-pip python-Cython python-devel python-numpy-devel freeglut-devel libgle-devel libqt4-devel python-qt4-devel libQGLViewer-devel vtk-devel
pip install h5py minieigen
git clone https://github.com/eudoxos/woodem woo
scons jobs=4 CXX='ccache g++' features=qt4,opengl,vtk,openmp,gts CPPPATH=/usr/lib64/python2.7/site-packages/numpy/core/include/:/usr/include/eigen3:/usr/include/vtk-6.1
sudo perl -pi -e's/\((isnan|isinf|isfinite)/\(std::$1/' /usr/include/vtk-6.0/vtkMath.h
