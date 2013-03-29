# encoding: utf-8

from woo.dem import *
import woo.core
import woo.dem
import woo.pyderived
import woo
import math
from minieigen import *
import numpy

class CylTriaxTest(woo.core.Preprocessor,woo.pyderived.PyWooObject):
	'Preprocessor for cylindrical triaxial test'
	_classTraits=None
	_PAT=woo.pyderived.PyAttrTrait # less typing
	_attrTraits=[
		# noGui would make startGroup being ignored
		_PAT(float,'isoStress',-1e4,unit='kPa',startGroup='General',doc='Confining stress (isotropic during compaction)'),
		_PAT(float,'maxStrainRate',1e-3,'Maximum strain rate during the compaction phase, and during the triaxial phase in axial sense'),
		_PAT(int,'nPar',2000,startGroup='Particles',doc='Number of particles'),
		_PAT(woo.dem.FrictMat,'mat',FrictMat(young=1e5,ktDivKn=.2,tanPhi=0.,density=1e8),'Material of particles.'),
		_PAT(woo.dem.FrictMat,'meshMat',None,'Material of boundaries; if not given, material of particles is used.'),
		_PAT(float,'tanPhi2',.6,'If :obj:`mat` defines :obj`tanPhi <woo.dem.FrictMat.tanPhi>`, it will be changed to this value progressively after the compaction phase.'),
		_PAT([Vector2,],'psd',[(2e-3,0),(2.5e-3,.2),(4e-3,1.)],unit=['mm','%'],doc='Particle size distribution of particles; first value is diameter, scond is cummulative mass fraction.'),
		_PAT(float,'sigIso',-50e3,unit='Pa',doc='Isotropic compaction stress, and lateral stress during the triaxial phase'),

		_PAT(Vector2,'radHt',Vector2(.02,.05),doc='Initial size of the cylinder (radius and height)'),

		_PAT(str,'reportFmt',"/tmp/{tid}.xhtml",startGroup="Outputs",doc="Report output format; :obj:`Scene.tags <woo.core.Scene.tags>` can be used."),
		_PAT(str,'packCacheDir',".","Directory where to store pre-generated feed packings; if empty, packing wil be re-generated every time."),
		_PAT(str,'saveFmt',"/tmp/{tid}-{stage}.bin.gz",'''Savefile format; keys are :obj:`Scene.tags <woo.core.Scene.tags>`; additionally ``{stage}`` will be replaced by
* ``init`` for stress-free but compact cloud,
* ``iso`` after isotropic compaction,
* ``backup-011234`` for regular backups, see :obj:`backupSaveTime`,
 'done' at the very end.
'''),
		_PAT(int,'backupSaveTime',1800,doc='How often to save backup of the simulation (0 or negative to disable)'),
		_PAT(float,'pWaveSafety',.7,startGroup='Tunables',doc='Safety factor for :obj:`woo.utils.pWaveDt` estimation.'),
		_PAT(float,'nonViscDamp',.4,'The value of :obj:`woo.dem.Leapfrog.damping`; oinly use if the contact model used has no internal damping.'),
		_PAT(float,'initPoro',.7,'Estimate of initial porosity, when generating loose cloud'),
		_PAT(Vector2i,'cylDiv',(50,4),'Number of segments for cylinder (first component) and height segments (second component)'),
		_PAT(float,'damping',.5,'Nonviscous damping'),
	]
	def __init__(self,**kw):
		woo.core.Preprocessor.__init__(self)
		self.wooPyInit(self.__class__,woo.core.Preprocessor,**kw)
	def __call__(self):
		# preprocessor builds the simulation when called
		return prepareCylTriax(self)

def prepareCylTriax(pre):
	import woo
	margin=.6
	rad,ht=pre.radHt[0],pre.radHt[1]
	bot,top=margin*ht,(1+margin)*ht
	xymin=Vector2(margin*rad,margin*rad)
	xymax=Vector2((margin+2)*rad,(margin+2)*rad)
	xydomain=Vector2((2*margin+2)*rad,(2*margin+2)*rad)
	xymid=.5*xydomain
	S=woo.core.Scene(fields=[DemField()])
	S.pre=pre.deepcopy()

	meshMask=0b0011
	spheMask=0b0001
	loneMask=0b0010

	S.dem.loneMask=loneMask
	if not pre.meshMat: pre.meshMat=pre.mat
	S.engines=[
		woo.dem.InsertionSortCollider([woo.dem.Bo1_Sphere_Aabb()]),
		woo.dem.BoxFactory(
			box=((xymin[0],xymin[1],bot),(xymax[0],xymax[1],top)),
			maxMass=-1,
			maxNum=-1,
			massFlowRate=0,
			maxAttempts=5000,
			generator=woo.dem.PsdSphereGenerator(psdPts=pre.psd,discrete=False,mass=True),
			materials=[pre.mat],
			shooter=None,
			mask=spheMask,
		)
	]
	S.one()
	print 'Created %d particles'%(len(S.dem.par))
	# remove particles not inside the cylinder
	out=0
	# radius minus polygonal imprecision (circle segment)
	innerRad=rad-rad*(1.-math.cos(.5*2*math.pi/pre.cylDiv[0]))
	print 'inner radius',rad,innerRad
	for p in S.dem.par:
		if (Vector2(p.pos[0],p.pos[1])-xymid).norm()+p.shape.radius>innerRad:
			out+=1
			S.dem.par.remove(p.id)
	print 'Removed %d particles outside the cylinder'%out
	sphereMass=sum([p.mass for p in S.dem.par])

	S.periodic=True
	S.cell.setBox(xydomain[0],xydomain[1],(2*margin+1)*ht)

	# make cylinder
	thetas=numpy.linspace(0,2*math.pi,num=pre.cylDiv[0],endpoint=True)
	xxyy=[Vector2(rad*math.cos(th)+xymid[0],rad*math.sin(th)+xymid[1]) for th in thetas]
	ff=woo.pack.gtsSurface2Facets(
		woo.pack.sweptPolylines2gtsSurface(
			[[Vector3(xymid[0],xymid[1],bot) for xy in xxyy]]+
			[[Vector3(xy[0],xy[1],z) for xy in xxyy] for z in numpy.linspace(bot,top,num=pre.cylDiv[1],endpoint=True)]+
			[[Vector3(xymid[0],xymid[1],top) for xy in xxyy]],
			threshold=rad*2*math.pi/(10.*pre.cylDiv[0])
		),mask=meshMask,mat=pre.meshMat,fixed=True)
	# add facet nodes to the simulation so that they move with the space
	nn=set()
	for f in ff: nn.update(set(f.shape.nodes))
	for n in nn: S.dem.nodes.append(n)
	# append facets as such
	S.dem.par.append(ff)

	S.dt=pre.pWaveSafety*woo.utils.pWaveDt(S)
	# setup engines
	S.engines=[
		InsertionSortCollider([Bo1_Sphere_Aabb(),Bo1_Facet_Aabb()],verletDist=-.05),
		ContactLoop([Cg2_Sphere_Sphere_L6Geom(),Cg2_Facet_Sphere_L6Geom()],[Cp2_FrictMat_FrictPhys()],[Law2_L6Geom_FrictPhys_IdealElPl()],applyForces=True),
		WeirdTriaxControl(goal=(pre.sigIso,pre.sigIso,pre.sigIso),maxStrainRate=(.2,.2,.2),relVol=math.pi*rad**2*ht/S.cell.volume,stressMask=0b0111,maxUnbalanced=0.05,mass=100*sphereMass,doneHook='import woo.pre.cylTriax; woo.pre.cylTriax.compactionDone(S)',label='triax'),
		woo.core.PyRunner(20,'import woo.pre.cylTriax; woo.pre.cylTriax.addPlotData(S)'),
		Leapfrog(damping=pre.damping,reset=True),
	]
	S.timingEnabled=True
	import woo.log
	woo.log.setLevel('InsertionSortCollider',woo.log.TRACE)
	woo.log.setLevel('WeirdTriaxControl',woo.log.TRACE)

	return S

def addPlotData(S):
	import woo
	t=woo.triax
	S.plot.addData(unbalanced=woo.utils.unbalancedForce(),i=S.step,
	sxx=t.stress[0,0],syy=t.stress[1,1],srr=.5*(t.stress[0,0]+t.stress[1,1]),szz=t.stress[2,2],
	exx=t.strain[0],eyy=t.strain[1],err=.5*(t.strain[0]+t.strain[1]),ezz=t.strain[2],
		# save all available energy data
		#Etot=O.energy.total()#,**O.energy
	)
	if not S.plot.plots:
		S.plot.plots={
			'i':('unbalanced',),'i ':('sxx','syy','srr','szz'),' i':('exx','eyy','err','ezz'),
			# energy plot
			#' i ':(O.energy.keys,None,'Etot'),
		}
		S.plot.xylabels={'i ':'Stress [Pa]',' i':'Strains [-]'}
		S.plot.labels={
			'sxx':r'$\sigma_{xx}$','syy':r'$\sigma_{yy}$','szz':r'$\sigma_{zz}$','srr':r'$\sigma_{rr}$',
			'exx':r'$\varepsilon_{xx}$','eyy':r'$\varepsilon_{yy}$','ezz':r'$\varepsilon_{zz}$','err':r'$\varepsilon_{rr}$'
		}




def compactionDone(S):
	print 'Compaction done at step',S.step
	import woo
	t=woo.triax
	# set the current cell configuration to be the reference one
	S.cell.trsf=Matrix3.Identity
	# change control type: keep constant confinement in x,y, 20% compression in z
	t.goal=(S.pre.sigIso,S.pre.sigIso,-.5)
	t.stressMask=0b0011 # z is strain-controlled, x,y stress-controlled
	# allow faster deformation along x,y to better maintain stresses
	t.maxStrainRate=(1.,1.,.1)
	# next time, call triaxFinished instead of compactionFinished
	t.doneHook='import woo.pre.cylTriax; woo.pre.cylTriax.triaxDone(S)'
	# do not wait for stabilization before calling triaxFinished
	t.maxUnbalanced=10

def triaxDone(S):
	print 'Triaxial done at step',S.step
	S.stop()


