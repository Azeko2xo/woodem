OpenSUSE 13.1
==============

If on one-user machine, make /usr/local owned by yourself -- for easy installation and updates::

   sudo chown $USER: -R /usr/local

Install all dependencies:

.. code-block:: bash

   # repository for libQGLViewer
   sudo zypper ar -f http://download.opensuse.org/repositories/KDE:/Qt/openSUSE_13.1/KDE:Qt.repo
   # repository for git>=1.8.5 (13.1 has 1.8.4)
   sudo zypper ar -f http://download.opensuse.org/repositories/devel:/tools:/scm/openSUSE_13.1/devel:tools:scm.repo
   # repository for gts-snapshot
   sudo zypper ar -f http://download.opensuse.org/repositories/home:/popinet/openSUSE_13.1/home:popinet.repo
   # reload repositories; type "a" ("trust always") for new repository keys
   sudo zypper update   
   # install everything what is packaged
   # IMPORTANT: there will be vendor cachanges (KDE:Qt.repo) for packages
   #   make sure you accept all of them!
   sudo zypper install git scons ccache gcc-c++ binutils-gold libeigen3-devel boost-devel ipython python-matplotlib python-numpy python-genshi python-xlwt python-xlrd python-h5py python-lockfile python-xlib python-imaging vtk-devel ipython hdf5-devel python-pip python-Cython python-devel python-numpy-devel python-psutil freeglut-devel libgle-devel libqt4-devel python-qt4-devel python-qt4-utils libQGLViewer-devel vtk-devel gts-snapshot-devel
   # patch vtk-6.0 headers, to work around http://www.paraview.org/Bug/view.php?id=14164
   sudo perl -pi -e's/\((isnan|isinf|isfinite)/\(std::$1/' /usr/include/vtk-6.0/vtkMath.h
   # locally compile python modules which are not packaged
   # do not use python-h5py package since there may be hdf5 version mismatch for some reason
   sudo pip install h5py minieigen colour_runner 

Now grab and compile woo itself:

.. code-block:: bash

   # get Woo source
   git clone https://github.com/eudoxos/woodem woo
   # compile (drink coffee); adjust jobs=... to number of cores, and at least 4GB RAM/job
   scons jobs=4 CXX='ccache g++' brief=0 features=qt4,opengl,vtk,openmp,gts CPPPATH=/usr/lib64/python2.7/site-packages/numpy/core/include/:/usr/include/eigen3:/usr/include/vtk-6.1 LIBPATH=/usr/lib64/vtk LIBDIR=/usr/local/lib64/python2.7/site-packages EXECDIR=/usr/local/bin
   # run self-tests
   woo --test
   # later, try integrated git update & rebuild
   woo -RR

