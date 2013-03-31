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
		_PAT(woo.dem.FrictMat,'mat',FrictMat(young=1e6,ktDivKn=.2,tanPhi=.5,density=1e8),'Material of particles.'),
		_PAT(float,'meshTanPhi',float('nan'),'Angle of friction of boundary walls; if nan, use the same as for particles, otherwise particle material is copied and tanPhi changed.'),
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
	if math.isnan(pre.meshTanPhi): meshMat=pre.mat
	else:
		meshMat=pre.mat.deepcopy()
		meshMat.tanPhi=pre.meshTanPhi
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
		),mask=meshMask,mat=meshMat,fixed=True)
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
		WeirdTriaxControl(goal=(pre.sigIso,pre.sigIso,pre.sigIso),maxStrainRate=(.005,.005,.005),relVol=math.pi*rad**2*ht/S.cell.volume,stressMask=0b0111,maxUnbalanced=0.05,mass=10.*sphereMass,doneHook='import woo.pre.cylTriax; woo.pre.cylTriax.compactionDone(S)',label='triax'),
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
	sxx,syy,szz=t.stress.diagonal()
	exx,eyy,ezz=t.strain
	p=t.stress.diagonal().sum()/3. # mean stress
	q=szz-.5*(sxx+syy)         # deviatoric stress?!
	qDivP=(q/p if p!=0 else float('nan'))
	eDev=ezz-t.strain.sum()/3. # ezz-mean
	eVol=t.strain.sum() # trace
	S.plot.addData(unbalanced=woo.utils.unbalancedForce(),i=S.step,
		sxx=sxx,syy=syy,srr=.5*(sxx+syy),szz=szz,
		exx=exx,eyy=eyy,err=.5*(exx+eyy),ezz=ezz,
		eDev=eDev,eVol=eVol,
		p=p,q=q,qDivP=qDivP,
		stressMask=t.stressMask, # to be able to select data recorded at the triaxial stage only
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
	t.goal=(S.pre.sigIso,S.pre.sigIso,-1.) # use ridiculous compression value here
	t.stressMask=0b0011 # z is strain-controlled, x,y stress-controlled
	# allow faster deformation along x,y to better maintain stresses
	t.maxStrainRate=(1.,1.,.005)
	# next time, call triaxFinished instead of compactionFinished
	t.doneHook='import woo.pre.cylTriax; woo.pre.cylTriax.triaxDone(S)'
	# do not wait for stabilization before calling triaxFinished
	t.maxUnbalanced=10

def plotBatchResults(db):
	'Hook called from woo.batch.writeResults'
	import pylab,re,math,woo.batch,os
	results=woo.batch.dbReadResults(db)
	out='%s.pdf'%re.sub('\.results$','',db)

	from matplotlib.ticker import FuncFormatter
	kiloPascal=FuncFormatter(lambda x,pos=0: '%g'%(1e-3*x))
	percent=FuncFormatter(lambda x,pos=0: '%g'%(1e2*x))

	fig=pylab.figure(figsize=(8,20))
	ed_qp=fig.add_subplot(311)
	ed_qp.set_xlabel(r'$\varepsilon_d$ [%]')
	ed_qp.set_ylabel(r'$q/p$')
	ed_qp.xaxis.set_major_formatter(percent)
	ed_qp.grid(True)

	ed_ev=fig.add_subplot(312)
	ed_ev.set_xlabel(r'$\varepsilon_d$ [%]')
	ed_ev.set_ylabel(r'$\varepsilon_v$ [%]')
	ed_ev.xaxis.set_major_formatter(percent)
	ed_ev.yaxis.set_major_formatter(percent)
	ed_ev.grid(True)

	p_q=fig.add_subplot(313)
	p_q.set_xlabel(r'$p$ [kPa]')
	p_q.set_ylabel(r'$q$ [kPa]')
	p_q.xaxis.set_major_formatter(kiloPascal)
	p_q.yaxis.set_major_formatter(kiloPascal)
	p_q.grid(True)

	for res in results:
		series,pre=res['series'],res['pre']
		title=res['title'] if res['title'] else res['sceneId']
		mask=series['stressMask']
		# skip the very first number, since that's the transitioning step and strains are still at their old value
		ed=series['eDev'][mask==0b011][1:]
		ev=series['eVol'][mask==0b011][1:]
		p=series['p'][mask==0b011][1:]
		q=series['q'][mask==0b011][1:]
		qDivP=series['qDivP'][mask==0b011][1:]
		ed_qp.plot(ed,qDivP,label=title,alpha=.6)
		ed_ev.plot(ed,ev,label=title,alpha=.6)
		p_q.plot(p,q,label=title,alpha=.6)
	ed_qp.invert_xaxis()
	ed_ev.invert_xaxis()
	ed_ev.invert_yaxis()
	p_q.invert_xaxis()
	p_q.invert_yaxis()
	for ax,loc in (ed_qp,'lower right'),(ed_ev,'lower right'),(p_q,'upper left'):
		l=ax.legend(loc=loc,labelspacing=.2,prop={'size':7})
		l.get_frame().set_alpha(.4)
	fig.savefig(out)
	print 'Batch figure saved to file://%s'%os.path.abspath(out)


def triaxDone(S):
	print 'Triaxial done at step',S.step
	S.stop()
	import woo.utils
	(repName,figs)=woo.utils.xhtmlReport(S,S.pre.reportFmt,'Cylindrical triaxial test',afterHead='',figures=[(None,f) for f in S.plot.plot(noShow=True,subPlots=False)],svgEmbed=True,show=True)
	woo.batch.writeResults(defaultDb='cylTriax.results',series=S.plot.data,postHooks=[plotBatchResults],simulationName='horse',report='file://'+repName)



