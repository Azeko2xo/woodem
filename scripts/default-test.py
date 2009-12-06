#!/usr/local/bin/yade-trunk
# coding=UTF-8
# this must be run inside yade
#
# pass 'mail:sender@somewhere.org,recipient@elsewhere.com' as an argument so that the crash report is e-mailed 
# using the default SMTP settings (sendmail?) on your system
#
import os,time,sys
from yade import *
import yade.runtime,yade.system,yade.config
simulFile='/tmp/yade-test-%d.xml'%(os.getpid()) # generated simulations here
pyCmdFile='/tmp/yade-test-%d.py'%(os.getpid()) # generated script here
msgFile='/tmp/yade-test-%d.msg'%(os.getpid()) # write message here
runSimul="""
# generated file
from yade import *
simulFile='%s'; msgFile='%s'; nIter=%d;
import time
try:
	O.load(simulFile)
	O.run(10); O.wait() # run first 10 iterations
	start=time.time(); O.run(nIter); O.wait(); finish=time.time() # run nIter iterations, wait to finish, measure elapsed time
	speed=nIter/(finish-start); open(msgFile,'w').write('%%g iter/sec'%%speed)
except:
	import sys, traceback
	traceback.print_exc()
	sys.exit(1)
print 'main: Yade: normal exit.'
O.exitNoBacktrace()
quit()
"""%(simulFile,msgFile,100)

runGenerator="""
#generated file
from yade import *
%%s(%%s).generate('%s')
print 'main: Yade: normal exit.'
O.exitNoBacktrace()
quit()
"""%(simulFile)


def crashProofRun(cmd,quiet=True):
	import subprocess,os,os.path,yade.runtime
	f=open(pyCmdFile,'w'); f.write(cmd); f.close(); 
	if os.path.exists(msgFile): os.remove(msgFile)
	p=subprocess.Popen([sys.executable,pyCmdFile],stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env=dict(os.environ,**{'PYTHONPATH':os.path.join(yade.config.prefix,'lib','yade'+yade.config.suffix,'py'),'DISPLAY':''}))
	pout=p.communicate()[0]
	retval=p.wait()
	if not quiet: print pout
	msg=''
	if os.path.exists(msgFile): msg=open(msgFile,'r').readlines()[0]
	if retval==0: return True,msg,pout
	else:
		# handle crash at exit :-(
		if 'main: Yade: normal exit.' in pout: return True,msg,pout
		return False,msg,pout

reports=[]
summary=[]
#broken=['SDECLinkedSpheres','SDECMovingWall','SDECSpheresPlane','ThreePointBending']
broken=[]
genParams={
	#'USCTGen':{'spheresFile':'examples/small.sdec.xyz'}
}

for pp in yade.system.childClasses('FileGenerator'):
	if pp in broken:
		summary.append(pp,'skipped (broken)','');
	params='' if pp not in genParams else (","+",".join(["%s=%s"%(k,repr(genParams[pp][k])) for k in genParams[pp]]))
	ok1,msg1,out1=crashProofRun(runGenerator%(pp,params))
	if not ok1:
		reports.append([pp,'generator CRASH',out1]); summary.append([pp,'generator CRASH'])
	else:
		ok2,msg2,out2=crashProofRun(runSimul)
		if not ok2: reports.append([pp,'simulation CRASH',out2]); summary.append([pp,'simulation CRASH'])
		else: summary.append([pp,'passed (%s)'%msg2])
	print summary[-1][0]+':',summary[-1][1]

# delete temporaries
for f in simulFile,msgFile,pyCmdFile:
	if os.path.exists(f): os.remove(f)

# handle crash reports, if any
if reports:
	mailFrom,mailTo=None,None
	for a in yade.runtime.argv:
		if 'mail:' in a: mailFrom,mailTo=a.replace('mail:','').split(',')
	reportText='\n'.join([80*'#'+'\n'+r[0]+': '+r[1]+'\n'+80*'#'+'\n'+r[2] for r in reports])
	if mailTo and mailFrom:
		from email.mime.text import MIMEText
		msg=MIMEText(reportText)
		msg['Subject']="Automated crash report for "+yade.runtime.executable+": "+",".join([r[0] for r in reports])
		msg['From']=mailFrom
		msg['To']=mailTo
		msg['Reply-To']='yade-dev@lists.launchpad.net'
		import smtplib
		s=smtplib.SMTP()
		s.connect()
		s.sendmail(mailFrom,[mailTo],msg.as_string())
		s.close()
		print "Sent crash report to ",mailTo
	else:
		print "\n\n=================================== PROBLEM DETAILS ===================================\n"
		print reportText

print "\n\n========================================= SUMMARY ======================================\n"
for l in summary: print "%30s: %s"%(l[0],l[1])
quit()
