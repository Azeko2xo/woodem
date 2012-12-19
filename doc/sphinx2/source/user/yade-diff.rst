*****************************
Woo for seasoned Yade users
*****************************

Woo is an evolution of `Yade <http://www.launchpad.net/yade>`_ and inherits some of its code and much of its ideas. We attempt to list key differences, which should help you get started with Woo, if you already have a good experience with Yade.

Code differences
================
#. Woo is a python-importable module. You can do ``import woo`` in any Python script.
#. Written in ``c++11``.
#. Supports additional :ref`serialization formats <serializationFormats>`, some of them human-readable.
#. Properly organized in python modules: :obj:`woo.core`, :obj:`woo.dem` and so on.
#. Python's standard setuptools can be used to build Woo, both under Windows and Linux.
#. Windows installer is provided.

Simulation differences
=======================
#. Many features in Yade are not newly implemented, such as contact models.
#. Each particle is defined via :obj:`Node` (or several nodes), which can be shared among particles.
#. :obj:`woo.dem.DemField` is only one of fields, which take part in the simulation; each :obj:`woo.core.Engine` can operate on several fields, if desired.
#. Contacts define local coordinates (via contact node), and their geometry is handles uniformly.
#. Unlimited number of :obj:`woo.core.Scene` objects, which can run in parallel. The one manipulated with the UI is always ``woo.master.scene``.

User interface
==============
#. Rendering code is moved away from :obj:`woo.gl.Renderer` to appropriate functors.
#. Nodes define :obj:`woo.gl.NodeGlRep` for their own representation in OpenGL.
#. Enhanced inspection and manipulation of objects through the UI -- choices, bitfields, arrays, hiding attributes.
#. Pure-python classes deriving from :obj:`woo.core.Object`, including user-iterface.
#. Simulations (preprocessors) can define their custom UI for given problem.

Batch processing
=================
#. XLS format supported at the input side.
#. Results from each job can be saved to sqlite database, and are automatically converted to XLS file.
#. Report-generation facilites.
