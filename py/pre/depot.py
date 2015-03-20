# encoding: utf-8

from woo.dem import *
from woo.fem import *
import woo.core, woo.dem, woo.pyderived, woo.models, woo.config
import math
from minieigen import *

class CylDepot(woo.core.Preprocessor,woo.pyderived.PyWooObject):
	'Deposition of particles inside cylindrical tube'
	_classTraits=None
	_PAT=woo.pyderived.PyAttrTrait # less typing
	#defaultPsd=[(.007,0),(.01,.4),(.012,.7),(.02,1)]
	defaultPsd=[(5e-3,.0),(6.3e-3,.12),(8e-3,.53),(10e-3,.8),(12.5e-3,.94),(20e-3,1)]
	def postLoad(self,I):
		if self.preCooked and (I==None or I==id(self.preCooked)):
			print 'Applying pre-cooked configuration "%s".'%self.preCooked
			if self.preCooked=='Berlin 1':
				self.gen=woo.dem.PsdSphereGenerator(psdPts=[(6.3e-3,0),(12.5e-3,.82222),(20e-3,1)],discrete=False)
				self.bias=woo.dem.LayeredAxialBias(axis=2,fuzz=0,layerSpec=[VectorX([12.5e-3,1,0,.177777]),VectorX([0,12.5e-3,.177777,1])])
				self.relSettle=.38
			elif self.preCooked=='Berlin 2':
				self.gen=woo.dem.PsdSphereGenerator(psdPts=[(6.3e-3,0),(12.5e-3,.41111),(20e-3,1)],discrete=False)
				self.bias=woo.dem.LayeredAxialBias(axis=2,fuzz=0,layerSpec=[VectorX([12.5e-3,1,0,.177777,.58888,1]),VectorX([0,12.5e-3,.177777,.58888])])
				self.relSettle=.37
			else: raise RuntimeError('Unknown precooked configuration "%s"'%self.preCooked)
			self.preCooked=''
		self.ht0=self.htDiam[0]/self.relSettle
		# if I==id(self.estSettle): 
	_attrTraits=[
		_PAT(str,'preCooked','',noDump=True,noGui=False,startGroup='Predefined config',choice=['','Berlin 1','Berlin 2'],triggerPostLoad=True,doc='Apply pre-cooked configuration (i.e. change other parameters); this option is not saved.'),
		_PAT(Vector2,'htDiam',(.45,.1),unit='m',doc='Height and diameter of the resulting cylinder; the initial cylinder has the height of :obj:`ht0`, and particles are, after stabilization, clipped to :obj:`htDiam`, the resulting height.'),
		_PAT(float,'relSettle',.3,triggerPostLoad=True,doc='Estimated relative height after deposition (e.g. 0.4 means that the sample will settle around 0.4 times the original height). This value has to be guessed, as there is no exact relation to predict the amount of settling; 0.3 is a good initial guess, but it may depend on the PSD.'),
		_PAT(float,'ht0',.9,guiReadonly=True,doc='Initial height (for loose sample), computed automatically from :obj:`relSettle` and :obj:`htDiam`.'),
		_PAT(woo.dem.ParticleGenerator,'gen',woo.dem.PsdSphereGenerator(psdPts=defaultPsd,discrete=False),'Object for particle generation'),
		_PAT(woo.dem.SpatialBias,'bias',woo.dem.PsdAxialBias(psdPts=defaultPsd,axis=2,fuzz=.1,discrete=True),doc='Uneven distribution of particles in space, depending on their radius. Use axis=2 for altering the distribution along the cylinder axis.'),
		_PAT(woo.models.ContactModelSelector,'model',woo.models.ContactModelSelector(name='linear',damping=.4,numMat=(1,1),matDesc=['everything'],mats=[woo.dem.FrictMat(density=2e3,young=2e5,tanPhi=0)]),doc='Contact model and materials.'),
		_PAT(int,'cylDiv',40,'Fineness of cylinder division'),
		_PAT(float,'unbE',0.005,':obj:`Unbalanced energy <woo._utils2.unbalancedEnergy>` as criterion to consider the particles settled.'),
		# STL output
		_PAT(str,'stlOut','',startGroup='STL output',filename=True,doc='Output file with triangulated particles (not the boundary); if empty, nothing will be exported at the end.'),
		_PAT(float,'stlTol',.2e-3,unit='m',doc='Tolerance for STL export (maximum distance between ideal shape and triangulation; passed to :obj:`_triangulated.spheroidsToStl)'),
		_PAT(Vector2,'extraHt',(.5,.5),unit='m',doc='Extra height to be added to bottom and top of the resulting packing, when the new STL-exported cylinder is created.'),
		_PAT(float,'cylAxDiv',-1.,'Fineness of division of the STL cylinder; see :obj:`woo.triangulated.cylinder` ``axDiv``. The default create nearly-square triangulation'),
	]
	def __init__(self,**kw):
		woo.core.Preprocessor.__init__(self)
		self.wooPyInit(self.__class__,woo.core.Preprocessor,**kw)
	def __call__(self):
		pre=self
		self.postLoad(None) # ensure consistency
		mat=pre.model.mats[0]
		S=woo.core.Scene(
			pre=self.deepcopy(),
			trackEnergy=True, # for unbalanced energy tracking
			dtSafety=.9,
			fields=[
				DemField(
					gravity=(0,0,-10),
					par=woo.triangulated.cylinder(Vector3(0,0,0),Vector3(0,0,pre.ht0),radius=pre.htDiam[1]/2.,div=pre.cylDiv,capA=True,capB=False,wallCaps=True,mat=mat)
				)
			],
			engines=DemField.minimalEngines(model=pre.model)+[
				woo.dem.CylinderInlet(
					node=woo.core.Node((0,0,0),Quaternion((0,1,0),-math.pi/2)),
					height=pre.ht0,
					radius=pre.htDiam[1]/2.,
					generator=pre.gen,
					spatialBias=pre.bias,
					maxMass=-1,maxNum=-1,massRate=0,maxAttempts=2000,materials=pre.model.mats,glColor=float('nan'),
					nDo=1 # place all particles at once, then let settle it all down
				),
				woo.core.PyRunner(100,'import woo.pre.depot; S.pre.checkProgress(S)'),
			],
		)
		if 'opengl' in woo.config.features: S.any=[woo.gl.Gl1_DemField(colorBy='radius')]
		return S

	def checkProgress(self,S):
		u=woo.utils.unbalancedEnergy(S)
		print("unbalanced E: %g/%g"%(u,S.pre.unbE))
		if not u<S.pre.unbE: return

		r,h=S.pre.htDiam[1]/2.,S.pre.htDiam[0]
		# check how much was the settlement
		zz=woo.utils.contactCoordQuantiles(S.dem,[.999])
		print 'Compaction done, settlement from %g (loose) to %g (dense); rel. %g, relSettle was %g.'%(S.pre.ht0,zz[0],zz[0]/S.pre.ht0,S.pre.relSettle)
		# delete everything abot; run this engine just once, explicitly
		woo.dem.BoxOutlet(box=((-r,-r,0),(r,r,h)))(S,S.dem)
		S.stop()

		# delete the triangulated cylinder
		for p in S.dem.par:
			if isinstance(p.shape,Facet): S.dem.par.remove(p.id)
		# create a new (CFD-suitable) cylinder
		# bits for marking the mesh parts
		S.lab.cylBits=8,16,32
		cylMasks=[DemField.defaultBoundaryMask | b for b in S.lab.cylBits]
		S.dem.par.add(woo.triangulated.cylinder(Vector3(0,0,-S.pre.extraHt[0]),Vector3(0,0,S.pre.htDiam[0]+S.pre.extraHt[1]),radius=S.pre.htDiam[1]/2.,div=S.pre.cylDiv,axDiv=S.pre.cylAxDiv,capA=True,capB=True,wallCaps=False,masks=cylMasks,mat=S.pre.model.mats[0]))

		if S.pre.stlOut:
			n=woo.triangulated.spheroidsToSTL(S.pre.stlOut,S.dem,tol=S.pre.stlTol,solid="particles")
			n+=woo.triangulated.facetsToSTL(S.pre.stlOut,S.dem,append=True,mask=S.lab.cylBits[0],solid="lateral")
			n+=woo.triangulated.facetsToSTL(S.pre.stlOut,S.dem,append=True,mask=S.lab.cylBits[1],solid="bottom")
			n+=woo.triangulated.facetsToSTL(S.pre.stlOut,S.dem,append=True,mask=S.lab.cylBits[2],solid="top")
			print 'Exported %d facets to %s'%(n,S.pre.stlOut)
		else:
			print 'Not running STL export (stlOut empty)'

