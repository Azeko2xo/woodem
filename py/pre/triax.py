# encoding: utf-8

from woo.dem import *
import woo.core
import woo.dem
import woo.pyderived
import math
from minieigen import *

class TriaxTest(woo.core.Preprocessor,woo.pyderived.PyWooObject):
	'Preprocessor for triaxial test'
	_classTraits=None
	_PAT=woo.pyderived.PyAttrTrait # less typing
	_attrTraits=[
		# noGui would make startGroup being ignored
		_PAT(float,'isoStress',-1e4,unit='kPa',startGroup='General',doc='Confining stress (isotropic during compaction)'),
		_PAT(float,'maxStrainRate',1e-3,'Maximum strain rate during the compaction phase, and during the triaxial phase in axial sense'),
		#_PAT(bool,'periodic',True,hideIf='True',doc='Use periodic boundary conditions'),

		_PAT(int,'nPar',2000,startGroup='Particles',doc='Number of particles'),
		_PAT(woo.dem.Material,'mat',FrictMat(young=1e5,ktDivKn=.2,tanPhi=0.,density=1e8),'Material of particles; depending on type of **mat**, contact model will be selected automatically.'),
		_PAT(float,'tanPhi2',.6,'If :obj:`mat` defines :obj`tanPhi <woo.dem.FrictMat.tanPhi>`, it will be changed to this value progressively after the compaction phase.'),
		_PAT([Vector2,],'psd',[(1e-3,0),(2e-3,.2),(4e-3,1.)],unit=['mm','%'],doc='Particle size distribution of particles; first value is diameter, scond is cummulative mass fraction.'),

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
	]
	def __init__(self,**kw):
		woo.core.Preprocessor.__init__(self)
		self.wooPyInit(self.__class__,woo.core.Preprocessor,**kw)
	def __call__(self):
		# preprocessor builds the simulation when called
		return prepareTriax(self)

def prepareTriax(pre):
	S=woo.core.Scene(fields=[DemField()])
	# hack: maximum volume occupied by particles - as if all had max radius
	Vmax=pre.nPar*(4/3)*math.pi*(pre.psd[-1][0])**3
	sp=woo.pack.makePeriodicFeedPack(Vmax**(1/3.),porosity=pre.initPoro,maxNum=pre.nPar,memoizeDir=pre.packCacheDir,returnSpherePack=True)
	sp.toSimulation(S,mat=S.pre.mat)
	raise NotImplementedError('TriaxTest is not yet fully implemented.')

