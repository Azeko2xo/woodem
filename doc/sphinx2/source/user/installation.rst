***************
Installing Woo
***************

Linux
=====

Linux is the platform of choice for both developing and using Woo. Installation can be done in several ways, trading fexibility for straightforwardness.

Package installation
	If your distribution is supported, this is the easiest way to install and to receive updates automatically, via your system's package manager. For now, only recent releases of Ubuntu have packages built automatically, those can be downloaded from `Woo-daily package archive <https://code.launchpad.net/~eudoxos/+archive/woo-daily>`_.
Compilation from source
	Source code is hosted at `Launchpad <http://www.launchpad.net/woo>`_ and can be obtained with the `Bazaar revision control system <http://bazaar.canonical.com>`_ by saying ``bzr checkout lp:woo``.

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

	Ubuntu machines can be easily set up for development with Woo using the `woo-install-ubuntu.sh <http://bazaar.launchpad.net/~eudoxos/woo/trunk/view/head:/scripts/woo-install-ubuntu.sh>`_ script.

	Compilation is driven using `scons <http://www.scons.org>`_ and takes a number of options. They are listed when ``scons -h`` is run from the source directory, and are remembered accross compilations (they can be given just the first time):

	features
		Comma-separated list of features to compile with. Important values are

		openmp
			Compile with OpenMP, to support multi-threaded computations. This feature is supported only by ``gcc`` (not ``clang``. Note that number of threads must be set :ref:`at runtime <Running_Woo>` using the ``-jN`` option.
		qt4
			Enable user interface based on Qt4.
		opengl
			Enable 3D display during simulations; requires ``qt4`` to be enabled as well.
		vtk
			Enable export to file using VTK file formats, for export to `Paraview <http://www.paraview.org>`_.
		gts
			Enable building the (adapted) :obj:`woo.gts` module.
	jobs
		Number of compilations to run simultaneously. Can be set to the number of cores your machine has, but make sure you have at least 4GB RAM per each job if you use ``gcc`` − Woo is extremely RAM-hungry during compilation, due to extensive use of templates.
	CPPPATH
		Path for preprocessor. Usually, you only need to set this if you have VTK or Eigen in non-standard locations.

	For quick development, woo takes the ``-R`` flag, which will recompile itself and run. The ``-RR`` flag will, in addition, update the source before recompiling.
		 	

Windows
=======

Running woo under Windows is supported, but with some limitations:

#. Only 64bit systems are supported (at the moment).
#. Compilation from source under Windows is not supported; since there is poor standardization for Windows development, the compilation process is tailored for a single insllation of development tools.
#. The computation is about 15% slower under Windows; the cause is − probably − less efficient locking provided by the OS, and perhaps also less agressive optimization, since the compiler has to optimize for the lowest common instruction set.

Binaries can be downloaded from `Launchpad download page <https://launchpad.net/woo/+download>`_, as follows:

#. Download and run ``Woo-libs-*.installer.exe`` with the highest version number.
#. Download and run ``Woo-main-*.installer.exe``. It must be installed into the same directory as libs (this is checked at install-time)
#. Download and run installers for any custom modules you may have.

The installer installs the code system-wide, supports unattended installation via the `/S` switch, and the uninstaller should remove all files. There are some glitches, but we're working on improving that. Install path of Woo is added to the ``PATH`` environment variable, so the command ``wwoo`` is understood wherever you are in the filesystem.
