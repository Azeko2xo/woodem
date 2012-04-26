from yade import utils,pack
from yade.core import *
from yade.dem import *
import yade.gl
import math

def run(ui): # use inputs as argument
	print 'Roro_.run(...)'
	s=Scene()
	de=DemField();
	s.fields=[de]
	rCyl=ui.cylDiameter/2.
	lastCylX=(ui.cylNum-1)*(ui.gap+2*rCyl)
	ymin,ymax=-ui.cylLength/2.,ui.cylLength/2.
	zmin,zmax=-rCyl,rCyl+6*rCyl
	xmin,xmax=-3*rCyl,lastCylX+3*rCyl
	rMin=ui.psd[0][0]/2.
	rMax=ui.psd[-1][0]/2.
	s.dt=.7*utils.spherePWaveDt(rMin,ui.material.density,ui.material.young)

	wallMask=0b00110
	loneMask=0b00100
	sphMask= 0b00011
	delMask= 0b00001
	
	de.par.append([
		utils.wall(ymin,axis=1,sense= 1,material=ui.material,mask=wallMask),
		utils.wall(ymax,axis=1,sense=-1,material=ui.material,mask=wallMask)
	])
	for i in range(0,ui.cylNum):
		x=i*(2*rCyl+ui.gap)
		c=utils.infCylinder((x,0,0),radius=rCyl,axis=1,material=ui.material,mask=wallMask)
		c.angVel=(0,ui.angVel,0)
		c.impose=AlignedHarmonicOscillations(
			amps=(.05*rCyl,float('nan'),.03*rCyl),
			freqs=(1/((4000+(i%3-1)*500)*s.dt),float('nan'),1/((8000+(i%4)*1000)*s.dt))
		)
		de.par.append(c)
	A,B,C,D=(-4*rCyl,ymin,rCyl),(0,ymin,rCyl),(0,ymax,rCyl),(-4*rCyl,ymax,rCyl)
	de.par.append([utils.facet(vertices,material=ui.material,mask=wallMask) for vertices in ((A,B,C),(C,D,A))])

	incl=ui.inclination*math.pi/180. # in radians
	grav=100.
	gravity=(grav*math.sin(incl),0,-grav*math.cos(incl))
	factStep=500
	s.engines=utils.defaultEngines(damping=.4,gravity=gravity,verletDist=.05*rMin)+[
		# what falls beyond
		BoxDeleter(stepPeriod=factStep,box=((xmin,ymin,zmin),(xmax,ymax,zmax)),color=.9,save=True,mask=delMask,label='fallThrough'),
		# what falls inside
		BoxDeleter(stepPeriod=factStep,inside=True,box=((lastCylX+rCyl,ymin,zmin),(xmax,ymax,zmax)),color=.9,save=True,mask=delMask,label='fallOver'),
		# generator
		BoxFactory(stepPeriod=factStep,box=((xmin,ymin,rCyl),(0,ymax,zmax)),color=.4,label='factory',
			massFlowRate=ui.massFlowRate,materials=[ui.material],
			generator=PsdSphereGenerator(psd=ui.psd),
			shooter=AlignedMinMaxShooter(dir=(1,0,-.1),vRange=(ui.flowVel,ui.flowVel)),
			mask=sphMask
		),

	]
	s.any=[yade.gl.Gl1_InfCylinder(wire=True),yade.gl.Gl1_Wall(div=3)]
	print 'Generated Rollenrost.'
	de.collectNodes()
	return s

def plotFinalPsd():
	import pylab
	import yade
	import os
	pylab.plot(*yade.fallThrough.psd(num=80,zip=False),label='fall through (mass %g)'%yade.fallThrough.mass)
	pylab.plot(*yade.fallOver.psd(num=80,zip=False),label='fall over (mass %g)'%yade.fallOver.mass)
	pylab.grid(True)
	pylab.legend(loc='lower right')
	pylab.xlabel('particle diameter [m]')
	pylab.ylabel('passing fraction')
	if 1: pylab.show()
	else:
		out=yade.O.tmpFilename()+".pdf"
		pylab.savefig(out)
		os.system("xdg-open '%s' &"%out) # not safe, but whatever here

# test drive
if __name__=='__main__':
	import yade.pre
	import yade.qt
	import yade.plot
	O.scene=yade.pre.Roro()()
	O.scene.engines=O.scene.engines+[PyRunner(20000,'yade.pre.Roro_.plotFinalPsd(); O.pause();',initRun=False)]
	O.saveTmp()
	yade.qt.View()
	O.run()

