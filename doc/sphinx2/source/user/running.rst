.. _Running_Woo:

************
Running Woo
************

Command-line options
=====================

Woo is primarily a computation program, with only optional graphical user-interface. To run woo, type ``woo`` in the terminal (use ``wwoo`` under Windows). A number of options can be specified, given using UNIX convention of ``--long-option`` or ``-s`` (short option). A help on all available options is obtained with ``woo -h``:

    usage: woo [-h] [--version] [-j THREADS] [--cores CORES] [--cl-dev CLDEV] [-n]
               [-D] [--quirks QUIRKS] [--flavor FLAVOR] [-c COMMANDS] [-e EXPR]
               [--paused] [--nice NICE] [-x] [-v] [-R] [--test] [--no-gdb]
               [--in-gdb] [--in-pdb] [--in-valgrind]
               ...

Options important for everyday use are the following ones:

``-j THREADS``
    Number of CPU cores to use for computation. Woo is parallelized using `OpenMP <http://www.openmp.org>`_, and by default runs on one core. Depending on hardware and the nature of simulation, reasonable value is usually around `-j4`. Woo must have been compiled with the ``openmp`` feature for this option to have effect.
``-e``
	Evaluate given expression (should yield a :obj:`Scene <woo.core.Scene>` or a :ref:`Preprocessor <woo.core.Preprocessor>`)
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

Once Woo starts, it shows an embedded command-prompt.

.. ipython::

    Woo [1]:

To get started with scripting Woo, you should get familiar with Python, for instance using `Python tutorial <http://docs.python.org/2/tutorial/>`_.


Direct import
==============

Woo can be used in python scripts. You only have to say ``import woo`` and everything should just work. Parameters which cannot be changed once ``woo`` is imported can be set via the ``wooOptions`` module:

.. code-block:: python

    import wooOptions
	 wooOptions.ompThreads=4
	 wooOptions.debug=True
	 import woo    # initializes OpenMP to 4 threads, and uses the debug build of woo

