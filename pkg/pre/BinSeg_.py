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

from PyQt4.QtGui import *
from PyQt4 import QtCore


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
	S.loneMask=loneMask

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


	S.engines=utils.defaultEngines(damping=pre.damping)+[
		BoxFactory(
			box=((J[0],ymin,J[1]),(I[0],ymax,I[1]+(I[0]-J[0]))),
			stepPeriod=pre.factStep,
			maxMass=-1,
			massFlowRate=pre.feedRate,
			glColor=.8,
			maxAttempts=100,
			generator=PsdSphereGenerator(psdPts=pre.psd,discrete=False,mass=True),
			materials=[pre.material],
			shooter=AlignedMinMaxShooter(dir=(0,0,-1),vRange=(0,0)),
			currRateSmooth=pre.rateSmooth,
			mask=sphMask,
			label='feed',
		),
		#PyRunner(200,'plot.addData(uf=utils.unbalancedForce(),i=O.scene.step)'),
		#PyRunner(600,'print "%g/%g mass, %d particles, unbalanced %g/'+str(goal)+'"%(woo.makeBandFeedFactory.mass,woo.makeBandFeedFactory.maxMass,len(S.dem.par),woo.utils.unbalancedForce(S))'),
		#PyRunner(200,'if woo.utils.unbalancedForce(S)<'+str(goal)+' and woo.makeBandFeedFactory.dead: S.stop()'),
		PyRunner(pre.factStep,'import woo.pre.BinSeg_; woo.pre.BinSeg_.savePlotData(S)'),
	]+[
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
	]


	S.dt=pre.pWaveSafety*utils.spherePWaveDt(pre.psd[0][0]/2.,pre.material.density,pre.material.young)

	S.pre=pre.deepcopy()

	try:
		import woo.gl
		S.any=[
			woo.gl.Renderer(iniUp=(0,0,1),iniViewDir=(0,1,0)),
			woo.gl.Gl1_DemField(colorBy=woo.gl.Gl1_DemField.colorDisplacement),
			woo.gl.Gl1_Wall(div=5),
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



def openHole(i):
	import woo
	if i not in (1,2): raise ValueError("Hole number must be 0 or 1.")
	S=woo.master.scene
	if type(S.pre)!=woo.pre.BinSeg: raise RuntimeError("Current Scene.pre is not a BinSeg.")
	ids=(S.pre.hole1ids if i==1 else S.pre.hole2ids)
	if len(ids)==0: raise RuntimeError("Hole already open.")
	assert len(ids)==2 and type(S.dem.par[ids[0]].shape)==woo.dem.Facet and type(S.dem.par[ids[1]].shape)==woo.dem.Facet
	if i==1: S.pre.hole1ids=[]
	else: S.pre.hole2ids=[]
	running=S.running
	S.stop(); S.wait()
	# stop to assure particles are not being accessed
	S.dem.par.remove(ids)
	if running: S.run()

def saveSpheres():
	import woo
	S=woo.master.scene
	if type(S.pre)!=woo.pre.BinSeg: raise RuntimeError("Current Scene.pre is not a BinSeg.")
	f=str(QFileDialog.getSaveFileName(None,'Save spheres to','.'))
	if not f: return
	import woo.pack
	sp=woo.pack.SpherePack()
	sp.fromSimulation(S)
	sp.save(f)
	print 'Spheres saved to',f

def finishSimulation():	
	import woo
	import numpy,pylab
	S=woo.master.scene
	S.stop(); S.wait()
	massScale=2 if S.pre.halfDp else 1.
	legendAlpha=.6

	figs=[]

	# http://www.mail-archive.com/matplotlib-users@lists.sourceforge.net/msg05474.html
	from matplotlib.ticker import FuncFormatter
	percent=FuncFormatter(lambda x,pos=0: '%g%%'%(100*x))
	milimeter=FuncFormatter(lambda x,pos=0: '%3g'%(1000*x))


	# per-bucket PSD
	def scaledFlowPsd(x,y): return numpy.array(x),massScale*numpy.array(y)

	if 1:
		fig=pylab.figure()
		for i,buck in enumerate(woo.bucket):
			dm=buck.diamMass()
			h,bins=numpy.histogram(dm[0],weights=dm[1],bins=50)
			h/=h.sum()
			pylab.plot(bins,[0]+list(h.cumsum()),label='hole %d'%(i+1),linewidth=2)
		pylab.plot(*woo.feed.generator.psd(normalize=True),label='feed',linewidth=2)

		pylab.ylim(ymin=-.05,ymax=1.05)
		pylab.grid(True)
		pylab.gca().xaxis.set_major_formatter(milimeter)
		pylab.xlabel('diameter [mm]')
		pylab.gca().yaxis.set_major_formatter(percent)
		pylab.ylabel('mass fraction')
		leg=pylab.legend(loc='best')
		leg.get_frame().set_alpha(legendAlpha)
		figs.append(('Per-hole PSD',fig))
	if 1:
		fig=pylab.figure()
		pylab.plot(*woo.feed.generator.psd(cumulative=False,normalize=False,num=20),label='feed')
		pylab.plot(*woo.bucket[0].psd(cumulative=False,normalize=False,num=20),label='hole 1')
		pylab.plot(*woo.bucket[1].psd(cumulative=False,normalize=False,num=20),label='hole 2')
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
	repName=S.pre.reportFmt.format(S=S,**(dict(S.tags)))
	rep=codecs.open(repName,'w','utf-8','replace')
	import os.path
	import woo.pre.Roro_
	print 'Writing report to file://'+os.path.abspath(repName)
	s=woo.pre.Roro_.xhtmlReportHead(S,'Report for BinSeg simulation')

	svgs=[]
	for name,fig in figs:
		svgs.append((name,woo.O.tmpFilename()+'.svg'))
		fig.savefig(svgs[-1][-1])
	s+='\n'.join(['<h2>'+svg[0]+'</h2>'+woo.pre.Roro_.svgFragment(open(svg[1]).read()) for svg in svgs])

	s+='</body></html>'




	s=s.replace('\xe2\x88\x92','-') # long minus
	s=s.replace('\xc3\x97','x') # × multiplicator
	try:
		rep.write(s)
	except UnicodeDecodeError as e:
		print e.start,e.end
		print s[max(0,e.start-20):min(e.end+20,len(s))]
		raise e



	


	

	






