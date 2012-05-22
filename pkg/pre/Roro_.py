from yade import utils,pack
from yade.core import *
from yade.dem import *
import yade.gl
import math
from miniEigen import *
import sys
import cStringIO as StringIO

def run(ui): # use inputs as argument
	print 'Roro_.run()'
	print 'Input parameters:'
	print ui.dumps(format='expr',noMagic=True)
	print ui.dumps(format='html',fragment=True)

	s=Scene()
	de=DemField();
	s.fields=[de]
	rCyl=ui.cylDiameter/2.
	lastCylX=(ui.cylNum-1)*(ui.gap+2*rCyl)
	cylX=[]
	ymin,ymax=-ui.cylLength/2.,ui.cylLength/2.
	zmin,zmax=-4*rCyl,rCyl+6*rCyl
	xmin,xmax=-3*rCyl,lastCylX+6*rCyl
	rMin=ui.psd[0][0]/2.
	rMax=ui.psd[-1][0]/2.
	s.dt=.7*utils.spherePWaveDt(rMin,ui.material.density,ui.material.young)

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
		c.impose=AlignedHarmonicOscillations(
			amps=(.05*rCyl,float('nan'),.03*rCyl),
			freqs=(1/((4000+(i%3-1)*500)*s.dt),float('nan'),1/((8000+(i%4)*1000)*s.dt))
		)
		de.par.append(c)
	A,B,C,D=(xmin,ymin,rCyl),(0,ymin,rCyl),(0,ymax,rCyl),(xmin,ymax,rCyl)
	de.par.append([utils.facet(vertices,material=ui.material,mask=wallMask) for vertices in ((A,B,C),(C,D,A))])

	incl=ui.inclination # is in radians
	grav=100.
	gravity=(grav*math.sin(incl),0,-grav*math.cos(incl))
	factStep=200
	s.engines=utils.defaultEngines(damping=.4,gravity=gravity,verletDist=.05*rMin)+[
		# what falls beyond
		# initially not saved
		BoxDeleter(stepPeriod=factStep,inside=True,box=((cylX[i],ymin,zmin),(cylX[i+1],ymax,-rCyl/2.)),color=.05*i,save=False,mask=delMask,label='aperture[%d]'%i) for i in range(0,ui.cylNum-1)
	]+[
		# this one should not collect any particles at all
		BoxDeleter(stepPeriod=factStep,box=((xmin,ymin,zmin),(xmax,ymax,zmax)),color=.9,save=False,mask=delMask,label='outOfDomain'),
		# what falls inside
		BoxDeleter(stepPeriod=factStep,inside=True,box=((lastCylX+rCyl,ymin,zmin),(xmax,ymax,zmax)),color=.9,save=True,mask=delMask,label='fallOver'),
		# generator
		BoxFactory(stepPeriod=factStep,box=((xmin,ymin,rCyl),(0,ymax,zmax)),color=.4,label='factory',
			massFlowRate=ui.massFlowRate,materials=[ui.material],
			generator=PsdSphereGenerator(psdPts=ui.psd,discrete=False,mass=True),
			shooter=AlignedMinMaxShooter(dir=(1,0,-.1),vRange=(ui.flowVel,ui.flowVel)),
			mask=sphMask,
			maxMass=-100, ## set negative, flip sign once we enter steady state
		),
		PyRunner(factStep,'import yade.pre.Roro_; yade.pre.Roro_.watchProgress()')
	]
	# improtant: save the preprocessor here!
	s.any=[yade.gl.Gl1_InfCylinder(wire=True),yade.gl.Gl1_Wall(div=3)]
	s.tags['preprocessor']=ui.dumps(format='pickle')
	print 'Generated Rollenrost.'
	de.collectNodes()
	return s

def watchProgress():
	'''initially, only the fallOver deleter saves particles; once particles arrive,
	it means we have reached some steady state; at that point, all objects (deleters,
	factory, ... are clear()ed so that PSD's and such correspond to the steady
	state only'''
	# not yet saving what falls through, i.e. not in stabilized regime yet
	if yade.aperture[0].save==False:
		# first particles have just fallen over
		# start saving apertures and reset our counters here
		if yade.fallOver.num>50: # FIXME: this number here is ugly
			yade.fallOver.clear()
			yade.factory.clear()
			yade.factory.maxMass=abs(yade.factory.maxMass)
			for ap in yade.aperture:
				ap.clear() # to clear overall mass, which gets counted even with save=False
				ap.save=True
			print 'Stabilized regime reached, counters started.'
	# already in the stable regime, end simulation at some point
	else:
		#remains=(yade.factory.maxMass-sum([a.mass for a in yade.aperture])-yade.fallOver.mass)/yade.factory.maxMass
		# if remains<.02:\n\tyade.pre.Roro_.plotFinalPsd(); print "Simulation finished."; O.pause()

		# factory has finished generating particles
		if yade.factory.mass>yade.factory.maxMass:
			plotFinalPsd()
			print 'Simulation finished.'
			yade.O.pause()

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
	ui=yade.pre.Roro.loads(yade.O.scene.tags['preprocessor'])
	#ui=[a for a in yade.O.scene.any if type(a)==yade.pre.Roro][0]
	print 'Parameters were:'
	ui.dump(sys.stdout,noMagic=True,format='expr')
	import matplotlib
	matplotlib.use('Agg')
	import pylab
	import os
	import numpy
	# http://www.mail-archive.com/matplotlib-users@lists.sourceforge.net/msg05474.html
	from matplotlib.ticker import FuncFormatter
	percent=FuncFormatter(lambda x,pos=0: '%g%%'%(100*x))

	fig=pylab.figure()
	fig.set_size_inches(8,24)
	ax=pylab.subplot(311)
	pylab.plot(*yade.factory.generator.inputPsd(scale=True),label='input PSD')
	pylab.plot(*yade.factory.generator.psd(),label='generated (mass %g)'%yade.factory.mass)
	for i in range(0,len(yade.aperture)): pylab.plot(*yade.aperture[i].psd(),label='aperture %d (mass %g)'%(i,yade.aperture[i].mass))
	pylab.plot(*yade.fallOver.psd(),label='fall over (mass %g)'%yade.fallOver.mass)
	pylab.grid(True)
	#ax.yaxis.set_major_formatter(percent)
	pylab.legend(loc='best')
	pylab.xlabel('particle diameter [m]')
	pylab.ylabel('passing fraction')

	ax=pylab.subplot(312)
	dd,mm=[],[] # lists with diameters and masses
	ff=yade.fallOver.diamMass()
	dd.append(ff[0]); mm.append(ff[1])
	dd.append([]); mm.append([]) # sum apertures together
	for a in yade.aperture:
		ff=a.diamMass()
		dd[-1].extend(ff[0]); mm[-1].extend(ff[1])
	pylab.hist(dd,weights=mm,bins=20,histtype='barstacked',label=['fallOver','apertures']) #+['aperture %d'%i for i in range(0,len(yade.aperture))])
	pylab.grid(True)
	pylab.xlabel('particle diameter [m]')
	pylab.ylabel('mass [kg]')
	pylab.legend(loc='best')

	ax=pylab.subplot(313)
	pylab.plot(*yade.factory.generator.inputPsd(scale=False),marker='o',label='prescribed')
	genPsd=yade.factory.generator.psd(normalize=True)
	inPsd=yade.factory.generator.inputPsd(scale=False)
	pylab.plot(*genPsd,label='generated')
	smallerPsd,biggerPsd=splitPsd(*inPsd,splitX=ui.gap)
	pylab.plot(*smallerPsd,marker='v',label='< gap')
	pylab.plot(*biggerPsd,marker='^',label='> gap')
	pylab.axvline(x=ui.gap,linewidth=2,ymin=0,ymax=1,color='g',label='gap')
	n,bins,patches=pylab.hist(dd[-1],weights=numpy.array(mm[-1])/sum([ap.mass for ap in yade.aperture]),histtype='step',label='apertures',normed=False,bins=60,cumulative=True,linewidth=2)
	patches[0].set_xy(patches[0].get_xy()[:-1])
	n,bins,patches=pylab.hist(dd[0],weights=mm[0],normed=True,bins=60,cumulative=True,histtype='step',label='fallOver',linewidth=2)
	patches[0].set_xy(patches[0].get_xy()[:-1])
	for a in yade.aperture:
		print a.mass,sum([dm[1] for dm in a.diamMass()]),'|',len(a.deleted),a.num
	pylab.ylim(ymin=-.05,ymax=1.05)

	#print 'smaller',smallerPsd
	#print 'bigger',biggerPsd
	pylab.grid(True)
	ax.yaxis.set_major_formatter(percent)
	pylab.xlabel('diameter [m]')
	pylab.ylabel('mass fraction')
	pylab.legend(loc='best')

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

