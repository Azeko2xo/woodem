# encoding: utf-8

__all__=['main','batch']

import sys, os
WIN=(sys.platform=='win32')


# pyInstaller provides its own site module, which does not define _Helper and perhaps others
# this avoids warning from IPython about the function missing
# TODO: report this as bug to pyInstaller
import site
if hasattr(sys,'frozen') and '_Helper' not in dir(site):
	class _Helper(object):
		def __repr__(self): return "Type help() for interactive help, or help(object) for help about object."
		def __call__(self, *args, **kwds):
			import pydoc; return pydoc.help(*args, **kwds)
	site._Helper=_Helper



def makeColorFuncs(colors,dumbWinColors=False):
	try:
		import colorama
		# work around http://code.google.com/p/colorama/issues/detail?id=16
		# not under Windows, or under Windows in dumb "terminal" (cmd.exe), where autodetection works
		if not WIN: colorama.init()
		# windows with proper terminal emulator
		elif 'TERM' in os.environ: colorama.init(autoreset=True,convert=False,strip=False)
		# dumb windows terminal - no colors for our messages, but IPython prompts is colorized properly
		else:
			if dumbWinColors: colorama.init() # used for batch output
			else: raise ImportError # this would break ipython prompt completely
		ret=[]
		for c in colors:
			if c in ('BRIGHT',): ret+=[lambda s,c=c: getattr(colorama.Style,c)+s+colorama.Style.RESET_ALL]
			else: ret+=[lambda s,c=c: getattr(colorama.Fore,c)+s+colorama.Fore.RESET]
		return ret
	except ImportError:
		return [lambda s: s]*len(colors)


def main(sysArgv=None):
	'''Function called by the woo executable.
	*sysArgv* (if specified) replaces sys.argv; sys.argv is used for option processing.
	'''
	import sys,os,os.path,time,re
	import wooOptions
	if sysArgv: sys.argv=sysArgv
	
	green,red,yellow,bright=makeColorFuncs(['GREEN','RED','YELLOW','BRIGHT'])

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
	par.add_argument('-e',help='Evaluate this expression (instead of loading file). It should be a scene object or a preprocessor, which will be run',dest='expression',metavar='EXPR')
	# par.add_argument('--update',help='Update deprecated class names in given script(s) using text search & replace. Changed files will be backed up with ~ suffix. Exit when done without running any simulation.',dest='updateScripts',action='store_true')
	par.add_argument('--paused',help='When proprocessor or simulation is given on the command-line, don\'t run it automatically (default)',action='store_true')
	par.add_argument('--nice',help='Increase nice level (i.e. decrease priority) by given number.',dest='nice',type=int)
	par.add_argument('-x',help='Exit when the script finishes',dest='exitAfter',action='store_true')
	par.add_argument('-v',help='Increase logging verbosity; first occurence sets default logging level to info (only available if built with log4cxx), second to debug, third to trace.',action='count',dest='verbosity')
	# par.add_argument('--generate-manpage',help="Generate man page documenting this program and exit",dest='manpage',metavar='FILE')
	if not WIN: par.add_argument('-R','--rebuild',help="Re-run build in the source directory, then run the updated woo with the same command line except --rebuild. The build flavor for this build and its stored parameters will be used. If given twice, update from the repository will be attempted before recompilation.",dest='rebuild',action='count')
	par.add_argument('--test',help="Run regression test suite and exit; the exists status is 0 if all tests pass, 1 if a test fails and 2 for an unspecified exception.",dest="test",action='store_true')
	par.add_argument('--no-gdb',help='Do not show backtrace when Woo crashes (only effective with \-\-debug).',dest='noGdb',action='store_true',)
	par.add_argument('--in-gdb',help='Run Woo inside gdb (must be in $PATH).',dest='inGdb',action='store_true')
	par.add_argument('--in-pdb',help='Run Woo inside pdb',dest='inPdb',action='store_true')
	par.add_argument('--in-valgrind',help='Run inside valgrind (must be in $PATH); automatically adds python ignore files',dest='inValgrind',action='store_true')
	par.add_argument('simulation',nargs=argparse.REMAINDER)
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
	if not WIN and opts.rebuild:
		wooOptions.quirks=0
		import subprocess, woo.config, glob
		if not woo.config.sourceRoot: raise RuntimeError('This build does not define woo.config.sourceRoot (packaged version?)')
		if opts.rebuild>1:
			cmd=['bzr','up',woo.config.sourceRoot]
			print 'Updating Woo using ',' '.join(cmd)
			if subprocess.call(cmd): raise RuntimeError('Error updating Woo from bzr repository.')
			# find updatable dirs in wooExtra
			for d in glob.glob(woo.config.sourceRoot+'/wooExtra/*')+glob.glob(woo.config.sourceRoot+'/wooExtra/*/*'):
				dd=os.path.abspath(d)
				if not os.path.isdir(dd): continue
				if os.path.exists(dd+'/.bzr'):
					print 'Updating '+dd
					if subprocess.call(['bzr','up',dd]): raise RuntimeError('Error updating %d from bzr.')
		# rebuild
		# FIXME: should be use opts.debug or wooOptions.debug here??
		cmd=['scons','-Q','-C',woo.config.sourceRoot,'flavor=%s!'%woo.config.flavor,'debug=%d'%(1 if opts.debug else 0),'execCheck=%s'%(os.path.abspath(sys.argv[0]))]
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
		if sum(['FirePro' in vga for vga in vgas]):
			print 'AMD FirePro GPU detected, will run inside gdb to avoid crash in buggy fglrx.so.'
			opts.inGdb=True
			# disable quirk to avoid infinite loop
			sys.argv=[sys.argv[0]]+['--quirks=%d'%(wooOptions.quirks&(~wooOptions.quirkFirePro))]+[a for a in sys.argv[1:] if not a.startswith('--quirks')]
	# re-run inside gdb
	if opts.inGdb:
		import tempfile, subprocess
		# windows can't open opened file, don't delete it then
		# Linux: delete afterwards, since gdb can open it in the meantime without problem
		gdbBatch=tempfile.NamedTemporaryFile(prefix='woo-gdb-',delete=(not WIN))
		sep='' if WIN else "'" # gdb seems to read files differently on windows??
		args=[sep+arg.replace("'",r"\'").replace('\\','/')+sep for arg in sys.argv if arg!='--in-gdb']
		gdbBatch.write('set pagination off\nrun '+' '.join(args)+'\n')
		if WIN: gdbBatch.close()
		else: gdbBatch.flush()  
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
	## run regression test suite and exit
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
	sys.stderr.write(green('Welcome to Woo '+woo.config.prettyVersion()+'%s%s\n'%(' (debug build)' if woo.config.debug else '',(', flavor '+woo.config.flavor if woo.config.flavor else ''))))
	import woo.log
	import woo.system
	import woo.runtime

	# continue option processing

	#if opts.updateScripts:
	#	woo.system.updateScripts(args)
	#	sys.exit(0)
	#if opts.manpage:
	#	import woo.manpage
	# woo.manpage.generate_manpage(par,woo.config.metadata,opts.manpage,section=1,seealso='woo%s-batch (1)'%suffix)
	#	print 'Manual page %s generated.'%opts.manpage
	#	sys.exit(0)
	if opts.nice:
		if WIN:
			try:
				import psutil
				if opts.nice<-10: wcl=psutil.HIGH_PRIORITY_PRIORITY_CLASS
				elif opts.nice<0: wcl=psutil.ABOVE_NORMAL_PRIORITY_CLASS
				elif opts.nice==0: wcl=psutil.NORMAL_PRIORITY_CLASS
				elif opts.nice<10: wcl=psutil.BELOW_NORMAL_PRIORITY_CLASS
				else: wcl=psutil.IDLE_PRIORITY_CLASS
				psutil.Process(os.getpid()).nice=wcl
			except ImportError:
				logging.warn('Nice value %d ignored, since module psutil could not be imported (Windows only)'%opts.nice)
		else: os.nice(opts.nice)


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

	#from math import *

	# avoid warnings from ipython import (http://stackoverflow.com/a/3924047/761090)
	import warnings
	warnings.filterwarnings('ignore',category=DeprecationWarning,module='IPython.frontend.terminal.embed')
	warnings.filterwarnings('ignore',category=DeprecationWarning,module='IPython.utils.io')

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
	#from woo import *
	#from math import *
	import woo.plot # monkey-patches, which require woo.runtime.hasDisplay (hence not importable from _monkey :/)

	if gui==None:
		ipythonSession(opts)
	elif gui=='qt4':
		## we already tested that DISPLAY is available and can be opened
		### otherwise Qt4 might crash at this point
		#import woo.qt # this handles all Qt imports
		ipythonSession(opts,qt4=True,qapp=woo.qt.wooQApp,qtConsole=woo.qt.useQtConsole)
	#woo.master.exitNoBacktrace()
	# uninstall crash handlers
	woo.master.disableGdb()
	return 0


def ipythonSession(opts,qt4=False,qapp=None,qtConsole=False):
	# prepare nice namespace for users
	import woo, woo.runtime, woo.config, wooExtra
	import sys, traceback, site
	#
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
	if woo.runtime.opts.expression:
		obj=eval(opts.expression)
		if isinstance(obj,woo.core.Scene): woo.master.scene=obj
		elif isinstance(obj,woo.core.Preprocessor): woo.master.scene=woo.batch.runPreprocessor(obj)
		else: raise ValueError('Expression given with -e must be a Scene or a Preprocessor, not a %s'%type(obj))
	if woo.runtime.opts.commands:
		exec(woo.runtime.opts.commands) in globals()
	if woo.runtime.opts.exitAfter: sys.exit(0)
	# common ipython configuration
	banner='[[ ^L clears screen, ^U kills line. '+', '.join(['F12 controller']+(['F11 3d view','F10 both'] if 'opengl' in woo.config.features else [])+(['F9 generator'] if (qt4) else [])+['F8 plot'])+'. ]]'
	ipconfig=dict( # ipython options, see e.g. http://www.cv.nrao.edu/~rreid/casa/tips/ipy_user_conf.py
		prompt_in1='Woo [\#]: ',
		prompt_in2='    .\D.: ',
		prompt_out=" -> [\#]: ",
		separate_in='',separate_out='',separate_out2='',
		readline_parse_and_bind=[
			'tab: complete',
			# only with the gui; the escape codes might not work on non-linux terminals.
			]
			+(['"\e[24~": "\C-Uwoo.qt.Controller();\C-M"']+(['"\e[23~": "\C-Uwoo.qt.View();\C-M"','"\e[21~": "\C-Uwoo.qt.Controller(), woo.qt.View();\C-M"'] if 'opengl' in woo.config.features else [])+['"\e[20~": "\C-Uwoo.qt.Generator();\C-M"'] if (qt4) else []) # F12,F11,F10,F9
			+['"\e[19~": "\C-Uimport woo.plot; woo.plot.plot();\C-M"', #F8
				'"\e[A": history-search-backward', '"\e[B": history-search-forward', # incremental history forward/backward
		]
	)
	# shortcuts don't really work under windows, show controller directly in that case
	if qt4 and WIN: woo.qt.Controller()

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




def batch(sysArgv=None):
	import os, sys, thread, time, logging, pipes, socket, xmlrpclib, re, shutil, random, os.path
	if sysArgv: sys.argv=sysArgv
	
	green,red,yellow,bright=makeColorFuncs(['GREEN','RED','YELLOW','BRIGHT'],dumbWinColors=True)
		
	#socket.setdefaulttimeout(10) 
	
	import wooOptions
	wooOptions.forceNoGui=True
	wooOptions.debug=False
	wooOptions.quirks=0
	import woo, woo.batch, woo.config, woo.remote
	
	match=re.match(r'(.*)[_-]batch(-script\.py|.exe)?$', sys.argv[0])
	if not match:
		print sys.argv
		raise RuntimeError(r'Batch executable "%s"does not match ".*[_-]batch(-script\.py)?"'%sys.argv[0])
	executable=match.group(1)
	print executable
	#re.sub('-batch(|.bat|.py)?$','\\1',sys.argv[0])
	
	
	class JobInfo():
		def __init__(self,num,id,command,hrefCommand,log,nCores,script,table,lineNo,affinity,resultsDb,debug,executable,nice):
			self.started,self.finished,self.duration,self.durationSec,self.exitStatus=None,None,None,None,None # duration is a string, durationSec is a number
			self.command=command; self.hrefCommand=hrefCommand; self.num=num; self.log=log; self.id=id; self.nCores=nCores; self.cores=set(); self.infoSocket=None
			self.script=script; self.table=table; self.lineNo=lineNo; self.affinity=affinity;
			self.resultsDb=resultsDb; self.debug=debug; self.executable=executable; self.nice=nice
			self.hasXmlrpc=False
			self.status='PENDING'
			self.threadNum=None
			self.winBatch=None
			self.plotsLastUpdate,self.plotsFile=0.,woo.master.tmpFilename()+'.'+woo.remote.plotImgFormat
			self.hrefCommand='WOO_BATCH=<a href="jobs/{self.num}/table">{self.table}:{self.lineNo}:{self.resultsDb}</a> {self.executable} <a href="jobs/{self.num}/script">{self.script}</a> &gt; <a href="jobs/{self.num}/log">{self.log}</a>'.format(self=self)
			if WIN:
				self.hrefCommand='<a href="jobs/{self.num}/winbatch">.bat</a>: '.format(self=self)+self.hrefCommand
		def prepareToRun(self):
			'Assemble command to run as needed'
			import pipes
			if WIN:
				self.winBatch=woo.master.tmpFilename()+'.bat'
				batch='set OMP_NUM_THREADS=%d\n'%job.nCores
				batch+='set WOO_BATCH={self.table}:{self.lineNo}:{self.resultsDb}\n'.format(self=self)
				#batch+='start /B /WAIT /LOW '
				batch+='{self.executable} -x -n {self.script} {debugFlag} > {self.log} 2>&1 \n'.format(self=self,debugFlag=('-D' if self.debug else ''))
				sys.stderr.write(batch)
				f=open(self.winBatch,'w')
				f.write(batch)
				f.close()
				self.command=self.winBatch
				self.hrefCommand+='<br><tt><a href="jobs/{self.num}/winbatch">{self.command}</a></tt>'.format(self=self)
			else:
				ompOpt,niceOpt,debugOpt='','',''
				if self.cores: ompOpt='--cores=%s'%(','.join(str(c) for c in self.cores))
				elif self.nCores: ompOpt='--threads=%d'%(self.nCores)
				if self.nice: niceOpt='--nice=%d'%self.nice
				if self.debug: debugOpt='--debug'
				self.command='WOO_BATCH={self.table}:{self.lineNo}:{self.resultsDb} {self.executable} -x -n {ompOpt} {niceOpt} {debugOpt} {self.script} > {logFile} 2>&1'.format(self=self,ompOpt=ompOpt,niceOpt=niceOpt,debugOpt=debugOpt,logFile=pipes.quote(self.log))
				self.hrefCommand+='<br><tt>'+self.command+'</tt>'
			
		def saveInfo(self):
			log=file(self.log,'a')
			log.write("""
=================== JOB SUMMARY ================
id      : %s
status  : %d (%s)
duration: %s
command : %s
started : %s
finished: %s
"""%(self.id,self.exitStatus,'OK' if self.exitStatus==0 else 'FAILED',self.duration,self.command,time.asctime(time.localtime(self.started)),time.asctime(time.localtime(self.finished))));
			log.close()
		def ensureXmlrpc(self):
			'Attempt to establish xmlrpc connection to the job (if it does not exist yet). If the connection could not be established, as magic line was not found in the log, return False.'
			if self.hasXmlrpc: return True
			try:
				for l in open(self.log,'r'):
					if not l.startswith('XMLRPC info provider on'): continue
					url=l[:-1].split()[4]
					self.xmlrpcConn=xmlrpclib.ServerProxy(url,allow_none=True)
					self.hasXmlrpc=True
					return True
			except IOError: pass
			if not self.hasXmlrpc: return False # catches the case where the magic line is not in the log yet
		def getInfoDict(self):
			if self.status!='RUNNING': return None
			if not self.ensureXmlrpc(): return None
			try: return self.xmlrpcConn.basicInfo()
			except: print 'Error getting simulation information via XMLRPC'
		def updatePlots(self):
			#global opts
			if self.status!='RUNNING': return
			if not self.ensureXmlrpc(): return
			if time.time()-self.plotsLastUpdate<opts.plotTimeout: return
			self.plotsLastUpdate=time.time()
			img=None
			try: img=self.xmlrpcConn.plot()
			except: print 'Error getting plot via XMLRPC'
			if not img:
				if os.path.exists(self.plotsFile): os.remove(self.plotsFile)
				return
			f=open(self.plotsFile,'wb')
			f.write(img.data)
			f.close()
			# print woo.remote.plotImgFormat,'(%d bytes) written to %s'%(os.path.getsize(self.plotsFile),self.plotsFile)
	
		def htmlStats(self):
			ret='<tr>'
			ret+='<td>%s</td>'%self.id
			if self.status=='PENDING': ret+='<td bgcolor="grey">(pending)</td>'
			elif self.status=='RUNNING': ret+='<td bgcolor="yellow">%s</td>'%t2hhmmss(time.time()-self.started)
			elif self.status=='DONE': ret+='<td bgcolor="%s">%s</td>'%('lime' if self.exitStatus==0 else 'red',self.duration)
			info=self.getInfoDict()
			self.updatePlots() # checks for last update time
			if info:
				runningTime=time.time()-self.started
				ret+='<td>'
				if info['stopAtStep']>0:
					ret+='<nobr>%2.2f%% done</nobr><br/><nobr>step %d/%d</nobr>'%(info['step']*100./info['stopAtStep'],info['step'],info['stopAtStep'])
					finishTime=str(time.ctime(time.time()+int(round(info['stopAtStep']/(info['step']/runningTime)))))
					ret+='<br/><font size="1"><nobr>%s finishes</nobr></font><br/>'%finishTime
				else: ret+='<nobr>step %d</nobr>'%(info['step'])
				if runningTime!=0: ret+='<br/><nobr>avg %g/sec</nobr>'%(info['step']/runningTime)
				ret+='<br/><nobr>%d particles</nobr><br/><nobr>%d contacts</nobr>'%(info['numBodies'],info['numIntrs'])
				if info['title']: ret+='<br/>title <i>%s</i>'%info['title']
				ret+='</td>'
			else:
				ret+='<td> (no info) </td>'
			ret+='<td>%d%s</td>'%(self.nCores,(' ('+','.join([str(c) for c in self.cores])+')') if self.cores and self.status=='RUNNING' else '')
			# TODO: make clickable so that it can be served full-size
			if os.path.exists(self.plotsFile):
				if 0: pass
					## all this mess to embed SVG; clicking on it does not work, though
					## question posted at http://www.abclinuxu.cz/poradna/linux/show/314041
					## see also http://www.w3schools.com/svg/svg_inhtml.asp and http://dutzend.blogspot.com/2010/04/svg-im-anklickbar-machen.html
					#img='<object data="/jobs/%d/plots" type="%s" width="300px" alt="[plots]"/>'%(self.num,woo.remote.plotImgMimetype)
					#img='<iframe src="/jobs/%d/plots" type="%s" width="300px" alt="[plots]"/>'%(self.num,woo.remote.plotImgMimetype)
					#img='<embed src="/jobs/%d/plots" type="%s" width="300px" alt="[plots]"/>'%(self.num,woo.remote.plotImgMimetype)a
				img='<img src="/jobs/%d/plots" width="300px" alt="[plots]">'%(self.num)
				ret+='<td><a href="/jobs/%d/plots">%s</a></td>'%(self.num,img)
			else: ret+='<td> (no plots) </td>'
			ret+='<td>%s</td>'%self.hrefCommand
			ret+='</tr>'
			return ret
	def t2hhmmss(dt): return '%02d:%02d:%02d'%(dt//3600,(dt%3600)//60,(dt%60))
	
	def totalRunningTime():
		tt0,tt1=[j.started for j in jobs if j.started],[j.finished for j in jobs if j.finished]+[time.time()]
		# it is safe to suppose that 
		if len(tt0)==0: return 0 # no job has been started at all
		return max(tt1)-min(tt0)
	
	def globalHtmlStats():
		t0=min([0]+[j.started for j in jobs if j.started!=None])
		unfinished=len([j for j in jobs if j.status!='DONE'])
		nUsedCores=sum([j.nCores for j in jobs if j.status=='RUNNING'])
		# global maxJobs
		if unfinished:
			ret='<p>Running for %s, since %s.</p>'%(t2hhmmss(totalRunningTime()),time.ctime(t0))
		else:
			failed=len([j for j in jobs if j.exitStatus!=0])
			lastFinished=max([j.finished for j in jobs])
			# FIXME: do not report sum of runnign time of all jobs, only the real timespan
			ret='<p><span style="background-color:%s">Finished</span>, idle for %s, running time %s since %s.</p>'%('red' if failed else 'lime',t2hhmmss(time.time()-lastFinished),t2hhmmss(sum([j.finished-j.started for j in jobs if j.started is not None])),time.ctime(t0))
		ret+='<p>Pid %d'%(os.getpid())
		if opts.globalLog: ret+=', log <a href="/log">%s</a>'%(opts.globalLog)
		ret+='</p>'
		allCores,busyCores=set(range(0,maxJobs)),set().union(*(j.cores for j in jobs if j.status=='RUNNING'))
		ret+='<p>%d cores available, %d used + %d free.</p>'%(maxJobs,nUsedCores,maxJobs-nUsedCores)
		# show busy and free cores; gives nonsense if not all jobs have CPU affinity set
		# '([%s] = [%s] + [%s])'%(','.join([str(c) for c in allCores]),','.join([str(c) for c in busyCores]),','.join([str(c) for s in (allCores-busyCores)]))
		ret+='<h3>Jobs</h3>'
		nFailed=len([j for j in jobs if j.status=='DONE' and j.exitStatus!=0])
		ret+='<p><b>%d</b> total, <b>%d</b> <span style="background-color:yellow">running</span>, <b>%d</b> <span style="background-color:lime">done</span>%s</p>'%(len(jobs),len([j for j in jobs if j.status=='RUNNING']), len([j for j in jobs if j.status=='DONE']),' (<b>%d <span style="background-color:red"><b>failed</b></span>)'%nFailed if nFailed>0 else '')
		return ret
	
	from BaseHTTPServer import BaseHTTPRequestHandler,HTTPServer
	import socket,re,SocketServer
	class HttpStatsServer(SocketServer.ThreadingMixIn,BaseHTTPRequestHandler):
		favicon=None # binary favicon, created when first requested
		def do_GET(self):
			if not self.path or self.path=='/': self.sendGlobal()
			else:
				if self.path=='/favicon.ico':
					if not self.__class__.favicon:
						import base64
						self.__class__.favicon=base64.b64decode(woo.remote.b64favicon)
					self.sendHttp(self.__class__.favicon,contentType='image/vnd.microsoft.icon')
					return
				elif self.path=='/log' and opts.globalLog:
					self.sendTextFile(opts.globalLog,refresh=opts.refresh)
					return
				jobMatch=re.match('/jobs/([0-9]+)/(.*)',self.path)
				if not jobMatch:
					self.send_error(404,self.path); return
				jobId=int(jobMatch.group(1))
				if jobId>=len(jobs):
					self.send_error(404,self.path); return
				job=jobs[jobId]
				rest=jobMatch.group(2)
				if rest=='plots':
					job.updatePlots() # internally checks for last update time
					self.sendFile(job.plotsFile,contentType=woo.remote.plotImgMimetype,refresh=(0 if job.status=='DONE' else opts.refresh))
				elif rest=='log':
					if not os.path.exists(job.log):
						self.send_error(404,self.path); return
					## once we authenticate properly, send the whole file
					## self.sendTextFile(jobs[jobId].log,refresh=opts.refresh)
					## now we have to filter away the cookie
					cookieRemoved=False; data=''
					for l in open(job.log):
						if not cookieRemoved and re.match('^.*TCP python prompt on .*, auth cookie.*$',l):
							ii=l.find('auth cookie `'); l=l[:ii+13]+'******'+l[ii+19:]; cookieRemoved=True
						data+=l
					self.sendHttp(data,contentType='text/plain;charset=utf-8;',refresh=(0 if job.status=='DONE' else opts.refresh))
				elif rest=='script':
					self.sendPygmentizedFile(job.script,linenostep=5)
				elif rest=='table':
					if not job.table: return
					if job.table.endswith('.xls'): self.sendFile(job.table,contentType='application/vnd.ms-excel')
					else: self.sendPygmentizedFile(job.table,hl_lines=[job.lineNo],linenostep=1)
				elif rest=='winbatch':
					if not job.winBatch: return
					self.sendPygmentizedFile(job.winBatch)
				else: self.send_error(404,self.path)
			return
		def log_request(self,req): pass
		def sendGlobal(self):
			html='<HTML><TITLE>Woo-batch at %s overview</TITLE><BODY>\n'%(socket.gethostname())
			html+=globalHtmlStats()
			html+='<TABLE border=1><tr><th>id</th><th>status</th><th>info</th><th>cores</th><th>plots</th><th>command</th></tr>\n'
			for j in jobs: html+=j.htmlStats()+'\n'
			html+='</TABLE></BODY></HTML>'
			self.sendHttp(html,contentType='text/html',refresh=opts.refresh) # refresh sent as header
		def sendTextFile(self,fileName,**headers):
			if not os.path.exists(fileName): self.send_error(404); return
			import codecs
			f=codecs.open(fileName,encoding='utf-8')
			self.sendHttp(f.read(),contentType='text/plain;charset=utf-8;',**headers)
		def sendFile(self,fileName,contentType,**headers):
			if not os.path.exists(fileName): self.send_error(404); return
			f=open(fileName,'rb')
			self.sendHttp(f.read(),contentType=contentType,**headers)
		def sendHttp(self,data,contentType,**headers):
			"Send file over http, using appropriate content-type. Headers are converted to strings. The *refresh* header is handled specially: if the value is 0, it is not sent at all."
			self.send_response(200)
			self.send_header('Content-type',contentType)
			if 'refresh' in headers and headers['refresh']==0: del headers['refresh']
			for h in headers: self.send_header(h,str(headers[h]))
			self.end_headers()
			self.wfile.write(data)
			# global httpLastServe
			httpLastServe=time.time()
		def sendPygmentizedFile(self,f,**kw):
			if not os.path.exists(f):
				self.send_error(404); return
			try:
				import codecs
				from pygments import highlight
				from pygments.lexers import PythonLexer
				from pygments.formatters import HtmlFormatter
				data=highlight(codecs.open(f,encoding='utf-8').read(),PythonLexer(),HtmlFormatter(linenos=True,full=True,encoding='utf-8',title=os.path.abspath(f),**kw))
				self.sendHttp(data,contentType='text/html;charset=utf-8;')
			except ImportError:
				self.sendTextFile(f)
	def runHttpStatsServer():
		maxPort=11000; port=9080
		while port<maxPort:
			try:
				server=HTTPServer(('',port),HttpStatsServer)
				import thread; thread.start_new_thread(server.serve_forever,())
				print "http://localhost:%d shows batch summary"%port
				import webbrowser
				webbrowser.open('http://localhost:%d'%port)
				break
			except socket.error:
				port+=1
		if port==maxPort:
			print "WARN: No free port in range 9080-11000, not starting HTTP stats server!"
	
	
	def runJob(job):
		import subprocess
		job.status='RUNNING'
		job.prepareToRun()
		job.started=time.time();
		
		print (bright(yellow('   #{job.num} ({job.id}{nCores}{cores}) started'))+' on {asctime}').format(job=job,nCores=('' if job.nCores==1 else '/%d'%job.nCores),cores=(' ['+','.join([str(c) for c in job.cores])+']') if job.cores else '',asctime=time.asctime())
		#print '#%d cores',%(job.num,job.cores)
		sys.stdout.flush()
		
		if WIN: job.exitStatus=subprocess.call(job.winBatch,shell=False)
		else: job.exitStatus=os.system(job.command)
		#job.exitStatus=0
		print '   #%d system exit status %d'%(job.num,job.exitStatus)
		if job.exitStatus!=0:
			try:  # fake normal exit, if crashing at the very end
				if len([l for l in open(job.log) if l.startswith('Woo: normal exit.')])>0: job.exitStatus=0
			except: pass
		job.status='DONE'
		job.finished=time.time()
		dt=job.finished-job.started;
		job.durationSec=dt
		job.duration=t2hhmmss(dt)
		colorize=(lambda s: bright(green(s))) if job.exitStatus==0 else (lambda s: bright(red(s)))
		havePlot=False
		if os.path.exists(job.plotsFile):
			f=(job.log[:-3] if job.log.endswith('.log') else job.log+'.')+woo.remote.plotImgFormat
			shutil.copy(job.plotsFile,f)
			job.plotsFile=f
			havePlot=True
		print (colorize('   #{job.num} ({job.id}{nCores}) {strStatus}')+'(exit status {job.exitStatus}), duration {job.duration}, log {job.log}{jobPlot}').format(job=job,nCores='' if job.nCores==1 else '/%d'%job.nCores,strStatus=('done    ' if job.exitStatus==0 else 'FAILED '),jobPlot=(', plot %s'%(job.plotsFile) if havePlot else ''))
		job.saveInfo()
		
	def runJobs(jobs,numCores):
		running,pending=0,len(jobs)
		inf=1000000
		while (running>0) or (pending>0):
			pending,running,done=sum([j.nCores for j in jobs if j.status=='PENDING']),sum([j.nCores for j in jobs if j.status=='RUNNING']),sum([j.nCores for j in jobs if j.status=='DONE'])
			numFreeCores=numCores-running
			minRequire=min([inf]+[j.nCores for j in jobs if j.status=='PENDING'])
			if minRequire==inf: minRequire=0
			#print pending,'pending;',running,'running;',done,'done;',numFreeCores,'free;',minRequire,'min'
			overloaded=False
			if minRequire>numFreeCores and running==0: overloaded=True # a job wants more cores than the total we have
			pendingJobs=[j for j in jobs if j.status=='PENDING']
			if opts.randomize: random.shuffle(pendingJobs)
			for j in pendingJobs:
				if j.nCores<=numFreeCores or overloaded:
					freeCores=set(range(0,numCores))-set().union(*(j.cores for j in jobs if j.status=='RUNNING'))
					#print 'freeCores:',freeCores,'numFreeCores:',numFreeCores,'overloaded',overloaded
					if not overloaded:
						# only set cores if CPU affinity is desired; otherwise, just numer of cores is used
						if j.affinity: j.cores=list(freeCores)[0:j.nCores] # take required number of free cores
					# if overloaded, do not assign cores directly
					thread.start_new_thread(runJob,(j,))
					break
			time.sleep(.5)
			sys.stdout.flush()
	
	
	import sys,re,argparse,os,multiprocessing
	numCores=multiprocessing.cpu_count()
	maxOmpThreads=numCores if 'openmp' in woo.config.features else 1
	
	parser=argparse.ArgumentParser(usage=sys.argv[0]+' [options] [ TABLE [SIMULATION.py] | SIMULATION.py[/nCores] [...] ]',description=sys.argv[0]+' runs woo simulation multiple times with different parameters.\n\nSee https://yade-dem.org/sphinx/user.html#batch-queuing-and-execution-woo-batch for details.\n\nBatch can be specified either with parameter table TABLE (must not end in .py), which is either followed by exactly one SIMULATION.py (must end in .py), or contains !SCRIPT column specifying the simulation to be run. The second option is to specify multiple scripts, which can optionally have /nCores suffix to specify number of cores for that particular simulation (corresponds to !THREADS column in the parameter table), e.g. sim.py/3.')
	parser.add_argument('-j','--jobs',dest='maxJobs',type=int,help="Maximum number of simultaneous threads to run (default: number of cores, further limited by OMP_NUM_THREADS if set by the environment: %d)"%numCores,metavar='NUM',default=numCores)
	parser.add_argument('--job-threads',dest='defaultThreads',type=int,help="Default number of threads for one job; can be overridden by per-job with !THREADS (or !OMP_NUM_THREADS) column. Defaults to 1.",metavar='NUM',default=1)
	parser.add_argument('--force-threads',action='store_true',dest='forceThreads',help='Force jobs to not use more cores than the maximum (see \-j), even if !THREADS colums specifies more.')
	parser.add_argument('--log',dest='logFormat',help='Format of job log files: must contain a $, %% or @, which will be replaced by script name, line number or by title column respectively. Directory for logs will be created automatically. (default: logs/$.@.log)',metavar='FORMAT',default='logs/$.@.log')
	parser.add_argument('--global-log',dest='globalLog',help='Filename where to redirect output of woo-batch itself (as opposed to \-\-log); if not specified (default), stdout/stderr are used',metavar='FILE')
	parser.add_argument('-l','--lines',dest='lineList',help='Lines of TABLE to use, in the format 2,3-5,8,11-13 (default: all available lines in TABLE)',metavar='LIST')
	parser.add_argument('--results',dest='resultsDb',help='File (SQLite database) where simulation should store its results (such as input/output files and some data); the default is to use {tableName}.results, if there is param table, otherwise (re)use batch.results.sqlite (stripping .batch)',default=None)
	parser.add_argument('--nice',dest='nice',type=int,help='Nice value of spawned jobs (default: 10)',default=10)
	parser.add_argument('--cpu-affinity',dest='affinity',action='store_true',help='Bind each job to specific CPU cores; cores are assigned in a quasi-random order, depending on availability at the moment the jobs is started. Each job can override this setting by setting AFFINE column.')
	parser.add_argument('--executable',dest='executable',help='Name of the program to run (default: %s). Jobs can override with !EXEC column.'%executable,default=executable,metavar='FILE')
	parser.add_argument('--rebuild',dest='rebuild',help='Run executable(s) with --rebuild prior to running any jobs.',default=False,action='store_true')
	parser.add_argument('--debug',dest='debug',action='store_true',help='Run the executable with --debug. Can be overriddenn per-job with !DEBUG column.',default=False)
	parser.add_argument('--gnuplot',dest='gnuplotOut',help='Gnuplot file where gnuplot from all jobs should be put together',default=None,metavar='FILE')
	parser.add_argument('--dry-run',action='store_true',dest='dryRun',help='Do not actually run (useful for getting gnuplot only, for instance)',default=False)
	parser.add_argument('--http-wait',action='store_true',dest='httpWait',help='Do not quit if still serving overview over http repeatedly',default=False)
	parser.add_argument('--generate-manpage',help='Generate man page documenting this program and exit',dest='manpage',metavar='FILE')
	parser.add_argument('--plot-update',type=int,dest='plotAlwaysUpdateTime',help='Interval (in seconds) at which job plots will be updated even if not requested via HTTP. Non-positive values will make the plots not being updated and saved unless requested via HTTP (see \-\-plot-timeout for controlling maximum age of those).  Plots are saved at exit under the same name as the log file, with the .log extension removed. (default: 120 seconds)',metavar='TIME',default=120)
	parser.add_argument('--plot-timeout',type=int,dest='plotTimeout',help='Maximum age (in seconds) of plots served over HTTP; they will be updated if they are older. (default: 30 seconds)',metavar='TIME',default=30)
	parser.add_argument('--refresh',type=int,dest='refresh',help='Refresh rate of automatically reloaded web pages (summary, logs, ...).',metavar='TIME',default=30)
	parser.add_argument('--timing',type=int,dest='timing',default=0,metavar='COUNT',help='Repeat each job COUNT times, and output a simple table with average/variance/minimum/maximum job duration; used for measuring how various parameters affect execution time. Jobs can override the global value with the !COUNT column.')
	parser.add_argument('--timing-output',type=str,metavar='FILE',dest='timingOut',default=None,help='With --timing, save measured durations to FILE, instead of writing to standard output.')
	parser.add_argument('--randomize',action='store_true',dest='randomize',help='Randomize job order (within constraints given by assigned cores).')
	parser.add_argument('--no-table',action='store_true',dest='notable',help='Treat all command-line argument as simulations to be run, either python scripts or saved simulations.')
	parser.add_argument('simulations',nargs=argparse.REMAINDER)
	#parser.add_argument('--serial',action='store_true',dest='serial',default=False,help='Run all jobs serially, even if there are free cores
	opts=parser.parse_args()
	args=opts.simulations
	
	logFormat,lineList,maxJobs,nice,executable,gnuplotOut,dryRun,httpWait,globalLog=opts.logFormat,opts.lineList,opts.maxJobs,opts.nice,opts.executable,opts.gnuplotOut,opts.dryRun,opts.httpWait,opts.globalLog
	
	if opts.manpage:
		import woo.manpage
		woo.config.metadata['short_desc']='batch system for computational platform Woo'
		woo.config.metadata['long_desc']='Manage batches of computation jobs for the Woo platform; batches are described using text-file tables with parameters which are passed to individual runs of woo. Jobs are being run with pre-defined number of computational cores as soon as the required number of cores is available. Logs of all computations are stored in files and the batch progress can be watched online at (usually) http://localhost:9080. Unless overridden, the executable woo%s is used to run jobs.'%(suffix)
		woo.manpage.generate_manpage(parser,woo.config.metadata,opts.manpage,section=1,seealso='woo%s (1)\n.br\nhttps://yade-dem.org/sphinx/user.html#batch-queuing-and-execution-yade-batch'%suffix)
		print 'Manual page %s generated.'%opts.manpage
		sys.exit(0)
	
	tailProcess=None
	def runTailProcess(globalLog):
		import subprocess
		tailProcess=subprocess.call(["tail","--line=+0","-f",globalLog])
	if not WIN and globalLog and False :
		print 'Redirecting all output to',globalLog
		# if not deleted, tail detects truncation and outputs multiple times
		if os.path.exists(globalLog): os.remove(globalLog) 
		sys.stderr=open(globalLog,"wb")
		sys.stderr.truncate()
		sys.stdout=sys.stderr
		# try to run tee in separate thread
		import thread
		thread.start_new_thread(runTailProcess,(globalLog,))
	
	if len([1 for a in args if re.match('.*\.py(/[0-9]+)?',a)])==len(args) and len(args)!=0 or opts.notable:
		# if all args end in .py, they are simulations that we will run
		table=None; scripts=args
	elif len(args)==2:
		table,scripts=args[0],[args[1]]
	elif len(args)==1:
		table,scripts=args[0],[]
	else:
		parser.print_help()
		sys.exit(1)
	
	print "Will run simulation(s) %s using `%s', nice value %d, using max %d cores."%(scripts,executable,nice,maxJobs)
	
	if table:
		reader=woo.batch.TableParamReader(table)
		params=reader.paramDict()
		availableLines=params.keys()
	
		print "Will use table `%s', with available lines"%(table),', '.join([str(i) for i in availableLines])+'.'
	
		if lineList:
			useLines=[]
			def numRange2List(s):
				ret=[]
				for l in s.split(','):
					if "-" in l: ret+=range(*[int(s) for s in l.split('-')]); ret+=[ret[-1]+1]
					else: ret+=[int(l)]
				return ret
			useLines0=numRange2List(lineList)
			for l in useLines0:
				if l not in availableLines: logging.warn('Skipping unavailable line %d that was requested from the command line.'%l)
				else: useLines+=[l]
		else: useLines=availableLines
		print "Will use lines ",', '.join([str(i)+' (%s)'%params[i]['title'] for i in useLines])+'.'
	else:
		print "Running %d stand-alone simulation(s) in batch mode."%(len(scripts))
		useLines=[]
		params={}
		for i,s in enumerate(scripts):
			fakeLineNo=-i-1
			useLines.append(fakeLineNo)
			params[fakeLineNo]={'title':'default','!SCRIPT':s}
			# fix script and set threads if script.py/num
			m=re.match('(.*)(/[0-9]+)$',s)
			if m:
				params[fakeLineNo]['!SCRIPT']=m.group(1)
				params[fakeLineNo]['!THREADS']=int(m.group(2)[1:])
	
	jobs=[]
	executables=set()
	logFiles=[]
	if opts.resultsDb==None:
		resultsDb='%s.results'%re.sub('\.(batch|table|xls)$','',table) if table else 'batch.results'
	else: resultsDb=opts.resultsDb
	print 'Results database is',os.path.abspath(resultsDb)
	
	
	for i,l in enumerate(useLines):
		script=scripts[0] if len(scripts)>0 else None
		envVars=[]
		nCores=opts.defaultThreads
		jobExecutable=executable
		jobAffinity=opts.affinity
		jobDebug=opts.debug
		jobCount=opts.timing
		for col in params[l].keys():
			if col[0]!='!': continue
			val=params[l][col]
			if col=='!OMP_NUM_THREADS' or col=='!THREADS': nCores=int(val)
			elif col=='!EXEC': jobExecutable=val
			elif col=='!SCRIPT': script=val
			elif col=='!AFFINITY': jobAffinity=eval(val)
			elif col=='!DEBUG': jobDebug=eval(val)
			elif col=='!COUNT': jobCount=eval(val)
			else: envVars+=['%s=%s'%(head[1:],values[l][col])]
		if not script:
			raise ValueError('When only batch table is given without script to run, it must contain !SCRIPT column with simulation to be run.')
		logFile=logFormat.replace('$',script).replace('%',str(l)).replace('@',params[l]['title'].replace('/','_'))
		if nCores>maxJobs:
			if opts.forceThreads:
				logging.info('Forcing job #%d to use only %d cores (max available) instead of %d requested'%(i,maxJobs,nCores))
				nCores=maxJobs
			else:
				logging.warning('WARNING: job #%d will use %d cores but only %d are available'%(i,nCores,maxJobs))
			if 'openmp' not in woo.config.features and nCores>1:
				logging.warning('Job #%d will be uselessly run with %d threads (compiled without OpenMP support).'%(i,nCores))
		executables.add(jobExecutable)
		# compose command-line: build the hyper-linked variant, then strip HTML tags (ugly, but ensures consistency)
		for j in range(0,opts.timing if opts.timing>0 else 1):
			jobNum=len(jobs)
			logFile2=logFile+('.%d'%j if opts.timing>0 else '')
			logDir=os.path.dirname(logFile2)
			if logDir and not os.path.exists(logDir):
				logging.warning("WARNING: creating log directory '%s'"%(logDir))
				os.makedirs(logDir)
			if WIN: logFile2=re.sub('[<>:"|?*=,]','_',logFile2)
			# append numbers to log file if it already exists, to prevent overwriting
			if logFile2 in logFiles:
				i=0;
				while logFile2+'.%d'%i in logFiles: i+=1
				logFile2+='.%d'%i
			logFiles.append(logFile2)
			
			hrefCmd=fullCmd=None
			desc=params[l]['title']
			if '!SCRIPT' in params[l].keys(): desc=script+'.'+desc # prepend filename if script is specified explicitly
			if opts.timing>0: desc+='[%d]'%j
			jobs.append(JobInfo(jobNum,desc,fullCmd,hrefCmd,logFile2,nCores,script=script,table=table,lineNo=l,affinity=jobAffinity,resultsDb=resultsDb,debug=jobDebug,executable=jobExecutable,nice=nice))
	
	print "Master process pid",os.getpid()
	
	if not WIN:
		# HACK: shell suspends the batch sometimes due to tty output, unclear why (both zsh and bash do that).
		#       Let's just ignore SIGTTOU here.
		import signal
		signal.signal(signal.SIGTTOU,signal.SIG_IGN)
	
	
	if opts.rebuild:
		print "Rebuilding all active executables, since --rebuild was specified"
		for e in executables:
			import subprocess
			if subprocess.call([e,'--rebuild','-x']+(['--debug'] if opts.debug else [])):
				 raise RuntimeError('Error rebuilding %s (--rebuild).'%e)
		print "Rebuilding done."
			
	
	print "Job summary:"
	for job in jobs:
		print (bright('   #{job.num} ({job.id}{cores})')+': WOO_BATCH={job.table}:{job.lineNo}:{job.resultsDb} {job.executable} {job.script} > {job.log}').format(job=job,cores='' if job.nCores==1 else '/%d'%job.nCores)
	sys.stdout.flush()
	
	
	httpLastServe=0
	runHttpStatsServer()
	if opts.plotAlwaysUpdateTime>0:
		# update plots periodically regardless of whether they are requested via HTTP
		def updateAllPlots():
			time.sleep(opts.plotAlwaysUpdateTime)
			for job in jobs: job.updatePlots()
		thread.start_new_thread(updateAllPlots,())
	
	# OK, go now
	if not dryRun: runJobs(jobs,maxJobs)
	
	print 'All jobs finished, total time ',t2hhmmss(totalRunningTime())
	
	plots=[]
	for j in jobs:
		if not os.path.exists(j.plotsFile): continue
		plots.append(j.plotsFile)
	if plots: print 'Plot files:',' '.join(plots)
	
	# for easy grepping in logfiles:
	print 'Log files:',' '.join([j.log for j in jobs])
	
	# write timing table
	if opts.timing>0:
		if opts.timingOut:
			print 'Writing gathered timing information to',opts.timingOut
			try:
				out=open(opts.timingOut,'w')
			except IOError:
				logging.warn('Unable to open file %s for timing output, writing to stdout.'%opts.timingOut)
				out=sys.stdout
		else:
			print 'Gathered timing information:'
			out=sys.stdout
		# write header
		out.write('## timing data, written '+time.asctime()+' with arguments\n##    '+' '.join(sys.argv)+'\n##\n')
		paramNames=params[params.keys()[0]].keys(); paramNames.sort()
		out.write('## line\tcount\tavg\tdev\trelDev\tmin\tmax\t|\t'+'\t'.join(paramNames)+'\n')
		import math
		for i,l in enumerate(useLines):
			jobTimes=[j.durationSec for j in jobs if j.lineNo==l and j.durationSec!=None]
			tSum=sum(jobTimes); tAvg=tSum/len(jobTimes)
			tMin,tMax=min(jobTimes),max(jobTimes)
			tDev=math.sqrt(sum((t-tAvg)**2 for t in jobTimes)/len(jobTimes))
			tRelDev=tDev/tAvg
			out.write('%d\t%d\t%.2f\t%.2f\t%.3g\t%.2f\t%.2f\t|\t'%(l,len(jobTimes),tAvg,tDev,tRelDev,tMin,tMax)+'\t'.join([params[l][p] for p in paramNames])+'\n')
	
	if not gnuplotOut:
		print 'Bye.'
	else:
		print 'Assembling gnuplot files…'
		for job in jobs:
			for l in file(job.log):
				if l.startswith('gnuplot '):
					job.plot=l.split()[1]
					break
		preamble,plots='',[]
		for job in jobs:
			if not 'plot' in job.__dict__:
				print "WARN: No plot found for job "+job.id
				continue
			for l in file(job.plot):
				if l.startswith('plot'):
					# attempt to parse the plot line
					ll=l.split(' ',1)[1][:-1] # rest of the line, without newline
					# replace title 'something' with title 'title': something'
					ll,nn=re.subn(r'title\s+[\'"]([^\'"]*)[\'"]',r'title "'+job.id+r': \1"',ll)
					if nn==0:
						logging.error("Plot line in "+job.plot+" not parsed (skipping): "+ll)
					plots.append(ll)
					break
				if not plots: # first plot, copy all preceding lines
					preamble+=l
		gp=file(gnuplotOut,'w')
		gp.write(preamble)
		gp.write('plot '+','.join(plots))
		print "gnuplot",gnuplotOut
		print "Plot written, bye."
	if httpWait and time.time()-httpLastServe<30:
		print "(continue serving http until no longer requested  as per --http-wait)"
		while time.time()-httpLastServe<30:
			time.sleep(1)
	
	if tailProcess: tailProcess.terminate()
	#woo.master.exitNoBacktrace()
	woo.master.disableGdb()
	return 0
	
