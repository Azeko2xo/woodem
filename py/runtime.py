# this module is populated at initialization from the c++ part of PythonUI
"""Runtime variables, populated at woo startup."""
# default value
hasDisplay=None 
ipython_version=0

# find out about which ipython version we use -- 0.10* and 0.11 are supported, but they have different internals
import IPython
try: # attempt to get numerical version
	if IPython.__version__.startswith('1.0'): ipython_version=100
	elif IPython.__version__.startswith('1.1'): ipython_version=110
	else: ipython_version=int(IPython.__version__.split('.',2)[1]) ## convert '0.10' to 10, '0.11.alpha1.bzr.r1223' to 11
except ValueError:
	print 'WARN: unable to extract IPython version from %s, defaulting to 0.13'%(IPython.__version__)
	ipython_version=13
if ipython_version not in (10,11,12,13,100,110): # versions that we are able to handle, round up or down correspondingly
	newipver=10 if ipython_version<10 else 12
	print 'WARN: unhandled IPython version %d.%d, assuming %d.%d instead.'%(ipython_version%100,ipython_version//100,newipver%100,newipver//100)
	ipython_version=newipver


