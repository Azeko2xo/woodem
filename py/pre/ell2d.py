# encoding: utf-8

from woo.dem import *
import woo.core
import woo.dem
import woo.pyderived
import woo.models
import math
from minieigen import *
nan=float('nan')

try:
	from PyQt4.QtGui import *
	from PyQt4.QtCore import *
except ImportError: pass



class EllGroup(woo.core.Preprocessor,woo.pyderived.PyWooObject):
	'Simulation of group of ellipsoids moving in 2-dimensional box.'
	_classTraits=None
	_PAT=woo.pyderived.PyAttrTrait # less typing
	_attrTraits=[
		_PAT(Vector2,'rRange',Vector2(.02,.04),unit='m',doc='Range (minimum and maximum) for particle radius (greatest semi-axis); if both are the same, all particles will have the same radius.'),
		_PAT(bool,'spheres',False,doc='Use spherical particles instead of elliptical'),
		_PAT(float,'semiMinRelRnd',0,hideIf='self.spheres',doc='Minimum semi-axis length relative to particle radius; minor semi-axes are randomly selected from (:obj:`semiMinRelRnd` … 1) × greatest semi-axis. If non-positive, :obj:`semiRelFixed` is used instead.'),
		_PAT(Vector3,'semiRelFixed',Vector3(1.,.5,.5),hideIf='self.spheres',doc='Fixed sizes of semi-axes relative (all elements should be ≤ 1). The :math:`z`-component is the out-of-plane size which only indirectly influences contact stiffnesses. This variable is only used if semi-axes are not assigned randomly (see :obj:`semiMinRelRnd`).'),
		_PAT(Vector2,'boxSize',Vector2(2,2),unit='m',doc='Size of the 2d domain in which particles move.'),
		_PAT(float,'vMax',1.,unit='m/s',doc='Maximum initial velocity of particle; assigned randomly from 0 to this value; intial angular velocity of all particles is zero.'),
		_PAT(woo.models.ContactModelSelector,'model',woo.models.ContactModelSelector(name='linear',surfEnergy=4.,restitution=1.,damping=0.01,alpha=.6,numMat=(1,2),matDesc=['ellipsoids','walls'],mats=[woo.dem.FrictMat(density=5000,young=1e6,tanPhi=0.0)]),doc='Select contact model. The first material is for particles; the second, optional, material is for walls at the boundary (the first material is used if there is no second one).'),

		_PAT(str,'exportFmt',"/tmp/ell2d-{tid}-",filename=True,doc="Prefix for saving :obj:`woo.dem.VtkExport` data, and :obj:`woo.pre.ell2d.ell2plot` data; formatted with ``format()`` providing :obj:`woo.core.Scene.tags` as keys."),
		_PAT(int,'vtkStep',0,"How often should :obj:`woo.dem.VtkExport` run. If non-positive, never run the export."),
		_PAT(int,'vtkEllLev',1,'Tesselation level of ellipsoids when expored as VTK meshes (see :obj:`woo.dem.VtkExport.ellLev`).'),
		_PAT(int,'ell2Step',0,"How often should :obj:`woo.pre.ell2d.ell2plot` run. If non-positive, never run that one."),
		_PAT(float,'dtSafety',.5,'Safety coefficient for critical timestep; should be smaller than one.'),
	]
	def __init__(self,**kw):
		woo.core.Preprocessor.__init__(self)
		self.wooPyInit(self.__class__,woo.core.Preprocessor,**kw)
	def __call__(self):
		import woo
		# preprocessor builds the simulation when called
		pre=self # more readable
		S=woo.core.Scene(fields=[woo.dem.DemField()],pre=self.deepcopy())
		# material definitions
		ellMat=pre.model.mats[0]
		wallMat=(pre.model.mats[1] if len(pre.model.mats)>1 else ellMat)

		ZZ=pre.rRange[1]*3;
		# only generate spheres randomly in 2d box
		S.engines=[woo.dem.BoxFactory2d(axis=2,box=((0,0,ZZ),(pre.boxSize[0],pre.boxSize[1],ZZ)),materials=[ellMat],generator=woo.dem.MinMaxSphereGenerator(dRange=2*pre.rRange),massFlowRate=0),woo.dem.InsertionSortCollider([woo.dem.Bo1_Sphere_Aabb()])]
		S.one()
		posRad=[(p.pos,p.shape.radius) for p in S.dem.par]
		# clear the dem field
		S.fields=[woo.dem.DemField()]


		import random

		def rndOri2d():
			q=Quaternion((0,0,1),2*math.pi*random.random()); q.normalize(); return q

		S.energy['kin0']=0
		for pos,rad in posRad:
			if not pre.spheres:
				if pre.semiMinRelRnd>0: semiAxes=[random.uniform(pre.semiMinRelRnd,1)*rad for i in (0,1,2)]
				else: semiAxes=Vector3(pre.semiRelFixed)*rad
				p=woo.utils.ellipsoid(center=pos,semiAxes=semiAxes,ori=rndOri2d(),mat=ellMat)
			else: p=woo.utils.sphere(center=pos,radius=rad,mat=ellMat)
			p.vel=rndOri2d()*Vector3(pre.vMax*random.random(),0,0)
			S.energy['kin0']-=p.Ek
			S.dem.par.append(p)
			p.blocked='zXY'
		S.dem.collectNodes()
		#for coord,axis,sense in [(0,0,+1),(pre.boxSize[0],0,-1),(0,1,+1),(pre.boxSize[1],1,-1)]:
		#	S.dem.par.append(woo.utils.wall(coord,axis=axis,sense=sense,mat=wallMat,visible=False))
		S.dem.par.append([
			woo.utils.wall(0,axis=0,mat=wallMat,visible=True),
			woo.utils.wall(0,axis=1,mat=wallMat,visible=True)
		])
		S.periodic=True
		S.cell.setBox((pre.boxSize[0],pre.boxSize[1],2*ZZ))

		S.engines=woo.utils.defaultEngines(model=pre.model,dynDtPeriod=10)+[
			# trace particles and color by z-angVel
			woo.dem.Tracer(num=100,compress=4,compSkip=1,glSmooth=True,glWidth=2,scalar=woo.dem.Tracer.scalarAngVel,vecAxis=2,stepPeriod=40,minDist=pre.rRange[0]),
			woo.core.PyRunner(100,'S.plot.addData(i=S.step,t=S.time,total=S.energy.total(),relErr=(S.energy.relErr() if S.step>100 else 0),**S.energy)'),
			woo.dem.VtkExport(stepPeriod=pre.vtkStep,out=pre.exportFmt,ellLev=pre.vtkEllLev,dead=(pre.vtkStep<=0)),
			woo.core.PyRunner(pre.ell2Step,'import woo.pre.ell2d; mx=woo.pre.ell2d.ell2plot(out="%s-%05d.png"%(S.expandTags(S.pre.exportFmt),engine.nDone),S=S,colorRange=(0,S.lab.maxEll2Color),bbox=((0,0),S.pre.boxSize)); S.lab.maxEll2Color=max(mx,S.lab.maxEll2Color)',dead=(pre.ell2Step<=0)),
			woo.dem.WeirdTriaxControl(goal=(-0,-0,0),stressMask=0,maxStrainRate=(.1,.1,0),mass=1.,label='triax'),
		]

		S.lab.leapfrog.kinSplit=True

		S.dtSafety=pre.dtSafety

		S.uiBuild='import woo.pre.ell2d; woo.pre.ell2d.ellGroupUiBuild(S,area)'

		S.lab.maxEll2Color=0. # max |angVel| for the start when plotting

		S.trackEnergy=True
		S.plot.plots={'i':('total','S.energy.keys()'),' t':('relErr')}
		S.plot.data={'i':[nan],'total':[nan],'relErr':[nan]} # to make plot displayable from the very start
	
		try:
			import woo.gl
			S.any=[
				woo.gl.Renderer(iniUp=(0,1,0),iniViewDir=(0,0,-1),grid=4)
			]
		except ImportError: pass

		return S


def ell2plot(out,S,bbox,colorRange,colorBy='angVel',**kw):
	from matplotlib.figure import Figure
	from matplotlib.backends.backend_agg import FigureCanvasAgg
	import matplotlib.collections, matplotlib.patches
	import numpy
	import math
	import woo.dem
	from minieigen import Vector2, Vector3
	fig=Figure()
	ax=fig.add_subplot(1,1,1)
	canvas=FigureCanvasAgg(fig)
	patches,colors=[],[]
	def flat(v): return Vector2(v[0],v[1])
	for p in S.dem.par:
		if not isinstance(p.shape,woo.dem.Ellipsoid): continue
		# project rotation onto the z-axis
		rotAxis,rotAngle=p.ori.toAxisAngle()
		if abs(rotAxis.dot(Vector3.UnitZ))<0.99: raise ValueError("Ellipsoid rotated other than along the z-axis?")
		if rotAxis.dot(Vector3.UnitZ)<0: rotAngle*=-1 # rotation along -z = - rotation along +z 
		patches.append(matplotlib.patches.Ellipse(xy=flat(p.pos),width=2*p.shape.semiAxes[0],height=2*p.shape.semiAxes[1],angle=math.degrees(rotAngle)))
		if colorBy=='angVel': colors.append(abs(p.angVel[2]))
		elif colorBy=='vel': colors.append(p.vel[2])
		else: raise ValueError('colorBy must be one of "angVel", "vel" (not %s)'%colorBy)
	coll=matplotlib.collections.PatchCollection(patches,cmap=matplotlib.cm.jet,alpha=.9)
	coll.set_array(numpy.array(colors))
	ax.add_collection(coll)
	# cbar=fig.colorbar(coll)
	coll.set_clim(*colorRange)
	ax.grid(True)
	ax.set_xlim(bbox[0][0],bbox[1][0])
	ax.set_ylim(bbox[0][1],bbox[1][1])
	ax.set_aspect('equal')
	fig.savefig(out)
	return max(colors)

def ellGroupUiBuild(S,area):
	grid=QGridLayout(area); grid.setSpacing(0); grid.setMargin(0)
	bHalf=QPushButton('Halve Ek')
	bDouble=QPushButton('Double Ek')
	boxEps=QDoubleSpinBox()
	boxEps.setValue(0.)
	boxEps.setSingleStep(0.1)
	boxEps.setRange(-2.,1.)

	grid.addWidget(bHalf,0,0)
	grid.addWidget(bDouble,0,1)
	grid.addWidget(QLabel('goal size'),1,0)
	grid.addWidget(boxEps,1,1)
	grid.boxEps=boxEps

	def ekAdjust(S,coeff,grid):
		with S.paused():
			for n in S.dem.nodes:
				n.dem.vel*=coeff**2
				n.dem.angVel*=coeff**2
	def epsChange(S,eps,grid):
		S.lab.triax.goal=(eps,eps,0)
	bHalf.clicked.connect(lambda: ekAdjust(S,.5,grid))
	bDouble.clicked.connect(lambda: ekAdjust(S,2.,grid))
	boxEps.valueChanged.connect(lambda eps: epsChange(S,eps,grid))
	
	def uiRefresh(grid,S,area):
		# update updatable stuff
		if S.lab.triax.goal[0]!=grid.boxEps.value(): grid.boxEps.setValue(S.lab.triax.goal[0])

	grid.refreshTimer=QTimer(grid)
	grid.refreshTimer.timeout.connect(lambda: uiRefresh(grid,S,area))
	grid.refreshTimer.start(500)
	


