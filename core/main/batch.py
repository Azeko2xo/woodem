#!/usr/bin/python
# encoding: utf-8
#
# vim: syntax=python
# portions © 2008 Václav Šmilauer <eudoxos@arcig.cz>

import os, sys, thread, time, logging, pipes, socket, xmlrpclib, re, shutil, random, os.path

#socket.setdefaulttimeout(10) 

import wooOptions
wooOptions.forceNoGui=True
wooOptions.debug=False
wooOptions.quirks=0
import woo, woo.batch, woo.config, woo.remote

if not sys.argv[0].endswith('-batch'): raise RuntimeError('Batch executable does not end with -batch!')
executable=re.sub('-batch$','',sys.argv[0])


class JobInfo():
	def __init__(self,num,id,command,hrefCommand,log,nCores,script,table,lineNo,affinity):
		self.started,self.finished,self.duration,self.durationSec,self.exitStatus=None,None,None,None,None # duration is a string, durationSec is a number
		self.command=command; self.hrefCommand=hrefCommand; self.num=num; self.log=log; self.id=id; self.nCores=nCores; self.cores=set(); self.infoSocket=None
		self.script=script; self.table=table; self.lineNo=lineNo; self.affinity=affinity
		self.hasXmlrpc=False
		self.status='PENDING'
		self.threadNum=None
		self.plotsLastUpdate,self.plotsFile=0.,woo.master.tmpFilename()+'.'+woo.remote.plotImgFormat
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
		for l in open(self.log,'r'):
			if not l.startswith('XMLRPC info provider on'): continue
			url=l[:-1].split()[4]
			self.xmlrpcConn=xmlrpclib.ServerProxy(url,allow_none=True)
			self.hasXmlrpc=True
			return True
		if not self.hasXmlrpc: return False # catches the case where the magic line is not in the log yet
	def getInfoDict(self):
		if self.status!='RUNNING': return None
		if not self.ensureXmlrpc(): return None
		return self.xmlrpcConn.basicInfo()
	def updatePlots(self):
		global opts
		if self.status!='RUNNING': return
		if not self.ensureXmlrpc(): return
		if time.time()-self.plotsLastUpdate<opts.plotTimeout: return
		self.plotsLastUpdate=time.time()
		img=self.xmlrpcConn.plot()
		if not img:
			if os.path.exists(self.plotsFile): os.remove(self.plotsFile)
			return
		f=open(self.plotsFile,'wb')
		f.write(img.data)
		f.close()
		#print woo.remote.plotImgFormat,'(%d bytes) written to %s'%(os.path.getsize(self.plotsFile),self.plotsFile)

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
	global maxJobs
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
					if not cookieRemoved and l.startswith('TCP python prompt on'):
						ii=l.find('auth cookie `'); l=l[:ii+13]+'******'+l[ii+19:]; cookieRemoved=True
					data+=l
				self.sendHttp(data,contentType='text/plain;charset=utf-8;',refresh=(0 if job.status=='DONE' else opts.refresh))
			elif rest=='script':
				self.sendPygmentizedFile(job.script,linenostep=5)
			elif rest=='table':
				if not job.table: return
				if job.table.endswith('.xls'): self.sendFile(job.table,contentType='application/vnd.ms-excel')
				else: self.sendPygmentizedFile(job.table,hl_lines=[job.lineNo],linenostep=1)
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
		f=open(fileName)
		self.sendHttp(f.read(),contentType=contentType,**headers)
	def sendHttp(self,data,contentType,**headers):
		"Send file over http, using appropriate content-type. Headers are converted to strings. The *refresh* header is handled specially: if the value is 0, it is not sent at all."
		self.send_response(200)
		self.send_header('Content-type',contentType)
		if 'refresh' in headers and headers['refresh']==0: del headers['refresh']
		for h in headers: self.send_header(h,str(headers[h]))
		self.end_headers()
		self.wfile.write(data)
		global httpLastServe
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
	job.status='RUNNING'
	job.started=time.time();
	print '#%d (%s%s%s) started on %s'%(job.num,job.id,'' if job.nCores==1 else '/%d'%job.nCores,(' ['+','.join([str(c) for c in job.cores])+']') if job.cores else '',time.asctime())
	#print '#%d cores',%(job.num,job.cores)
	if job.cores:
		def coresReplace(s): return s.replace('[threadspec]','--cores=%s'%(','.join(str(c) for c in job.cores)))
		job.command=coresReplace(job.command)
		job.hrefCommand=coresReplace(job.hrefCommand)
	else:
		job.command=job.command.replace('[threadspec]','--threads=%d'%job.nCores); job.hrefCommand=job.hrefCommand.replace('[threadspec]','--threads=%d'%job.nCores)
	job.exitStatus=os.system(job.command)
	#job.exitStatus=0
	print '#%d system exit status %d'%(job.num,job.exitStatus)
	if job.exitStatus!=0 and len([l for l in open(job.log) if l.startswith('Woo: normal exit.')])>0: job.exitStatus=0
	job.finished=time.time()
	dt=job.finished-job.started;
	job.durationSec=dt
	job.duration=t2hhmmss(dt)
	strStatus='done   ' if job.exitStatus==0 else 'FAILED '
	job.status='DONE'
	havePlot=False
	if os.path.exists(job.plotsFile):
		f=(job.log[:-3] if job.log.endswith('.log') else job.log+'.')+woo.remote.plotImgFormat
		shutil.copy(job.plotsFile,f)
		job.plotsFile=f
		havePlot=True
	print "#%d (%s%s) %s (exit status %d), duration %s, log %s%s"%(job.num,job.id,'' if job.nCores==1 else '/%d'%job.nCores,strStatus,job.exitStatus,job.duration,job.log,(', plot %s'%(job.plotsFile) if havePlot else ''))
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


import sys,re,argparse,os
def getNumCores():
	nCpu=len([l for l in open('/proc/cpuinfo','r') if l.find('processor')==0])
	if os.environ.has_key("OMP_NUM_THREADS"): return min(int(os.environ['OMP_NUM_THREADS']),nCpu)
	return nCpu
numCores=getNumCores()
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
if globalLog and False:
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
		# append numbers to log file if it already exists, to prevent overwriting
		if logFile2 in logFiles:
			i=0;
			while logFile2+'.%d'%i in logFiles: i+=1
			logFile2+='.%d'%i
		logFiles.append(logFile2)
		env='WOO_BATCH='
		if table: env+='<a href="jobs/%d/table">%s:%d:%s</a>'%(jobNum,table,l,resultsDb) # keep WOO_BATCH empty (but still defined) if running a single simulation
		env+=' DISPLAY= %s '%(' '.join(envVars))
		cmd='%s%s [threadspec] %s -x <a href="jobs/%d/script">%s</a>'%(jobExecutable,' --debug' if jobDebug else '','--nice=%s'%nice if nice!=None else '',i,script)
		log='> <a href="jobs/%d/log">%s</a> 2>&1'%(jobNum,pipes.quote(logFile2))
		hrefCmd=env+cmd+log
		fullCmd=re.sub('(<a href="[^">]+">|</a>)','',hrefCmd)
		desc=params[l]['title']
		if '!SCRIPT' in params[l].keys(): desc=script+'.'+desc # prepend filename if script is specified explicitly
		if opts.timing>0: desc+='[%d]'%j
		jobs.append(JobInfo(jobNum,desc,fullCmd,hrefCmd,logFile2,nCores,script=script,table=table,lineNo=l,affinity=jobAffinity))

print "Master process pid",os.getpid()

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
	print '   #%d (%s%s):'%(job.num,job.id,'' if job.nCores==1 else '/%d'%job.nCores),job.command
#sys.stdout.flush()


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
woo.master.exitNoBacktrace()
