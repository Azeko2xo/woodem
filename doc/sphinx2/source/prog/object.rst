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

The macros is conventionally named as e.g. ``woo_zoo_Rabbit__...``, where ``dem`` is module (e.g. ``core`` or other, not just ``dem``). The ``...`` describe the comma-separated arguments in the macro:

* ``woo_zoo_Rabbit__CLASS_BASE_DOC`` for class name, base class name, and class docstring (exposed in Python); these three are mandatory, additional fields are added as needed,
* ``woo_zoo_Rabbit__CLASS_BASE_DOC_ATTRS`` adds attribute specifications (described below)
* ``woo_zoo_Rabbit__CLASS_BASE_DOC_ATTRS_PY`` adds python initialization block
* ``woo_zoo_Rabbit__CLASS_BASE_DOC_ATTRS_CTOR_PY`` allows to insert things into the default constructor (rarely needed)
* ``woo_zoo_Rabbit__CLASS_BASE_DOC_ATTRS_INI_CTOR_PY`` allows additionally to specify initializers for other variables (seldom needed)

A simple class declarations may look like this in the ``.hpp`` file::

   struct Animal: public Object{
      #define woo_zoo_Animal__CLASS_BASE_DOC \
         Animal,Object,"Any animal"
      WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_zoo_Animal__CLASS_BASE_DOC);
   };
   WOO_REGISTER_OBJECT(Animal); // must be out of the body

   struct Rabbit: public Animal{
      /* macro is defined on multiple lines, joined by trailing backslash */
      #define woo_zoo_Rabbit__CLASS_BASE_DOC_ATTRS \
         Rabbit,Animal,"This is a rabbit class, blah blah.", \
         ((int,legs,4,,"Number of legs")) \
         ((Real,age,0,,"Age of the rabbit"))

     WOO_DECL__CLASS_BASE_DOC_ATTRS(woo_zoo_Rabbit__CLASS_BASE_DOC_ATTRS);
   };
   WOO_REGISTER_OBJECT(Rabbit);

And in the ``.cpp`` file::

   // all exported classes; may be used several times in the same file with different modules/classes
   WOO_PLUGIN(dem,(Animal)(Rabbit)); 
   WOO_IMPL__CLASS_BASE_DOC_ATTRS(woo_zoo_Animal__CLASS_BASE_DOC);
   WOO_IMPL__CLASS_BASE_DOC_ATTRS(woo_zoo_Rabbit__CLASS_BASE_DOC_ATTRS);

