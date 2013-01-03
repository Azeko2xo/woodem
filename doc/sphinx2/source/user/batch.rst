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


Batch example
==============

Let us use the :obj:`woo.pre.horse.FallingHorse` preprocessor as the basis for our example; suppose we want to study the influence of the :obj:`pWaveSafety <woo.pre.horse.FallingHorse.pWaveSafety>` parameter, which will vary between 0.1 to 0.9. This example is shown in `examples/horse-batch <http://bazaar.launchpad.net/~eudoxos/woo/trunk/files/head:/examples/horse-batch/>`_ in the source distribution.

Inputs
------

As exaplained above, we need one file with preprocessor and one file describing how to vary preprocessor parameters.

Preprocessor
^^^^^^^^^^^^

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
^^^^^

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

This table will run 6 simulations. The **title** column is optional; if not given, simulation title is created from values of other parameters.

.. note:: Simulation title can be used as basis for output files. In particular, Windows systems don't allow many characters in filenames, which can lead to errors. Therefore, specifying the **title** column without dangerous characters is always advisable under Windows.

Running
-------

The batch is the ready to be run from the terminal::

	$ woo-batch --job-threads=2 dt.xls dt.preprocessor

The ``--job-threads=2`` option instructs the batch system to run each simulation on 2 cores, and will use all available cores (since ``-j`` was not given). Terminal output looks similar to

.. image:: fig/batch-terminal.*

You can see how the batch system manages cores of the machine and schedules simulations to be run one after another. Each job leaves its output in logfile, usually under :samp:`logs/*.log`. Those files can be inspected if something goes wrong.

Batch also automatically opens webpage (served by the batch process), usually at ``http://localhost:9080`` (depending on free ports on the machine), showing current status of the batch. It gives quick visual overview, easy access to log files, shows and updates :obj:`plots <S.plot.plots>`:

.. image:: fig/batch-html.*

Outputs
--------

Report
^^^^^^^

Every simulation (not just in batch) may generate report which summarizes its inputs and outputs in a human-readable form:

.. image:: fig/batch-report.png
	:width: 400px

Results database
^^^^^^^^^^^^^^^^^

Finished jobs write some resulting data to results database in the `SQLite <http://www.sqlite.org>`_. The database is (usually) named the same as parameter table, but with the :file:`results` suffix (:file:`dt.results` in our example).

Contained data are serialized using the neutral `JSON <http://en.wikipedia.org/wiki/Json>`_ representation, so that it can be processed with any language (Python, JavaScript, Matlab, …). Working with the database directly is an advanced topic not covered in this brief introduction.

.. note:: The database file is never deleted, only appended to. Running the same batch several times will therefore leave results of old batches intact.

Results XLS
^^^^^^^^^^^^

The database file is (usually) converted to a ``xls`` file after every write (:file:`dt.xls`). It contains most data in the database, and is suitable for human post-processing, such as creating ad-hoc figures or aggregating results in a non-automatic manner.

The first worksheet contains each simulation in one column:

============================================  =============================================  ============================================  ================================================
title                                         dt.05                                          dt.1                                          dt.2
batchtable                                    dt.xls                                         dt.xls                                        dt.xls
batchTableLine                                6                                              5                                             4
finished                                      2013-01-03  23:21:55.011794                    2013-01-03  23:21:34.595636                   2013-01-03  23:20:35.838987
sceneId                                       20130103T231904p19387                          20130103T231843p19371                         20130103T231837p19356
duration                                      171                                            171                                           118
formatNumber                                  3                                              3                                             3
misc.report                                   file:///tmp/dt.05.20130103T231904p19387.xhtml  file:///tmp/dt.1.20130103T231843p19371.xhtml  file:///tmp/dt.2.20130103T231837p19356.xhtml
misc.simulationName                           horse                                          horse                                         horse
plots.t                                       relErr                                         relErr                                        relErr
plots.i.0                                     total                                          total                                         total
plots.i.1                                     S.energy.keys()                                S.energy.keys()                               S.energy.keys()
pre.__class__                                 woo.pre.horse.FallingHorse                     woo.pre.horse.FallingHorse                    woo.pre.horse.FallingHorse
pre.damping                                   0.2                                            0.2                                           0.2
pre.gravity.0                                 0.0                                            0.0                                           0.0
pre.gravity.1                                 0.0                                            0.0                                           0.0
pre.gravity.2                                 -9.81                                          -9.81                                         -9.81
pre.halfThick                                 0.002                                          0.002                                         0.002
pre.mat.__class__                             woo.dem.FrictMat                               woo.dem.FrictMat                              woo.dem.FrictMat
pre.mat.density                               1000.0                                         1000.0                                        1000.0
pre.mat.id                                    -1                                             -1                                            -1
pre.mat.ktDivKn                               0.2                                            0.2                                           0.2
pre.mat.tanPhi                                0.546302489844                                 0.546302489844                                0.546302489844
pre.mat.young                                 50000.0                                        50000.0                                       50000.0
pre.meshMat                                   None                                           None                                          None
pre.pWaveSafety                               0.05                                           0.1                                           0.2
pre.pattern                                   hexa                                           hexa                                          hexa
pre.radius                                    0.002                                          0.002                                         0.002
pre.relEkStop                                 0.02                                           0.02                                          0.02
pre.relGap                                    0.25                                           0.25                                          0.25
pre.reportFmt                                 /tmp/{tid}.xhtml                               /tmp/{tid}.xhtml                              /tmp/{tid}.xhtml
============================================  =============================================  ============================================  ================================================

Other worksheets contain number series for each single simulation; worksheets are named using ``title`` and ``sceneId`` (e.g. ``dt.7_20130103T231904p19387``)

====== ======================= ==== ====================== ====================== ====== ======= ============= ======================
elast  grav                    i    kinetic                nonviscDamp            plast  relErr  t             total
====== ======================= ==== ====================== ====================== ====== ======= ============= ======================
NaN    NaN                     NaN  NaN                    NaN                    NaN    NaN     NaN           NaN
NaN    0                       0    0                      NaN                    NaN    0       0             0
NaN    -2.42618457156355E-005  10   1.94633917852084E-005  4.85206822100937E-006  NaN    0       0.0025455844  5.39152126996832E-008
NaN    -0.0001024389           20   0.000082005            2.04874799044186E-005  NaN    0       0.0050911688  5.39152127070668E-008
NaN    -0.0002345312           30   0.0001876789           4.6905934128109E-005   NaN    0       0.0076367532  5.39152127214935E-008
NaN    -0.0004205387           40   0.0003364848           8.41074308920784E-005  NaN    0       0.0101823376  5.39152127212767E-008
NaN    -0.0006604614           50   0.000528423            0.000132092            NaN    0       0.0127279221  5.39152126490417E-008
NaN    -0.0009542993           60   0.0007634933           0.0001908596           NaN    0       0.0152735065  5.39152126933856E-008
 ⋮         ⋮                    ⋮           ⋮                     ⋮                ⋮     ⋮            ⋮           ⋮ 
====== ======================= ==== ====================== ====================== ====== ======= ============= ======================


Results hooks
^^^^^^^^^^^^^

Simulations may define their own routines for generating aggregate data; they get called (via ``postHooks`` argument to :obj:`woo.batch.writeResults`) after every write to the databse. Usually, they can produce aggregate figure for the whole batch, as in the horse example:

.. image:: fig/batch-aggregate.*


