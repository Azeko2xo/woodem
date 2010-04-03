# -*- encoding=utf-8 -*-
from __future__ import division

from yade import utils,plot,pack,timing,eudoxos
import time, sys, os, copy

#import matplotlib
#matplotlib.rc('text',usetex=True)
#matplotlib.rc('text.latex',preamble=r'\usepackage{concrete}\usepackage{euler}')



"""
A fairly complex script performing uniaxial tension-compression test on hyperboloid-shaped specimen.

Most parameters of the model (and of the setup) can be read from table using yade-multi.

After the simulation setup, tension loading is run and stresses are periodically saved for plotting
as well as checked for getting below the maximum value so far. This indicates failure (see stopIfDamaged
function). After failure in tension, the original setup is loaded anew and the sense of loading reversed.
After failure in compression, strain-stress curves are saved via plot.saveGnuplot and we exit,
giving some useful information like peak stresses in tension/compression.

Running this script for the first time can take long time, as the specimen is prepared using triaxial
compression. Next time, however, an attempt is made to load previously-generated packing 
(from /tmp/triaxPackCache.sqlite) and this expensive procedure is avoided.

The specimen length can be specified, its diameter is half of the length and skirt of the hyperboloid is 
4/5 of the width.

The particle size is constant and can be specified using the sphereRadius parameter.

The 3d display has displacement scaling applied, so that the fracture looks more spectacular. The scale
is 1000 for tension and 100 for compression.

"""



# default parameters or from table
utils.readParamsFromTable(noTableOk=True, # unknownOk=True,
	young=24e9,
	poisson=.2,
	G_over_E=.20,

	sigmaT=3.5e6,
	frictionAngle=atan(0.8),
	epsCrackOnset=1e-4,
	relDuctility=30,

	intRadius=1.5,
	dtSafety=.8,
	damping=0.4,
	strainRateTension=.05,
	strainRateCompression=1,
	setSpeeds=True,
	# 1=tension, 2=compression (ANDed; 3=both)
	doModes=3,

	specimenLength=.15,
	sphereRadius=3.5e-3,

	# isotropic confinement (should be negative)
	isoPrestress=0,
)

if 'description' in O.tags.keys(): O.tags['id']=O.tags['id']+O.tags['description']


# make geom; the dimensions are hard-coded here; could be in param table if desired
# z-oriented hyperboloid, length 20cm, diameter 10cm, skirt 8cm
# using spheres 7mm of diameter
concreteId=O.materials.append(CpmMat(young=young,frictionAngle=frictionAngle,poisson=poisson,density=4800,sigmaT=sigmaT,relDuctility=relDuctility,epsCrackOnset=epsCrackOnset,G_over_E=G_over_E,isoPrestress=isoPrestress))

#spheres=pack.randomDensePack(pack.inHyperboloid((0,0,-.5*specimenLength),(0,0,.5*specimenLength),.25*specimenLength,.17*specimenLength),spheresInCell=2000,radius=sphereRadius,memoizeDb='/tmp/triaxPackCache.sqlite',material=concreteId)
spheres=pack.randomDensePack(pack.inAlignedBox((-.25*specimenLength,-.25*specimenLength,-.5*specimenLength),(.25*specimenLength,.25*specimenLength,.5*specimenLength)),spheresInCell=2000,radius=sphereRadius,memoizeDb='/tmp/triaxPackCache.sqlite')
O.bodies.append(spheres)
bb=utils.uniaxialTestFeatures()
negIds,posIds,axis,crossSectionArea=bb['negIds'],bb['posIds'],bb['axis'],bb['area']
O.dt=dtSafety*utils.PWaveTimeStep()
print 'Timestep',O.dt

mm,mx=[pt[axis] for pt in utils.aabbExtrema()]
coord_25,coord_50,coord_75=mm+.25*(mx-mm),mm+.5*(mx-mm),mm+.75*(mx-mm)
area_25,area_50,area_75=utils.approxSectionArea(coord_25,axis),utils.approxSectionArea(coord_50,axis),utils.approxSectionArea(coord_75,axis)

O.engines=[
	ForceResetter(),
	BoundDispatcher([Bo1_Sphere_Aabb(aabbEnlargeFactor=intRadius,label='is2aabb'),]),
	InsertionSortCollider(sweepLength=.05*sphereRadius,nBins=5,binCoeff=5),
	InteractionDispatchers(
		[Ig2_Sphere_Sphere_Dem3DofGeom(distFactor=intRadius,label='ss2d3dg')],
		[Ip2_CpmMat_CpmMat_CpmPhys()],
		[Law2_Dem3DofGeom_CpmPhys_Cpm(epsSoft=0)], # deactivated
	),
	NewtonIntegrator(damping=damping,label='damper'),
	CpmStateUpdater(realPeriod=1),
	UniaxialStrainer(strainRate=strainRateTension,axis=axis,asymmetry=0,posIds=posIds,negIds=negIds,crossSectionArea=crossSectionArea,blockDisplacements=False,blockRotations=False,setSpeeds=setSpeeds,label='strainer'),
	PeriodicPythonRunner(virtPeriod=3e-5/strainRateTension,realPeriod=1,command='addPlotData()',label='plotDataCollector',initRun=True),
	PeriodicPythonRunner(realPeriod=4,command='stopIfDamaged()',label='damageChecker'),
]
#O.miscParams=[Gl1_CpmPhys(dmgLabel=False,colorStrain=False,epsNLabel=False,epsT=False,epsTAxes=False,normal=False,contactLine=True)]

# plot stresses in ¼, ½ and ¾ if desired as well; too crowded in the graph that includes confinement, though
plot.plots={'eps':('sigma',)} #'sigma.25','sigma.50','sigma.75')}
plot.maxDataLen=4000

O.saveTmp('initial');

O.timingEnabled=False

global mode
mode='tension' if doModes & 1 else 'compression'

def initTest():
	global mode
	print "init"
	if O.iter>0:
		O.wait();
		O.loadTmp('initial')
		print "Reversing plot data"; plot.reverseData()
	strainer.strainRate=abs(strainRateTension) if mode=='tension' else -abs(strainRateCompression)
	try:
		from yade import qt
		renderer=qt.Renderer()
		renderer.dispScale=(1000,1000,1000) if mode=='tension' else (100,100,100)
	except ImportError: pass
	print "init done, will now run."
	O.step(); O.step(); # to create initial contacts
	# now reset the interaction radius and go ahead
	ss2d3dg.distFactor=-1.
	is2aabb.aabbEnlargeFactor=-1.
	O.run()

def stopIfDamaged():
	global mode
	if O.iter<2 or not plot.data.has_key('sigma'): return # do nothing at the very beginning
	sigma,eps=plot.data['sigma'],plot.data['eps']
	extremum=max(sigma) if (strainer.strainRate>0) else min(sigma)
	minMaxRatio=0.5 if mode=='tension' else 0.5
	if extremum==0: return
	# uncomment to get graph for the very first time stopIfDamaged() is called
	#eudoxos.estimatePoissonYoung(principalAxis=axis,stress=strainer.avgStress,plot=True,cutoff=0.3)
	print O.tags['id'],mode,strainer.strain,sigma[-1]
	import sys;	sys.stdout.flush()
	if abs(sigma[-1]/extremum)<minMaxRatio or abs(strainer.strain)>(5e-3 if isoPrestress==0 else 5e-2):
		if mode=='tension' and doModes & 2: # only if compression is enabled
			mode='compression'
			#O.save('/tmp/uniax-tension.xml.bz2')
			print "Damaged, switching to compression... "; O.pause()
			# important! initTest must be launched in a separate thread;
			# otherwise O.load would wait for the iteration to finish,
			# but it would wait for initTest to return and deadlock would result
			import thread; thread.start_new_thread(initTest,())
			return
		else:
			print "Damaged, stopping."
			ft,fc=max(sigma),min(sigma)
			print 'Strengths fc=%g, ft=%g, |fc/ft|=%g'%(fc,ft,abs(fc/ft))
			title=O.tags['description'] if 'description' in O.tags.keys() else O.tags['params']
			print 'gnuplot',plot.saveGnuplot(O.tags['id'],title=title)
			print 'Bye.'
			# O.pause()
			sys.exit(0)
		
def addPlotData():
	yade.plot.addData({'t':O.time,'i':O.iter,'eps':strainer.strain,'sigma':strainer.avgStress+isoPrestress,
		'sigma.25':utils.forcesOnCoordPlane(coord_25,axis)[axis]/area_25+isoPrestress,
		'sigma.50':utils.forcesOnCoordPlane(coord_50,axis)[axis]/area_50+isoPrestress,
		'sigma.75':utils.forcesOnCoordPlane(coord_75,axis)[axis]/area_75+isoPrestress,
		})

initTest()
# sleep forever if run by yade-multi, exit is called from stopIfDamaged
if os.environ.has_key('PARAM_TABLE'): time.sleep(1e12)



