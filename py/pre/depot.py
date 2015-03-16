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
	defaultPsd=[(.007,0),(.01,.4),(.012,.7),(.02,1)]
	def postLoad(self,I):
		self.ht0=self.htDiam[0]/self.relSettle
		# if I==id(self.estSettle): 
	_attrTraits=[
		_PAT(Vector2,'htDiam',(.45,.1),unit='m',doc='Height and diameter of the resulting cylinder; the initial cylinder has the height of :obj:`ht0`, and particles are, after stabilization, clipped to :obj:`htDiam`, the resulting height.'),
		_PAT(float,'relSettle',.3,triggerPostLoad=True,doc='Estimated relative height after deposition (e.g. 0.4 means that the sample will settle around 0.4 times the original height). This value has to be guessed, as there is no exact relation to predict the amount of settling; 0.3 is a good initial guess, but it may depend on the PSD.'),
		_PAT(float,'ht0',.9,guiReadonly=True,doc='Initial height (for loose sample), computed automatically from :obj:`relSettle` and :obj:`htDiam`.'),
		_PAT(woo.dem.ParticleGenerator,'gen',woo.dem.PsdSphereGenerator(psdPts=defaultPsd,discrete=True),'Object for particle generation'),
		_PAT(woo.dem.SpatialBias,'bias',woo.dem.PsdAxialBias(psdPts=defaultPsd,axis=2,fuzz=.1,discrete=True),doc='Uneven distribution of particles in space, depending on their radius. Use axis=2 for altering the distribution along the cylinder axis.'),
		_PAT(woo.models.ContactModelSelector,'model',woo.models.ContactModelSelector(name='linear',damping=.4,numMat=(1,1),matDesc=['everything'],mats=[woo.dem.FrictMat(density=2e3,young=2e5,tanPhi=0)]),doc='Contact model and materials.'),
		_PAT(int,'cylDiv',40,'Fineness of cylinder division'),
		_PAT(str,'stlOut','',filename=True,doc='Output file with triangulated particles (not the boundary); if empty, nothing will be exported at the end.'),
		_PAT(float,'stlTol',.2e-3,unit='m',doc='Tolerance for STL export (maximum distance between ideal shape and triangulation; passed to :obj:`_triangulated.spheroidsToStl`)'),
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
		print("unbalanced E: %g/0.001"%u)
		if not u<0.05: return

		r,h=S.pre.htDiam[1]/2.,S.pre.htDiam[0]
		# check how much was the settlement
		zz=woo.utils.contactCoordQuantiles(S.dem,[.999])
		print 'Compaction done, settlement from %g (loose) to %g (dense); rel. %g, relSettle was %g.'%(S.pre.ht0,zz[0],zz[0]/S.pre.ht0,S.pre.relSettle)
		# run this engine just once, explicitly
		woo.dem.BoxOutlet(box=((-r,-r,0),(r,r,h)))(S,S.dem)
		S.stop()
		if S.pre.stlOut:
			n=woo.triangulated.spheroidsToSTL(S.pre.stlOut,S.dem,tol=S.pre.stlTol)
			print 'Exported %d facets to %s'%(n,S.pre.stlOut)
		else:
			print 'Not running STL export (stlOut empty)'

