# encoding: utf-8

from woo.dem import *
import woo.core
import woo.dem
import woo.pyderived
import woo.triangulated
import math
from minieigen import *
import woo.models

class TriaxTest(woo.core.Preprocessor,woo.pyderived.PyWooObject):
	r'''Preprocessor for triaxial test with rigid boundary. The test is run in 2 stages:
	
		* **compaction** where random loose packing is compressed to attain :math:`\sigma_{\rm iso}` (:obj:`sigIso`) in all directions. The compaction finishes when the stress level is sufficiently close to :obj:`sigIso` and unbalanced force drops below :obj:`maxUnbalanced`.

		* **Triaxial compression**: displacement-controlled compression along the ``z``-axis, with strain rate increasing until :obj:`maxRates` is reached; the test finished when axial strain attains :obj:`stopStrain`.

	.. youtube:: qWZBCQbS6x4

	'''
	_classTraits=None
	_PAT=woo.pyderived.PyAttrTrait # less typing

	def postLoad(self,I):
		if self.preCooked:
			print 'Applying pre-cooked configuration "%s".'%self.preCooked
			if self.preCooked=='Spheres in cylinder':
				self.generator=woo.dem.PsdSphereGenerator(psdPts=[(.01,0),(.04,1.)])
				self.shape='cylinder'
			elif self.preCooked=='Capsules in cylinder':
				self.generator=woo.dem.PsdCapsuleGenerator(psdPts=[(.01,0),(.04,1.)],shaftRadiusRatio=(.6,1.2))
				self.shape='cylinder'
			elif self.preCooked=='Ellipsoids in box':
				self.generator=woo.dem.PsdEllipsoidGenerator(psdPts=[(.01,0),(.04,1.)],axisRatio2=(.5,.9),axisRatio3=(.3,.7))
				self.shape='box'
			elif self.preCooked=='Sphere clumps in box':
				self.generator=woo.dem.PsdClumpGenerator(psdPts=[(.01,0),(.04,1.)],clumps=[
					SphereClumpGeom(centers=[(0,0,-.15),(0,0,-.05),(0,0,.05)],radii=(.05,.08,.05),scaleProb=[(0,1.)]),
					SphereClumpGeom(centers=[(.05,0,0) ,(0,.05,0) ,(0,0,.05)],radii=(.05,.05,.05),scaleProb=[(0,.5)]),
				])
				self.shape='box'
			else: raise RuntimeError('Unknown precooked configuration "%s"'%self.preCooked)
			self.preCooked=''

	_attrTraits=[
		_PAT(str,'preCooked','',noDump=True,noGui=False,startGroup='Predefined config',choice=['','Spheres in cylinder','Capsules in cylinder','Ellipsoids in box','Sphere clumps in box'],triggerPostLoad=True,doc='Apply pre-cooked configuration (i.e. change other parameters); this option is not saved.'),

		# noGui would make startGroup being ignored
		_PAT(float,'sigIso',-500e3,unit='kPa',startGroup='General',doc='Confining stress (isotropic during compaction)'),
		_PAT(Vector2,'maxRates',(2e-1,2e-1),'Maximum strain rate during the compaction phase, and during the triaxial phase in axial sense'),
		_PAT(float,'stopStrain',-.3,unit=r'%',doc='Goal value of axial deformation in the triaxial phase'),
		_PAT(str,'shape','cell',choice=('cell','box','cylinder'),doc='Shape of the volume being compressed; *cell* is rectangular periodic cell, *box* is rectangular :obj:`~woo.dem.Wall`-delimited box, *cylinder* is triangulated cylinder aligned with the :math:`z`-axis'),
		_PAT(Vector3,'iniSize',(.3,.3,.6),unit='m',doc='Initial size of the volume; when :obj:`shape` is ``cylinder``, the second (:math:`y`) dimension is ignored.'),
		_PAT(woo.dem.ParticleGenerator,'generator',woo.dem.PsdCapsuleGenerator(psdPts=[(.01,0),(.04,1.)],shaftRadiusRatio=(.6,1.2)),
			# woo.dem.PsdEllipsoidGenerator(psdPts=[(.01,0),(.04,1.)],axisRatio2=(.5,.9),axisRatio3=(.3,.7))
			# woo.dem.PsdSphereGenerator(psdPts=[(.01,0),(.04,1.)])
			'Particle generator; partices are then randomly placed in the volume.'),
		_PAT(woo.models.ContactModelSelector,'model',woo.models.ContactModelSelector(name='linear',damping=.7,numMat=(1,2),matDesc=['particles','boundary'],mats=[woo.dem.FrictMat(density=1e7,young=1e8)]),doc='Select contact model. The first material is for particles; the second, optional, material, is for the boundary (the first material is used if there is no second one).'),

		_PAT(str,'reportFmt',"/tmp/{tid}.xhtml",startGroup="Outputs",doc="Report output format; :obj:`Scene.tags <woo.core.Scene.tags>` can be used."),
		_PAT(str,'saveFmt',"/tmp/{tid}-{stage}.bin.gz",'''Savefile format; keys are :obj:`Scene.tags <woo.core.Scene.tags>`; additionally ``{stage}`` will be replaced by
* ``init`` for stress-free but compact cloud,
* ``iso`` after isotropic compaction,
* ``backup-011234`` for regular backups, see :obj:`backupSaveTime`, 'done' at the very end.
'''),
		# _PAT(int,'backupSaveTime',1800,doc='How often to save backup of the simulation (0 or negative to disable)'),
		_PAT(float,'dtSafety',.7,startGroup='Tunables',doc='See :obj:`woo.core.Scene.dtSafety`.'),
		_PAT(float,'maxUnbalanced',.1,'Maximum unbalanced force at the end of compaction'),
		_PAT(int,'cylDiv',40,hideIf='self.shape!="cylinder"',doc='Number of segments to approximate the cylinder with.'),
		_PAT(float,'massFactor',.2,'Multiply real mass of particles by this number to obtain the :obj:`woo.dem.WeirdTriaxControl.mass` control parameter'),
		_PAT(float,'rateStep',.01,'Increase strain rate by this relative amount at the beginning of the triaxial phase, until the value given in :obj:`maxRates` is reached.'),
		# _PAT(float,'damping',.4,'Numerical damping (in addition to material damping, if any.')
	]
	def __init__(self,**kw):
		woo.core.Preprocessor.__init__(self)
		self.wooPyInit(self.__class__,woo.core.Preprocessor,**kw)
	def __call__(self):
		# preprocessor builds the simulation when called
		return prepareTriax(self)


def prepareTriax(pre):
	import woo
	S=woo.core.Scene(fields=[DemField()])
	S.pre=pre.deepcopy()
	S.lab.partMat=pre.model.mats[0]
	wallMat=S.lab.partMat if len(pre.model.mats)==1 else pre.model.mats[1]
	# initially no friciton
	S.lab.partMat.tanPhi=0.

	partMask=0b0001
	wallMask=0b0011
	loneMask=0b0010
	S.dem.loneMask=loneMask

	margin=.1

	factoryKw=dict(maxMass=-1,maxNum=-1,generator=pre.generator,massRate=0,maxAttempts=5000,materials=[S.lab.partMat],shooter=None,mask=partMask,collideExisting=False)

	if pre.shape=='cell':
		factory=woo.dem.BoxFactory(box=((0,0,0),pre.iniSize),**factoryKw)
		S.lab.relVol=1.
		margin=0. # no margins, use the full volume
	elif pre.shape=='box':
		mid=(margin+.5)*pre.iniSize;
		for ax in (0,1,2):
			for sense in (-1,1):
				# make copy of mid
				pos=Vector3(mid); pos[ax]-=sense*.5*pre.iniSize[ax]
				S.dem.par.append(woo.utils.wall(pos,sense=sense,axis=ax,mat=wallMat,mask=wallMask))
		factory=woo.dem.BoxFactory(box=(margin*pre.iniSize,(1+margin)*pre.iniSize),**factoryKw)
		S.lab.relVol=AlignedBox3((0,0,0),pre.iniSize).volume()/AlignedBox3((0,0,0),(1+2*margin)*pre.iniSize).volume()
	elif pre.shape=='cylinder':
		d,h=pre.iniSize[0],pre.iniSize[2]
		bot=Vector3((.5+margin)*d,(.5+margin)*d,margin*h)
		top=bot+(0,0,h)
		factory=woo.dem.CylinderFactory(node=woo.core.Node(pos=bot,ori=Quaternion((0,1,0),-math.pi/2.)),radius=.5*d,height=h,**factoryKw)
		# axDiv to make sure we don't have facet larger than half of the cell (collider limitation)
		S.dem.par.append(woo.triangulated.cylinder(bot,top,radius=.5*d,div=pre.cylDiv,axDiv=3,capA=True,capB=True,wallCaps=True,mat=wallMat,mask=wallMask))
		S.lab.relVol=math.pi*d*h/AlignedBox3((0,0,0),(1+2*margin)*pre.iniSize).volume()
		if isinstance(pre.generator,woo.dem.PsdEllipsoidGenerator): raise NotImplementedError('It is not possible to combine ellipsoids with cylindrical boundary, since Ellipsoid+Facet collisions are not (yet) supported.')
	else: raise RuntimeError('Triax.shape must be one of "cell", "box" or "cylinder".')

	# generate as many particles as possible
	# list(woo.system.childClasses(woo.dem.BoundFunctor)) 
	S.engines=[factory]
	S.one()

	S.dem.collectNodes()
	print 'Number of nodes:',len(S.dem.nodes)

	S.periodic=True
	S.cell.setBox((1+2*margin)*pre.iniSize)
	S.dtSafety=pre.dtSafety

	S.engines=[
		woo.dem.WeirdTriaxControl(goal=(pre.sigIso,pre.sigIso,pre.sigIso),maxStrainRate=(pre.maxRates[0],pre.maxRates[0],pre.maxRates[0]),relVol=S.lab.relVol,stressMask=0b01111,globUpdate=1,maxUnbalanced=pre.maxUnbalanced,mass=pre.massFactor*sum([n.dem.mass for n in S.dem.nodes]),doneHook='import woo.pre.triax; woo.pre.triax.compactionDone(S)',label='triax',absStressTol=1e4,relStressTol=1e-2),
		woo.core.PyRunner(20,'import woo.pre.cylTriax; woo.pre.triax.addPlotData_checkProgress(S)')
	]+woo.utils.defaultEngines(model=pre.model,dynDtPeriod=100)
		
	S.lab.stage='compact'
	S.lab._setWritable('stage')

	try:
		import woo.gl
		S.any=[woo.gl.Renderer(dispScale=(5,5,2),rotScale=0,cell=True if pre.shape=='cell' else False),woo.gl.Gl1_DemField(colorBy=woo.gl.Gl1_DemField.colorRadius),woo.gl.Gl1_CPhys(),woo.gl.Gl1_Wall(div=3)]
	except ImportError: pass
	
	return S

def addPlotData_checkProgress(S):
	assert S.lab.stage in ('compact','triax')
	import woo
	t=S.lab.triax

	sxx,syy,szz=t.stress.diagonal() 
	dotE=S.cell.gradV.diagonal()
	dotEMax=t.maxStrainRate
	# net volume
	vol=S.cell.volume*S.lab.relVol
	# current radial stress
	srr=.5*(sxx+syy) 
	# mean stress
	p=t.stress.diagonal().sum()/3.
	# deviatoric stress
	q=szz-.5*(sxx+syy) 
	qDivP=(q/p if p!=0 else float('nan'))
	
	if S.lab.stage=='compact':
		## t.strain is log(l/l0) for all components
		exx,eyy,ezz=t.strain 
		err=.5*(exx+eyy)
		# volumetric strain is not defined directly, and it is not needed either		
		eVol=float('nan')
	else:
		# triaxial phase:
		# only axial strain (ezz) and volumetric strain (eVol) are known
		#
		# set the initial volume, if not yet done
		if not hasattr(S.lab,'netVol0'): S.lab.netVol0=S.cell.volume*S.lab.relVol
		# axial strain is known; xy components irrelevant (inactive)
		ezz=t.strain[2] 
		# current volume / initial volume
		eVol=math.log(vol/S.lab.netVol0) 
		# radial strain
		err=.5*(eVol-ezz) 
		# undefined
		exx=eyy=float('nan') 

	# deviatoric strain
	eDev=ezz-(1/3.)*(2*err+ezz) # FIXME: is this correct?!

	S.plot.addData(unbalanced=woo.utils.unbalancedForce(),i=S.step,
		sxx=sxx,syy=syy,srr=.5*(sxx+syy),szz=szz,
		exx=exx,eyy=eyy,err=err,ezz=ezz,
		dotE=dotE,dotErr=.5*(dotE[0]+dotE[1]),
		dotEMax=dotEMax,
		dotEMax_z_neg=-dotEMax[2],
		eDev=eDev,eVol=eVol,
		vol=vol,
		p=p,q=q,qDivP=qDivP,
		isTriax=(1 if S.lab.stage=='triax' else 0), # to be able to filter data
		# parTanPhi=S.lab.partMat.tanPhi,
		#memTanPhi=S.lab.memMat.tanPhi,
		#suppTanPhi=S.lab.suppMat.tanPhi
		# save all available energy data
		#Etot=O.energy.total()#,**O.energy
	)

	if not S.plot.plots:
		S.plot.plots={
			'i':('unbalanced',None,'vol'),'i ':('srr','szz'),' i':('err','ezz','eVol'),'i  ':('dotE_z','dotEMax_z'),
			'eDev':(('qDivP','g-'),None,('eVol','r-')),'p':('q',),
			# energy plot
			#' i ':(O.energy.keys,None,'Etot'),
		}
		S.plot.xylabels={'i ':('step','Stress [Pa]',),' i':('step','Strains [-]','Strains [-]')}
		S.plot.labels={
			'sxx':r'$\sigma_{xx}$','syy':r'$\sigma_{yy}$','szz':r'$\sigma_{zz}$','srr':r'$\sigma_{rr}$','surfLoad':r'$\sigma_{\rm hydro}$',
			'exx':r'$\varepsilon_{xx}$','eyy':r'$\varepsilon_{yy}$','ezz':r'$\varepsilon_{zz}$','err':r'$\varepsilon_{rr}$','eVol':r'$\varepsilon_{v}$','vol':'volume','eDev':r'$\varepsilon_d$','qDivP':'$q/p$','p':'$p$','q':'$q$',
			'dotE_x':r'$\dot\varepsilon_{xx}$','dotE_y':r'$\dot\varepsilon_{yy}$','dotE_z':r'$\dot\varepsilon_{zz}$','dotE_rr':r'$\dot\varepsilon_{rr}$','dotEMax_z':r'$\dot\varepsilon_{zz}^{\rm max}$','dotEMax_z_neg':r'$-\dot\varepsilon_{zz}^{\rm max}$'
		}


	## adjust rate in the triaxial stage
	if S.lab.stage=='triax':
		t.maxStrainRate[2]=min(t.maxStrainRate[2]+S.pre.rateStep*S.pre.maxRates[1],S.pre.maxRates[1])
		S.lab.contactLoop.updatePhys=False # was turned on when changing friction angle


def compactionDone(S):
	# if S.lab.compactMemoize:
	print 'Compaction done at step',S.step
	import woo
	t=S.lab.triax
	# set the current cell configuration to be the reference one
	S.cell.trsf=Matrix3.Identity
	S.cell.refHSize=S.cell.hSize
	S.cell.nextGradV=Matrix3.Zero # avoid spurious strain from the last gradV value
	##  S.lab.leapfrog.damping=.7 # increase damping to a larger value
	t.stressMask=0b0000 # strain control in all directions
	t.goal=(0,0,S.pre.stopStrain)
	t.maxStrainRate=(0,0,S.pre.rateStep*S.pre.maxRates[1]) # start with small rate, increases later
	t.maxUnbalanced=10 # don't care about that
	t.doneHook='import woo.pre.triax; woo.pre.triax.triaxDone(S)'

	# recover friction angle
	S.lab.partMat.tanPhi=S.pre.model.mats[0].tanPhi
	# force update of contact parameters
	S.lab.contactLoop.updatePhys=True


	try:
		import woo.gl
		woo.gl.Gl1_DemField.updateRefPos=True
	except ImportError: pass

	S.lab.stage='triax'

	if S.pre.saveFmt:
		out=S.pre.saveFmt.format(stage='compact',S=S,**(dict(S.tags)))
		print 'Saving to',out
		S.save(out)


def triaxDone(S):
	print 'Triaxial done at step',S.step
	if S.pre.saveFmt:
		out=S.pre.saveFmt.format(stage='done',S=S,**(dict(S.tags)))
		print 'Saving to',out
		S.save(out)
	S.stop()
	import woo.utils
	(repName,figs)=woo.utils.htmlReport(S,S.pre.reportFmt,'Triaxial test with rigid boundary',afterHead='',figures=[(None,f) for f in S.plot.plot(noShow=True,subPlots=False)],svgEmbed=True,show=True)
	woo.batch.writeResults(S,defaultDb='triax.hdf5',series=S.plot.data,postHooks=[woo.pre.cylTriax.plotBatchResults],simulationName='triax',report='file://'+repName)


