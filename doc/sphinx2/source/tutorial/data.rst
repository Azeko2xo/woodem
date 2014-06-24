################
Internal data
################

.. admonition:: Overview

   This chapter explains how to obtain various data from simulation and how to plot them. Using per-simulation labels for things is exaplained, and scene tags are introduced.


Plotting
========

In the previous chapter, a simple simulation of a sphere falling onto the plane was created. The only output so far was the 3d view. Let's see how we can plot some data, such as :math:`z(t)`, i.e. the evolution of the :math:`z`-coordinate of the sphere over time. It consists of two separate tasks:

* Collecting data to plot, since we need to keep history;
* Saying how to plot which of the collected data.


Plot data are stored in :obj:`S.plot.data <woo.core.Plot.data>`, keyed by some unique name, in series ("rows"); data are always added to all "columns" (a `Not a Number <http://en.wikipedia.org/wiki/NaN>`__ is automatically added to rows which are not defined at the time-point).

We can define what to plot by setting :obj:`S.plot.plots <woo.core.Plot.plots>`. It is a `dictionary <http://www.tutorialspoint.com/python/python_dictionary.htm>`__, which is given in the form of ``{key:value,...}``. The ``key`` is the :math:`x`-axis data key, value is a `tuple <http://www.tutorialspoint.com/python/python_tuples.htm>`__. Better give an example, in the following.

Simplified interface
-------------------------

With the simplified interface, expressions to be evaluated are used as plot keys directly::

   S.plot.plots={'S.time': ('S.dem.par[1].pos[2]',)}


This would plot :math:`z`-position (``pos[2]``, as ``pos`` is a vector in 3d) of the 1st particle (the sphere) against time. We need to add an engine which will save the data for us::

   S.engines=S.engines+[PyRunner(10,'S.plot.autoData()')]

:obj:`~woo.core.PyRunner` runs an arbitrary python code :obj:`every n steps <woo.core.PeriodicEngine.stepPeriod>`; so we use previous-defined engines and add the :obj:`~woo.core.PyRunner` to it, which will call :obj:`S.plot.autoData() <woo.core.Plot.autoData>` every 10 steps. :obj:`~woo.core.PyRunner` automatically sets several variables which can be used inside expressions -- one of them is ``S``, the current scene (for which the engine is called).

.. plot::
   :include-source:

   execfile('basic-1.py')                                  # re-use the simple setup for brevity
   S.plot.plots={'S.time': ('S.dem.par[1].pos[2]',)}       # define what to plot
   S.engines=S.engines+[PyRunner(10,'S.plot.autoData()')]  # add the engine
   S.throttle=0                                            # run at full speed now
   S.run(2000,True)                                        # run 2000 steps
   S.plot.plot()                                           # show the plot

Separate :math:`y` axes
''''''''''''''''''''''''

To make the plot prettier, :math:`x`-axis and :math:`y`-axis title can be set using the ``name=expression`` syntax. Multiple variables can be plotted on one :math:`y`-axis; using ``None`` for key will put the rest on the :math:`y_2` (right) :math:`y`-axis. The following will plot :math:`z` positions of both particles on the left and :math:`E_k` (kinetic energy) of the sphere on the right::

   S.plot.plots={'t=S.time': (     # complex expressions can span multiple lines in python :)
      '$z_1$=S.dem.par[1].pos[2]',
      '$z_0$=S.dem.par[0].pos[2]',
      None,                        # remaining things go to the y2 axis
      '$E_k$=S.dem.par[1].Ek'
   )}

.. plot::

   execfile('basic-1.py')
   S.plot.plots={'t=S.time': ('$z_1$=S.dem.par[1].pos[2]','$z_0$=S.dem.par[0].pos[2]',None,'$E_k$=S.dem.par[1].Ek')}
   S.engines=S.engines+[PyRunner(10,'S.plot.autoData()')]
   S.throttle=0
   S.run(2000,True)
   S.plot.plot()

Note that we used math expressions ``$...$`` for symbols in line labels; the syntax is very similar to `LaTeX <http://en.wikipedia.org/LaTeX>`__ and is `documented in detail <http://matplotlib.org/1.3.1/users/mathtext.html>`__ in the underlying plotting library, `Matplotlib <http://matplotlib.org>`__.

.. _data-sphere-with-energy-tracking:

Subfigures
''''''''''

The :obj:`S.plot.plots <woo.core.Plot.plots>` dictionary can contain multiple keys; each of them then creates a new sub-figure.

There is a limited support for specifying style of individual lines by using ``(key,format)`` instead of ``key``, where format is Matlab-like syntax explained in detail `here <http://matplotlib.org/1.3.1/api/pyplot_api.html#matplotlib.pyplot.plot>`__; it consists of color and line/marker abbreviation such as ``'g--'`` specifying dashed green line.

This example also demonstrates that ``**expression`` denotes a dictionary-like object which contains values to be plotted (this syntax is similar to `keyword argument syntax <https://docs.python.org/2/tutorial/controlflow.html#keyword-arguments>`__ in Python); this feature is probably only useful with :obj:`energy tracking <woo.core.EnergyTracker>` where keys are not known a-priori.

We add another figure showing energies on the left; :obj:`energy total <woo.core.EnergyTracker.total>` (":math:`\sum` energy") should be zero, as energy should be conserved, and :obj:`relative error <woo.core.EnergyTracker.relErr>` will be plotted on the right::

   S.plot.plots={
      't=S.time':(
         '$z_1$=S.dem.par[1].pos[2]',
         '$z_0$=S.dem.par[0].pos[2]',
       ),
       ' t=S.time':(                                 # note the space before 't': explained below
         '**S.energy',                               # plot all energy fields on the first axis
         r'$\sum$ energy=S.energy.total()',          # compute total energy
         None,                                       # put the rest on the y2 axis
         ('relative error=S.energy.relErr()','g--')  # plot relative error of energy measures, dashed green
       )
   }

   # energy is not tracked by default, it must be turned on explicitly
   S.trackEnergy=True
   
.. note:: We had to prepend space to the second :math:`x`-axis in " t=S.time"; otherwise, the second value would overwrite the first one, as dictionary must have unique keys. Spaces are stripped away, so they don't appear in the output.

.. plot::

   execfile('data-plot-energy.py')
   S.run(2000,True)
   S.plot.plot()
   import pylab
   pylab.gcf().set_size_inches(8,12)    # make higher so that it is readable
   pylab.gcf().subplots_adjust(left=.1,right=.95,bottom=.05,top=.95) # make more compact

Complex interface
------------------

The complex interface requires one to assign keys to data using :obj:`S.plot.addData <woo.core.Plot.addData>`. The advantage is that arbitrary computations can be performed on the data before they are stored. One usually creates a function to store the data in this way::

   # use PyRunner to call our function periodically
   S.engines=S.engines+[PyRunner(5,'myAddPlotData(S)')]
   # define function which will be called by the PyRunner above
   def myAddPlotData(S):
      ## do any computations here
      uf=woo.utils.unbalancedForce(S)
      ## store the data, use any keys you want
      S.plot.addData(t=S.time,i=S.step,z1=S.dem.par[1].pos[1],Ek=S.dem.par[1].Ek,unbalanced=uf)
   # specify plots as usual
   S.plot.plots={'t':('z1',None,('Ek','r.-')),'i':('unbalanced',)}

.. plot::
   :context:

   execfile('basic-1.py')
   S.engines=S.engines+[PyRunner(5,'myAddPlotData(S)')]
   # define function which will be called by the PyRunner above
   def myAddPlotData(S):
      uf=woo.utils.unbalancedForce(S)
      S.plot.addData(t=S.time,i=S.step,z1=S.dem.par[1].pos[1],Ek=S.dem.par[1].Ek,unbalanced=uf)
   ## hack to make this possible when generating docs
   __builtins__['myAddPlotData']=myAddPlotData
   S.plot.plots={'t':('z1',None,('Ek','r.-')),'i':('unbalanced',)}
   S.throttle=0
   S.run(2000,True)
   S.plot.saveDataTxt('basic-1.data.txt') # for the example below
   S.plot.plot()


Save plot data
---------------

Collected data can be saved to file using :obj:`S.plot.saveDataTxt <woo.core.Plot.saveDataTxt>`. This function creates (optionally compressed) text file with numbers written in columns; it can be readily loaded into `NumPy <http://numpy.org>`__ via `numpy.genfromtxt <http://docs.scipy.org/doc/numpy/reference/generated/numpy.genfromtxt.html>`__ or into a spreadsheet program::

   S.plot.saveDataTxt('basic-1.data.txt')

produces (only the beginning of the file is shown):

.. literalinclude:: basic-1.data.txt
   :linenos:
   :lines: 1-15


Labels
======

When plotting, we were accessing the sphere using the ``S.dem.par[1]`` notation; that is not convenient. For this reason, there exists a special place for putting labeled :obj:`objects <woo.core.Object>`, ``S.lab`` (it is a per-scene isntance of the :obj:`woo.core.LabelMapper` object) which can then be accessed by readable names.

Engines, when they have :obj:`~woo.core.Engine.label` set in the constructor (as is the case with :obj:`woo.dem.DemField.minimalEngines`), are added to ``S.lab`` automatically, other objects are to be added manually as needed.

.. ipython::
   
   @suppress
   Woo [1]: from woo.core import *; from woo.dem import *; import woo

   Woo [1]: S=Scene(engines=[PyRunner(10,'print "Step number %d"%S.step',label='foo')],fields=[DemField()])

   Woo [1]: S.engines 

   Woo [1]: S.lab.foo            # the same engine accessed using the label
 
   Woo [1]: S.lab.foo.command    # its properties can be accessed

   Woo [1]: s=Sphere.make((0,1,2),1)    # create a new spherical particle

   Woo [1]: S.lab.sphere=s              # give it the label "sphere"

   Woo [1]: S.dem.par.append(s)         # add it to the simulation

   Woo [1]: S.dem.par[0]==S.lab.sphere  # it is one and the same particle

   Woo [1]: S.dem.par[0].pos[2], S.lab.sphere.pos[2]  # the z-position can be accessed both ways


.. _tutorial-tags:

Tags
=====

Each :obj:`~woo.core.Scene` defines :obj:`~woo.core.Scene.tags` of which some are filled-in automatically. It is a dictionary-like object. Some of the useful tags are ``id`` (unique identifier of the simulation, composed of time, date and process number) and ``title`` (by default empty, but used in batch simulations), ``tid`` (title+id concatenated, for readability and uniqueness).

.. ipython::

   Woo [1]: S.tags.keys()

   Woo [1]: S.tags['isoTime']

   Woo [1]: S.tags['idt']

   Woo [1]: S.tags['id']

   Woo [1]: S.tags['whatever']='whatever you want'

   Woo [1]: S.tags['whatever']

Tags can be embedded in some parameters in ``{}`` and are expanded by :obj:`woo.core.Scene.expandTags` (e.g. :obj:`VtkExport.out <woo.dem.VtkExport.out>` can contain ``{tid}``, ensuring output data will not overwrite each other):

.. ipython::
  
   Woo [1]: S.expandTags('/this/is/some/filename-{tid}')

