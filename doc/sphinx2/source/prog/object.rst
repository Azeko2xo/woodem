woo::Object
============

All objects in Woo derive from the :obj:`woo.core.Object` (``woo::Object``) class. This base object automatically provides, together with some additional scaffolding described below, basic services:

1. Serialization and deserialization -- all objects can be saved and loaded again.
2. Information for showing each object in the GUI via attribute traits.
3. Exposing c++ classes to Python via boost::python.
4. Elegant type-casting and RTTI methods (``cast`` and ``isA`` templates).
5. Some introspection capabilities, used mainly for dispatching (class index), and fully available from Python.
6. Guarantess that all attributes are initialized (as long as they provide an initialization value


``WOO_DECL__`` and ``WOO_IMPL__`` macro family
-----------------------------------------------

The following macros are the standard (and the only supported) way to declare an objects deriving from ``woo::Object``. The complexity is a necessary compromise for two contradicting requirements:

1. Have all the important data in one place, for programmer's convenience. Not half in header and half in implementation file.
2. Put only as little data as possible into the header file, so that compilation only ``#include``'ing header is not slowed down and takes less RAM (boost::python's templates are rather memory-hungry).

Therefore, a macro is defined, which contains all the data necessary, and is passed to ``WOO_DECL__`` in the header and ``WOO_IMPL__`` in the implementation file.

The macros is conventionally named as e.g. ``woo_zoo_Rabbit__...``, where ``zoo`` is module (e.g. ``core``, ``dem`` or other). The ``...`` describe the comma-separated argument types in the macro:

* ``woo_zoo_Rabbit__CLASS_BASE_DOC`` for class name, base class name, and class docstring (exposed in Python); these three are mandatory, additional fields are added as needed,
* ``woo_zoo_Rabbit__CLASS_BASE_DOC_ATTRS`` adds attribute specifications (described below)
* ``woo_zoo_Rabbit__CLASS_BASE_DOC_ATTRS_PY`` adds python initialization block
* ``woo_zoo_Rabbit__CLASS_BASE_DOC_ATTRS_CTOR_PY`` allows to insert things into the default constructor (rarely needed)
* ``woo_zoo_Rabbit__CLASS_BASE_DOC_ATTRS_INI_CTOR_PY`` allows additionally to specify initializers for other variables (seldom needed)

Note that the arguments must be separated by literal comma (``,``), and no other free commas (not enclosed by parentheses ``()``) are allowed (this is a limitation of the C preprocessor).

A simple class declarations may look like this in the hypothetical ``pkg/zoo/Animal.hpp`` file:

.. code-block:: c++

   struct Animal: public Object{
      #define woo_zoo_Animal__CLASS_BASE_DOC \
         Animal,Object,"Any animal"
      WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_zoo_Animal__CLASS_BASE_DOC);
   };
   WOO_REGISTER_OBJECT(Animal); // must be out of the body

   struct Rabbit: public Animal{
      /* macro is defined on multiple lines, joined by trailing backslash */
      #define woo_zoo_Rabbit__CLASS_BASE_DOC_ATTRS_PY \
         Rabbit,Animal,"This is a rabbit class, blah blah.", \
         ((int,legs,4,,"Number of legs")) \
         ((Real,age,0,,"Age of the rabbit"))

     WOO_DECL__CLASS_BASE_DOC_ATTRS_PY(woo_zoo_Rabbit__CLASS_BASE_DOC_ATTRS_PY);
   };
   WOO_REGISTER_OBJECT(Rabbit);

And in the ``.cpp`` file:

.. code-block:: c++

   #include<woo/pkg/zoo/Animal.hpp>
   // all exported classes; may be used several times in the same file with different modules/classes
   WOO_PLUGIN(zoo,(Animal)(Rabbit)); 
   WOO_IMPL__CLASS_BASE_DOC_ATTRS(woo_zoo_Animal__CLASS_BASE_DOC);
   WOO_IMPL__CLASS_BASE_DOC_ATTRS(woo_zoo_Rabbit__CLASS_BASE_DOC_ATTRS);

Other macros
-------------
There are two more macros which *must* be used with every new class:

* ``WOO_REGISTER_OBJECT(Rabbit);`` in the header file, after the class declaration and *outside* of the class body; this is necessary for proper support of ``boost::serialization``;
* ``WOO_PLUGIN(zoo,(Animal)(Rabbit));`` in the implementation file; internally, this macro puts the classes to the given module (``woo.zoo`` in this example), and completes the serialization support. It may appear more than once in one implementation file (though not with the same classes).


Attributes
-----------

The ``ATTR`` part of the ``WOO_DECL__`` macros is a list of ``((`` and ``))`` enclosed 5-tuples describing each attribute (memeber variable, in strict c++ terminology). The components of the tuple are as follows:

1. c++ type of the attribute; it is subject to the following:

   * The type must not contain comma due to macro processing; if you need something like ``std::map<int,int>``, use ``typedef`` inside the class body to declare a shorthand for your type which does not contain one.
   * Unless the attribute is declared with ``Attr::noSave`` (see below), it must be type which ``boost::serialization`` knows how to serialize. Most useful types (``int``, ``Real``, Eigen vectors and matrices, ``std::vector``, ``std::map``, ``boost::multiarray``, ``shared_ptr`` to anything deriving from ``woo::Object``, … are handled just fine)
   * The type should have a converter for python (unless declared with ``Attr::hidden``, see below); if it does not have converter, compilation will succeed, but accessing the attribute from python will raise ``TypeError``.

2. Attribute name; must be a valid c++ (and python) identifier. The name is, of course, case-sensitive.

3. Default value; if not given (blank), the attribute will be default-initialized. It is strongly recommended to initialize all attributes, to catch any possible problems coming from initialization not being done.

4. Attribute traits (discussed below)

5. Docstring for the attribute, which will show up in the automatically generated documentation, is also accessible with ``?`` from IPython prompt, and is shown as tooltip in the UI. Use `Sphinx formatting <http://sphinx-doc.org/rest.html>`__.

Attribute traits
------------------

Attribute traits is piece of information statically attached in to every attribute; that means, all instances of the attribute share the trait. The trait is defined e.g. in the following way::

   AttrTrait<Attr::noSave|Attr::triggerPostLoad>().noGui().pyByRef()

The ``Attr::noSave|Attr::triggerPostLoad`` is template parameter neede for compile-time switches. If there are no flags, the template still must be spelled out, such as in ``AttrTrait<>().noGui()``. The trailing methods, which can be chained as they return reference to the attribute trait instance, contain information which is not used until runtime. Traits are fully described by their source in :woosrc:`lib/object/AttrTrait.hpp`. An overview of those which are used the most follows.

Compile-time flags
"""""""""""""""""""
These flags are given as template argument to `AttrTrait<...>()`, combined with the ``|`` (bit-wise OR) operator if needed.

* ``Attr::noSave``: the attribute will not be saved, when saving via ``boost::serialization``. After loading a saved instance, it will keep the default value. This is useful for types not supported by ``boost::serialization`` or for saving the amount of data saved if they are redundant and can be reconstructed after the class is loaded. 

* ``Attr::triggerPostLoad``: when the attribute is modified from Python, the ``postLoad(...)`` function is called, with the address of the member variable passed as the second argument.

* ``Attr::hidden``: attribute not exposed to Python at all.

* ``Attr::namedEnum``: signals that the attribute will be exposed as named enumeration (more on that below); this must be complemented by ``.namedEnum(...)`` specified on the trait, defining which are the admissible values and their names.

Runtime traits
""""""""""""""

* ``.noDump()``: do not dump this attribute when serializing to formats without ``boost::serialization`` formats, such as JSON, Python expression, Pickle, HTML and others. Useful to avoid large nested structures inflating those representations with useless data.

* ``.pyByRef()``: expose the attribute by reference to Python. The default is to expose by-value, with the exception of ``Eigen``'s matrices and vectors (see ``py_wrap_ref`` template in :woosrc:`lib/object/Object.hpp`). ``shared_ptr`` exposed by-value in reality exposes reference to the object. This attribute is only rarely useful.

* ``.readonly()``: the attribute is no writable from python; this includes passing the value to the constructor.

* ``.namedEnum({{1,{"one","eins","just one"}},{0,{"zero","null","nothing"}}})``: in conjunction with the ``Attr::namedEnum`` flag described above, define string aliases for integral values, using initializer list. The first string is the preferred name (always returned) while the other ones are aliases, which can be also used. Assigning the integral value is still possible.

THe following traits influence how is the attribute displayed in the GUI:

* ``.noGui()``: do not show this attribute in the GUI, even though it is exposed to Python.

* ``.rgbColor()``: used for ``Vector3r`` attributes with the meaning of color; color picker icon will be displayed in the UI, for picking color visually.

* ``.filename()``, ``.existingFilename()``, ``.dirname()``: in the UI, show picker for (possibly non-existing) filename, existing filename, and directory, respectively.

* ``.angleUnit()``, ``.timeUnit()``, ``.lenUnit()``, ``.areaUnit()``, ``.volUnit()``, ``.velUnit()``, ``.accelUnit()``, ``.massUnit()``, ``.angVelUnit()``, ``.angMomUnit()``, ``.inertiaUnit()``, ``.forceUnit()``, ``.torqueUnit()``, ``.pressureUnit()``, ``.stiffnessUnit()``, ``.massRateUnit()``, ``.densityUnit()``, ``.fractionUnit()``, ``.surfEnergyUnit()``: self-explanatory unit specification for given attribute. The value is **always internally stored in base unit** (usually the SI), but the UI may scale the quantity and present it in a readable way. The units are accessible from the ``woo.unit`` dicionary, e.g. as ``woo.unit['t/h']``, which contains the multiplier.

* ``.multiUnit()``: for 2d arrays (such as list of ``Vector2r``), use different units for every column; this must be followed by unit traits; e.g. PSD points declared as ``vector<Vector2r>`` may say ``.multiUnit().lenUnit().fractionUnit()``.

* ``.range(Vector2i(a,b))``, ``.range(Vector2r(a,b))``: show UI slider for integral/float values between ``a`` and ``b``.

* ``.choice({1,2,3})``: UI choice from integral values

* ``.bits({"bit 0","bit 1"})``: create checkboxes for individual bits of an integer attribute (starting from the least significant, i.e. the rightmost bit)

* ``.hideIf("self.foo=='bar'")``: the attribute will be dynamically hidden from the UI if the expression given evaluates to ``True``; ``self`` is the current instance.

* ``.startGroup("Name")``: start attribute group named ``Name``, shown as collapsible group of attributes, and also shown in the documentation as an attribute group.

* ``.buttons({"button label","python command to be executed","label",...})``: create button(s) in the UI, where each triplet specifies one button; it will be created after the attribute itself by default, which can be changed by setting the second optional argument ``showBefore`` to ``true``.

