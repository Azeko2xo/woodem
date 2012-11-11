#!/usr/bin/python
# -*- coding: utf-8 -*-
# © 2009 Václav Šmilauer <eudoxos@arcig.cz>
"""Script showing how to use GTS to import arbitrary triangulated surface,
which can further be either filled with packing (if used as predicate) or converted
to facets representing the surface."""

if 0:
	import sys
	sys.path.append('/usr/local/lib/woo')
	import wooOptions
	wooOptions.ompThreads=4
	wooOptions.debug=True
	wooOptions.forceNoGui=True

from woo import pack,utils
from woo.core import *
from woo.dem import *
try: import woo.gl
except: pass
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

S=woo.master.scene
if surf.is_closed():
	pred=pack.inGtsSurface(surf)
	aabb=pred.aabb()
	dim0=aabb[1][0]-aabb[0][0]; radius=dim0/40. # get some characteristic dimension, use it for radius
	S.dem.par.append(pack.regularHexa(pred,radius=radius,gap=radius/4.))
	surf.translate(0,0,-(aabb[1][2]-aabb[0][2])) # move surface down so that facets are underneath the falling spheres
S.dem.par.append(pack.gtsSurface2Facets(surf,wire=True))

S.engines=[
	ForceResetter(),
	InsertionSortCollider([Bo1_Sphere_Aabb(),Bo1_Facet_Aabb()],label='collider'),
	ContactLoop(
		[Cg2_Sphere_Sphere_L6Geom(),Cg2_Facet_Sphere_L6Geom()],
		[Cp2_FrictMat_FrictPhys()],
		[Law2_L6Geom_FrictPhys_IdealElPl()],
	),
	Leapfrog(damping=.1),
	PyRunner(2000,'import woo.timing; woo.timing.stats(); S.stop();'),
	PyRunner(10,'S.plot.addData(i=S.step,total=S.energy.total(),**S.energy)'),
	PyRunner(100,'print dict(S.energy)')
]
S.dem.gravity=(0,0,-5000)
S.dt=.7*utils.pWaveDt()
S.saveTmp()
woo.master.timingEnabled=True
S.trackEnergy=True
from woo import plot
S.plot.plots={'i':('total',S.energy.keys,)}
S.plot.plot()

from woo import timing
try:
	from woo import qt
	qt.View()
except ImportError: pass
