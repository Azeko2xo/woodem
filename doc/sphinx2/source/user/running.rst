.. _Running_Woo:

************
Running Woo
************

Command-line options
=====================

Woo is primarily a computation program, with only optional graphical user-interface. To run woo, type ``woo`` in the terminal (use ``wwoo`` under Windows). A number of options can be specified, given using UNIX convention of ``--long-option`` or ``-s`` (short option). A help on all available options is obtained with ``woo -h``::

    usage: woo [-h] [--version] [-j THREADS] [--cores CORES] [--cl-dev CLDEV] [-n]
               [-D] [--quirks QUIRKS] [--flavor FLAVOR] [-c COMMANDS] [-e EXPR]
               [--paused] [--nice NICE] [-x] [-v] [-R] [--test] [--no-gdb]
               [--in-gdb] [--in-pdb] [--in-valgrind]

Options important for everyday use are the following ones:

``-j THREADS``
    Number of CPU cores to use for computation. Woo is parallelized using `OpenMP <http://www.openmp.org>`_, and runs on 4 cores by default (or less, if the machine has less than 4). Depending on hardware and the nature of simulation, reasonable value is usually around `-j4`. Woo must have been compiled with the ``openmp`` feature for this option to have any effect.
``-e``
	Evaluate given expression (should yield a :obj:`Scene <woo.core.Scene>` or a :obj:`Preprocessor <woo.core.Preprocessor>`)
``-n``
    Run without graphical interface.
``-x``
    End once the simulation (or script) finishes.
``--paused``
    When loading a simulation from the command line (below), don't have it run immediately.
``-R``
	Recompile Woo before running. Useful during development when sources are modified.
``-RR``
	Update sources from the repository, recompile and run.

Woo takes an additional argument (existing file) coming after options. It can be

#. ``*.py``: python script which will be interpreted by Woo.
#. Saved simulation (in any :ref:`supported format <serializationFormats>`), which will be loaded and run by Woo.
#. Preprocessor, which will be run to create a new simulation, which will be run by Woo.

Once Woo starts, it shows an embedded command-prompt based on `IPython <http://www.ipython.org>`_. It can be used as general-purpose python console and to inspect and modify simulations:

.. image:: fig/woo-terminal.png

..
	.. ipython::
	    Woo [1]: 1+1
		 Woo [1]: woo.master.scene.dt


To get started with scripting Woo, you should get familiar with Python, for instance using `Python tutorial <http://docs.python.org/2/tutorial/>`_.


Import as python module
========================

Woo can be used in python scripts. You only have to say ``import woo`` and everything should just work. Parameters which cannot be changed once ``woo`` is imported can be set via the :obj:`wooMain.options` object:

.. code-block:: python

    import wooMain
    wooMain.options.ompThreads=4
    wooMain.options.debug=True
    import woo    # initializes OpenMP to 4 threads, and uses the debug build of woo

Graphical interface
===================

Graphical interface is entirely optional in Woo, simulations can run without it (with the `-n` switch, when no `$DISPLAY` is available, in batch, or if compiled without the ``qt4`` feature).

Controller
----------
The main Woo window, called :obj:`controller <woo.qt.ControllerClass>`, is brought up automatically at startup if a simulation/script is given on the command-line (under Windows, the controller is always shown at startup). The controller can be manually brought up by pressing ``F12`` in the terminal, or by typing ``woo.qt.Controller()``. The window (when the first, *Simulation* tab, is active) looks like this:

.. image:: fig/controller-simulation.*

It is divided in several blocks --

A. Time display (simulation time, clock time, step number, timestep)
B. Loading/saving simulation, file where the simulation was last saved
C. Running controls:
	* start/stop
	* advance by one timestep (or multiple steps, or substep)
	* reload from last saved file
D. Display controls (toggle)
	* 3d (OpenGL) window
	* 2d plot window
	* Inspector
E. Area for simulation-specific controls, if defined (:obj:`woo.core.Scene.uiBuild`)

The interface can display structured objects -- for example, unde :menuselection:`Controller --> Display` we see something like this:

.. image:: fig/controller-display.png

Blue object/attribute labels are always *active*:

* *left-click* opens online documentation for that particular class/attribute.
* *middle-click* will copy *path* to that object to the clipboard, which can be then pasted into python.
* each attribute has tooltip showing full documentation for that attribute; just hover over the label.

.. image:: fig/object-editor-tooltip.png

For the object, attributes can be displayed either as variable names, or as their documentation, units can be enabled/disabled, and per-attribute checkboxes can be added for easy input-checking (when all values must be set):

.. image:: fig/object-editor-context-menu.png

3d rendering
------------

3d view
^^^^^^^^

The 3d windows is opened by clicking the "3D" button in the **D** part of the controller.

.. image:: fig/controller-3d.png

It is navigated with mouse similar to other 3d software:

* *left drag* rotates
* *right drag* moves
* *middle drag* (wheel) zooms
* *double left-click* sets view angle to the closes multiple of 45°
* :kbd:`Alt` + *left-click* selects object (and shows distance to previous selection)

Many keyboard shortcuts are defined of which the most important ones are:

* :kbd:`h` shows help;
* :kbd:`t` toggles perspective/orthographic camera;
* :kbd:`c` centers the view around whole scene;
* :kbd:`Alt-c` centers the view intelligently around that part of the scene where most particles are;
* :kbd:`a` toggles display of axes;
* :kbd:`g` displays axes grids (cycles between all possible combinations);
* :kbd:`x`, :kbd:`X`, :kbd:`y`, :kbd:`Y`, :kbd:`z`, :kbd:`Z`: make selected axes point upwars and align the other two, i.e. show respectively the ``zx``, ``yx``, ``xy``, ``zy``, ``yz``, ``xz`` plane;
* :kbd:`s` toggles displacement/rotations scaling (see :ref:`woo.gl.Renderer.scaleOn`);
* :kbd:`d` selects which time information is displayed;
* :kbd:`Ctrl-c` copies the view to clipboard, as raster image (can be pasted to documents/graphics editors).

Colorscales can be manipulated using mouse:

* *wheel* changes size
* *right-draw* moves, and toggles portrait/landscape when touching the edge
* *left-click* resets the range an sets to auto-adjust

A movie from the 3d view can be made by checking the :menuselection:`Controller -> Video --> Take snapshots` first, and, when sufficient number of snapshots will have been save, clicking :menuselection:`Controller --> Video --> Make video`.

Display control
^^^^^^^^^^^^^^^^^

The *Display* tab of the controller configures the 3D display. Woo dispatches OpenGL display of all objects to objects (always called ``Gl1_*``) responsible for actual drawing, which is also how this dialogue is organized.

.. image:: fig/controller-display.png


:obj:`Renderer <woo.gl.Renderer>` configures global view properties -- initial orientation, displacement scaling, lighting, clipping, and which general items are displayed.

:obj:`Gl1_DemField <woo.dem.Gl1_DemField>` (shown on the image) is reponsible for displaying contents of DEM simulations (:obj:`woo.dem.DemField`) -- particles, contacts between particles and so on. For instance, particles corresponding to the :obj:`shape <woo.gl.Gl1_DemField.shape>` attribute are colored using the method specified with :obj:`colorBy <woo.gl.Gl1_DemField.colorBy>`. Other particles (not matching :obj:`shape <woo.gl.Gl1_DemField.shape>`, or not able to be colored using :obj:`colorBy <woo.gl.Gl1_DemField.colorBy>`, e.g. non-spherical particle by radius) are colored using :obj:`colorBy2 <woo.gl.Gl1_DemField.colorBy2>`.

Display of each particle's :obj:`shape <woo.dem.Shape>` is dispatched to :obj:`Gl1_* <woo.gl.GlShapeFunctor>` objects (e.g. :obj:`woo.gl.Gl1_Sphere`, :obj:`woo.gl.Gl1_Facet`, …), which control shape-specific options, such as display quality.

.. _preprocessor_gui:

Preprocessor
------------

Preprocessors can be set and run from the *Preprocess* tab, which can be opened directly from the terminal with ``F9`` (Linux-only).

.. image:: fig/controller-preprocessor.*

In the top selection, all available preprocessors are listed. Preprocessor can be modified, loaded and saved. Once you have set all parameters, the *play* button bottom right will create new simulation and switch to the *Simulation* tab automatically.

Unit specifications are only representation. Technically is Woo unit-agnostic, practically, `SI units <http://en.wikipedia.org/wiki/Si_units>`_ are used everywhere. See :obj:`woo._units` for details.

The preprocessor can be saved for later use; it is saved, by default, as python expression. which is human-readable and easily editable::

	##woo-expression##
	#: import woo.pre.horse,woo.dem
	woo.pre.horse.FallingHorse(
		radius=0.002,
		relGap=0.25,
		halfThick=0.002,
		relEkStop=0.02,
		damping=0.2,
		gravity=(0.0, 0.0, -9.81),
		pattern='hexa',
		mat=woo.dem.FrictMat(density=1000.0, id=-1, young=50000.0, tanPhi=0.5463024898437905, ktDivKn=0.2),
		meshMat=None,
		pWaveSafety=0.7,
		reportFmt='/tmp/{tid}.xhtml',
		vtkStep=40,
		vtkPrefix='/tmp/{tid}-'
	)
