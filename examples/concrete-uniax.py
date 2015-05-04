import woo, woo.batch, math
from woo.dem import *
from woo.core import *
from woo.gl import *
from minieigen import *


S=woo.master.scene=Scene(fields=[DemField()],dtSafety=.9)

woo.batch.readParamsFromTable(S,noTableOk=True,
	young=24e9,
	ktDivKn=0.,

	sigmaT=3.5e6,
	tanPhi=0.8,
	epsCrackOnset=1e-4,
	relDuctility=30,

	distFactor=1.5,
	# dtSafety=.8,
	damping=0.4,
	#strainRateTension=.05,
	strainRateCompression=1e-2,
	#setSpeeds=True,
	# 1=tension, 2=compression (ANDed; 3=both)
	#doModes=3,

	specimenLength=.15,
	sphereRadius=2e-3,

	# isotropic confinement (should be negative)
	isoPrestress=0,
)

mat=ConcreteMat(young=S.lab.table.young,tanPhi=S.lab.table.tanPhi,ktDivKn=S.lab.table.ktDivKn,density=4800,sigmaT=S.lab.table.sigmaT,relDuctility=S.lab.table.relDuctility,epsCrackOnset=S.lab.table.epsCrackOnset,isoPrestress=S.lab.table.isoPrestress)

# sps=woo.pack.SpherePack()
S.lab.minRad=.17*S.lab.table.specimenLength
sp=woo.pack.randomDensePack(
	woo.pack.inHyperboloid((0,0,-.5*S.lab.table.specimenLength),(0,0,.5*S.lab.table.specimenLength),.25*S.lab.table.specimenLength,S.lab.minRad),
	spheresInCell=2000,
	radius=S.lab.table.sphereRadius,
	memoizeDb='/tmp/triaxPackCache.sqlite'
)
sp.cellSize=(0,0,0) # make aperiodic

sp.toSimulation(S,mat=mat)
S.dem.collectNodes()
S.engines=DemField.minimalEngines(model=woo.models.ContactModelSelector(name='concrete',mats=[mat],distFactor=S.lab.table.distFactor,damping=S.lab.table.damping))+[PyRunner(100,'addPlotData(S)')]

# take boundary nodes an prescribe velocity to those
lc=.5*S.lab.table.specimenLength-3*S.lab.table.sphereRadius
# impositions for top/bottom
S.lab.topImpose,S.lab.botImpose=VelocityAndReadForce(dir=Vector3.UnitZ),VelocityAndReadForce(dir=-Vector3.UnitZ)
for n in S.dem.nodes:
	if abs(n.pos[2])>lc:
		n.dem.impose=(S.lab.topImpose if n.pos[2]>0 else S.lab.botImpose)

def addPlotData(S):
	u=S.lab.topImpose.dist+S.lab.botImpose.dist
	eps=u/S.lab.table.specimenLength
	sig=-.5*(S.lab.topImpose.sumF+S.lab.botImpose.sumF)/(math.pi*S.lab.minRad**2)
	S.plot.addData(eps=eps,sig=sig,u=u,Ftop=-S.lab.topImpose.sumF,Fbot=-S.lab.botImpose.sumF)
	stage=(0 if S.lab.topImpose.vel>0 else 1)
	if S.step>1000 and abs(-S.lab.topImpose.sumF)<.5*abs(max(S.plot.data['Ftop']) if stage==0 else min(S.plot.data['Ftop'])):
		if stage==0:
			S.stop()
			p=woo.master.scene.plot
			p.splitData()
			S=woo.master.reload() # hope this will work and not freeze
			S.plot=p
			start(S,-1)
			S.run()
		else: S.stop()

# create initial contacts
S.one()
# reset distFactor
for f in S.lab.collider.boundDispatcher.functors+S.lab.contactLoop.geoDisp.functors:
	if hasattr(f,'distFactor'): f.distFactor=1.
# this is the initial state
rr=woo.gl.Renderer(dispScale=(10000,10000,10000))
woo.gl.Gl1_DemField.colorBy='ref. displacement'
woo.gl.Gl1_DemField.vecAxis='xy'
S.plot.plots={'eps':('sig',),'u':('Ftop','Fbot')}

S.saveTmp('init')

def start(S,sign=+1):
	# impose velocity on bottom/top particles
	v=S.lab.table.strainRateCompression*S.lab.table.specimenLength
	print sign*v
	S.lab.topImpose.vel=S.lab.botImpose.vel=sign*v
start(S,+1)
S.run()
