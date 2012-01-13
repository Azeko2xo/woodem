#!/usr/bin/python
# -*- coding: utf-8 -*-
# © 2009 Václav Šmilauer <eudoxos@arcig.cz>
"""Script showing how to use GTS to import arbitrary triangulated surface,
which can further be either filled with packing (if used as predicate) or converted
to facets representing the surface."""

from yade import pack,utils
from yade.core import *
from yade.dem import *
import gts, os.path

# coarsen the original horse if we have it
# do nothing if we have the coarsened horse already
if not os.path.exists('horse.coarse.gts'):
	if os.path.exists('horse.gts'):
		surf=gts.read(open('horse.gts')); surf.coarsen(1000); surf.write(open('horse.coarse.gts','w'))
	else:
		print """horse.gts not found, you need to download input data:

		wget http://gts.sourceforge.net/samples/horse.gts.gz
		gunzip horse.gts.gz
		"""
		quit()

surf=gts.read(open('horse.coarse.gts'))

if surf.is_closed():
	pred=pack.inGtsSurface(surf)
	aabb=pred.aabb()
	dim0=aabb[1][0]-aabb[0][0]; radius=dim0/40. # get some characteristic dimension, use it for radius
	O.dem.par.append(pack.regularHexa(pred,radius=radius,gap=radius/4.))
	surf.translate(0,0,-(aabb[1][2]-aabb[0][2])) # move surface down so that facets are underneath the falling spheres
O.dem.par.append(pack.gtsSurface2Facets(surf,wire=True))

O.scene.engines=[
	ForceResetter(),
	InsertionSortCollider([Bo1_Sphere_Aabb(),Bo1_Facet_Aabb()],label='collider'),
	ContactLoop(
		[Cg2_Sphere_Sphere_L6Geom(),Cg2_Facet_Sphere_L6Geom()],
		[Cp2_FrictMat_FrictPhys()],
		[Law2_L6Geom_FrictPhys_IdealElPl()],
	),
	IntraForce([In2_Sphere_ElastMat()]),
	Gravity(gravity=(0,0,-5000)),
	Leapfrog(damping=.1),
	PyRunner(2000,'timing.stats(); O.pause();'),
	PyRunner(10,'addPlotData()')
]
O.scene.dt=.7*utils.pWaveDt()
O.saveTmp()
O.timingEnabled=True
O.scene.trackEnergy=True
from yade import plot
plot.plots={'i':('total',O.scene.energy.keys,)}
def addPlotData(): plot.addData(i=O.scene.step,total=O.scene.energy.total(),**O.scene.energy)
plot.plot()

from yade import timing
from yade import qt
qt.View()
