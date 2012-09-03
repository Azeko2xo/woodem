# encoding: utf-8
from woo import utils,pack
from woo.core import *
from woo.dem import *
from woo.pre import *
import woo.plot
import math
import os.path
from miniEigen import *
import sys
import cStringIO as StringIO
import numpy
import pprint
nan=float('nan')

try:
	from PyQt4.QtGui import *
	from PyQt4.QtCore import *
except ImportError: pass

# http://www.mail-archive.com/matplotlib-users@lists.sourceforge.net/msg05474.html
from matplotlib.ticker import FuncFormatter
percent=FuncFormatter(lambda x,pos=0: '%g%%'%(100*x))
milimeter=FuncFormatter(lambda x,pos=0: '%3g'%(1000*x))
# transparency of figure legends
legendAlpha=.6




def yRectPlate(xz0,xz1,yy,shift=True,halfThick=0.,**kw):
	xx,zz=(xz0[0],xz1[0]),(xz0[1],xz1[1])
	if halfThick!=0.:
		if shift:
			A,B=Vector2(xx[0],zz[0]),Vector2(xx[1],zz[1])
			dx,dz=Vector2(+(B[1]-A[1]),-(B[0]-A[0])).normalized()*halfThick
			xx,zz=[x+dx for x in xx],[z+dz for z in zz]
		kw['halfThick']=abs(halfThick) # copy arg
	A,B,C,D=(xx[0],yy[0],zz[0]),(xx[1],yy[0],zz[1]),(xx[0],yy[1],zz[0]),(xx[1],yy[1],zz[1])
	return [utils.facet(vertices,**kw) for vertices in (A,B,C),(C,B,D)]

def run(pre):
	de=DemField(gravity=(0,0,-pre.gravity))
	S=Scene(fields=[de])
	if not pre.wallMaterial: pre.wallMaterial=pre.material

	# particle masks
	wallMask=0b00110
	loneMask=0b00100 # particles with this mask don't interact with each other
	sphMask= 0b00011
	delMask= 0b00001 # particles which might be deleted by deleters
	S.dem.loneMask=loneMask

	pre.holeMask=wallMask


	wd,ht,dp=pre.size

	#
	#
	#   ^ +z
	#   |
	#
	#
	#   A                               feedPos[1]   <-feedPos[0]->
	#   +                                    +======+              + 
	#   |                                    J      I              | H
	#   |                                                          |
	#   |                                                          |
	#   |                                                          |
	#   |                                                          |
	#   |                                                          |
	#   |                                                          |
	#   |                                                          |
	#   |<--hole1[0]---> <-hole1[1]->       K        h2[1]   h2[0] |
	#   +---------------+============+------+------+=======+-------+    ~~~~~~~> +x
	#   B=(0,0)         C            D      |      E       F       G
   #                                       |
   #                                       |
   #                                       + L
	#                                        
	#
	B=Vector2(0,0)
	A=B+(0,ht)
	C=B+(pre.hole1[0]-pre.halfThick,0)
	D=C+(pre.hole1[1]+2*pre.halfThick,0)
	G=Vector2(wd,0)
	F=G-(pre.hole2[0]+pre.halfThick,0)
	E=F-(pre.hole2[1]+2*pre.halfThick,0)
	H=G+(0,ht)
	I=H-(pre.feedPos[0],0)
	J=I-(pre.feedPos[1],0)
	K=.5*(D+E)-Vector2(0,-pre.halfThick)
	L=K-(0,2*pre.hole1[1])

	feedHt=pre.feedPos[1]

	xmin,xmax=0,wd
	zmin,zmax=L[1],ht+feedHt
	ymin,ymax=(0 if pre.halfDp else -dp/2.),dp/2.

	wallKw=dict(mat=pre.wallMaterial,mask=wallMask,halfThick=pre.halfThick)
	yy=(ymin,ymax)
	for a,b in ((A,B),(B,C),(D,E),(F,G),(G,H),(K,L)):
		de.par.append(yRectPlate(a,b,yy,**wallKw))
	# holes with ledges
	for i,(a,b) in enumerate([(C,D),(E,F)]):
		if pre.holeLedge>0 and not pre.halfDp: de.par.append(yRectPlate(a,b,(ymin,ymin+pre.holeLedge),**wallKw))
		# TODO: save ids of those, so that they can be opened during simulation
		ids=de.par.append(yRectPlate(a,b,(ymin+(0 if pre.halfDp else pre.holeLedge),ymax-pre.holeLedge),**wallKw))
		if pre.holeLedge>0: de.par.append(yRectPlate(a,b,(ymax-pre.holeLedge,ymax),**wallKw))
		if i==0: pre.hole1ids=ids
		else: pre.hole2ids=ids
	
	## side walls
	noFrictMat=pre.wallMaterial.deepcopy()
	noFrictMat.tanPhi=0
	de.par.append([
		utils.wall((0,ymax,0),sense=-1,axis=1,mat=pre.wallMaterial,mask=wallMask,glAB=((zmin,xmin),(zmax,xmax))),
		utils.wall((0,ymin,0),sense=1,axis=1,mat=(noFrictMat if pre.halfDp else pre.wallMaterial),mask=wallMask,glAB=((zmin,xmin),(zmax,xmax))),
	])
	## domain walls
	de.par.append([
		utils.wall((0,0,zmin),sense=1,axis=2,mat=pre.wallMaterial,mask=wallMask,glAB=((xmin,ymin),(xmax,ymax))),
		utils.wall((xmin,0,0),sense=1,axis=0,mat=pre.wallMaterial,mask=wallMask,glAB=((ymin,zmin),(ymax,zmax))),
		utils.wall((xmax,0,0),sense=-1,axis=0,mat=pre.wallMaterial,mask=wallMask,glAB=((ymin,zmin),(ymax,zmax))),
	])

	## initial spheres
	if pre.loadSpheresFrom:
		import woo.pack
		sp=woo.pack.SpherePack()
		sp.load(pre.loadSpheresFrom)
		sp.toSimulation(S,mat=pre.material,mask=sphMask)


	S.engines=utils.defaultEngines(damping=pre.damping,dontCollect=True)+[
		BoxDeleter(
			stepPeriod=pre.factStep,
			inside=True,
			box=box,
			glColor=.3+.5*i,
			save=True,
			mask=delMask,
			currRateSmooth=pre.rateSmooth,
			label='bucket[%d]'%i
		)
		for i,box in enumerate([((xmin,ymin,zmin),(L[0],ymax,0)),((L[0],ymin,zmin),(xmax,ymax,0))])
	]+[
		BoxFactory(
			box=((J[0],ymin,J[1]),(I[0],ymax,I[1]+(I[0]-J[0]))),
			stepPeriod=pre.factStep,
			maxMass=-1,
			massFlowRate=pre.feedRate,
			glColor=.8,
			maxAttempts=100,
			atMaxAttempts=BoxFactory.maxAttError,
			generator=PsdSphereGenerator(psdPts=pre.psd,discrete=False,mass=True),
			materials=[pre.material],
			shooter=AlignedMinMaxShooter(dir=(0,0,-1),vRange=(0,0)),
			currRateSmooth=pre.rateSmooth,
			mask=sphMask,
			label='feed',
		),
		PyRunner(pre.factStep,'import woo.pre.BinSeg_; woo.pre.BinSeg_.adjustFeedRate(S)',label='feedAdjuster',dead=True),
		#PyRunner(600,'print "%g/%g mass, %d particles, unbalanced %g/'+str(goal)+'"%(woo.makeBandFeedFactory.mass,woo.makeBandFeedFactory.maxMass,len(S.dem.par),woo.utils.unbalancedForce(S))'),
		#PyRunner(200,'if woo.utils.unbalancedForce(S)<'+str(goal)+' and woo.makeBandFeedFactory.dead: S.stop()'),
		PyRunner(pre.factStep,'import woo.pre.BinSeg_; woo.pre.BinSeg_.savePlotData(S)'),
	]

	S.dem.collectNodes()


	S.dt=pre.pWaveSafety*utils.spherePWaveDt(pre.psd[0][0]/2.,pre.material.density,pre.material.young)

	S.pre=pre.deepcopy()

	S.uiBuild='import woo.pre; woo.pre.BinSeg_.uiBuild(S,area)'

	try:
		import woo.gl
		S.any=[
			woo.gl.Renderer(iniUp=(0,0,1),iniViewDir=(0,1,0)),
			woo.gl.Gl1_DemField(colorBy=woo.gl.Gl1_DemField.colorDisplacement),
			woo.gl.Gl1_Wall(div=1),
		]

	except ImportError: pass


	return S

def savePlotData(S):
	import woo
	import woo.plot
	woo.plot.addData(i=S.step,t=S.time,feedRate=woo.feed.currRate,hole1rate=woo.bucket[0].currRate,hole2rate=woo.bucket[1].currRate,numPar=len(S.dem.par),holeRate=sum([woo.bucket[i].currRate for i in (0,1)]))
	if not woo.plot.plots:
		woo.plot.plots={'t':('feedRate','hole1rate','hole2rate','holeRate')}
		#if S.trackEnergy: woo.plot.plots.update({'  t':(S.energy,None,('ERelErr','g--'))})

def adjustFeedRate(S):
	'Change feed massFlowRate so that it comes close to holes deletion rate'
	import woo
	fr=woo.feed.currRate
	br=sum([b.currRate for b in woo.bucket])
	woo.feed.massFlowRate=fr+S.pre.feedAdjustCoeff*(br-fr)
	print 'New feed rate %g (old %g, hole rate %g)'%(woo.feed.massFlowRate,fr,br)



def holeOpen(i,S):
	assert i in (1,2)
	return S.dem.par[(S.pre.hole1ids if i==1 else S.pre.hole2ids)[0]].mask==S.dem.loneMask

def toggleHole(i):
	import woo
	if i not in (1,2): raise ValueError("Hole number must be 0 or 1.")
	S=woo.master.scene
	if type(S.pre)!=woo.pre.BinSeg: raise RuntimeError("Current Scene.pre is not a BinSeg.")
	ids=(S.pre.hole1ids if i==1 else S.pre.hole2ids)
	holeOpen=S.dem.par[ids[0]].mask==S.dem.loneMask
	with S.paused():
		if holeOpen:
			#print 'Closing hole ',i
			S.dem.par.reappear(ids,mask=S.pre.holeMask,removeOverlapping=True)
		else:
			#print 'Opening hole ',i
			S.dem.par.disappear(ids,mask=S.dem.loneMask)
		#for i in ids:
		#	print '#%d:\n\t%s\n\tmask:%d\n\tcon:%s'%(i,'visible' if S.dem.par[i].shape.visible else 'invisible',S.dem.par[i].mask,str(S.dem.par[i].con))

def saveSpheres(S):
	import woo
	if type(S.pre)!=woo.pre.BinSeg: raise RuntimeError("Current Scene.pre is not a BinSeg.")
	if len(woo.qt.views())>0: woo.qt.views()[0].close()
	f=str(QFileDialog.getSaveFileName(None,'Save spheres to','.'))
	if not f: return
	import woo.pack
	sp=woo.pack.SpherePack()
	sp.fromSimulation(S)
	sp.save(f)
	print 'Spheres saved to',f

def feedHolesPsdTable(S,massScale=1.):
	psdSplits=[df[0] for df in S.pre.psd]
	feedHolesMasses=[]
	for ddmm in (woo.feed.generator.diamMass(),woo.bucket[0].diamMass(),woo.bucket[1].diamMass()):
		mm=[]
		for i in range(len(psdSplits)-1):
			dmdm=zip(*ddmm)
			mm.append(sum([dm[1]*massScale for dm in zip(*ddmm) if (dm[0]>=psdSplits[i] and dm[0]<psdSplits[i+1])]))
		feedHolesMasses.append(mm)
		#print buck.mass,sum(mm)
	colNames=['feed','hole 1','hole 2']
	from genshi.builder import tag as t
	tab=t.table(
		t.tr(t.th('diameter',t.br,'[mm]'),*tuple([t.th(colName,t.br,'[mass %]',colspan=2) for colName in colNames])),
		cellpadding='2px',frame='box',rules='all'
	)
	dScale=1e3 # m to mm
	for i in range(len(psdSplits)-1):
		tr=t.tr(t.td('%g-%g'%(dScale*psdSplits[i],dScale*psdSplits[i+1])))
		for bm in feedHolesMasses:
			bMass=sum(bm) # if it is zero, don't error on integer division by zero
			if bMass>0:
				tr.append(t.td('%.4g'%(100*bm[i]/bMass),align='right'))
				tr.append(t.td('%.4g'%(100*sum(bm[:i+1])/bMass),align='right'))
			else:
				tr.append(t.td('-',align='center',colspan=2))
		tab.append(tr)
	colTotals=sum([sum(bm) for bm in feedHolesMasses])
	tab.append(t.tr(t.th('mass [kg]',rowspan=2),*tuple([t.th('%.4g'%sum(bm),colspan=2) for bm in feedHolesMasses])))
	tab.append(t.tr(*tuple([t.td('%.3g %%'%(sum(bm)*100./sum(feedHolesMasses[0])),colspan=2,align='right') for bm in feedHolesMasses])))
	return unicode(tab.generate().render('xhtml'))

def feedHolesPsdFigure():
	import pylab
	fig=pylab.figure()
	for i,buck in enumerate(woo.bucket):
		if buck.num<=1: continue # bucket empty, or just one particle (range would be automatically -.4 to +.4 around that single particle)
		dm=buck.diamMass()
		h,bins=numpy.histogram(dm[0],weights=dm[1],bins=50)
		h/=h.sum()
		pylab.plot(bins,[0]+list(h.cumsum()),label='hole %d'%(i+1),linewidth=2)
	if woo.feed.num>1:
		pylab.plot(*woo.feed.generator.psd(normalize=True),label='feed',linewidth=2)

	pylab.ylim(ymin=-.05,ymax=1.05)
	pylab.grid(True)
	pylab.gca().xaxis.set_major_formatter(milimeter)
	pylab.xlabel('diameter [mm]')
	pylab.gca().yaxis.set_major_formatter(percent)
	pylab.ylabel('mass fraction')
	leg=pylab.legend(loc='best')
	leg.get_frame().set_alpha(legendAlpha)
	return fig


def finishSimulation():	
	import woo
	import numpy,pylab
	S=woo.master.scene
	S.stop(); S.wait()
	massScale=2 if S.pre.halfDp else 1.

	figs=[]


	# per-bucket PSD
	def scaledFlowPsd(x,y): return numpy.array(x),massScale*numpy.array(y)

	if 1:
		figs.append(('Per-hole PSD',feedHolesPsdFigure()))
	if 1:
		fig=pylab.figure()
		if woo.feed.num: pylab.plot(*woo.feed.generator.psd(cumulative=False,normalize=False,num=20),label='feed')
		if woo.bucket[0].num: pylab.plot(*woo.bucket[0].psd(cumulative=False,normalize=False,num=20),label='hole 1')
		if woo.bucket[1].num: pylab.plot(*woo.bucket[1].psd(cumulative=False,normalize=False,num=20),label='hole 2')
		pylab.grid(True)
		pylab.gca().xaxis.set_major_formatter(milimeter)
		pylab.xlabel('diameter [mm]')
		#pylab.gca().yaxis.set_major_formatter(percent)
		pylab.ylabel('histogram')
		leg=pylab.legend(loc='best')
		leg.get_frame().set_alpha(legendAlpha)
		figs.append(('PSD',fig))


	try:
		import pylab
		fig=pylab.figure()
		import woo.plot
		d=woo.plot.data
		pylab.plot(d['t'],massScale*numpy.array(d['feedRate']),label='feed')
		pylab.plot(d['t'],massScale*numpy.array(d['hole1rate']),label='hole1')
		pylab.plot(d['t'],massScale*numpy.array(d['hole2rate']),label='hole2')
		pylab.plot(d['t'],massScale*numpy.array(d['holeRate']),label='holes')
		pylab.ylim(ymin=0)
		leg=pylab.legend(loc='lower left')
		leg.get_frame().set_alpha(legendAlpha)
		pylab.grid(True)
		pylab.xlabel('time [s]')
		pylab.ylabel('mass rate [kg/s]')
		figs.append(('Regime',fig))
	except (KeyError,IndexError) as e:
		print 'No woo.plot plots done due to lack of data:',str(e)


	import codecs
	repName=unicode(S.pre.reportFmt).format(S=S,**(dict(S.tags)))
	rep=codecs.open(repName,'w','utf-8','replace')
	import os.path
	import woo.pre.Roro_
	print 'Writing report to file://'+os.path.abspath(repName)
	s=woo.pre.Roro_.xhtmlReportHead(S,'Report for BinSeg simulation')
	s+='<h2>Mass</h2>'+feedHolesPsdTable(S,massScale=massScale)

	svgs=[]
	for name,fig in figs:
		svgs.append((name,woo.O.tmpFilename()+'.svg'))
		fig.savefig(svgs[-1][-1])
	s+='\n'.join(['<h2>'+svg[0]+'</h2>'+woo.pre.Roro_.svgFileFragment(svg[1]) for svg in svgs])

	s+='</body></html>'




	s=s.replace(u'\xe2\x88\x92','-') # long minus
	s=s.replace(u'\xc3\x97','x') # × multiplicator
	try:
		rep.write(s)
	except UnicodeDecodeError as e:
		print e.start,e.end
		print s[max(0,e.start-20):min(e.end+20,len(s))]
		raise e

	return os.path.abspath(repName)


def uiRefresh(grid,S,area):
	grid.feedButt.setChecked(not woo.feed.dead)
	grid.feedButt.setText('Start feed' if woo.feed.dead else 'Stop feed')
	areOpen=[None,holeOpen(1,S),holeOpen(2,S)]
	for butt,i in (grid.h1butt,1),(grid.h2butt,2):
		butt.setChecked(areOpen[i])
		butt.setText('Close %d'%i if areOpen[i] else 'Open %d'%i)
	if areOpen[1]==areOpen[2]:
		op=areOpen[1]
		grid.h12butt.setChecked(op)
		grid.h12butt.setText('Close 1+2' if op else 'Open 1+2')
		grid.h12butt.setEnabled(True)
	else:
		grid.h12butt.setText('one closed, one open')
		grid.h12butt.setEnabled(False)

def holeClicked(S,i,grid):
	import woo
	assert i in (1,2); assert type(S.pre)==woo.pre.BinSeg
	ids=(S.pre.hole1ids if i==1 else S.pre.hole2ids)
	op=holeOpen(i,S)
	with S.paused():
		if op:
			#print 'Closing hole ',i
			S.dem.par.reappear(ids,mask=S.pre.holeMask,removeOverlapping=True)
		else:
			#print 'Opening hole ',i
			S.dem.par.disappear(ids,mask=S.dem.loneMask)
	#if not holeOpen(0,S) and not holeOpen(1,S) and not woo.feedAdjuster.dead:
	#	autoAdjustChanged(S,grid,0)
def resetCounters(S):
	import woo
	with S.paused():
		for b in woo.bucket: b.clear()
		woo.feed.clear()

def hole12Clicked(S,grid):
	holeClicked(S,1,grid)
	holeClicked(S,2,grid)
def feedToggled(S,checked,grid):
	import woo
	woo.feed.dead=not checked
	# HACK: since the engine was dead for a long time,
	# it would attempt to catch up now and generate all the mass
	# at once, which would exhaust its attempts for sure
	# therefore, make it think it has just run now
	# by setting stepLast
	if not woo.feed.dead:
		woo.feed.stepLast=S.step
	grid.adjCheckbox.setEnabled(not woo.feed.dead)
def reportClicked(S,grid):
	rep=finishSimulation()
	import webbrowser
	webbrowser.open(rep)
def autoAdjustChanged(S,grid,state):
	import woo
	if state==0:
		# stop auto-adjust
		woo.feedAdjuster.dead=True
		woo.feed.atMaxAttempts=woo.dem.BoxFactory.maxAttError
	else:
		woo.feedAdjuster.dead=False
		woo.feed.atMaxAttempts=woo.dem.BoxFactory.maxAttWarn

def uiBuild(S,area):
	grid=QGridLayout(area); grid.setSpacing(0); grid.setMargin(0)
	f=QPushButton('Stop feed')
	adj=QCheckBox('Auto-adjust rate')
	s=QPushButton('Save spheres')
	h1=QPushButton('Open 1')
	h2=QPushButton('Open 2')
	h12=QPushButton('Open 1+2')
	r=QPushButton('Report')
	reset=QPushButton('Reset counters')
	psd=QPushButton('PSD')
	for b in (f,h1,h2,h12): b.setCheckable(True)
	f.toggled.connect(lambda checked: feedToggled(S,checked,grid))
	h1.clicked.connect(lambda: holeClicked(S,1,grid))
	h2.clicked.connect(lambda: holeClicked(S,2,grid))
	h12.clicked.connect(lambda: hole12Clicked(S,grid))
	s.clicked.connect(lambda: saveSpheres(S))
	reset.clicked.connect(lambda: resetCounters(S))
	psd.clicked.connect(lambda: feedHolesPsdFigure().show())
	r.clicked.connect(lambda: reportClicked(S,grid))
	adj.stateChanged.connect(lambda state: autoAdjustChanged(S,grid,state))
	grid.feedButt=f
	grid.adjCheckbox=adj
	grid.h1butt,grid.h2butt,grid.h12butt=h1,h2,h12
	grid.addWidget(adj,0,1)
	grid.addWidget(f,0,2)
	grid.addWidget(s,1,1)
	grid.addWidget(h1,2,0)
	grid.addWidget(h2,2,2)
	grid.addWidget(h12,3,0,1,3)
	grid.addWidget(reset,4,0)
	grid.addWidget(psd,4,1)
	grid.addWidget(r,5,1)
	grid.refreshTimer=QTimer(grid)
	grid.refreshTimer.timeout.connect(lambda: uiRefresh(grid,S,area))
	grid.refreshTimer.start(500)
	#grid.addWidget(QLabel('Hi there!'))
	

	






