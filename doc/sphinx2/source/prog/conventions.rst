Conventions
=============


Style
-----

* Tabs for indentation, compact formatting.
* Avoid spurious spaces, like before parentheses, except in complex expressions.
* Comment so that code functionality is visible, don't comment when the code says it better than the comment.
* Use ``struct`` instead of ``class`` (everything public by default).
* Start header files with ``#pragma once``.
* Headers are ``.hpp``, implementations are ``.cpp``.
* Include all headers using ``#include<woo/pkg/dem/Particle.hpp>``, do **not** use ``#include"Particle.hpp"``.
* Do not say **using namespace**, not in headers and not in implementation files.
* Put only short methods inline in headers, when important for performance.
* Mark const arguments with ``const``.
* Pass by const reference as much as possible.
* Mark methods as ``const`` if they can be.
* When overriding virtual function, mark it with ``WOO_CXX11_OVERRIDE`` (expands to the ``override`` keyword for compilers which support it).
* Put assertions in the code, they expand to nothing in non-debug builds.
* Be defensive; for user-called functions, check that arguments make sense, raise exception if they don't.
* Document everything.
* Use logging macros as necessary.

Types
-------

Many types are declared in :woosrc:`lib/base/Types.hpp`. Those include types imported from ``std`` or ``boost`` namespaces (like ``shared_ptr``, ``make_shared``, ``vector``, ``string``, ``to_string``, ``cout``, ``cerr``, ``min``, ``max``, ``isnan`` and many others). A shorthand for ``boost::python`` is defined, namely ``py`` (so one can use ``py::extract`` and such).

Be conservative with memory usage, so unless there is a reason for that, use ``int`` rather than ``long``. For floating-point numbers, Woo uses the type ``Real`` everywhere, which is typedef for ``double`` (by default, that is).

Math types
"""""""""""

`Eigen <http://eigen.tuxfamily.org>`__ is our math library. Frequently used types are ``typedef``'d with the ``r`` suffix for ``Real`` and ``i`` for ``int``:

* vectors with ``Real`` scalars: ``Vector2r``, ``Vector3r``, ``Vector6r``, ``VectorXr`` (dynamic-sized)
* vectors with ``int`` scalars: ``Vector2i``, ``Vector3i``, ``Vector6i``
* matrices with ``Real`` scalars:: ``Matrix3r`` (3×3), ``Matrix6r`` (6×6), ``MatrixXr`` (dynamic-sized)
* aligned boxes ``AlignedBox2r``, ``AlignedBox3r``, ``AlignedBox2i``, ``AlignedBox3i``
* quaternions ``Quaternionr``, angle-axis rotation ``AngleAxisr``

Two importants constants are ``NaN`` and ``Inf``. For :math:`\pi`, use plain old ``M_PI``.

Feature macros
--------------

Woo has a number of features which can be enabled/disabled at compile-time. Those are passed as ``features=...`` argument to scons, can be retrived from python as :obj:`woo.config.features`` and are available in c++ as macros ``WOO_``, e.g. ``WOO_OPENGL``. Note that builds without some feature must still compile and work, so any feature-specific code must be guarded by macros, e.g. for OpenGL::

   #ifdef WOO_OPENGL
      /* ... */
   #endif

Non-exhaustive list of features is:

* ``WOO_DEBUG``: defined in debug builds.
* ``WOO_OPENGL``: support for OpenGL 3d graphics; disabled on headless (cluster) builds.
* ``WOO_OPENCL``: support for OpenCL; experimental only.
* ``WOO_QT4``: support for Qt4 user interface.
* ``WOO_VTK``: have headers and libs of `Visualization ToolKit <http://vtk.org>`__.
* ``WOO_GTS``: have headers and libs of `GNU Triangulated Surface <http://gts.sf.net>`__. Those are very useful for working with facets, don't disable.

Namespaces
-----------

Woo is at the moment not very consistent putting everything into the ``woo`` namespace. As Woo is dypically used as python module, the organization into Python modules (which is independent) is more important.

Logging
--------

Logging macros are for helping development or diagnostics, defined in :woosrc:`lib/base/Logging.hpp`. The logging backed is currently `log4cxx <http://log4cxx.apache.org>`__, but this may change in the future. Logging is controlled per-class. Any class wishing to use logging should declare logger within the class body in the header file using ``WOO_DECL_LOGGER`` and do additional setup (allocation) in the implementation file using ``WOO_IMPL_LOGGER(Rabbit)`` at the top-level scope.

The lower macros are ignored in non-debug builds:

* `LOG_TRACE`, `LOG_DEBUG`, `LOG_INFO`: increasing severity levels. Tracing is for low-level debugging with possibly copious output, debug is more severe. Info level should provide manageable amount of information even when globally enabled.

* `LOG_WARN` is used for warning about non-fatal conditions.
* `LOG_ERROR` informs about erroneous condition; if the condition likely caused by the user, it may be better to throw an exception instead, since that allows for better diagnostics.
* `LOG_FATAL` informs about a fatal condition, and is often followed by crash or an exception being thrown.

Renaming classes
-----------------

Sometimes, classes are to be renamed for consistency. This may pose some problems with backwards-compatibility. For this reason, there is :obj:`woo._monkey.aliases._deprecated`: old name may still be used, but there will be a waning issued, along with the location of the offending code.


Unit tests
----------

Make sure your modifications don't break existing code. There is a (rather limited) unit test suite which should be always run after modifications; it is launched with ``woo --test``.

When adding new functionality, adding a unit test for it is the best way to keep the interface stable (other people will see if they break it immediately).
