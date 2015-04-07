from woo.dem import *
from woo.core import *
from minieigen import *
import woo
import math
r1=r2=r3=1e-2 # sphere radii
eps=1e-5 # initial overlap, must be positive
q=Quaternion.Identity # rotate the local axis (not used currently)
# our 3 spheres, the middle one will move
mat=IceMat() # use IceMat with everything default
s1=Sphere.make(q*((-r2-r1)+eps,0,0),radius=r1,mat=mat,fixed=True)
s2=Sphere.make(q*(0,0,0),radius=r2,mat=mat,fixed=True)
s3=Sphere.make(q*(r2+r3-eps,0,0),radius=r3,mat=mat,fixed=True)
# create the scene objects
S=woo.master.scene=woo.core.Scene(
	# put the 3 spheres in here
	fields=[DemField(par=[s1,s2,s3])],
	# not really needed, but perhaps one day...
	trackEnergy=True,
	# usual engines plus collecting data for plotting
	engines=DemField.minimalEngines(model=woo.models.ContactModelSelector(name='ice'))+[
		PyRunner(1,'S.plot.addData(i=S.step,z2=S.lab.s2.pos[2],F2=S.lab.s2.f[2],Fx01=(S.dem.con[0,1].phys.force[0] if S.dem.con.existsReal(0,1) else float("nan")),Fx12=(S.dem.con[1,2].phys.force[0] if S.dem.con.existsReal(1,2) else float("nan")))')
	]
)
# minimalEngines with ContactModeSelector creates the right functors
# the Cp2 functor is accessible as S.lab.cp2
# this sets all bonds to initially bonded and unbreakable
S.lab.cp2.bonds0=15 
# shorthands for particles, to be used inside addData where script variables are not in the scope
S.lab.s1,S.lab.s2,S.lab.s3=s1,s2,s3
# collect nodes which will be motion-integrated
S.dem.collectNodes()
# set velocity of the middle sphere
s2.vel=q*(0,0,1e-2)
# limit the simulation velocity (delay inserted after each step)
S.throttle=1e-4
# plots
S.plot.plots={'i':('z2'),'z2':('F2',None,'Fx01','Fx12')}
S.saveTmp()
# check if the contact is created right:
S.one()
print 'Bond: ',S.dem.con[0,1].phys.bonds

