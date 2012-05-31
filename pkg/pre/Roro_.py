# encoding: utf-8
from yade import utils,pack
from yade.core import *
from yade.dem import *
from yade.pre import *
import yade.gl
import math
from miniEigen import *
import sys
import cStringIO as StringIO
nan=float('nan')

def run(ui): # use inputs as argument
	print 'Roro_.run()'
	print 'Input parameters:'
	print ui.dumps(format='expr',noMagic=True)
	#print ui.dumps(format='html',fragment=True)

	# this is useful supposing the simulation will run in the same process
	import matplotlib
	matplotlib.use('Agg') # make sure the headless backend is used

	s=Scene()
	de=DemField();
	s.fields=[de]
	rCyl=ui.cylDiameter/2.
	lastCylX=(ui.cylNum-1)*(ui.gap+2*rCyl)
	cylX=[]
	ymin,ymax=-ui.cylLenSim/2.,ui.cylLenSim/2.
	zmin,zmax=-4*rCyl,rCyl+6*rCyl
	xmin,xmax=-3*rCyl,lastCylX+6*rCyl
	rMin=ui.psd[0][0]/2.
	rMax=ui.psd[-1][0]/2.
	s.dt=ui.pWaveSafety*utils.spherePWaveDt(rMin,ui.material.density,ui.material.young)

	wallMask=0b00110
	loneMask=0b00100
	sphMask= 0b00011
	delMask= 0b00001
	
	de.par.append([
		utils.wall(ymin,axis=1,sense= 1,glAB=((zmin,xmin),(zmax,xmax)),material=ui.material,mask=wallMask),
		utils.wall(ymax,axis=1,sense=-1,glAB=((zmin,xmin),(zmax,xmax)),material=ui.material,mask=wallMask)
	])
	for i in range(0,ui.cylNum):
		x=i*(2*rCyl+ui.gap)
		cylX.append(x)
		c=utils.infCylinder((x,0,0),radius=rCyl,axis=1,glAB=(ymin,ymax),material=ui.material,mask=wallMask)
		c.angVel=(0,ui.angVel,0)
		qv,qh=ui.quivVPeriod,ui.quivHPeriod
		c.impose=AlignedHarmonicOscillations(
			amps=(ui.quivAmp[0]*rCyl,nan,ui.quivAmp[1]*rCyl),
			freqs=(
				1./((qh[0]+(i%int(qh[2]))*(qh[1]-qh[0])/int(qh[2]))*s.dt),
				nan,
				1./((qv[0]+(i%int(qv[2]))*(qv[1]-qv[0])/int(qv[2]))*s.dt)
			)
		)
		de.par.append(c)
	A,B,C,D=(xmin,ymin,rCyl),(0,ymin,rCyl),(0,ymax,rCyl),(xmin,ymax,rCyl)
	de.par.append([utils.facet(vertices,material=ui.material,mask=wallMask) for vertices in ((A,B,C),(C,D,A))])

	incl=ui.inclination # is in radians
	grav=ui.gravity
	gravity=(grav*math.sin(incl),0,-grav*math.cos(incl))
	factStep=ui.factStepPeriod
	s.engines=utils.defaultEngines(damping=.4,gravity=gravity,verletDist=.05*rMin)+[
		# what falls beyond
		# initially not saved
		BoxDeleter(stepPeriod=factStep,inside=True,box=((cylX[i],ymin,zmin),(cylX[i+1],ymax,-rCyl/2.)),color=.05*i,save=False,mask=delMask,currRateSmooth=ui.rateSmooth,label='aperture[%d]'%i) for i in range(0,ui.cylNum-1)
	]+[
		# this one should not collect any particles at all
		BoxDeleter(stepPeriod=factStep,box=((xmin,ymin,zmin),(xmax,ymax,zmax)),color=.9,save=False,mask=delMask,label='outOfDomain'),
		# what falls inside
		BoxDeleter(stepPeriod=factStep,inside=True,box=((lastCylX+rCyl,ymin,zmin),(xmax,ymax,zmax)),color=.9,save=True,mask=delMask,currRateSmooth=ui.rateSmooth,label='fallOver'),
		# generator
		BoxFactory(stepPeriod=factStep,box=((xmin,ymin,rCyl),(0,ymax,zmax)),color=.4,label='factory',
			massFlowRate=ui.massFlowRate*ui.cylRelLen, # mass flow rate for the simulated part only
			currRateSmooth=ui.rateSmooth,
			materials=[ui.material],
			generator=PsdSphereGenerator(psdPts=ui.psd,discrete=False,mass=True),
			shooter=AlignedMinMaxShooter(dir=(1,0,-.1),vRange=(ui.flowVel,ui.flowVel)),
			mask=sphMask,
			maxMass=-1, ## do not limit, before steady state is reached
		),
		PyRunner(factStep,'import yade.pre.Roro_; yade.pre.Roro_.watchProgress()'),
		PyRunner(factStep,'import yade.pre.Roro_; yade.pre.Roro_.savePlotData()'),
	]
	# improtant: save the preprocessor here!
	s.any=[yade.gl.Gl1_InfCylinder(wire=True),yade.gl.Gl1_Wall(div=3)]
	s.pre=ui
	#s.tags['preprocessor']=ui.dumps(format='pickle')
	print 'Generated Rollenrost.'
	de.collectNodes()
	return s

def watchProgress():
	'''initially, only the fallOver deleter saves particles; once particles arrive,
	it means we have reached some steady state; at that point, all objects (deleters,
	factory, ... are clear()ed so that PSD's and such correspond to the steady
	state only'''
	pre=yade.O.scene.pre
	# not yet saving what falls through, i.e. not in stabilized regime yet
	if yade.aperture[0].save==False:
		# first particles have just fallen over
		# start saving apertures and reset our counters here
		if yade.fallOver.num>pre.steadyOver:
			yade.fallOver.clear()
			yade.factory.clear()
			yade.factory.maxMass=pre.cylRelLen*pre.time*pre.massFlowRate # required mass scaled to simulated part
			for ap in yade.aperture:
				ap.clear() # to clear overall mass, which gets counted even with save=False
				ap.save=True
			print 'Stabilized regime reached at step %d, counters engaged.'%yade.O.scene.step
	# already in the stable regime, end simulation at some point
	else:
		# factory has finished generating particles
		if yade.factory.mass>yade.factory.maxMass:
			try:
				if not yade.O.scene.lastSave.startswith('/tmp'):
					out='/tmp/'+yade.O.scene.tags['id']+'.bin.gz'
					yade.O.scene.save(out)
					print 'Saved to',out
				plotFinalPsd()
			except:
				import traceback
				traceback.print_exc()
				print 'Error during post-processing.'
			print 'Simulation finished.'
			yade.O.pause()

def savePlotData():
	import yade
	if yade.aperture[0].save==False: return # not in the steady state yet
	import yade.plot
	# save unscaled data here!
	sc=yade.O.scene
	apRate=sum([a.currRate for a in yade.aperture])
	overRate=yade.fallOver.currRate
	yade.plot.addData(i=sc.step,t=sc.time,genRate=yade.factory.currRate,apRate=apRate,overRate=overRate,delRate=apRate+overRate)

def splitPsd(xx0,yy0,splitX):
	'''Split input *psd0* (given by *xx0* and *yy0*) at diameter (x-axis) specified by *splitX*; two PSDs are returned, one grows from splitX and has maximum value for all points x>=splitX; the second PSD is zero for all values <=splitX, then grows to the maximum value proportionally. The maximum value is the last point of psd0, thus both normalized and non-normalized input can be given. If splitX falls in the middle of *psd0* intervals, it is interpolated.'''
	maxY=float(yy0[-1])
	assert(len(xx0)==len(yy0))
	import bisect
	ix=bisect.bisect(xx0,splitX)
	#print 'split index %d/%d'%(ix,len(xx0))
	if ix==0: return (xx0,[0.]*len(xx0)),(xx0,yy0)
	if ix==len(xx0): return (xx0,yy0),(xx0,[maxY]*len(xx0))
	splitY=yy0[ix-1]+(yy0[ix]-yy0[ix-1])*(splitX-xx0[ix-1])/(xx0[ix]-xx0[ix-1])
	smallTrsf,bigTrsf=lambda y: 0.+(y-0)*1./(splitY/maxY),lambda y: 0.+(y-splitY)*1./(1-splitY/maxY)
	#print 'splitY/maxY = %g/%g'%(splitY,maxY)
	xx1,yy1,xx2,yy2=[],[],[],[]
	for i in range(0,ix):
		xx1.append(xx0[i]); yy1.append(smallTrsf(yy0[i]))
		xx2.append(xx0[i]); yy2.append(0.)
	xx1.append(splitX); xx2.append(splitX);
	yy1.append(maxY); yy2.append(0.)
	for i in range(ix,len(xx0)):
		xx1.append(xx0[i]); yy1.append(1.)
		xx2.append(xx0[i]); yy2.append(bigTrsf(yy0[i]))
	return (xx1,yy1),(xx2,yy2)

def svgFragment(data):
	return data[data.find('<svg '):]

def psdFeedApertureFalloverTable(inPsd,feedDM,apDM,overDM,splitD):
	import collections, numpy
	TabLine=collections.namedtuple('TabLine','dMin dMax dAvg inFrac feedFrac feedSmallFrac feedBigFrac overFrac')
	dScale=1e3 # m → mm
	# must convert to list to get slices right in the next line
	edges=list(inPsd[0]) # those will be displayed in the table
	# enlarge a bit so that marginal particles, if any, fit in comfortably; used for histogram computation
	bins=numpy.array([.99*edges[0]]+edges[1:-1]+[1.01*edges[-1]]) 
	edges=numpy.array(edges) # convert back
	# cumulative; otherwise use numpy.diff
	inHist=numpy.diff(inPsd[1]) # non-cumulative
	feedHist,b=numpy.histogram(feedDM[0],weights=feedDM[1],bins=bins,density=True)
	feedHist/=sum(feedHist)
	feedSmallHist,b=numpy.histogram(feedDM[0][feedDM[0]<splitD],weights=feedDM[1][feedDM[0]<splitD],bins=bins,density=True)
	feedSmallHist/=sum(feedSmallHist)
	feedBigHist,b=numpy.histogram(feedDM[0][feedDM[0]>=splitD],weights=feedDM[1][feedDM[0]>=splitD],bins=bins,density=True)
	feedBigHist/=sum(feedBigHist)
	overHist,b=numpy.histogram(overDM[0],weights=overDM[1],bins=bins,density=True)
	overHist/=sum(overHist)
	#print 'lengths bins=%d, in=%d feed=%d feedSmall=%d feedBig=%d over=%d'%(len(bins),len(inHist),len(feedHist),len(feedSmallHist),len(feedBigHist),len(overHist))
	tab=[]
	for i in range(0,len(inHist)):
		tab.append(TabLine(dMin=edges[i],dMax=edges[i+1],dAvg=.5*(edges[i]+edges[i+1]),inFrac=inHist[i],feedFrac=feedHist[i],feedSmallFrac=feedSmallHist[i],feedBigFrac=feedBigHist[i],overFrac=overHist[i]))
	from genshi.builder import tag as t
	return t.table(
		t.tr(
			t.th('Diameter',t.br,'[mm]'),t.th('Average',t.br,'[mm]'),t.th('user input',t.br,'[mass %]',colspan=2),t.th('feed',t.br,'[mass %]',colspan=2),t.th('feed < %g mm'%(dScale*splitD),t.br,'[mass %]',colspan=2),t.th('feed > %g mm'%(dScale*splitD),t.br,'[mass %]',colspan=2),t.th('fall over',t.br,'[mass %]',colspan=2),align='center'
		),
		*tuple([t.tr(
			t.td('%g-%g'%(dScale*tt.dMin,dScale*tt.dMax),align='left'),t.td('%g'%(dScale*tt.dAvg),align='right'),
			t.td('%.4g'%(1e2*tt.inFrac)),t.td('%.4g'%(1e2*sum([ttt.inFrac for ttt in tab[:i+1]]))),
			t.td('%.4g'%(1e2*tt.feedFrac)),t.td('%.4g'%(1e2*sum([ttt.feedFrac for ttt in tab[:i+1]]))),
			t.td('%.4g'%(1e2*tt.feedSmallFrac)),t.td('%.4g'%(1e2*sum([ttt.feedSmallFrac for ttt in tab[:i+1]]))),
			t.td('%.4g'%(1e2*tt.feedBigFrac)),t.td('%.4g'%(1e2*sum([ttt.feedBigFrac for ttt in tab[:i+1]]))),
			t.td('%.4g'%(1e2*tt.overFrac)),t.td('%.4g'%(1e2*sum([ttt.overFrac for ttt in tab[:i+1]]))),
			align='right'
		) for i,tt in enumerate(tab)])
		,cellpadding='2px',frame='box',rules='all'
	).generate().render('xhtml')

	

def plotFinalPsd():
	# generator parameters
	import yade
	import yade.pre
	pre=yade.O.scene.pre
	#ui=[a for a in yade.O.scene.any if type(a)==yade.pre.Roro][0]
	#print 'Parameters were:'
	#pre.dump(sys.stdout,noMagic=True,format='expr')
	import matplotlib
	matplotlib.use('Agg')
	import pylab
	import os
	import numpy
	# http://www.mail-archive.com/matplotlib-users@lists.sourceforge.net/msg05474.html
	from matplotlib.ticker import FuncFormatter
	percent=FuncFormatter(lambda x,pos=0: '%g%%'%(100*x))
	milimeter=FuncFormatter(lambda x,pos=0: '%3g'%(1000*x))

	# should this be scaled to t/year as well?
	massScale=(
		(1./pre.cylRelLen)*(1./pre.time) # in kg/sec
		*3600*24*365/1000.  # in t/y
	)

	def scaledFlowPsd(x,y): return numpy.array(x),massScale*numpy.array(y)
	def unscaledPsd(x,y): return numpy.array(x),numpy.array(y)

	figs=[]
	fig=pylab.figure()
	#fig.set_size_inches(16,16)
	#ax=pylab.subplot(221)
	inPsd=scaledFlowPsd(*yade.factory.generator.inputPsd(scale=True))
	#print 'massScale',massScale
	#print 'inPsd',inPsd
	genPsd=scaledFlowPsd(*yade.factory.generator.psd())
	pylab.plot(*inPsd,label='input PSD')
	pylab.plot(*genPsd,label='generated (%g t/y)'%(yade.factory.mass*massScale))
	for i in range(0,len(yade.aperture)):
		try:
			apPsd=scaledFlowPsd(*yade.aperture[i].psd())
			pylab.plot(*apPsd,label='aperture %d (%g t/y)'%(i,yade.aperture[i].mass*massScale))
		except ValueError: pass
	overPsd=scaledFlowPsd(*yade.fallOver.psd())
	pylab.plot(*overPsd,label='fall over (%g t/y)'%(yade.fallOver.mass*massScale))
	pylab.axvline(x=pre.gap,linewidth=2,ymin=0,ymax=1,color='g',label='gap')
	pylab.grid(True)
	pylab.legend(loc='best')
	pylab.gca().xaxis.set_major_formatter(milimeter)
	pylab.xlabel('particle diameter [mm]')
	pylab.ylabel('flow [t/y]')
	figs.append(fig)

	#ax=pylab.subplot(222)
	fig=pylab.figure()
	dOver,mOver=scaledFlowPsd(*yade.fallOver.diamMass())
	dAper,mAper=numpy.array([]),numpy.array([])
	for a in yade.aperture:
		try:
			d,m=scaledFlowPsd(*a.diamMass())
			dAper=numpy.hstack([dAper,d]); mAper=numpy.hstack([mAper,m])
		except ValueError: pass
	pylab.hist([dOver,dAper],weights=[mOver,mAper],bins=20,histtype='barstacked',label=['fallOver','apertures'])
	#+['aperture %d'%i for i in range(0,len(yade.aperture))])
	pylab.axvline(x=pre.gap,linewidth=2,ymin=0,ymax=1,color='g',label='gap')
	pylab.grid(True)
	pylab.gca().xaxis.set_major_formatter(milimeter)
	pylab.xlabel('particle diameter [mm]')
	pylab.ylabel('flow [t/y]')
	pylab.legend(loc='best')
	figs.append(fig)

	feedTab=psdFeedApertureFalloverTable(
		unscaledPsd(*yade.factory.generator.inputPsd()),
		feedDM=genPsd,
		apDM=(dAper,mAper),
		overDM=(dOver,mOver),
		splitD=pre.gap
	)


	#ax=pylab.subplot(223)
	fig=pylab.figure()
	pylab.plot(*yade.factory.generator.inputPsd(scale=False),marker='o',label='prescribed')
	genPsd=yade.factory.generator.psd(normalize=True)
	inPsd=yade.factory.generator.inputPsd(scale=False)
	pylab.plot(*genPsd,label='generated')
	smallerPsd,biggerPsd=splitPsd(*inPsd,splitX=pre.gap)
	pylab.plot(*smallerPsd,marker='v',label='< gap')
	pylab.plot(*biggerPsd,marker='^',label='> gap')
	pylab.axvline(x=pre.gap,linewidth=2,ymin=0,ymax=1,color='g',label='gap')
	n,bins,patches=pylab.hist(dAper,weights=mAper/sum([ap.mass for ap in yade.aperture]),histtype='step',label='apertures',normed=True,bins=60,cumulative=True,linewidth=2)
	patches[0].set_xy(patches[0].get_xy()[:-1])
	n,bins,patches=pylab.hist(dOver,weights=mOver,normed=True,bins=60,cumulative=True,histtype='step',label='fallOver',linewidth=2)
	patches[0].set_xy(patches[0].get_xy()[:-1])
	#for a in yade.aperture:
	#	print a.mass,sum([dm[1] for dm in a.diamMass()]),'|',len(a.deleted),a.num
	pylab.ylim(ymin=-.05,ymax=1.05)

	#print 'smaller',smallerPsd
	#print 'bigger',biggerPsd
	pylab.grid(True)
	pylab.gca().xaxis.set_major_formatter(milimeter)
	pylab.xlabel('diameter [mm]')
	pylab.gca().yaxis.set_major_formatter(percent)
	pylab.ylabel('mass fraction')
	pylab.legend(loc='best')
	figs.append(fig)

	try:
		#ax=pylab.subplot(224)
		fig=pylab.figure()
		import yade.plot
		d=yade.plot.data
		pylab.plot(d['t'],massScale*numpy.array(d['genRate']),label='generate')
		pylab.plot(d['t'],massScale*numpy.array(d['delRate']),label='delete')
		pylab.plot(d['t'],massScale*numpy.array(d['apRate']),label='apertures')
		pylab.plot(d['t'],massScale*numpy.array(d['overRate']),label='fallOver')
		pylab.legend(loc='lower left')
		pylab.grid(True)
		pylab.xlabel('time [s]')
		pylab.ylabel('mass rate [t/y]')
		figs.append(fig)
	except KeyError:
		print 'No yade.plot plots done due to lack of data'
		# after loading no data are recovered

	svgs=[]
	for f in figs:
		svgs.append(yade.O.tmpFilename()+'.svg')
		f.savefig(svgs[-1])
		
	#if 0: pylab.show()
	#else:
	#	figOut=yade.O.tmpFilename()+".svg"
	#	fig.savefig(figOut)
	#	os.system("xdg-open '%s' &"%figOut)

	import time
	xmlhead='''<?xml version="1.0" encoding="UTF-8"?>
	<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd"> 
	<html xmlns="http://www.w3.org/1999/xhtml" xmlns:svg="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">
	'''
	# <meta http-equiv="content-type" content="text/html;charset=UTF-8" />
	html=('''<head><title>Report for Roller screen simulation</title></head><body>
		<h1>Report for Roller screen simulation</h1>
		<h2>General</h2>
		<table>
			<tr><td>started</td><td align="right">{started}</td></tr>
			<tr><td>duration</td><td align="right">{duration:g} s</td></tr>
			<tr><td>number of cores used</td><td align="right">{nCores}</td></tr>
			<tr><td>average speed</td><td align="right">{stepsPerSec:g} steps/sec</td></tr>
			<tr><td>engine</td><td align="right">{engine}</td></tr>
			<tr><td>compiled with</td><td align="right">{compiledWith}</td></tr>
		</table>
		'''.format(started=time.ctime(time.time()-yade.O.realtime),duration=yade.O.realtime,nCores=yade.O.numThreads,stepsPerSec=yade.O.scene.step/yade.O.realtime,engine='wooDem '+yade.config.version+'/'+yade.config.revision+(' (debug)' if yade.config.debug else ''),compiledWith=','.join(yade.config.features))
		+'<h2>Input data</h2>'+pre.dumps(format='html',fragment=True)
		+'<h2>Outputs</h2>'
		+'\n'.join([svgFragment(open(svg).read()) for svg in svgs])
		+'<h2>Tables</h2>'
		+feedTab
		+'</body></html>'
	)
	if 0: # pure genshi version; probably does not work
		html=(tag.head(tag.title('Report for Roller screen simulation')),tag.body(
			tag.h1('Report for Roller screen simulation'),
			tag.h2('General'),
			tag.table(
				tag.tr(tag.td('started'),tag.td('%g s'%(time.ctime(time.time()-yade.O.realtime)),align='right')),
				tag.tr(tag.td('duration'),tag.td('%g s'%(yade.O.realtime),align='right')),
				tag.tr(tag.td('number of cores'),tag.td(str(yade.O.numThreads),align='right')),
				tag.tr(tag.td('average speed'),tag.td('%g steps/sec'%(yade.O.scene.step/yade.O.realtime))),
				tag.tr(tag.td('engine'),tag.td('wooDem '+yade.config.version+'/'+yade.config.revision+(' (debug)' if yade.config.debug else ''))),
				tag.tr(tag.td('compiled with'),tag.td(', '.join(yade.config.features))),
			),
			tag.h2('Input data'),
			pre.dumps(format='genshi'),
			tag.h2('Tables'),
			feedTab,
			tag.h2('Graphs'),
			*tuple([XMLParser(StringIO.StringIO(svgFragment(open(svg).read()))).parse() for svg in svgs])
		)).render('xhtml')

	# to play with that afterwards
	yade.html=html
	#from genshi.input import HTMLParser
	#import codecs, StringIO
	import codecs
	repName=yade.O.scene.tags['id']+'-report.xhtml'
	rep=codecs.open(repName,'w','utf-8')
	import os.path
	print 'Report written to file://'+os.path.abspath(repName)
	#rep.write(HTMLParser(StringIO.StringIO(html),'[filename]').parse().render('xhtml',doctype='xhtml').decode('utf-8'))
	rep.write(xmlhead+html)

# test drive
if __name__=='__main__':
	import yade.pre
	import yade.qt
	import yade.log
	#yade.log.setLevel('PsdParticleGenerator',yade.log.TRACE)
	O.scene=yade.pre.Roro()()
	O.timingEnabled=True
	O.saveTmp()
	#O.scene.saveTmp()
	yade.qt.View()
	O.run()

