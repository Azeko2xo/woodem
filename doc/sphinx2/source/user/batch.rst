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

Let us use the :obj:`woo.pre.horse.FallingHorse` preprocessor as the basis for our example; suppose we want to study the influence of the :obj:`pWaveSafety <woo.pre.horse.FallingHorse.pWaveSafety>` parameter, which will vary between 0.1 to 0.9. This example is shown in `examples/horse-batch <http://bazaar.launchpad.net/~eudoxos/woo/trunk/files/head:/examples/gts-horse/>`_ in the source distribution.

As exaplained above, we need one file with preprocessor and one file describing how to vary preprocessor parameters.

Preprocessor can be saved from the :ref:`user interface <preprocessor_gui>` as text, but any loadable format is acceptable. Text file is the easiest to be modified by hand.
