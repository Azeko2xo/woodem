'''Module for manipulating locally-stored object library'''

import os, os.path
from os.path import join

import woo
import woo._monkey.io
import logging
libDir=woo.master.confDir+'/library' # must NOT include the trailing slash
libObjs=None

def refresh():
	'Load or refresh the library.'
	objs={}
	for root,dirs,files in os.walk(libDir):
		for f in files:
			ext=os.path.splitext(f)[-1].lower()
			ff=os.path.join(root,f)
			assert ff.startswith(libDir+'/')
			libPath=ff[len(libDir)+1:]
			if ext=='xls':
				raise NotImplementedError('Reading library objects from XLS not yet implemented.')
				# load multiple objects from the XLS file
				for key,val in loadFromXLS(ff):
					objs[tuple((libPath+'/'+key).split('/'))]=val
			else:
				try:
					obj=woo._monkey.io.Object_load(None,ff)
					objs[tuple(libPath.split('/'))]=obj
				except:
					logging.warn('Loading library object from %s failed (skipped):\n\n'%ff)
					import traceback
					traceback.print_exc()
	global libObjs
	libObjs=objs

def ensureLoaded():
	global libObjs
	if libObjs==None: refresh()

def checkout(types=None):
	'Return library objects matching given *types*. If *types* is ``None``, return all objects. Types can be a type or a 1-tuple with one type, in which case lists of those instances are matched.'
	ensureLoaded()
	global libObjs
	import copy
	if types==None: return copy.deepcopy(libObjs)
	if isinstance(types,list) or isinstance(types,tuple): seqType=types[0]
	else: seqType=None
	seqObj=(seqType and isinstance(seqType(),woo.core.Object))
	ret={}
	for key,val in libObjs.items():
		if seqType==None:
			if isinstance(val,types): ret[key]=val
		else:
			if isinstance(val,woo.core.Object): continue # sequence required, object found
			# perhaps sequences on both sides
			# print seqType,type(val)
			try:
				for item in val:
					if not seqObj:
						# print key,item,seqType.__name__
						try: seqType(item) # attempt conversion (like (0,1) to Vector2
						except: raise TypeError # catch-all exception
					else:
						if not isinstance(item,seqType): raise TypeError
				ret[key]=val
			except TypeError: pass # non-iterable types
	return ret

