from woo.core import *
from woo.dem import *
import woo, math
from minieigen import *
# use the same material for both capsules and boundaries, for simplicity
capsMat=wallMat=woo.dem.FrictMat(ktDivKn=.2,tanPhi=.2,density=1000,young=1e7)
# create the scene object, set the master scene
S=woo.master.scene=Scene(fields=[DemField(gravity=(0,0,-10))])
# add the bottom plane (wall)
S.dem.par.add(Wall.make(0,axis=2,sense=1,mat=wallMat,color=0,glAB=((-.2,-.2),(.2,.2))))
# create bottle mesh from the STL
bottle=woo.utils.importSTL('pill-bottle.coarse2.stl',mat=wallMat,scale=0.001,shift=(.056,.027,-0.01),ori=Quaternion((1,0,0),math.pi/2.),color=-.25)
# create node which will serve as "handle" to move the bottle
S.lab.botNode=Node(pos=(0,0,.04)) 
# add bottle as clump;
# center is the centroid normally, but the mesh has no mass, thus reference point must be given
S.dem.par.addClumped(bottle,centralNode=S.lab.botNode)

S.dtSafety=.5

import woo.gl
woo.gl.Renderer.engines=False
woo.gl.Renderer.allowFast=False

# particle factory, with many parameters
# when the factory finishes, it will call the pillsDone function, defined below
factory=CylinderFactory(stepPeriod=100,node=Node(pos=(0,0,.17),ori=Quaternion((0,1,0),math.pi/2.)),radius=.018,height=.05,generator=PharmaCapsuleGenerator(),materials=[capsMat],massRate=0,maxMass=.12,label='feed',attemptPar=100,color=-1,doneHook='pillsDone(S)')

# add factory (and optional Paraview export) to engines
S.engines=DemField.minimalEngines(damping=.3)+[
	factory,
	## comment out to enable export for Paraview:
	VtkExport(out='/tmp/bottle.',ascii=True,stepPeriod=500) 
]
# save the scene
S.saveTmp()

def pillsDone(S):
	# once the bottle is full, wait for another 0.2s
	#then start moving the bottle by interpolating prescribed positions and orientations
	S.lab.botNode.dem.impose=InterpolatedMotion(t0=S.time+0.2,poss=[S.lab.botNode.pos,(0,.05,.08),(0,.05,.09),(0,.04,.13)],oris=[Quaternion.Identity,((1,0,0),.666*math.pi),((1,0,0),.85*math.pi),((1,0,0),.9*math.pi)],times=[0,.3,.5,1.6])
