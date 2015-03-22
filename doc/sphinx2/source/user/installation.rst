***************
Installing Woo
***************

Linux
=====

Linux is the platform of choice for both developing and using Woo. Installation can be done in several ways, trading fexibility for straightforwardness.

Package installation
---------------------

.. admonition:: Ubuntu

   Installation from pre-compiled packages is the recommended method of installation. You will receive updates automatically via your system's package manager. `Woo-daily package archive <https://code.launchpad.net/~eudoxos/+archive/woo-daily>`_ contains the packages. Open the terminal and type::

      sudo add-apt-repository ppa:eudoxos/woo-daily
      sudo apt-get update
      sudo apt-get install python-woo

Compilation from source
-----------------------
Source code is hosted at `GitHub <https://github.com/eudoxos/woodem>`__ and can be otained with the `Git revision control system <http://git-scm.com/>`__ by saying ``git clone https://github.com/eudoxos/woodem.git``.

(We still also use the hosting and `Launchpad <http://www.launchpad.net/woo>`_ and sources synced from GitHub can be obtained with the `Bazaar  <http://bazaar.canonical.com>`_ by saying ``bzr checkout lp:woo``. Prefer to use git, however, since updates via ``-RR`` are more reliable, and git is also much faster).

Woo is written in `C++11 <http://en.wikipedia.org/wiki/C%2B%2B11>`_ and can be compiled with gcc>=4.6 or clang >= 3.1. Note that clang does not support all features (``openmp`` in particular).

Woo depends on a number of libraries. Those are in particular

#. `boost <http://www.boost.org>`_, version >= 1.48.
#. `python <http://www.python.org>`_, version 2.7 (versions 3.x are currently unsupported)
#. `eigen <http://eigen.tuxfamily.org>`_ (math routines)
#. `Qt4 <http://qt.digia.com>`_ and `PyQt4 <http://www.riverbankcomputing.co.uk/software/pyqt>`_ (optional: user interface)
#. `VTK <http://www.vtk.org>`_ (optional: export to `Paraview <http://www.paraview.org>`_)
#. `GTS <http://gts.sourceforge.net>`_ (optional: manipulation of triangulated surfaces)
#. `FreeGLUT <http://freeglut.sourceforge.net>`_, `GLE <http://www.linas.org/gle>`_ (optional: OpenGL display)
#. A number of python modules: ``matplotlib`` (plotting), ``genshi`` (HTML export), ``xlrd`` & ``xlwt`` (XLS export), ``minieigen`` (math in python), ``xlib``, ``psutil``, …

Ubuntu machines can be easily set up for development with Woo using the `woo-install-ubuntu.sh <http://bazaar.launchpad.net/~eudoxos/woo/trunk/view/head:/scripts/woo-install-ubuntu.sh>`_ script. This script also installs packages neede for building.

Compilation is driven using `scons <http://www.scons.org>`_ and takes a number of options. They are listed when ``scons -h`` is run from the source directory, and are remembered accross compilations (they can be given just the first time):

-  ``features``: Comma-separated list of features to compile with. Important values are

   * ``openmp``: Compile with OpenMP, to support multi-threaded computations. This feature is supported only by ``gcc`` (not ``clang``). Note that number of threads can be set :ref:`at runtime <Running_Woo>` using the ``-jN`` option.
   * ``qt4``: Enable user interface based on Qt4.
   * ``opengl``: Enable 3D display during simulations; requires ``qt4`` to be enabled as well.
   * ``vtk``: Enable export to file using VTK file formats, for export to `Paraview <http://www.paraview.org>`_.
   * ``gts``: Enable building the (adapted) :obj:`woo.gts` module.

- ``jobs``: Number of compilations to run simultaneously. Can be set to the number of cores your machine has, but make sure you have at least 3GB RAM per each job if you use ``gcc`` − Woo is quite RAM-hungry during compilation, due to extensive use of templates, especially in boost::python.
- ``CPPPATH``: Path for preprocessor. Usually, you only need to set this if you have VTK or Eigen in non-standard locations.
- ``CXX``: The compiler to use.

A typical first-compile command line may look like this::

   scons jobs=4 CXX='ccache g++' features=qt4,opengl,vtk,openmp,gts

For quick development, woo takes the ``-R`` flag, which will recompile itself (with remembered options) and run. The ``-RR`` flag will, in addition, update the source from upstream before recompiling (if managed with git/bzr).

Virtual environment
^^^^^^^^^^^^^^^^^^^

Python allows for creation of separate `Virtual environments <http://docs.python-guide.org/en/latest/dev/virtualenvs/>`__ which is an isolated working copy of Pyhon. This is useful if you want to keep multiple version of Woo around simultaneously, or don't want to install system-wide (where different users need a different version, and perhaps don't have permissions to install to system directories).

If you want to use SCons for building (which is quite useful for keeping your installation up-to-date, and necessary if you want to modify the source code), do something like the following example. You need to have all required headers and libraries installed somewhere where they will be found. This example compiles Woo without the ``opengl`` and ``qt4`` features, for running Woo without the graphical interface (useful e.g. on computing nodes in clusters).

.. code-block:: bash

    # install the support for virtual environments
    pip install virtualenv
    # create the virtual environment in some directory
    virtualenv my/venv
    # sets environment variables (e.g. $PATH) so that venv commands are found first
    source my/venv/bin/activate
    # install scons (needs the --egg option)
    pip install --egg scons
    # install all required python modules, this may take a while
    # note: headers for HDF5 must be installed (libhdf5-dev)
    pip install cython minieigen ipython numpy matplotlib genshi xlwt xlrd h5py lockfile psutil pillow bzr colour-runner
    # if you need the GUI, also run this (and add opengl,qt4 features to scons below)
    pip install svn+https://svn.code.sf.net/p/python-xlib/code/trunk/  # xlib
    ln -s /usr/lib/python2.7/dist-packages/{sip*,PyQt4} $VIRTUAL_ENV/lib/python2.7/site-packages
    # checkout the source from Launchpad
    bzr co lp:woo woo
    ### for wooExtra modules (if you need some)
    ## create directory for extras
    mkdir woo/wooExtra
    ## checkout extras, put them under there so that they are installed automatically
    bzr co URL woo/wooExtra/...    
    cd woo
    # compile the source
    scons features='vtk,gts,openmp' BUNCH OF OTHER OPTIONS
    # run self-tests to check that everything is OK
    woo --test
    # exit the virtual environment
    deactivate                       

The ``woo`` executable remembers virtual python used during the build (in `shebang <http://en.wikipedia.org/wiki/Shebang_%28Unix%29>`__), so you can also execute it *without* activating the virtual environment (by saying ``my/venv/bin/woo``) the next time, and it *should* work (including recompilation with ``-R`` or ``-RR``), **unless** you have another installation of woo system-wide (in that case, make sure you always activate the virtual environment properly).

.. note:: There is a script for quick creation of virtual installation, which is useful e.g. for keeping exact installed version despite ongoing development. It is located in :woosrc:`scripts/make-venv.sh` and can be run e.g. as::

    $ scripts/make-venv.sh path/to/venv name /woo/source/tree

  which will create the virtual environment and compile and install Woo in it.

Distribution-specific instructions
----------------------------------

.. toctree::
   dist/suse.rst


Windows
=======

Running woo under Windows is supported, but with some limitations:

#. Only 64bit systems are supported.
#. Compilation from source under Windows is not supported; since there is poor standardization for Windows development, the compilation process is tailored for a single insllation of development tools.
#. The computation is about 15% slower under Windows; the cause is − probably − less efficient locking provided by the OS, and perhaps also less agressive optimization, since the compiler has to optimize for the lowest common instruction set.

Binaries can be downloaded from `Launchpad download page <https://launchpad.net/woo/+download>`_, as follows:

#. Download and run ``Woo-libs-*-installer.exe`` with the highest version number.
#. Download and run ``Woo-main-*-installer.exe``. It must be installed into the same directory as libs (this is checked at install-time)
#. Download and run any installers for custom modules you may have (``Woo-wooExtra.*-installer.exe``)

The installer installs the code system-wide, supports unattended installation via the `/S` switch, and the uninstaller should remove all files. Uninstaller for ``Woo-libs`` must be run after all other components have been uninstalled. Installation directory of Woo is added to the ``PATH`` environment variable, so the command ``wwoo`` is understood wherever you are in the filesystem.

An icon is added to the start menu under ``Woo/woo``.
