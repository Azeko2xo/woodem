# coding: utf-8
# 2009 © Václav Šmilauer <eudoxos@arcig.cz>

"""
Functions for accessing woo's internals; only used internally.
"""
import sys
from woo._customConverters import *
from woo import runtime
from woo import config
import woo.core

def childClasses(base,recurse=True,includeBase=False):
	"""Enumerate classes deriving from given base (as string), recursively by default. Returns set."""
	ret=set(woo.master.childClassesNonrecursive(base)); ret2=set();
	if includeBase: ret|=set([base])
	if not recurse: return ret
	for bb in ret:
		ret2|=childClasses(bb)
	return ret | ret2

_allSerializables=[c.__name__ for c in woo.core.Object._derivedCxxClasses]
## set of classes for which the proxies were created
_proxiedClasses=set()

## deprecated classes
# if old class name is used, the new object is constructed and a warning is issued about old name being used
# keep chronologically ordered, oldest first; script/rename-class.py appends at the end
_deprecated={
	### END_RENAMED_CLASSES_LIST ### (do not delete this line; scripts/rename-class.py uses it
}

def updateScripts(scripts):
	## Thanks goes to http://code.activestate.com/recipes/81330-single-pass-multiple-replace/
	from UserDict import UserDict
	import re,os
	class Xlator(UserDict):
		"An all-in-one multiple string substitution class; adapted to match only whole words"
		def _make_regex(self): 
			"Build a regular expression object based on the keys of the current dictionary"
			return re.compile(r"(\b%s\b)" % "|".join(self.keys()))  ## adapted here 
		def __call__(self, mo): 
			"This handler will be invoked for each regex match"
			# Count substitutions
			self.count += 1 # Look-up string
			return self[mo.string[mo.start():mo.end()]]
		def xlat(self, text):
			"Translate text, returns the modified text."
			# Reset substitution counter
			self.count = 0 
			# Process text
			return self._make_regex().sub(self, text)
	# use the _deprecated dictionary for translation, but only when matching on words boundary
	xlator=Xlator(_deprecated)
	if len(scripts)==0: print "No scripts given to --update. Nothing to do."
	for s in scripts:
		if not s.endswith('.py'): raise RuntimeError("Refusing to do --update on file '"+s+"' (not *.py)")
		txt=open(s).read()
		txt2=xlator.xlat(txt)
		if xlator.count==0: print "%s: already up-to-date."%s
		else:
			os.rename(s,s+'~')
			out=open(s,'w'); out.write(txt2); out.close()
			print "%s: %d subtitution%s made, backup in %s~"%(s,xlator.count,'s' if xlator.count>1 else '',s)


def cxxCtorsDict(proxyNamespace=__builtins__):
	"""Return dictionary of class constructors for woo's c++ types, which should be used to update a namespace.

	Root classes are those that are directly wrapped by boost::python. These are only put to the dict.

	Derived classes (from these root classes) are faked by creating a callable which invokes appropriate root class constructor with the derived class parameter and passes remaining arguments to it.

	Classes that are neither root nor derived are exposed via callable object that constructs a Object of given type and passes the parameters.
	"""
	proxyNamespace={}

	# deprecated names
	for oldName in _deprecated.keys():
		class warnWrap:
			def __init__(self,_old,_new):
				# assert(proxyNamespace.has_key(_new))
				self.old,self.new=_old,_new
			def __call__(self,*args,**kw):
				import warnings; warnings.warn("Class `%s' was renamed to (or replaced by) `%s', update your code! (you can run 'woo --update script.py' to do that automatically)"%(self.old,self.new),DeprecationWarning,stacklevel=2);
				return woo.wrapper.__dict__[self.new](*args,**kw)
		proxyNamespace[oldName]=warnWrap(oldName,_deprecated[oldName])
	return proxyNamespace


def setExitHandlers():
	"""Set exit handler to avoid gdb run if log4cxx crashes at exit."""
	# avoid backtrace at regular exit, even if we crash
	if 'log4cxx' in config.features:
		__builtins__['quit']=woo.master.exitNoBacktrace
		sys.exit=woo.master.exitNoBacktrace
	# this seems to be not needed anymore:
	#sys.excepthook=sys.__excepthook__ # apport on ubuntu overrides this, we don't need it


# consistency check
# if there are no serializables, then plugins were not loaded yet, probably
if(len(_allSerializables)==0):
	raise ImportError("No classes deriving from Object found; you must call woo.boot.initialize to load plugins before importing woo.system.")

