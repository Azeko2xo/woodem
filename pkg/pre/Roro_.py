# encoding: utf-8
from woo import utils,pack
from woo.core import *
from woo.dem import *
from woo.pre import *
import woo.plot
import math
import os.path
from miniEigen import *
import sys
import cStringIO as StringIO
import numpy
import pprint
nan=float('nan')

# assure compatibility
if not hasattr(ConveyorFactory,'generator'):
	ConveyorFactory.generator=property(lambda x: x)

def ySpannedFacets(xx,yy,zz,**kw):
	A,B,C,D=(xx[0],yy[0],zz[0]),(xx[1],yy[0],zz[1]),(xx[0],yy[1],zz[0]),(xx[1],yy[1],zz[1])
	return [utils.facet(vertices,**kw) for vertices in (A,B,C),(C,B,D)]


def run(pre): # use inputs as argument
	print 'Roro_.run()'
	#print 'Input parameters:'
	#print pre.dumps(format='expr',noMagic=True)
	#print pre.dumps(format='html',fragment=True)

	s=Scene()
	de=DemField()
	s.fields=[de]

	if pre.variant not in ('plain','customer1'): raise ValueError("Roro.variant must be one of 'plain', 'customer1' (not '%s')"%pre.variant)
	if pre.gap<0: raise ValueError("Roro.gap must be positive (are cylinders overlapping?)")
	if (pre.time>0 and pre.mass>0) or (pre.time<=0 and pre.mass<=0): raise ValueError("Exactly one of Roro.time or Roro.mass must be positive.")
	
	# generate cylinder coordinates, or use those given by the user
	if pre.cylXzd:
		if pre.variant!='plain': raise ValueError("Roro.cylXzd can be given as coordinates only with Roro.variant 'plain' (not '%s')."%pre.variant)
		cylXzd=pre.cylXzd
	else:
		cylXzd=[]
		dCyl=pre.cylDiameter
		xz0=(0,-dCyl/2) # first cylinder is right below the feed end at (0,0)
		if pre.variant=='customer1': xz0=(0,0)
		for i in range(0,pre.cylNum):
			dist=i*(dCyl+pre.gap) # distance from the first cylinder's center
			cylXzd.append(Vector3(xz0[0]+math.cos(pre.inclination)*dist,xz0[1]-math.sin(pre.inclination)*dist,dCyl))
	cylXMax=max([xzd[0] for xzd in cylXzd])
	cylZMin=min([xzd[1] for xzd in cylXzd])
	cylDMax=max([xzd[2] for xzd in cylXzd])
	print 'Cylinder coordinates and diameters:'
	pprint.pprint(cylXzd)

	rMin=pre.psd[0][0]/2.
	rMax=pre.psd[-1][0]/2.
	s.dt=pre.pWaveSafety*utils.spherePWaveDt(rMin,pre.material.density,pre.material.young)

	# particle masks
	wallMask=0b00110
	loneMask=0b00100 # particles with this mask don't interact with each other
	sphMask= 0b00011
	delMask= 0b00001 # particles which might be deleted by deleters
	
	###
	### conveyor feed
	###
	print 'Preparing packing for conveyor feed, be patient'
	cellLen=15*pre.psd[-1][0]
	cc,rr=makeBandFeedPack(dim=(cellLen,pre.cylLenSim,pre.conveyorHt),psd=pre.psd,mat=pre.material,gravity=(0,0,-pre.gravity),porosity=.7,memoizeDir=pre.feedCacheDir)
	vol=sum([4/3.*math.pi*r**3 for r in rr])
	conveyorVel=(pre.massFlowRate*pre.cylRelLen)/(pre.material.density*vol/cellLen)
	print 'Feed velocity %g m/s to match feed mass %g kg/m (volume=%g m³, len=%gm, ρ=%gkg/m³) and massFlowRate %g kg/s (%g kg/s over real width)'%(conveyorVel,pre.material.density*vol/cellLen,vol,cellLen,pre.material.density,pre.massFlowRate*pre.cylRelLen,pre.massFlowRate)

	ymin,ymax=-pre.cylLenSim/2.,pre.cylLenSim/2.
	if pre.variant=='plain':
		# define simulation domain
		zmin,zmax=cylZMin-2*cylDMax,max(4*cylDMax,4*pre.conveyorHt)
		xmin,xmax=-3*cylDMax,cylXzd[-1][0]+3*cylDMax
		factoryNode=Node(pos=(xmin,0,0))

		de.par.append(ySpannedFacets((xmin,0),(ymin,ymax),(0,0),mat=pre.material,mask=wallMask,fakeVel=(conveyorVel,0,0)))
	elif pre.variant=='customer1':
		# F                E is cylinder top; D is feed conveyor start, |D-E|=10cm
		# ____ E           cylinder radius 4cm, centered at D=C-(0,4cm)
		#   (_)  C         B-C vertical, |B-C|=27cm; C is at the leftmost point of the cylinder
		# D   |  
		#     |            |A-B|=28cm
		#   B ^-_          A-B in inclined by 27° more than the screen
		#        ^-_  A    cylinder below touched at point A
		#          ( )      centered at (0,0)
		#
		A=cylXzd[0][2]*.5*Vector2(math.sin(pre.inclination),math.cos(pre.inclination))
		abAngle=pre.inclination+math.radians(27)
		abLen=0.28
		B=A+abLen*Vector2(-math.cos(abAngle),math.sin(abAngle))
		upCylRad=0.04
		C=B+Vector2(0,0.27)
		D=C+Vector2(-upCylRad,0)
		E=D+Vector2(0,upCylRad)
		F=E+Vector2(-.1,0)
		for i in 'ABCDEF': print i,eval(i)
		yy=(ymin,ymax)
		kw=dict(mat=pre.material,mask=wallMask)
		de.par.append(ySpannedFacets((A[0],B[0]),yy,(A[1],B[1]),**kw)+ySpannedFacets((B[0],C[0]),yy,(B[1],C[1]),**kw)+ySpannedFacets((E[0],F[0]),yy,(E[1],F[1]),**kw))
		feedCyl=utils.infCylinder((D[0],0,D[1]),radius=upCylRad,axis=1,glAB=yy,**kw)
		feedCyl.angVel=Vector3(0,conveyorVel/upCylRad,0)
		de.par.append(feedCyl)

		zmin,zmax=cylZMin-2*cylDMax,F[1]+3*pre.conveyorHt
		xmin,xmax=F[0],cylXzd[-1][0]+3*cylDMax
		factoryNode=Node(pos=(F[0],0,F[1]+rMax))
	else: assert False

	factStep=pre.factStepPeriod
	factory=ConveyorFactory(
		stepPeriod=factStep,
		material=pre.material,
		centers=cc,radii=rr,
		cellLen=cellLen,
		barrierColor=.3,
		#color=.4, # random colors
		node=factoryNode,
		mask=sphMask,
		vel=conveyorVel,
		label='factory',
		maxMass=-1, # not limited, until steady state is reached
		currRateSmooth=pre.rateSmooth,
	)

	###
	### walls and cylinders
	###		
	sideWallMat=pre.material.deepcopy()
	sideWallMat.tanPhi=0.0
	de.par.append([
		utils.wall(ymin,axis=1,sense= 1,visible=False,glAB=((zmin,xmin),(zmax,xmax)),mat=sideWallMat,mask=wallMask),
		utils.wall(ymax,axis=1,sense=-1,visible=False,glAB=((zmin,xmin),(zmax,xmax)),mat=sideWallMat,mask=wallMask),
		utils.wall(xmin,axis=0,sense= 1,visible=False,glAB=((ymin,zmin),(ymax,zmax)),mat=pre.material,mask=wallMask),
		utils.wall(xmax,axis=0,sense=-1,visible=False,glAB=((ymin,zmin),(ymax,zmax)),mat=pre.material,mask=wallMask),
		utils.wall(zmin,axis=2,sense= 1,visible=False,glAB=((xmin,ymin),(xmax,ymax)),mat=pre.material,mask=wallMask),
	])
	for i,xzd in enumerate(cylXzd):
		x,z,d=xzd
		c=utils.infCylinder((x,0,z),radius=d/2.,axis=1,glAB=(ymin,ymax),mat=pre.material,mask=wallMask)
		c.angVel=(0,pre.angVel,0)
		qv,qh=pre.quivVPeriod,pre.quivHPeriod
		c.impose=AlignedHarmonicOscillations(
			amps=(pre.quivAmp[0]*d/2.,nan,pre.quivAmp[1]*d/2.),
			freqs=(
				1./((qh[0]+(i%int(qh[2]))*(qh[1]-qh[0])/int(qh[2]))*s.dt),
				nan,
				1./((qv[0]+(i%int(qv[2]))*(qv[1]-qv[0])/int(qv[2]))*s.dt)
			)
		)
		de.par.append(c)
		de.par.append(ySpannedFacets((x,x),(ymin,ymax),(zmin,z-d/2.),mat=pre.material,mask=wallMask,visible=False))

	###
	### engines
	###

	s.engines=utils.defaultEngines(damping=0.05,gravity=(0,0,-pre.gravity),verletDist=.05*rMin)+[
		# what falls beyond
		# initially not saved
		BoxDeleter(stepPeriod=factStep,inside=True,box=((cylXzd[i][0],ymin,zmin),(cylXzd[i+1][0],ymax,min(cylXzd[i][1]-cylXzd[i][2]/2.,cylXzd[i+1][1]-cylXzd[i+1][2]/2.))),glColor=.05*i,save=False,mask=delMask,currRateSmooth=pre.rateSmooth,label='aperture[%d]'%i) for i in range(0,len(cylXzd)-1)
	]+[
		# this one should not collect any particles at all
		BoxDeleter(stepPeriod=factStep,box=((xmin,ymin,zmin),(xmax,ymax,zmax)),glColor=.9,save=False,mask=delMask,label='outOfDomain'),
		# what falls inside
		BoxDeleter(stepPeriod=factStep,inside=True,box=((cylXzd[-1][0]+cylXzd[-1][2]/2.,ymin,zmin),(xmax,ymax,zmax)),glColor=.9,save=True,mask=delMask,currRateSmooth=pre.rateSmooth,label='fallOver'),
		# generator
		factory,
		PyRunner(factStep,'import woo.pre.Roro_; woo.pre.Roro_.savePlotData(S)'),
		PyRunner(factStep,'import woo.pre.Roro_; woo.pre.Roro_.watchProgress(S)'),
	]+(
		[] if (not pre.vtkPrefix or pre.vtkFreq<=0) else [VtkExport(out=pre.vtkPrefix.format(**dict(s.tags)),stepPeriod=pre.vtkFreq*pre.factStepPeriod,what=VtkExport.all)]
	)+(
		[] if pre.backupSaveTime<=0 else [PyRunner(realPeriod=pre.backupSaveTime,stepPeriod=-1,command='S.save(S.pre.saveFmt.format(stage="backup-%06d"%(S.step),S=S,**(dict(S.tags))))')]
	)

	# improtant: save the preprocessor here!
	s.pre=pre.deepcopy() # avoids bug http://gpu.doxos.eu/trac/ticket/81, which might get triggered here
	de.collectNodes()

	if pre.mass:
		import woo
		for a in woo.aperture: a.save=True
		woo.factory.save=True
		woo.factory.maxMass=pre.mass*pre.cylRelLen

	# when running with gui, set initial view setup
	# the import fails if headless
	try:
		import woo.gl
		# set other display options and save them (static attributes)
		s.any=[
			woo.gl.Renderer(iniUp=(0,0,1),iniViewDir=(0,1,0)),
			#woo.gl.Gl1_DemField(glyph=woo.gl.Gl1_DemField.glyphVel),
			woo.gl.Gl1_DemField(colorBy=woo.gl.Gl1_DemField.colorVel),
			woo.gl.Gl1_InfCylinder(wire=True),
			woo.gl.Gl1_Wall(div=1),
		]
	except ImportError: pass

	print 'Generated Rollenrost.'
	out=pre.saveFmt.format(stage='init',S=s,**(dict(s.tags)))
	s.save(out)
	print 'Saved initial simulation also to ',out


	woo.plot.reset()

	return s


def makeBandFeedPack(dim,psd,mat,gravity,porosity=.5,dontBlock=False,memoizeDir=None):
	cellSize=(dim[0],dim[1],(1+2*porosity)*dim[2])
	print 'cell size',cellSize,'target height',dim[2]
	if memoizeDir:
		params=str(dim)+str(cellSize)+str(psd)+mat.dumps(format='expr')+str(Gravity)+str(porosity)
		import hashlib
		paramHash=hashlib.sha1(params).hexdigest()
		memoizeFile=memoizeDir+'/'+paramHash+'.bandfeed'
		print 'Memoize file is ',memoizeFile
		if os.path.exists(memoizeDir+'/'+paramHash+'.bandfeed'):
			print 'Returning memoized result'
			sp=pack.SpherePack()
			sp.load(memoizeFile)
			return zip(*sp)
	S=Scene(fields=[DemField()])
	S.periodic=True
	S.cell.setBox(cellSize)
	p=pack.sweptPolylines2gtsSurface([utils.tesselatePolyline([Vector3(x,0,cellSize[2]),Vector3(x,0,0),Vector3(x,cellSize[1],0),Vector3(x,cellSize[1],cellSize[2])],maxDist=min(cellSize[1],cellSize[2])/3.) for x in numpy.linspace(0,cellSize[0],num=4)])
	S.dem.par.append(pack.gtsSurface2Facets(p,mask=0b011))
	S.loneMask=0b010

	massToDo=porosity*mat.density*dim[0]*dim[1]*dim[2]
	print 'Will generate %g mass'%massToDo

	## FIXME: decrease friction angle to help stabilization
	mat0,mat=mat,mat.deepcopy()
	mat.tanPhi=min(.2,mat0.tanPhi)

	S.engines=utils.defaultEngines(gravity=gravity,damping=.3)+[
		BoxFactory(
			box=((.01*cellSize[0],.01*cellSize[1],.3*cellSize[2]),cellSize),
			stepPeriod=200,
			maxMass=massToDo,
			massFlowRate=0,
			maxAttempts=20,
			generator=PsdSphereGenerator(psdPts=psd,discrete=False,mass=True),
			materials=[mat],
			shooter=AlignedMinMaxShooter(dir=(0,0,-1),vRange=(0,0)),
			mask=1,
			label='makeBandFeedFactory',
			#periSpanMask=1, # x is periodic
		),
		#PyRunner(200,'plot.addData(uf=utils.unbalancedForce(),i=O.scene.step)'),
		PyRunner(600,'print "%g/%g mass, %d particles, unbalanced %g/.15"%(woo.makeBandFeedFactory.mass,woo.makeBandFeedFactory.maxMass,len(S.dem.par),woo.utils.unbalancedForce(S))'),
		PyRunner(200,'if woo.utils.unbalancedForce(S)<.15 and woo.makeBandFeedFactory.dead: S.stop()'),
	]
	S.dt=.7*utils.spherePWaveDt(psd[0][0],mat.density,mat.young)
	if dontBlock: return S
	else: S.run()
	S.wait()
	cc,rr=[],[]
	for p in S.dem.par:
		if not type(p.shape)==Sphere: continue
		c,r=S.cell.canonicalizePt(p.pos),p.shape.radius
		if c[2]+r>dim[2]: continue
		cc.append(Vector3(c[0],c[1]-.5*dim[1],c[2])); rr.append(r)
	if memoizeDir:
		sp=pack.SpherePack()
		for c,r in zip(cc,rr): sp.add(c,r)
		print 'Saving to',memoizeFile
		sp.save(memoizeFile)
	return cc,rr


def watchProgress(S):
	'''initially, only the fallOver deleter saves particles; once particles arrive,
	it means we have reached some steady state; at that point, all objects (deleters,
	factory, ... are clear()ed so that PSD's and such correspond to the steady
	state only'''
	import woo
	pre=S.pre
	## wait for steady state, then simulate pre.time time of steady state
	if pre.time>0:
		assert pre.mass<=0
		# not yet saving what falls through, i.e. not in stabilized regime yet
		if woo.aperture[0].save==False:
			# first particles have just fallen over
			# start saving apertures and reset our counters here
			#if woo.fallOver.num>pre.steadyOver:
			influx,efflux=woo.factory.currRate,(woo.fallOver.currRate+woo.outOfDomain.currRate+sum([a.currRate for a in woo.aperture]))
			if efflux>pre.steadyFlowFrac*influx:
				#print efflux,'>',pre.steadyFlowFrac,'*',influx
				woo.fallOver.clear()
				woo.factory.clear() ## FIXME
				woo.factory.maxMass=pre.time*pre.cylRelLen*pre.massFlowRate # required mass scaled to simulated part
				for ap in woo.aperture:
					ap.clear() # to clear overall mass, which gets counted even with save=False
					ap.save=True
				print 'Stabilized regime reached (influx %g, efflux %g) at step %d (t=%gs), counters engaged.'%(influx,efflux,S.step,S.time)
				#out='/tmp/steady-'+S.tags['id.d']+'.bin.gz'
				out=S.pre.saveFmt.format(stage='steady',S=S,**(dict(S.tags)))
				S.save(out)
				print 'Saved to',out
		# already in the stable regime, end simulation at some point
		else:
			# factory has finished generating particles
			if woo.factory.mass>woo.factory.maxMass:
				print 'All mass generated at step %d (t=%gs, mass %g/%g), ending.'%(S.step,S.time,woo.factory.mass,woo.factory.maxMass)
				import woo.plot, pickle
				try:
					out=S.pre.saveFmt.format(stage='done',S=S,**(dict(S.tags)))
					if S.lastSave==out:
						woo.plot.data=pickle.loads(S.tags['plot.data'])
						print 'Reloaded plot data'
					else:
						S.tags['plot.data']=pickle.dumps(woo.plot.data)
						S.save(out)
						print 'Saved (incl. plot.data) to',out
					writeReport(S)
				except:
					import traceback
					traceback.print_exc()
					print 'Error during post-processing.'
				print 'Simulation finished.'
				S.stop()
	elif pre.mass>0:
		assert pre.time<=0
		## after the mass is reached, continue for some time (say 20% of the simulation up to now), then finish
		if woo.factory.mass>=woo.factory.maxMass:
			m=sum([n.dem.mass for n in S.dem.nodes])
			if m<pre.residueMassFrac*woo.factory.maxMass:
				try:
					writeReport(S)
				except:
					import traceback
					traceback.print_exc()
					print 'Error during post-processing.'
				print 'Simulation finished.'
				S.stop()
	else: assert False

def savePlotData(S):
	import woo
	#if woo.aperture[0].save==False: return # not in the steady state yet
	import woo.plot
	# save unscaled data here!
	apRate=sum([a.currRate for a in woo.aperture])
	overRate=woo.fallOver.currRate
	lostRate=woo.outOfDomain.currRate
	woo.plot.addData(i=S.step,t=S.time,genRate=woo.factory.currRate,apRate=apRate,overRate=overRate,lostRate=lostRate,delRate=apRate+overRate+lostRate,numPar=len(S.dem.par),genMass=woo.factory.mass)
	if not woo.plot.plots:
		woo.plot.plots={'t':('genRate','apRate','lostRate','overRate','delRate',None,('numPar','g--')),'t ':('genMass')}

def splitPsd(xx0,yy0,splitX):
	'''Split input *psd0* (given by *xx0* and *yy0*) at diameter (x-axis) specified by *splitX*; two PSDs are returned, one grows from splitX and has maximum value for all points x>=splitX; the second PSD is zero for all values <=splitX, then grows to the maximum value proportionally. The maximum value is the last point of psd0, thus both normalized and non-normalized input can be given. If splitX falls in the middle of *psd0* intervals, it is interpolated.'''
	maxY=float(yy0[-1])
	assert(len(xx0)==len(yy0))
	import bisect
	ix=bisect.bisect(xx0,splitX)
	#print 'split index %d/%d'%(ix,len(xx0))
	if ix==0: return (xx0,[0.]*len(xx0)),(xx0,yy0)
	if ix==len(xx0): return (xx0,yy0),(xx0,[maxY]*len(xx0))
	splitY=yy0[ix-1]+(yy0[ix]-yy0[ix-1])*(splitX-xx0[ix-1])/(xx0[ix]-xx0[ix-1])
	smallTrsf,bigTrsf=lambda y: 0.+(y-0)*1./(splitY/maxY),lambda y: 0.+(y-splitY)*1./(1-splitY/maxY)
	#print 'splitY/maxY = %g/%g'%(splitY,maxY)
	xx1,yy1,xx2,yy2=[],[],[],[]
	for i in range(0,ix):
		xx1.append(xx0[i]); yy1.append(smallTrsf(yy0[i]))
		xx2.append(xx0[i]); yy2.append(0.)
	xx1.append(splitX); xx2.append(splitX);
	yy1.append(maxY); yy2.append(0.)
	for i in range(ix,len(xx0)):
		xx1.append(xx0[i]); yy1.append(1.)
		xx2.append(xx0[i]); yy2.append(bigTrsf(yy0[i]))
	return (xx1,yy1),(xx2,yy2)

def svgFragment(data):
	return data[data.find('<svg '):]

def psdFeedApertureFalloverTable(inPsd,feedDM,apDM,overDM,splitD):
	import collections, numpy
	TabLine=collections.namedtuple('TabLine','dMin dMax dAvg inFrac feedFrac feedSmallFrac feedBigFrac overFrac')
	feedD,feedM=feedDM
	dScale=1e3 # m → mm
	# must convert to list to get slices right in the next line
	edges=list(inPsd[0]) # those will be displayed in the table
	# enlarge a bit so that marginal particles, if any, fit in comfortably; used for histogram computation
	bins=numpy.array([.99*edges[0]]+edges[1:-1]+[1.01*edges[-1]]) 
	edges=numpy.array(edges) # convert back
	# cumulative; otherwise use numpy.diff
	inHist=numpy.diff(inPsd[1]) # non-cumulative
	feedHist,b=numpy.histogram(feedD,weights=feedM,bins=bins,density=True)
	feedHist/=sum(feedHist)
	feedSmallHist,b=numpy.histogram(feedD[feedD<splitD],weights=feedM[feedD<splitD],bins=bins,density=True)
	feedSmallHist/=sum(feedSmallHist)
	feedBigHist,b=numpy.histogram(feedD[feedD>=splitD],weights=feedM[feedD>=splitD],bins=bins,density=True)
	feedBigHist/=sum(feedBigHist)
	overHist,b=numpy.histogram(overDM[0],weights=overDM[1],bins=bins,density=True)
	overHist/=sum(overHist)
	tab=[]
	for i in range(0,len(inHist)):
		tab.append(TabLine(dMin=edges[i],dMax=edges[i+1],dAvg=.5*(edges[i]+edges[i+1]),inFrac=inHist[i],feedFrac=feedHist[i],feedSmallFrac=feedSmallHist[i],feedBigFrac=feedBigHist[i],overFrac=overHist[i]))
	from genshi.builder import tag as t
	return t.table(
		t.tr(
			t.th('Diameter',t.br,'[mm]'),t.th('Average',t.br,'[mm]'),t.th('user input',t.br,'[mass %]',colspan=2),t.th('feed',t.br,'[mass %]',colspan=2),t.th('feed < %g mm'%(dScale*splitD),t.br,'[mass %]',colspan=2),t.th('feed > %g mm'%(dScale*splitD),t.br,'[mass %]',colspan=2),t.th('fall over',t.br,'[mass %]',colspan=2),align='center'
		),
		*tuple([t.tr(
			t.td('%g-%g'%(dScale*tt.dMin,dScale*tt.dMax),align='left'),t.td('%g'%(dScale*tt.dAvg),align='right'),
			t.td('%.4g'%(1e2*tt.inFrac)),t.td('%.4g'%(1e2*sum([ttt.inFrac for ttt in tab[:i+1]]))),
			t.td('%.4g'%(1e2*tt.feedFrac)),t.td('%.4g'%(1e2*sum([ttt.feedFrac for ttt in tab[:i+1]]))),
			t.td('%.4g'%(1e2*tt.feedSmallFrac)),t.td('%.4g'%(1e2*sum([ttt.feedSmallFrac for ttt in tab[:i+1]]))),
			t.td('%.4g'%(1e2*tt.feedBigFrac)),t.td('%.4g'%(1e2*sum([ttt.feedBigFrac for ttt in tab[:i+1]]))),
			t.td('%.4g'%(1e2*tt.overFrac)),t.td('%.4g'%(1e2*sum([ttt.overFrac for ttt in tab[:i+1]]))),
			align='right'
		) for i,tt in enumerate(tab)])
		,cellpadding='2px',frame='box',rules='all'
	).generate().render('xhtml')

def efficiencyTableFigure(S,pre):
	# split points
	diams=[p[0] for p in pre.psd]
	if pre.gap not in diams: diams.append(pre.gap)
	diams.sort()
	diams=diams[1:-1] # smallest and largest are meaningless (would have 0%/100% everywhere)
	import numpy
	# each line is for one aperture
	# each column is for one fraction
	data=numpy.zeros(shape=(len(woo.aperture),len(diams)))
	for apNum,aperture in enumerate(woo.aperture):
		xMin,xMax=aperture.box.min[0],aperture.box.max[0]
		massTot=0.
		for p in S.dem.par:
			# only spheres above the aperture count
			if not isinstance(p.shape,woo.dem.Sphere) or p.pos[0]<xMin or p.pos[1]>xMax: continue
			massTot+=p.mass
			for i,d in enumerate(diams):
				if 2*p.shape.radius>d: continue
				data[apNum][i]+=p.mass
		data[apNum]/=massTot
	from genshi.builder import tag as t
	table=t.table(
		[t.tr([t.th()]+[t.th('Aperture %d'%(apNum+1)) for apNum in range(len(woo.aperture))])]+
		[t.tr([t.th('< %.4g mm'%(1e3*diams[dNum]))]+[t.td('%.1f %%'%(1e2*data[apNum][dNum]),align='right') for apNum in range(len(woo.aperture))]) for dNum in range(len(diams))]
		,cellpadding='2px',frame='box',rules='all'
	).generate().render('xhtml')
	import pylab
	fig=pylab.figure()
	for dNum,d in reversed(list(enumerate(diams))): # displayed from the bigger to smaller, to make legend aligned with lines
		pylab.plot(numpy.arange(len(woo.aperture))+1,data[:,dNum],marker='o',label='< %.4g mm'%(1e3*d))
	from matplotlib.ticker import FuncFormatter
	percent=FuncFormatter(lambda x,pos=0: '%g%%'%(100*x))
	pylab.gca().yaxis.set_major_formatter(percent)
	pylab.ylabel('Mass fraction')
	pylab.xlabel('Aperture number')
	pylab.ylim(ymin=0,ymax=1)
	l=pylab.legend()
	l.get_frame().set_alpha(.6)
	pylab.grid(True)
	return table,fig

def writeReport(S):
	# generator parameters
	import woo
	import woo.pre
	pre=S.pre
	#pre=[a for a in woo.O.scene.any if type(a)==woo.pre.Roro][0]
	#print 'Parameters were:'
	#pre.dump(sys.stdout,noMagic=True,format='expr')

	import matplotlib.pyplot as pyplot
	pyplot.switch_backend('Agg')

	#import matplotlib
	#matplotlib.use('Agg')
	import pylab
	import os
	import numpy
	# http://www.mail-archive.com/matplotlib-users@lists.sourceforge.net/msg05474.html
	from matplotlib.ticker import FuncFormatter
	percent=FuncFormatter(lambda x,pos=0: '%g%%'%(100*x))
	milimeter=FuncFormatter(lambda x,pos=0: '%3g'%(1000*x))
	megaUnit=FuncFormatter(lambda x,pos=0: '%3g'%(1e-6*x))

	legendAlpha=.6

	# should this be scaled to t/year as well?
	massScale=(
		(1./pre.cylRelLen)*(1./pre.time) # in kg/sec
		*3600*24*365/1000.  # in t/y
	)

	def scaledFlowPsd(x,y,massFactor=1.): return numpy.array(x),massFactor*massScale*numpy.array(y)
	def unscaledPsd(x,y): return numpy.array(x),numpy.array(y)

	figs=[]
	fig=pylab.figure()

	inPsd=scaledFlowPsd([p[0] for p in pre.psd],[p[1]*(1./pre.psd[-1][1]) for p in pre.psd],massFactor=woo.factory.mass)
	inPsdUnscaled=unscaledPsd([p[0] for p in pre.psd],[p[1]*(1./pre.psd[-1][1]) for p in pre.psd])

	feedPsd=scaledFlowPsd(*woo.factory.generator.psd())
	pylab.plot(*inPsd,label='user',marker='o')
	pylab.plot(*feedPsd,label='feed (%g Mt/y)'%(woo.factory.mass*massScale*1e-6))
	overPsd=scaledFlowPsd(*woo.fallOver.psd())
	pylab.plot(*overPsd,label='fall over (%.3g Mt/y)'%(woo.fallOver.mass*massScale*1e-6))
	for i in range(0,len(woo.aperture)):
		try:
			apPsd=scaledFlowPsd(*woo.aperture[i].psd())
			pylab.plot(*apPsd,label='aperture %d (%.3g Mt/y)'%(i,woo.aperture[i].mass*massScale*1e-6))
		except ValueError: pass
	pylab.axvline(x=pre.gap,linewidth=5,alpha=.3,ymin=0,ymax=1,color='g',label='gap')
	pylab.grid(True)
	leg=pylab.legend(loc='best')
	leg.get_frame().set_alpha(legendAlpha)
	pylab.gca().xaxis.set_major_formatter(milimeter)
	pylab.gca().yaxis.set_major_formatter(megaUnit)
	pylab.xlabel('diameter [mm]')
	pylab.ylabel('cumulative flow [Mt/y]')
	figs.append(('Cumulative flow',fig))

	#ax=pylab.subplot(222)
	fig=pylab.figure()
	dOver,mOver=scaledFlowPsd(*woo.fallOver.diamMass())
	dAper,mAper=numpy.array([]),numpy.array([])
	for a in woo.aperture:
		try:
			d,m=scaledFlowPsd(*a.diamMass())
			dAper=numpy.hstack([dAper,d]); mAper=numpy.hstack([mAper,m])
		except ValueError: pass
	hh=pylab.hist([dOver,dAper],weights=[mOver,mAper],bins=20,histtype='barstacked',label=['fallOver','apertures'])
	flowBins=hh[1]
	feedD,feedM=scaledFlowPsd(*woo.factory.generator.diamMass())
	pylab.plot(.5*(flowBins[1:]+flowBins[:-1]),numpy.histogram(feedD,weights=feedM,bins=flowBins)[0],label='feed',alpha=.3,linewidth=2,marker='o')
	pylab.axvline(x=pre.gap,linewidth=5,alpha=.3,ymin=0,ymax=1,color='g',label='gap')
	pylab.grid(True)
	pylab.gca().xaxis.set_major_formatter(milimeter)
	pylab.gca().yaxis.set_major_formatter(megaUnit)
	pylab.xlabel('particle diameter [mm]')
	pylab.ylabel('flow [Mt/y]')
	leg=pylab.legend(loc='best')
	leg.get_frame().set_alpha(legendAlpha)
	figs.append(('Flow',fig))


	feedTab=psdFeedApertureFalloverTable(
		inPsd=inPsdUnscaled,
		feedDM=unscaledPsd(*woo.factory.generator.diamMass()),
		apDM=(dAper,mAper),
		overDM=(dOver,mOver),
		splitD=pre.gap
	)

	effTab,effFig=efficiencyTableFigure(S,pre)
	figs.append(('Sieving efficiency',effFig))

	#ax=pylab.subplot(223)
	fig=pylab.figure()
	#print inPsdUnscaled
	#print inPsd
	pylab.plot(*inPsdUnscaled,marker='o',label='user')
	feedPsd=unscaledPsd(*woo.factory.generator.psd(normalize=True))
	pylab.plot(*feedPsd,label=None)
	smallerPsd,biggerPsd=splitPsd(*inPsdUnscaled,splitX=pre.gap)
	#print smallerPsd
	#print biggerPsd
	pylab.plot(*smallerPsd,marker='v',label='user <')
	pylab.plot(*biggerPsd,marker='^',label='user >')

	splitD=pre.gap
	feedD,feedM=unscaledPsd(*woo.factory.generator.diamMass())
	feedHist,feedBins=numpy.histogram(feedD,weights=feedM,bins=60)
	feedHist/=feedHist.sum()
	binMids=feedBins[1:] #.5*(feedBins[1:]+feedBins[:-1])
	feedSmallHist,b=numpy.histogram(feedD[feedD<splitD],weights=feedM[feedD<splitD],bins=feedBins,density=True)
	feedSmallHist/=feedSmallHist.sum()
	feedBigHist,b=numpy.histogram(feedD[feedD>=splitD],weights=feedM[feedD>=splitD],bins=feedBins,density=True)
	feedBigHist/=feedBigHist.sum()
	pylab.plot(binMids,feedHist.cumsum(),label=None)
	pylab.plot(binMids,feedSmallHist.cumsum(),label=None) #'feed <')
	pylab.plot(binMids,feedBigHist.cumsum(),label=None) #'feed >')
	feedD=feedPsd[0]

	pylab.axvline(x=pre.gap,linewidth=5,alpha=.3,ymin=0,ymax=1,color='g',label='gap')
	if 0:
		n,bins,patches=pylab.hist(dAper,weights=mAper/sum([ap.mass for ap in woo.aperture]),histtype='step',label='apertures',normed=True,bins=60,cumulative=True,linewidth=2)
		patches[0].set_xy(patches[0].get_xy()[:-1])
		n,bins,patches=pylab.hist(dOver,weights=mOver,normed=True,bins=60,cumulative=True,histtype='step',label='fallOver',linewidth=2)
		patches[0].set_xy(patches[0].get_xy()[:-1])
	else:
		apHist,apBins=numpy.histogram(dAper,weights=mAper,bins=feedBins)
		apHist/=apHist.sum()
		overHist,overBins=numpy.histogram(dOver,weights=mOver,bins=feedBins)
		overHist/=overHist.sum()
		pylab.plot(binMids,apHist.cumsum(),label='apertures',linewidth=3)
		pylab.plot(binMids,overHist.cumsum(),label='fall over',linewidth=3)
	#for a in woo.aperture:
	#	print a.mass,sum([dm[1] for dm in a.diamMass()]),'|',len(a.deleted),a.num
	pylab.ylim(ymin=-.05,ymax=1.05)

	#print 'smaller',smallerPsd
	#print 'bigger',biggerPsd
	pylab.grid(True)
	pylab.gca().xaxis.set_major_formatter(milimeter)
	pylab.xlabel('diameter [mm]')
	pylab.gca().yaxis.set_major_formatter(percent)
	pylab.ylabel('mass fraction')
	leg=pylab.legend(loc='best')
	leg.get_frame().set_alpha(legendAlpha)
	figs.append(('Efficiency',fig))

	try:
		#ax=pylab.subplot(224)
		fig=pylab.figure()
		import woo.plot
		d=woo.plot.data
		d['genRate'][-1]=d['genRate'][-2] # replace trailing 0 by the last-but-one value
		pylab.plot(d['t'],massScale*numpy.array(d['genRate']),label='feed')
		pylab.plot(d['t'],massScale*numpy.array(d['apRate']),label='apertures')
		pylab.plot(d['t'],massScale*numpy.array(d['lostRate']),label='(lost)')
		pylab.plot(d['t'],massScale*numpy.array(d['overRate']),label='fallOver')
		pylab.plot(d['t'],massScale*numpy.array(d['delRate']),label='delete')
		pylab.ylim(ymin=0)
		#pylab.axvline(x=(S.time-pre.time),linewidth=5,alpha=.3,ymin=0,ymax=1,color='r',label='steady')
		pylab.axvspan(S.time-pre.time,S.time,alpha=.2,facecolor='r',label='steady')
		leg=pylab.legend(loc='lower left')
		leg.get_frame().set_alpha(legendAlpha)
		pylab.grid(True)
		pylab.xlabel('time [s]')
		pylab.ylabel('mass rate [t/y]')
		figs.append(('Regime',fig))
	except KeyError as e:
		print 'No woo.plot plots done due to lack of data:',str(e)
		# after loading no data are recovered

	svgs=[]
	for name,fig in figs:
		svgs.append((name,woo.O.tmpFilename()+'.svg'))
		fig.savefig(svgs[-1][-1])
		

	import time
	xmlhead='''<?xml version="1.0" encoding="UTF-8"?>
	<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd"> 
	<html xmlns="http://www.w3.org/1999/xhtml" xmlns:svg="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink">
	'''
	# <meta http-equiv="content-type" content="text/html;charset=UTF-8" />
	html=('''<head><title>Report for Roller screen simulation</title></head><body>
		<h1>Report for Roller screen simulation</h1>
		<h2>General</h2>
		<table>
			<tr><td>title</td><td align="right">{title}</td></tr>
			<tr><td>id</td><td align="right">{id}</td></tr>
			<tr><td>operator</td><td align="right">{user}</td></tr>
			<tr><td>started</td><td align="right">{started}</td></tr>
			<tr><td>duration</td><td align="right">{duration:g} s</td></tr>
			<tr><td>number of cores used</td><td align="right">{nCores}</td></tr>
			<tr><td>average speed</td><td align="right">{stepsPerSec:g} steps/sec</td></tr>
			<tr><td>engine</td><td align="right">{engine}</td></tr>
			<tr><td>compiled with</td><td align="right">{compiledWith}</td></tr>
		</table>
		'''.format(title=(S.tags['title'] if S.tags['title'] else '<i>[none]</i>'),id=S.tags['id'],user=S.tags['user'].decode('utf-8'),started=time.ctime(time.time()-woo.O.realtime),duration=woo.O.realtime,nCores=woo.O.numThreads,stepsPerSec=S.step/woo.O.realtime,engine='wooDem '+woo.config.version+'/'+woo.config.revision+(' (debug)' if woo.config.debug else ''),compiledWith=','.join(woo.config.features))
		+'<h2>Input data</h2>'+pre.dumps(format='html',fragment=True,showDoc=True)
		+'<h2>Outputs</h2>'
		+'<h3>Feed</h3>'+feedTab
		+'<h3>Sieving</h3>'+effTab
		+'\n'.join(['<h3>'+svg[0]+'</h3>'+svgFragment(open(svg[1]).read()) for svg in svgs])
		+'</body></html>'
	)
	if 0: # pure genshi version; probably does not work
		html=(tag.head(tag.title('Report for Roller screen simulation')),tag.body(
			tag.h1('Report for Roller screen simulation'),
			tag.h2('General'),
			tag.table(
				tag.tr(tag.td('started'),tag.td('%g s'%(time.ctime(time.time()-woo.O.realtime)),align='right')),
				tag.tr(tag.td('duration'),tag.td('%g s'%(woo.O.realtime),align='right')),
				tag.tr(tag.td('number of cores'),tag.td(str(woo.O.numThreads),align='right')),
				tag.tr(tag.td('average speed'),tag.td('%g steps/sec'%(S.step/woo.O.realtime))),
				tag.tr(tag.td('engine'),tag.td('wooDem '+woo.config.version+'/'+woo.config.revision+(' (debug)' if woo.config.debug else ''))),
				tag.tr(tag.td('compiled with'),tag.td(', '.join(woo.config.features))),
			),
			tag.h2('Input data'),
			pre.dumps(format='genshi'),
			tag.h2('Tables'),
			tag.h3('Feed'),
			feedTab,
			tag.h3('Sieving'),
			effTab,
			tag.h2('Graphs'),
			*tuple([XMLParser(StringIO.StringIO(svgFragment(open(svg).read()))).parse() for svg in svgs])
		)).render('xhtml')

	# to play with that afterwards
	woo.html=html
	#from genshi.input import HTMLParser
	#import codecs, StringIO
	import codecs
	repName=S.pre.reportFmt.format(S=S,**(dict(S.tags)))
	rep=codecs.open(repName,'w','utf-8','replace')
	import os.path
	print 'Writing report to file://'+os.path.abspath(repName)
	#rep.write(HTMLParser(StringIO.StringIO(html),'[filename]').parse().render('xhtml',doctype='xhtml').decode('utf-8'))
	s=xmlhead+html
	#s=s.replace('\xe2\x88\x92','-')
	try:
		rep.write(s)
	except UnicodeDecodeError as e:
		print e.start,e.end
		print s[max(0,e.start-20):min(e.end+20,len(s))]
		raise e
		
		

	# save sphere's positions
	from woo import pack
	sp=pack.SpherePack()
	sp.fromSimulation(S)
	packName=S.tags['id.d']+'-spheres.csv'
	sp.save(packName)
	print 'Particles saved to',os.path.abspath(packName)

# test drive
if __name__=='__main__':
	import woo.pre
	import woo.qt
	import woo.log
	#woo.log.setLevel('PsdParticleGenerator',woo.log.TRACE)
	woo.master.scene=S=woo.pre.Roro()()
	S.timingEnabled=True
	S.saveTmp()
	woo.qt.View()
	S.run()

