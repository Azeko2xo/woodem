.. _batch:

*************
Batch system
*************

Command-line options
====================

Batch system allows one to run one simulation with different parameters and collect results from all simulations; it does so by managining CPU resources (cores) on the machine and running each sub-simulation when there is enough resources.

Usually, a batch is described by two files:

1. simulation itself, which is one of
	* :obj:`woo.core.Preprocessor` or :obj:`woo.core.Scene` (saved in any supported file formats)
	* python script to be run
2. batch table
	Simple file, plain-text or XLS, where each column is one parameter, and each line is one simulation.

Batch is invoked using the ``woo-batch`` command (``wwoo-batch`` under Windows) and full help is obtained with ``--help``::

    usage: woo-batch [-h] [-j NUM] [--job-threads NUM] [--force-threads]
                 [--log FORMAT] [--global-log FILE] [-l LIST]
                 [--results RESULTSDB] [--nice NICE] [--cpu-affinity]
                 [--executable FILE] [--rebuild] [--debug] [--gnuplot FILE]
                 [--dry-run] [--http-wait] [--generate-manpage FILE]
                 [--plot-update TIME] [--plot-timeout TIME] [--refresh TIME]
                 [--timing COUNT] [--timing-output FILE] [--randomize]
                 [--no-table]


Let us summarize important parameters for everyday use:

``-j THREADS``
	Number of threads (cores) used *by the batch system*. Defaults to number of cores on you machine, which is usually what you want.
``--job-threads``
	Number of threads *for each simulation*. Defaults to 1. Depending on size of the batch, you may want to increase this number. E.g. running a batch 2 simulations on a 6-core machine can take advantage of setting ``--job-threads=3``.
``--results``
	Results from each simulation are written incrementally to an `SQLite <http://www.sqlite.org>`_ file. This file, by default, named same as the batch table, but with the ``.results`` extension, but it can be changed.


.. _batch_example:

Batch example
==============

Let us use the :obj:`woo.pre.horse.FallingHorse` preprocessor as the basis for our example; suppose we want to study the influence of the :obj:`pWaveSafety <woo.pre.horse.FallingHorse.pWaveSafety>` parameter, which will vary between 0.1 to 0.9. This example is shown in `examples/horse-batch <http://bazaar.launchpad.net/~eudoxos/woo/trunk/files/head:/examples/horse-batch/>`_ in the source distribution.

As exaplained above, we need one file with preprocessor and one file describing how to vary preprocessor parameters.

Preprocessor
------------

Preprocessor can be saved from the :ref:`user interface <preprocessor_gui>` as text, but any loadable format is acceptable. Text file is the easiest to be inspected/modified by hand. The whole file must be a valid python expression::

	##woo-expression##
	#: import woo.pre.horse,woo.dem,math
	woo.pre.horse.FallingHorse(
		radius=2*woo.unit['mm'],    ## same as 2e-3
		pattern='hexa',
		mat=woo.dem.FrictMat(
			density=1e3,
			young=5e4,
			ktDivKn=.2,
			tanPhi=math.tan(.5)
		)
	)

The ``##woo-expression##`` denotes format of the file (for auto-detection; file extension is irrelevant), special comments starting with ``#:`` are executed before the expression is evaluted. Unit multipliers can be used via the :obj:`woo.unit` dictionary, as with ``mm``. Arbitrary (including nested) expressions can be used (``math.tan(.5)``, for instance). Unspecified parameters take their default values.

Table
-----

Batch table is tabular representation of data, where each row represents one simulation, and each column one parameter value. The table can be given in text (space-separated columns) or as XLS file, where the first worksheet is used. ``#`` can be used for comments. The first non-empty line are column headers, each non-empty line afterwards is one simulation.

===== ===========
title	pWaveSafety
===== ===========
dt.9	0.9
dt.7	0.7
dt.4	0.4
dt.2	0.2
dt.1	0.1
dt.05	0.05
===== ===========

This table will run 6 simulations. The **title** column is optional; if not given, simulation title is created from values of other parameters. For details of table syntax (including default values, repeating previous values and comments) see :obj:`woo.batch.TableParamReader`.

.. note:: Simulation title can be used as basis for output files. In particular, Windows systems don't allow many characters in filenames, which can lead to errors. Therefore, specifying the **title** column without dangerous characters is always advisable under Windows.

Launching the batch
--------------------

The batch is the ready to be run from the terminal::

	$ woo-batch --job-threads=2 dt.xls dt.preprocessor

The ``--job-threads=2`` option instructs the batch system to run each simulation on 2 cores, and will use all available cores (since ``-j`` was not given). Terminal output looks similar to

.. image:: fig/batch-terminal.*

You can see how the batch system manages cores of the machine and schedules simulations to be run one after another. Each job leaves its output in logfile, usually under :samp:`logs/*.log`. Those files can be inspected if something goes wrong.

Batch also automatically opens webpage (served by the batch process), usually at ``http://localhost:9080`` (depending on free ports on the machine), showing current status of the batch. It gives quick visual overview, easy access to log files, shows and updates :obj:`plots <S.plot.plots>`:

.. image:: fig/batch-html.*

Batch simulations produce per-simulation and aggregate results, as explained in the :ref:`Postprocessing` section.
