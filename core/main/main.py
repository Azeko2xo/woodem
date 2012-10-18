#!/usr/bin/python
# encoding: utf-8
# syntax:python

import sys,os,os.path,time,re
import wooOptions

# handle command-line options first
import argparse
par=argparse.ArgumentParser(prog=os.path.basename(sys.argv[0]),description="Woo: open-source platform for dynamic compuations, http://woodem.eu.")
par.add_argument('--version',action='store_true',)
#
# those are actually parsed by wooOptions
#
par.add_argument('-j','--threads',help='Number of OpenMP threads to run; defaults to 1. Equivalent to setting OMP_NUM_THREADS environment variable.',dest='threads',type=int)
par.add_argument('--cores',help='Set number of OpenMP threads (as --threads) and in addition set affinity of threads to the cores given.',dest='cores')
par.add_argument('--cl-dev',help='Numerical couple (comma-separated) givin OpenCL platform/device indices. This is machine-dependent value',dest='clDev')
par.add_argument('-n',help="Run without graphical interface (equivalent to unsetting the DISPLAY environment variable)",dest='nogui',action='store_true')
par.add_argument('-D','--debug',help='Run the debug build, if available.',dest='debug',action='store_true')
# quirks set flags in wooOptions
par.add_argument('--quirks',help='Bitmask for workarounds for broken configurations; all quirks are enabled by default. 1: set LIBGL_ALWAYS_SOFTWARE=1 for Intel GPUs (determined from `lspci | grep VGA`) (avoids GPU freeze), 2: set --in-gdb when on AMD FirePro GPUs to avoid crash in fglrx.so',dest='quirks',type=int,default=3)
par.add_argument('--flavor',help='Build flavor of woo to use.',type=str,default=wooOptions.flavorFromArgv0(sys.argv[0]))
#
# end wooOptions parse
#
par.add_argument('-c',help='Run these python commands after the start (use -x to exit afterwards)',dest='commands',metavar='COMMANDS')
par.add_argument('--update',help='Update deprecated class names in given script(s) using text search & replace. Changed files will be backed up with ~ suffix. Exit when done without running any simulation.',dest='updateScripts',action='store_true')
par.add_argument('--paused',help='When proprocessor or simulation is given on the command-line, don\'t run it automatically (default)',action='store_true')
par.add_argument('--nice',help='Increase nice level (i.e. decrease priority) by given number.',dest='nice',type=int)
par.add_argument('-x',help='Exit when the script finishes',dest='exitAfter',action='store_true')
par.add_argument('-v',help='Increase logging verbosity; first occurence sets default logging level to info (only available if built with log4cxx), second to debug, third to trace.',action='count',dest='verbosity')
par.add_argument('--generate-manpage',help="Generate man page documenting this program and exit",dest='manpage',metavar='FILE')
par.add_argument('-R','--rebuild',help="Re-run build in the source directory, then run the updated woo with the same command line except --rebuild. The build flavor for this build and its stored parameters will be used. If given twice, update from the repository will be attempted before recompilation.",dest='rebuild',action='count')
par.add_argument('--test',help="Run regression test suite and exit; the exists status is 0 if all tests pass, 1 if a test fails and 2 for an unspecified exception.",dest="test",action='store_true')
#par.add_argument('--performance',help='Starts a test to measure the productivity',dest='performance',action='store_true')
par.add_argument('--no-gdb',help='Do not show backtrace when Woo crashes (only effective with \-\-debug).',dest='noGdb',action='store_true',)
par.add_argument('--in-gdb',help='Run Woo inside gdb (must be in $PATH).',dest='inGdb',action='store_true')
par.add_argument('--in-pdb',help='Run Woo inside pdb',dest='inPdb',action='store_true')
par.add_argument('--in-valgrind',help='Run inside valgrind (must be in $PATH); automatically adds python ignore files',dest='inValgrind',action='store_true')
par.add_argument('simulation',nargs=argparse.REMAINDER)
#par.disable_interspersed_args()
opts=par.parse_args()
args=opts.simulation

wooOptions.useKnownArgs(sys.argv)


# show version and exit
if opts.version:
	wooOptions.quirks=0
	import woo.config
	print '%s (%s)'%(woo.config.prettyVersion(),','.join(woo.config.features))
	sys.exit(0)
# re-build woo so that the binary is up-to-date
if opts.rebuild:
	wooOptions.quirks=0
	import subprocess, woo.config
	if opts.rebuild>1:
		cmd=['bzr','up',woo.config.sourceRoot]
		print 'Updating Woo using ',' '.join(cmd)
		if subprocess.call(cmd): raise RuntimeError('Error updating Woo from bzr repository.')
	# rebuild
	cmd=['scons','-Q','-C',woo.config.sourceRoot,'flavor=%s!'%woo.config.flavor,'debug=%d'%(1 if woo.config.debug else 0),'execCheck=%s'%(os.path.abspath(sys.argv[0]))]
	print 'Rebuilding Woo using',' '.join(cmd)
	if subprocess.call(cmd): raise RuntimeError('Error rebuilding Woo (--rebuild).')
	# run ourselves
	if '--rebuild' in sys.argv:
		argv=[v for v in sys.argv if v!='--rebuild']
	else:
		argv=[(v.replace('R','') if re.match('^-[^-]*',v) else v) for v in sys.argv if v!='-R' and v!='-RR']
	print 'Running Woo using',' '.join(argv)
	sys.exit(subprocess.call(argv))
# QUIRK running in gdb
if wooOptions.quirks & wooOptions.quirkFirePro and (not wooOptions.forceNoGui and 'DISPLAY' in os.environ) and not opts.inGdb:
	vgas=os.popen("LC_ALL=C lspci | grep VGA").readlines()
	if len(vgas)==1 and 'FirePro' in vgas[0]:
		print 'AMD FirePro GPU detected, will run inside gdb to avoid crash in buggy fglrx.so.'
		opts.inGdb=True
		# disable quirk to avoid infinite loop
		sys.argv=[sys.argv[0]]+['--quirks=%d'%(wooOptions.quirks&(~wooOptions.quirkFirePro))]+[a for a in sys.argv[1:] if not a.startswith('--quirks')]
# re-run inside gdb
if opts.inGdb:
	import tempfile, subprocess
	gdbBatch=tempfile.NamedTemporaryFile()
	args=["'"+arg.replace("'",r"\'")+"'" for arg in sys.argv if arg!='--in-gdb']
	gdbBatch.write('set pagination off\nrun '+' '.join(args)+'\n')
	gdbBatch.flush()
	if opts.debug:
		print 'Spawning: gdb -x '+gdbBatch.name+' '+sys.executable
	sys.exit(subprocess.call(['gdb']+([] if opts.debug else ['-batch-silent'])+['-x',gdbBatch.name,sys.executable]))
if opts.inValgrind:
	import subprocess,urllib,os.path
	saveTo='/tmp/valgrind-python.supp'
	if not os.path.exists(saveTo):
		print 'Downloading '+saveTo
		urllib.urlretrieve('http://svn.python.org/projects/python/trunk/Misc/valgrind-python.supp',saveTo)
	else:
		print 'Using alread-downloaded '+saveTo
	args=['valgrind','--suppressions='+saveTo,sys.executable]+[a for a in sys.argv if a!='--in-valgrind']
	print 'Running',' '.join(args)
	sys.exit(subprocess.call(args))

# run this instance inside pdb
if opts.inPdb:
	import pdb
	pdb.set_trace()
# run regression test suite and exit
if opts.test:
	import woo.tests
	woo.tests.testAll(sysExit=True)

# c++ boot code checks for WOO_DEBUG at some places; debug verbosity is equivalent
# do this early, to have debug messages in the boot code (plugin registration etc)
#print opts.verbosity,type(opts.verbosity)
if opts.verbosity and opts.verbosity>1: os.environ['WOO_DEBUG']='1'


# initialization and c++ plugins import
import woo
# other parts we will need soon
import woo.config
sys.stderr.write('Welcome to Woo '+woo.config.prettyVersion()+'%s\n'%(' (debug build)' if woo.config.debug else ''))
import woo.log
import woo.system
import woo.runtime

# continue option processing

if opts.updateScripts:
	woo.system.updateScripts(args)
	sys.exit(0)
if opts.manpage:
	import woo.manpage
	woo.manpage.generate_manpage(par,woo.config.metadata,opts.manpage,section=1,seealso='woo%s-batch (1)'%suffix)
	print 'Manual page %s generated.'%opts.manpage
	sys.exit(0)
if opts.nice:
	os.nice(opts.nice)
if opts.noGdb:
	woo.master.disableGdb()
if 'log4cxx' in woo.config.features and opts.verbosity:
	woo.log.setLevel('',[woo.log.INFO,woo.log.DEBUG,woo.log.TRACE][min(opts.verbosity,2)])
if 'opencl' in woo.config.features and opts.clDev:
	woo.master.defaultClDev=clDev


## modify sys.argv in-place so that it can be handled by userSession
woo.runtime.origArgv=sys.argv[:]
sys.argv=woo.runtime.argv=args
woo.runtime.opts=opts

# be helpful when not overridden
def onSelection(o):
	print "You selected an object. Define your own useful *onSelection(obj)* function to process it."

from math import *

def userSession(qt4=False,qapp=None,qtConsole=False):
	# prepare nice namespace for users
	import woo, woo.runtime
	import sys, traceback
	# start non-blocking qt4 app here; need to ask on the mailing list on how to make it functional
	# with ipython >0.11, start the even loop early (impossible with 0.10, which is thread-based)
	# http://mail.scipy.org/pipermail/ipython-user/2011-July/007931.html mentions this
	if False and qt4 and woo.runtime.ipython_version>=11:
		import IPython.lib.guisupport
		IPython.lib.guisupport.start_event_loop_qt4()
	if len(sys.argv)>0:
		arg0=sys.argv[0]
		if qt4: woo.qt.Controller();
		if arg0.endswith('.py'):
			# the script is reponsible for consuming all other arguments
			def runScript(script):
				sys.stderr.write("Running script "+arg0+'\n')
				try:
					execfile(script,globals())
				except SystemExit: raise
				except: # all other exceptions
					traceback.print_exc()
					if woo.runtime.opts.exitAfter: sys.exit(1)
				if woo.runtime.opts.exitAfter: sys.exit(0)
			global opts
			__opts=opts
			runScript(arg0)
			opts=__opts # in case the script uses opts as well :| (ugly)
		else:
			if len(sys.argv)>1: raise RuntimeError('Extra arguments to file to load: '+' '.join(sys.argv[1:]))
			# attempt to load this file
			try:
				obj=woo.core.Object.load(arg0)
			except:
				traceback.print_exc()
				print 'Error loading file (wrong format?)',arg0
				sys.exit(1)
			if isinstance(obj,woo.core.Scene):
				sys.stderr.write("Running simulation "+arg0+'\n')
				woo.master.scene=obj
			elif isinstance(obj,woo.core.Preprocessor):
				sys.stderr.write("Using preprocessor "+arg0+'\n')
				import woo.batch
				woo.master.scene=woo.batch.runPreprocessor(obj,arg0)
			else:
				print 'ERROR: Object loaded from "%s" is a %s (must be Scene or a Preprocessor)'%(arg0,type(obj).__name__)
				sys.exit(1)
			if not opts.paused:
				woo.master.scene.run() # run the new simulation
				if woo.runtime.opts.exitAfter: woo.master.scene.wait() 
	if woo.runtime.opts.commands:
		exec(woo.runtime.opts.commands) in globals()
	if woo.runtime.opts.exitAfter: sys.exit(0)
	# common ipython configuration
	banner='[[ ^L clears screen, ^U kills line. '+', '.join((['F12 controller','F11 3d view','F10 both','F9 generator'] if (qt4) else [])+['F8 plot'])+'. ]]'
	ipconfig=dict( # ipython options, see e.g. http://www.cv.nrao.edu/~rreid/casa/tips/ipy_user_conf.py
		prompt_in1='Woo [\#]: ',
		prompt_in2='    .\D.: ',
		prompt_out=" -> [\#]: ",
		separate_in='',separate_out='',separate_out2='',
		readline_parse_and_bind=[
			'tab: complete',
			# only with the gui; the escape codes might not work on non-linux terminals.
			]
			+(['"\e[24~": "\C-Uwoo.qt.Controller();\C-M"','"\e[23~": "\C-Uwoo.qt.View();\C-M"','"\e[21~": "\C-Uwoo.qt.Controller(), woo.qt.View();\C-M"','"\e[20~": "\C-Uwoo.qt.Generator();\C-M"'] if (qt4) else []) #F12,F11,F10,F9
			+['"\e[19~": "\C-Uimport woo.plot; woo.plot.plot();\C-M"', #F8
				'"\e[A": history-search-backward', '"\e[B": history-search-forward', # incremental history forward/backward
		]
	)

	# show python console
	# handle both ipython 0.10 and 0.11 (incompatible API)
	# print 'ipython version', woo.runtime.ipython_version
	if woo.runtime.ipython_version==10:
		from IPython.Shell import IPShellEmbed
		ipshell=IPShellEmbed(banner=banner,rc_override=ipconfig)
		ipshell()
		# save history -- a workaround for atexit handlers not being run (why?)
		# http://lists.ipython.scipy.org/pipermail/ipython-user/2008-September/005839.html
		import IPython.ipapi
		IPython.ipapi.get().IP.atexit_operations()
	elif woo.runtime.ipython_version in (11,12,13):
		if qtConsole:
			qapp.start()
		else:
			from IPython.frontend.terminal.embed import InteractiveShellEmbed
			# use the dict to set attributes
			ipconfig['banner1']=banner+'\n' # called banner1 in >=0.11, not banner as in 0.10
			for k in ipconfig: setattr(InteractiveShellEmbed,k,ipconfig[k])
			ipshell=InteractiveShellEmbed()
			if woo.runtime.ipython_version==12:
				ipshell.prompt_manager.in_template= 'Woo [\#]: '
				ipshell.prompt_manager.in2_template='    .\D.: '
				ipshell.prompt_manager.out_template=' -> [\#]: '
			ipshell()
			# similar to the workaround, as for 0.10 (perhaps not needed?)
			ipshell.atexit_operations()
	else: raise RuntimeError("Unhandled ipython version %d"%woo.runtime.ipython_version)

# avoid warnings from ipython import (http://stackoverflow.com/a/3924047/761090)
import warnings
warnings.filterwarnings('ignore',category=DeprecationWarning,module='IPython.frontend.terminal.embed')
warnings.filterwarnings('ignore',category=DeprecationWarning,module='IPython.utils.io')
warnings.filterwarnings('ignore',category=DeprecationWarning,module='Xlib')


#
# select gui to use
#
try:
	import woo.qt # this import fails if running with -n or $DISPLAY can't be connected to
	gui='qt4'
except ImportError:
	gui=None

# run remote access things, before actually starting the user session
from woo import remote
woo.remote.useQThread=(gui=='qt4')
woo.remote.runServers()

# for scripts
from woo import *
from math import *
import woo.plot # monkey-patches, which require woo.runtime.hasDisplay (hence not importable from _monkey :/)

if gui==None:
	userSession()
elif gui=='qt4':
	## we already tested that DISPLAY is available and can be opened
	### otherwise Qt4 might crash at this point
	#import woo.qt # this handles all Qt imports
	userSession(qt4=True,qapp=woo.qt.wooQApp,qtConsole=woo.qt.useQtConsole)

woo.master.exitNoBacktrace()

