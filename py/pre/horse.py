# encoding: utf-8

from woo.dem import *
import woo.core
import woo.dem
import woo.pyderived
import math
from minieigen import *

class FallingHorse(woo.core.Preprocessor,woo.pyderived.PyWooObject):
	'Sample for the falling horse simulation'
	_classTraits=None
	_PAT=woo.pyderived.PyAttrTrait # less typing
	_attrTraits=[
		_PAT(float,'radius',.002,unit='mm',doc='Radius of spheres (fill of the upper horse)'),
		_PAT(float,'relGap',.25,doc='Gap between particles in pattern, relative to :obj:`radius`'),
		_PAT(float,'halfThick',0.,unit='mm',doc='Half-thickness of the mesh.'),
		_PAT(woo.dem.FrictMat,'mat',woo.dem.FrictMat(density=1e3,young=5e4,ktDivKn=.2,tanPhi=math.tan(.5)),'Material for falling particles'),
		_PAT(woo.dem.FrictMat,'meshMat',None,'Material for the meshed horse; if not given, :obj:`mat` is used here as well.'),
		_PAT(float,'damping',.2,'The value of :obj:`woo.dem.Leapfrog.damping`, for materials without internal damping'),
		_PAT(Vector3,'gravity',(0,0,-9.81),'Gravity acceleration vector'),
		_PAT(str,'pattern','hexa',choice=['hexa','ortho'],doc='Pattern to use when filling the volume with spheres'),
		_PAT(float,'pWaveSafety',.7,startGroup='Tunables',doc='Safety factor for :obj:`woo.utils.pWaveDt` estimation.'),
		_PAT(str,'reportFmt',"/tmp/{tid}.xhtml",startGroup="Outputs",doc="Report output format; :obj:`Scene.tags <woo.core.Scene.tags>` can be used."),
	]
	def __init__(self,**kw):
		woo.core.Preprocessor.__init__(self)
		self.wooPyInit(self.__class__,woo.core.Preprocessor,**kw)
	def __call__(self):
		# preprocessor builds the simulation when called
		return prepareHorse(self)

def prepareHorse(pre):
	import woo.gts as gts # not sure whether this is necessary
	import woo.pack, woo.utils, woo.core, woo
	import pkg_resources
	S=woo.core.Scene(fields=[DemField()])
	S.pre=pre.deepcopy() # so that our manipulation does not affect input fields
	S.dem.gravity=pre.gravity
	if not pre.meshMat: pre.meshMat=pre.mat.deepcopy()

	# load the horse
	surf=gts.read(open(pkg_resources.resource_filename('woo','resources/horse.coarse.gts')))
	if not surf.is_closed(): raise RuntimeError('Horse surface not closed?!')
	pred=woo.pack.inGtsSurface(surf)
	aabb=pred.aabb()
	# filled horse above
	if pre.pattern=='hexa': packer=woo.pack.regularHexa
	elif pre.pattern=='ortho': packer=woo.pack.regularOrtho
	else: raise ValueError('FallingHorse.pattern must be one of hexa, ortho (not %s)'%pre.pattern)
	S.dem.par.append(packer(pred,radius=pre.radius,gap=pre.relGap*pre.radius,mat=pre.mat))
	# meshed horse below
	xSpan,ySpan,zSpan=aabb[1][0]-aabb[0][0],aabb[1][1]-aabb[0][1],aabb[1][2]-aabb[0][2]
	surf.translate(0,0,-zSpan)
	zMin=aabb[0][2]-(aabb[1][2]-aabb[0][2])
	xMin,yMin,xMax,yMax=aabb[0][0]-zSpan,aabb[0][1]-zSpan,aabb[1][0]+zSpan,aabb[1][1]+zSpan
	S.dem.par.append(woo.pack.gtsSurface2Facets(surf,wire=True,mat=pre.meshMat,halfThick=pre.halfThick))
	S.dem.par.append(woo.utils.wall(zMin,axis=2,sense=1,mat=pre.meshMat,glAB=((xMin,yMin),(xMax,yMax))))
	S.dem.saveDeadNodes=True # for traces, if used
	S.dem.collectNodes()
	
	nan=float('nan')

	S.engines=woo.utils.defaultEngines(damping=pre.damping)+[
		#woo.core.PyRunner(2000,'import woo.timing; woo.timing.stats();'),
		woo.core.PyRunner(10,'S.plot.addData(i=S.step,total=S.energy.total(),**S.energy)'),
		BoxDeleter(box=((xMin,yMin,zMin-.1*zSpan),(xMax,yMax,aabb[1][2]+.1*zSpan)),inside=False,stepPeriod=100)
	]+([Tracer(stepPeriod=20,num=16,compress=0,compSkip=2,dead=False,scalar=Tracer.scalarVel,label='_tracer')] if 'opengl' in woo.config.features else [])

	S.trackEnergy=True
	S.plot.plots={'i':('total','S.energy.keys()')}
	S.plot.data={'i':[nan],'total':[nan]} # to make plot displayable from the very start
	#woo.master.timingEnabled=True
	S.dt=pre.pWaveSafety*woo.utils.pWaveDt(S)
	S.pre=pre.deepcopy()
	return S

