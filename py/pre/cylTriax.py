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
		_PAT(Vector3,'maxRates',(2e-1,5e-2,1.),'Maximum strain rates during the compaction phase (for all axes), and during the triaxial phase in axial sense and radial sense.'),
		_PAT(float,'massFactor',.5,'Multiply real mass of particles by this number to obtain the :obj:`woo.dem.WeirdTriaxControl.mass` control parameter'),
		_PAT(int,'nPar',2000,startGroup='Particles',doc='Number of particles'),
		_PAT(woo.dem.FrictMat,'mat',FrictMat(young=1e6,ktDivKn=.2,tanPhi=.4,density=1e8),'Material of particles.'),
		_PAT(float,'meshTanPhi',float('nan'),'Angle of friction of boundary walls; if nan, use the same as for particles, otherwise particle material is copied and tanPhi changed.'),
		_PAT([Vector2,],'psd',[(2e-3,0),(2.5e-3,.2),(4e-3,1.)],unit=['mm','%'],doc='Particle size distribution of particles; first value is diameter, scond is cummulative mass fraction.'),
		_PAT(float,'sigIso',-50e3,unit='Pa',doc='Isotropic compaction stress, and lateral stress during the triaxial phase'),
		_PAT(float,'stopStrain',-.2,doc='Goal value of axial deformation in the triaxial phase'),

		_PAT(Vector2,'radHt',Vector2(.02,.05),doc='Initial size of the cylinder (radius and height)'),

		_PAT(woo.dem.ElastMat,'circMat',woo.dem.ElastMat(young=1e3,density=1.),'Material for circumferential trusses (simulating the membrane). If *None*, membrane will not be simulated. The membrane is only added after the compression phase.'),
		_PAT(float,'circAvgThick',-.001,'Average thickness of circumferential membrane; if negative, relative to cylinder radius (``radHt[0]``).'),

		_PAT(str,'reportFmt',"/tmp/{tid}.xhtml",startGroup="Outputs",doc="Report output format; :obj:`Scene.tags <woo.core.Scene.tags>` can be used."),
		_PAT(str,'packCacheDir',".","Directory where to store pre-generated feed packings; if empty, packing wil be re-generated every time."),
		#_PAT(str,'saveFmt',"/tmp/{tid}-{stage}.bin.gz",'''Savefile format; keys are :obj:`Scene.tags <woo.core.Scene.tags>`; additionally ``{stage}`` will be replaced by
		#* ``init`` for stress-free but compact cloud,
		#* ``iso`` after isotropic compaction,
		#* ``backup-011234`` for regular backups, see :obj:`backupSaveTime`,
		# 'done' at the very end.
		#'''),
		#_PAT(int,'backupSaveTime',1800,doc='How often to save backup of the simulation (0 or negative to disable)'),
		_PAT(float,'pWaveSafety',.1,startGroup='Tunables',doc='Safety factor for :obj:`woo.utils.pWaveDt` estimation.'),
		#_PAT(float,'initPoro',.7,'Estimate of initial porosity, when generating loose cloud'),
		_PAT(Vector2i,'cylDiv',(50,20),'Number of segments for cylinder (first component) and height segments (second component); the second component is ignored if *circMat* is given, in which case vertical segmentation is chosen so that tiles are as close to square'),
		_PAT(float,'damping',.5,'Nonviscous damping'),
		_PAT(int,'vtkStep',0,'Periodicity of saving VTK exports'),
		_PAT(str,'vtkFmt','/tmp/{title}.{id}-','Prefix for VTK exports')
	]
	def __init__(self,**kw):
		woo.core.Preprocessor.__init__(self)
		self.wooPyInit(self.__class__,woo.core.Preprocessor,**kw)
	def __call__(self):
		# preprocessor builds the simulation when called
		return prepareCylTriax(self)


def mkFacetCyl(aabb,cylDiv,topMat,sideMat,botMat,topMask,sideMask,botMask,topBlock,sideBlock,botBlock,mass,inertia):
	'Make closed cylinder from facets. Z is axis of the cylinder. The position is determined by aabb; the cylinder may be elliptical, if the x and y dimensions are different. Return list of particles and list of nodes. The first two nodes in the list are bottom central node and top central node.'
	r1,r2=.5*aabb.sizes()[0],.5*aabb.sizes()[1]
	C=aabb.center()
	zMin,zMax=aabb.min[2],aabb.max[2]

	centrals=[woo.core.Node(pos=Vector3(C[0],C[1],zMin)),woo.core.Node(pos=Vector3(C[0],C[1],zMax))]
	for c in centrals:
		c.dem=woo.dem.DemData()
		c.dem.mass=mass
		c.dem.inertia=inertia
		c.dem.blocked='xyzXYZ'

	retParticles=[]

	# nodes on the perimeter
	thetas=numpy.linspace(0,2*math.pi,num=cylDiv[0],endpoint=False)
	xxyy=[Vector2(r1*math.cos(th)+C[0],r2*math.sin(th)+C[1]) for th in thetas]
	zz=numpy.linspace(zMin,zMax,num=cylDiv[1],endpoint=True)
	nnn=[[woo.core.Node(pos=Vector3(xy[0],xy[1],z)) for xy in xxyy] for z in zz]
	for i,nn in enumerate(nnn):
		if i==0: blocked=botBlock
		elif i==(len(nnn)-1): blocked=topBlock
		else: blocked=sideBlock
		for n in nn:
			n.dem=woo.dem.DemData()
			n.dem.mass=mass
			n.dem.inertia=inertia
			n.dem.blocked=blocked
	def mkCap(nn,central,mask,mat):
		ret=[]
		for i in range(len(nn)):
			ret.append(woo.dem.Particle(material=mat,shape=Facet(nodes=[nn[i],nn[(i+1)%len(nn)],central]),mask=mask))
			nn[i].dem.parCount+=1
			nn[(i+1)%len(nn)].dem.parCount+=1
			central.dem.parCount+=1
		return ret
	retParticles+=mkCap(nnn[0],central=centrals[0],mask=botMask,mat=topMat)
	retParticles+=mkCap(list(reversed(nnn[-1])),central=centrals[-1],mask=topMask,mat=botMat) # reverse to have normals outside
	def mkAround(nnAC,nnBD,mask,mat):
		ret=[]
		for i in range(len(nnAC)):
			A,B,C,D=nnAC[i],nnBD[i],nnAC[(i+1)%len(nnAC)],nnBD[(i+1)%len(nnBD)]
			ret+=[woo.dem.Particle(material=mat,shape=FlexFacet(nodes=fNodes),mask=mask) for fNodes in ((A,B,D),(A,D,C))]
			A.dem.parCount+=2
			D.dem.parCount+=2
			B.dem.parCount+=1
			C.dem.parCount+=1
		return ret
	for i in range(0,len(nnn)-1):
		retParticles+=mkAround(nnn[i],nnn[i+1],mask=sideMask,mat=sideMat)
	for p in retParticles: p.shape.wire=True
	import itertools
	return retParticles,centrals+list(itertools.chain.from_iterable(nnn))

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
	# always square mesh?
	if 1 or pre.circMat:
		pre.cylDiv[1]=int(round(pre.radHt[1]/(2*math.pi*pre.radHt[0]/pre.cylDiv[0]))) # try to be as square as possible
	S.pre=pre.deepcopy()

	meshMask=0b0011
	spheMask=0b0001
	loneMask=0b0010

	S.dem.loneMask=loneMask
	if math.isnan(pre.meshTanPhi): meshMat=pre.mat
	else:
		meshMat=pre.mat.deepcopy()
		meshMat.tanPhi=pre.meshTanPhi
	# reset friction for spheres, but keep the value of tanPhi in S.pre.mat.tanPhi
	sphMat=pre.mat.deepcopy();
	sphMat.tanPhi=0.

	# radius minus polygonal imprecision (circle segment)
	innerRad=rad-rad*(1.-math.cos(.5*2*math.pi/pre.cylDiv[0]))

	
	S.dem.par.append(woo.pack.randomLoosePsd(predicate=woo.pack.inCylinder(centerBottom=(xymid[0],xymid[1],bot),centerTop=(xymid[0],xymid[1],top),radius=innerRad),psd=pre.psd,mat=sphMat))

	sphereMass=sum([p.mass for p in S.dem.par])

	S.periodic=True
	S.cell.setBox(xydomain[0],xydomain[1],(2*margin+1)*ht)

	nodeMass=1.*sphMat.density*(4/3.)*math.pi*(.5*pre.psd[-1][0])**3
	nodeInertia=(2/5.)*(.4*pre.psd[-1][0])**2*nodeMass

	particles,nodes=mkFacetCyl(AlignedBox3((xymin[0],xymin[1],bot),(xymax[0],xymax[1],top)),cylDiv=pre.cylDiv,topMask=meshMask,botMask=meshMask,sideMask=meshMask,sideBlock='xyzXYZ',botBlock='xyzXYZ',topBlock='xyzXYZ',mass=nodeMass,inertia=nodeInertia*Vector3(1,1,1),topMat=meshMat,sideMat=meshMat,botMat=meshMat)

	S.lab.cylNodes=nodes

	S.dem.par.append(particles)
	#S.dem.nodes.extend(nodes)
	S.dem.collectNodes() # collect both facets and spheres

	S.dt=pre.pWaveSafety*woo.utils.pWaveDt(S)
	# setup engines
	S.engines=[
		InsertionSortCollider([Bo1_Sphere_Aabb(),Bo1_Facet_Aabb()],verletDist=-.05),
		ContactLoop([Cg2_Sphere_Sphere_L6Geom(),Cg2_Facet_Sphere_L6Geom()],[Cp2_FrictMat_FrictPhys()],[Law2_L6Geom_FrictPhys_IdealElPl()],applyForces=True,label='contactLoop'), 
		IntraForce([In2_Sphere_ElastMat(),In2_FlexFacet_ElastMat(thickness=pre.psd[0][0],bendThickness=pre.psd[0][0],bending=True)],label='intraForce',dead=True), # ContactLoop applies first
		WeirdTriaxControl(goal=(pre.sigIso,pre.sigIso,pre.sigIso),maxStrainRate=(pre.maxRates[0],pre.maxRates[0],pre.maxRates[0]),relVol=math.pi*rad**2*ht/S.cell.volume,stressMask=0b0111,maxUnbalanced=0.15,mass=pre.massFactor*sphereMass,doneHook='import woo.pre.cylTriax; woo.pre.cylTriax.compactionDone(S)',label='triax',absStressTol=1e4,relStressTol=1e-2),
		woo.core.PyRunner(20,'import woo.pre.cylTriax; woo.pre.cylTriax.addPlotData(S)'),
		VtkExport(out=pre.vtkFmt,stepPeriod=pre.vtkStep,what=VtkExport.all,dead=(pre.vtkStep<=0 or not pre.vtkFmt)),
		Leapfrog(damping=pre.damping,reset=True),
	]
	S.timingEnabled=True
	import woo.log
	#woo.log.setLevel('InsertionSortCollider',woo.log.TRACE)
	#woo.log.setLevel('WeirdTriaxControl',woo.log.TRACE)

	try:
		import woo.gl
		S.any=[woo.gl.Renderer(dispScale=(5,5,5),rotScale=10),woo.gl.Gl1_DemField(),woo.gl.Gl1_CPhys(),woo.gl.Gl1_FlexFacet(phiSplit=False,phiWd=1,relPhi=.02)]
	except ImportError: pass

	return S

def addPlotData(S):
	import woo
	t=S.lab.triax
	sxx,syy,szz=t.stress.diagonal()
	exx,eyy,ezz=t.strain
	p=t.stress.diagonal().sum()/3. # mean stress
	q=szz-.5*(sxx+syy)         # deviatoric stress?!
	qDivP=(q/p if p!=0 else float('nan'))
	eDev=ezz-t.strain.sum()/3. # ezz-mean
	eVol=(t.strain.sum() if t.stressMask==0b0011 else float('nan')) # trace; don't save in the compression phase
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
			'i':('unbalanced',),'i ':('sxx','syy','srr','szz'),' i':('exx','eyy','err','ezz','eVol'),
			# energy plot
			#' i ':(O.energy.keys,None,'Etot'),
		}
		S.plot.xylabels={'i ':('step','Stress [Pa]',),' i':('step','Strains [-]','Strains [-]')}
		S.plot.labels={
			'sxx':r'$\sigma_{xx}$','syy':r'$\sigma_{yy}$','szz':r'$\sigma_{zz}$','srr':r'$\sigma_{rr}$',
			'exx':r'$\varepsilon_{xx}$','eyy':r'$\varepsilon_{yy}$','ezz':r'$\varepsilon_{zz}$','err':r'$\varepsilon_{rr}$','eVol':r'$\varepsilon_{v}$'
		}

def velocityFieldPlots(S,nameBase):
	import woo
	from woo import post2d
	flattener=post2d.CylinderFlatten(useRef=False,axis=2,origin=(.5*S.cell.size[0],.5*S.cell.size[1],(.6/2.2*S.cell.size[2])))
	#maxVel=float('inf') #5e-2
	#exVel=lambda p: p.vel if p.vel.norm()<=maxVel else p.vel/(p.vel.norm()/maxVel)
	exVel=lambda p: p.vel
	exVelNorm=lambda p: exVel(p).norm()
	import pylab
	fVRaw=pylab.figure()
	post2d.plot(post2d.data(S,exVel,flattener),alpha=.3,minlength=.3,cmap='jet')
	fV2=pylab.figure()
	post2d.plot(post2d.data(S,exVel,flattener,stDev=.5*S.pre.psd[0][0],div=(80,80)),minlength=.6,cmap='jet')
	fV1=pylab.figure()
	post2d.plot(post2d.data(S,exVelNorm,flattener,stDev=.5*S.pre.psd[0][0],div=(80,80)),cmap='jet')
	outs=[]
	for name,fig in [('particle-velocity',fVRaw),('smooth-velocity',fV2),('smooth-velocity-norm',fV1)]:
		out=nameBase+'.%s.png'%name
		fig.savefig(out)
		outs.append(out)
	return outs

def compactionDone(S):
	print 'Compaction done at step',S.step
	import woo
	t=S.lab.triax
	# set the current cell configuration to be the reference one
	S.cell.trsf=Matrix3.Identity
	# change control type: keep constant confinement in x,y, 20% compression in z
	t.goal=(0,0,S.pre.stopStrain) # use ridiculous compression value here
	t.stressMask=0b0000 # z is strain-controlled, x,y stress-controlled
	# allow faster deformation along x,y to better maintain stresses
	t.maxStrainRate=(0,0,S.pre.maxRates[1])
	# next time, call triaxFinished instead of compactionFinished
	t.doneHook='import woo.pre.cylTriax; woo.pre.cylTriax.triaxDone(S)'
	# do not wait for stabilization before calling triaxFinished
	t.maxUnbalanced=10
	##
	## restore friction angle of particles
	for p in S.dem.par:
		if type(p.shape)!=woo.dem.Sphere: continue
		p.mat.tanPhi=S.pre.mat.tanPhi
		break # shared material, once is enough
	try:
		import woo.gl
		woo.gl.Gl1_DemField.updateRefPos=True
	except ImportError: pass
	# reset contacts so that they pick up new friction angle
	for c in S.dem.con: c.resetPhys()

	# make the membrane flexible: apply force on the membrane
	S.lab.contactLoop.applyForces=False
	S.lab.intraForce.dead=False
	# free the nodes
	top,bot=S.lab.cylNodes[:2]
	tol=1e-3*abs(top.pos[2]-bot.pos[2])
	for n in S.lab.cylNodes[2:]:
		# supports may move in-plane, no rotation
		if abs(n.pos[2]-top.pos[2])<tol or abs(n.pos[2]-bot.pos[2])<tol:
			n.dem.blocked='zXYZ' 
		else: n.dem.blocked='' # don't rotate unless we have bending at some point

	S.save('/tmp/compact.gz')

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



