# module for setting woo import options
# before woo itself is imported
#
# in the future, it could be used to select a particular woo version, for instance
# see http://bytes.com/topic/python/answers/42887-how-pass-parameter-when-importing-module
#
forceNoGui=False
ompThreads=0
ompCores=[]
flavor=''
debug=False
clDev=None
quirks=3
quirkIntel,quirkFirePro=1,2

def flavorFromArgv0(argv0,batch=False):
	import re
	m=re.match('.*/woo(-[a-zA-Z_-]*|)'+('-batch' if batch else ''),argv0)
	if m:
		if m.group(1)=='': return ''
		return m.group(1)[1:] # strip leading dash

def useKnownArgs(argv):
	global forceNoGui,ompThreads,ompCores,flavor,debug,clDev,quirks

	import argparse, sys
	par=argparse.ArgumentParser(prog='wooOptions module')
	par.add_argument('-n',help="Run without graphical interface (equivalent to unsetting the DISPLAY environment variable)",dest='nogui',action='store_true')
	par.add_argument('-j','--threads',help='Number of OpenMP threads to run; defaults to 1. Equivalent to setting OMP_NUM_THREADS environment variable.',dest='threads',type=int)
	par.add_argument('--cores',help='Set number of OpenMP threads (as --threads) and in addition set affinity of threads to the cores given.',dest='cores')
	par.add_argument('-D','--debug',help='Run the debug build, if available.',dest='debug',action='store_true')
	par.add_argument('--cl-dev',help='Numerical couple (comma-separated) givin OpenCL platform/device indices. This is machine-dependent value.',dest='clDev')
	par.add_argument('--quirks',help='Bitmask for workarounds for broken configurations; all quirks are enabled by default. 1: set LIBGL_ALWAYS_SOFTWARE=1 for Intel GPUs (determined from `lspci | grep VGA`) (avoids GPU freeze), 2: set --in-gdb when on AMD FirePro GPUs to avoid crash in fglrx.so',dest='quirks',type=int,default=3)
	par.add_argument('--flavor',help='Build flavor of woo to use.',type=str,default=flavorFromArgv0(sys.argv[0]))

	opts,unhandled=par.parse_known_args(argv)


	if opts.nogui: forceNoGui=True
	if opts.cores:
		if opts.threads: warnings.warn('--threads ignored, since --cores specified.')
		try:
			ompCores=[int(i) for i in opts.cores.split(',')]
		except ValueError:
			raise ValueError('Invalid --cores specification %s, should be a comma-separated list of non-negative integers'%opts.cores)
		ompThreads=len(cores)
	if opts.threads: ompThreads=opts.threads
	if opts.debug: debug=True
	if opts.clDev: clDev=opts.clDev
	quirks=opts.quirks
	flavor=opts.flavor

#os.environ['GOMP_CPU_AFFINITY']=' '.join([str(cores[0])]+[str(c) for c in cores])
#os.environ['OMP_NUM_THREADS']=str(len(cores))
#elif opts.threads: os.environ['OMP_NUM_THREADS']=str(opts.threads)
#else: os.environ['OMP_NUM_THREADS']='1'





