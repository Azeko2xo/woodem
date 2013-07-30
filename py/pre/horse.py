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
		_PAT(float,'radius',.002,unit='mm',startGroup='General',doc='Radius of spheres (fill of the upper horse)'),
		_PAT(float,'relGap',.25,doc='Gap between particles in pattern, relative to :obj:`radius`'),
		_PAT(float,'halfThick',.002,unit='mm',doc='Half-thickness of the mesh.'),
		_PAT(float,'relEkStop',.02,'Stop when kinetic energy drops below this fraction of gravity work (and step number is greater than 100)'),
		_PAT(float,'damping',.2,'The value of :obj:`woo.dem.Leapfrog.damping`, for materials without internal damping'),
		_PAT(Vector3,'gravity',(0,0,-9.81),'Gravity acceleration vector'),
		_PAT(str,'pattern','hexa',choice=['hexa','ortho'],doc='Pattern to use when filling the volume with spheres'),
		_PAT(woo.dem.FrictMat,'mat',woo.dem.FrictMat(density=1e3,young=5e4,ktDivKn=.2,tanPhi=math.tan(.5)),startGroup='Material',doc='Material for particles'),
		_PAT(woo.dem.FrictMat,'meshMat',None,'Material for the meshed horse; if not given, :obj:`mat` is used here as well.'),
		_PAT(bool,'deformable',False,startGroup='Deformability',doc='Whether the meshed horse is deformable. Note that deformable horse does not track energy and disables plotting.'),
		_PAT(float,'pWaveSafety',.7,startGroup='Tunables',doc='Safety factor for :obj:`woo.utils.pWaveDt` estimation.'),
		_PAT(str,'reportFmt',"/tmp/{tid}.xhtml",filename=True,startGroup="Outputs",doc="Report output format; :obj:`Scene.tags <woo.core.Scene.tags>` can be used."),
		_PAT(int,'vtkStep',40,"How often should VtkExport run. If non-positive, never run the export."),
		_PAT(str,'vtkPrefix',"/tmp/{tid}-",filename=True,doc="Prefix for saving :obj:`woo.dem.VtkExport` data; formatted with ``format()`` providing :obj:`woo.core.Scene.tags` as keys."),
		_PAT(bool,'grid',False,'Use grid collider (experimental)'),
	]
	def __init__(self,**kw):
		woo.core.Preprocessor.__init__(self)
		self.wooPyInit(self.__class__,woo.core.Preprocessor,**kw)
	def __call__(self):
		# preprocessor builds the simulation when called
		return prepareHorse(self)

def prepareHorse(pre):
	import sys, os, woo
	import woo.gts as gts # not sure whether this is necessary
	import woo.pack, woo.utils, woo.core, woo
	import pkg_resources
	S=woo.core.Scene(fields=[DemField()])
	for a in ['reportFmt','vtkPrefix']: setattr(pre,a,woo.utils.fixWindowsPath(getattr(pre,a)))
	S.pre=pre.deepcopy() # so that our manipulation does not affect input fields
	S.dem.gravity=pre.gravity
	if not pre.meshMat: pre.meshMat=pre.mat.deepcopy()
	

	# load the horse
	# FIXME: pyinstaller should support pkg_resources on package which is frozen as well :|
	if hasattr(sys,'frozen'): surf=gts.read(open(sys._MEIPASS+'/data/horse.coarse.gts','r'))
	else: surf=gts.read(pkg_resources.resource_stream('woo','data/horse.coarse.gts'))
	
	if not surf.is_closed(): raise RuntimeError('Horse surface not closed?!')
	pred=woo.pack.inGtsSurface(surf)
	aabb=pred.aabb()
	# filled horse above
	if pre.pattern=='hexa': packer=woo.pack.regularHexa
	elif pre.pattern=='ortho': packer=woo.pack.regularOrtho
	else: raise ValueError('FallingHorse.pattern must be one of hexa, ortho (not %s)'%pre.pattern)
	S.dem.par.append(packer(pred,radius=pre.radius,gap=pre.relGap*pre.radius,mat=pre.mat))
	# meshed horse below
	xSpan,ySpan,zSpan=aabb.sizes() # aabb[1][0]-aabb[0][0],aabb[1][1]-aabb[0][1],aabb[1][2]-aabb[0][2]
	surf.translate(0,0,-zSpan)
	zMin=aabb[0][2]-(aabb[1][2]-aabb[0][2])
	xMin,yMin,xMax,yMax=aabb[0][0]-zSpan,aabb[0][1]-zSpan,aabb[1][0]+zSpan,aabb[1][1]+zSpan
	S.dem.par.append(woo.pack.gtsSurface2Facets(surf,wire=False,flex=pre.deformable,mat=pre.meshMat,halfThick=pre.halfThick,fixed=(not pre.deformable)))
	S.dem.par.append(woo.utils.wall(zMin,axis=2,sense=1,mat=pre.meshMat,glAB=((xMin,yMin),(xMax,yMax))))
	S.dem.saveDeadNodes=True # for traces, if used
	S.dem.collectNodes() # collects also mesh nodes, if deformable; good :-)
	
	nan=float('nan')


	if not pre.grid: collider=InsertionSortCollider([Bo1_Sphere_Aabb(),Bo1_Facet_Aabb(),Bo1_Wall_Aabb()],verletDist=0.01)
	else: collider=GridCollider([Grid1_Sphere(),Grid1_Facet(),Grid1_Wall()],label='collider')

	if not pre.deformable:
		S.engines=woo.utils.defaultEngines(damping=pre.damping,
			cp2=(Cp1_PelletMat_PelletPhys if isinstance(pre.mat,woo.dem.PelletMat) else None),
			law=(Law2_L6Geom_PelletPhys_Pellet(plastSplit=True) if isinstance(pre.mat,woo.dem.PelletMat) else None),
			grid=pre.grid
		)+[
			woo.core.PyRunner(10,'S.plot.addData(i=S.step,t=S.time,total=S.energy.total(),relErr=(S.energy.relErr() if S.step>100 else 0),**S.energy)'),
			woo.core.PyRunner(50,'import woo.pre.horse\nif S.step>100 and S.energy["kinetic"]<S.pre.relEkStop*abs(S.energy["grav"]): woo.pre.horse.finished(S)'),
		]
		S.trackEnergy=True
		S.plot.plots={'i':('total','S.energy.keys()'),' t':('relErr')}
		S.plot.data={'i':[nan],'total':[nan],'relErr':[nan]} # to make plot displayable from the very start
	else:
		# more complicated here
		# go through facet's nodes, give them some mass
		nodeM=1e-3*pre.mat.density*(4/3)*math.pi*pre.radius**3
		nodeI=1e3*(2/5.)*nodeM*pre.radius**2
		for p in S.dem.par:
			if type(p.shape)!=FlexFacet: continue
			for n in p.shape.nodes:
				n.dem.mass=nodeM
				n.dem.inertia=nodeI*Vector3(1,1,1)
				n.dem.gravitySkip=True
				#if n.pos[2]<zMin: n.dem.blocked='xyzXYZ'

		S.engines=[
			Leapfrog(reset=True,damping=pre.damping),
			collider,
			ContactLoop([Cg2_Sphere_Sphere_L6Geom(),Cg2_Facet_Sphere_L6Geom(),Cg2_Wall_Sphere_L6Geom()],[Cp2_FrictMat_FrictPhys()],[Law2_L6Geom_FrictPhys_IdealElPl()],applyForces=False), # forces are applied in IntraForce
			IntraForce([In2_FlexFacet_ElastMat(bending=True,thickness=(pre.radius if pre.halfThick<=0 else float('nan'))),In2_Sphere_ElastMat()]),
		]

	if pre.grid:
		c=S.lab.collider
		zMax=max([p.shape.nodes[0].pos[2] for p in S.dem.par])
		c.domain=((xMin,yMin,zMin-.01*zSpan),(xMax,yMax,zMax))
		c.minCellSize=2*pre.radius
		c.verletDist=.5*pre.radius

	S.engines=S.engines+[
		BoxDeleter(box=((xMin,yMin,zMin-.1*zSpan),(xMax,yMax,aabb[1][2]+.1*zSpan)),inside=False,stepPeriod=100),
		VtkExport(out=pre.vtkPrefix,stepPeriod=pre.vtkStep,what=VtkExport.all,dead=(pre.vtkStep<=0 or not pre.vtkPrefix))
		]+([Tracer(stepPeriod=20,num=16,compress=0,compSkip=2,dead=False,scalar=Tracer.scalarVel,label='_tracer')] if 'opengl' in woo.config.features else [])

	#woo.master.timingEnabled=True
	S.dt=pre.pWaveSafety*woo.utils.pWaveDt(S)
	import woo.config
	if 'opengl' in woo.config.features:
		S.any=[
			woo.gl.Gl1_DemField(shape=woo.gl.Gl1_DemField.shapeSpheres,colorBy=woo.gl.Gl1_DemField.colorVel,deadNodes=False)
		]
	return S

def plotBatchResults(db):
	'Hook called from woo.batch.writeResults'
	import pylab,re,math,woo.batch,os
	results=woo.batch.dbReadResults(db)
	out='%s.pdf'%re.sub('\.results$','',db)
	fig=pylab.figure()
	ax1=fig.add_subplot(211)
	ax1.set_xlabel('Time [s]')
	ax1.set_ylabel('Kinetic energy [J]')
	ax1.grid(True)
	ax2=fig.add_subplot(212)
	ax2.set_xlabel('Time [s]')
	ax2.set_ylabel('Relative energy error')
	ax2.grid(True)
	for res in results:
		series=res['series']
		pre=res['pre']
		if not res['title']: res['title']=res['sceneId']
		ax1.plot(series['t'],series['kinetic'],label=res['title'],alpha=.6)
		ax2.plot(series['t'],series['relErr'],label=res['title'],alpha=.6)
	for ax,loc in (ax1,'lower left'),(ax2,'lower right'):
		l=ax.legend(loc=loc,labelspacing=.2,prop={'size':7})
		l.get_frame().set_alpha(.4)
	fig.savefig(out)
	print 'Batch figure saved to file://%s'%os.path.abspath(out)

	

def finished(S):
	import os,re,woo.batch,woo.utils,codecs
	S.stop()
	repName=os.path.abspath(S.pre.reportFmt.format(S=S,**(dict(S.tags))))
	woo.batch.writeResults(S,defaultDb='horse.results',series=S.plot.data,postHooks=[plotBatchResults],simulationName='horse',report='file://'+repName)


	# energy plot, to show how to add plot to the report
	# instead of doing pylab stuff here, just get plot object from woo
	figs=S.plot.plot(noShow=True,subPlots=False)
	
	repExtra=re.sub('\.xhtml$','',repName)
	svgs=[]
	for i,fig in enumerate(figs):
		svgs.append(('Figure %d'%i,repExtra+'.fig-%d.svg'%i))
		fig.savefig(svgs[-1][1])

	rep=codecs.open(repName,'w','utf-8','replace')
	html=woo.utils.htmlReportHead(S,'Report for falling horse simulation')+'<h2>Figures</h2>'+(
		u'\n'.join(['<h3>'+svg[0]+u'</h3>'+'<img src="%s" alt="%s"/>'%(os.path.basename(svg[1]),svg[0]) for svg in svgs])
		+'</body></html>'
	)
	rep.write(html)
	rep.close()
	if not woo.batch.inBatch():
		import webbrowser
		webbrowser.open('file://'+os.path.abspath(repName),new=2)
