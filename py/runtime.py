# this module is populated at initialization from the c++ part of PythonUI
"""Runtime variables, populated at woo startup."""
# default value
import wooMain
hasDisplay=None
if wooMain.options.fakeDisplay: hasDisplay=False # we would crash really

# cache the value returned
_ipython_version=None

def ipython_version():
	# find out about which ipython version we use -- 0.10* and 0.11 are supported, but they have different internals
	global _ipython_version
	if not _ipython_version==None: return _ipython_version
	# workaround around bug in IPython 0.13 (and perhaps later): frozen install does not define sys.argv
	import sys
	fakedArgv=False
	if hasattr(sys,'frozen') and len(sys.argv)==0:
		sys.argv=['wwoo']
		fakedArgv=True
	import IPython
	try: # attempt to get numerical version
		if IPython.__version__.startswith('1.0'): ret=100
		elif IPython.__version__.startswith('1.1'): ret=110
		elif IPython.__version__.startswith('1.2'): ret=120
		else: ret=int(IPython.__version__.split('.',2)[1]) ## convert '0.10' to 10, '0.11.alpha1.bzr.r1223' to 11
	except ValueError:
		print 'WARN: unable to extract IPython version from %s, defaulting to 0.13'%(IPython.__version__)
		ret=13
	if ret not in (10,11,12,13,100,110,120): # versions that we are able to handle, round up or down correspondingly
		newipver=10 if ret<10 else 110
		print 'WARN: unhandled IPython version %d.%d, assuming %d.%d instead.'%(ret%100,ret//100,newipver%100,newipver//100)
		ret=newipver
	_ipython_version=ret
	if fakedArgv: sys.argv=[]
	return ret


