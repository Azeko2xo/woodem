from yade import utils,pack
from yade.core import *
from yade.dem import *
from yade.pre import Roro
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
		PyRunner(factStep,'yade.pre.Roro_.savePlotData()'),
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
			plotFinalPsd()
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


def plotFinalPsd():
	# generator parameters
	import yade
	import yade.pre
	pre=yade.O.scene.pre
	#ui=[a for a in yade.O.scene.any if type(a)==yade.pre.Roro][0]
	print 'Parameters were:'
	pre.dump(sys.stdout,noMagic=True,format='expr')
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
	def scaledNormPsd(x,y): return numpy.array(x),numpy.array(y)

	fig=pylab.figure()
	fig.set_size_inches(16,16)
	ax=pylab.subplot(221)
	inPsd=scaledFlowPsd(*yade.factory.generator.inputPsd(scale=True))
	#print 'massScale',massScale
	#print 'inPsd',inPsd
	genPsd=scaledFlowPsd(*yade.factory.generator.psd())
	pylab.plot(*inPsd,label='input PSD')
	pylab.plot(*genPsd,label='generated (%g t/y)'%(yade.factory.mass*massScale))
	for i in range(0,len(yade.aperture)):
		apPsd=scaledFlowPsd(*yade.aperture[i].psd())
		pylab.plot(*apPsd,label='aperture %d (%g t/y)'%(i,yade.aperture[i].mass*massScale))
	overPsd=scaledFlowPsd(*yade.fallOver.psd())
	pylab.plot(*overPsd,label='fall over (%g t/y)'%(yade.fallOver.mass*massScale))
	pylab.axvline(x=pre.gap,linewidth=2,ymin=0,ymax=1,color='g',label='gap')
	pylab.grid(True)
	pylab.legend(loc='best')
	ax.xaxis.set_major_formatter(milimeter)
	pylab.xlabel('particle diameter [mm]')
	pylab.ylabel('flow [t/y]')

	ax=pylab.subplot(222)
	dd,mm=[],[] # lists with diameters and masses
	ff=scaledFlowPsd(*yade.fallOver.diamMass())
	dd.append(ff[0]); mm.append(ff[1])
	dd.append([]); mm.append([]) # sum apertures together
	for a in yade.aperture:
		ff=scaledFlowPsd(*a.diamMass())
		dd[-1].extend(ff[0]); mm[-1].extend(ff[1])
	pylab.hist(dd,weights=mm,bins=20,histtype='barstacked',label=['fallOver','apertures']) #+['aperture %d'%i for i in range(0,len(yade.aperture))])
	pylab.axvline(x=pre.gap,linewidth=2,ymin=0,ymax=1,color='g',label='gap')
	pylab.grid(True)
	ax.xaxis.set_major_formatter(milimeter)
	pylab.xlabel('particle diameter [mm]')
	pylab.ylabel('flow [t/y]')
	pylab.legend(loc='best')


	ax=pylab.subplot(223)
	pylab.plot(*scaledNormPsd(*yade.factory.generator.inputPsd(scale=False)),marker='o',label='prescribed')
	genPsd=scaledNormPsd(*yade.factory.generator.psd(normalize=True))
	inPsd=scaledNormPsd(*yade.factory.generator.inputPsd(scale=False))
	pylab.plot(*genPsd,label='generated')
	smallerPsd,biggerPsd=splitPsd(*inPsd,splitX=pre.gap)
	pylab.plot(*smallerPsd,marker='v',label='< gap')
	pylab.plot(*biggerPsd,marker='^',label='> gap')
	pylab.axvline(x=pre.gap,linewidth=2,ymin=0,ymax=1,color='g',label='gap')
	n,bins,patches=pylab.hist(dd[-1],weights=numpy.array(mm[-1])/sum([ap.mass for ap in yade.aperture]),histtype='step',label='apertures',normed=True,bins=60,cumulative=True,linewidth=2)
	patches[0].set_xy(patches[0].get_xy()[:-1])
	n,bins,patches=pylab.hist(dd[0],weights=mm[0],normed=True,bins=60,cumulative=True,histtype='step',label='fallOver',linewidth=2)
	patches[0].set_xy(patches[0].get_xy()[:-1])
	#for a in yade.aperture:
	#	print a.mass,sum([dm[1] for dm in a.diamMass()]),'|',len(a.deleted),a.num
	pylab.ylim(ymin=-.05,ymax=1.05)

	#print 'smaller',smallerPsd
	#print 'bigger',biggerPsd
	pylab.grid(True)
	ax.xaxis.set_major_formatter(milimeter)
	pylab.xlabel('diameter [mm]')
	ax.yaxis.set_major_formatter(percent)
	pylab.ylabel('mass fraction')
	pylab.legend(loc='best')

	ax=pylab.subplot(224)
	d=yade.plot.data
	pylab.plot(d['t'],massScale*numpy.array(d['genRate']),label='generate')
	pylab.plot(d['t'],massScale*numpy.array(d['delRate']),label='delete')
	pylab.plot(d['t'],massScale*numpy.array(d['apRate']),label='apertures')
	pylab.plot(d['t'],massScale*numpy.array(d['overRate']),label='fallOver')
	pylab.legend(loc='lower left')
	pylab.grid(True)
	pylab.xlabel('time [s]')
	pylab.ylabel('mass rate [t/y]')


	if 0: pylab.show()
	else:
		out=yade.O.tmpFilename()+".pdf"
		fig.savefig(out)
		os.system("xdg-open '%s' &"%out)

# test drive
if __name__=='__main__':
	import yade.pre
	import yade.qt
	import yade.log
	yade.log.setLevel('PsdParticleGenerator',yade.log.TRACE)
	O.scene=yade.pre.Roro()()
	O.timingEnabled=True
	O.saveTmp()
	yade.qt.View()
	O.run()

