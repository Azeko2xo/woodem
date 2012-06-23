#!/usr/local/bin/woo-trunk -x
# -*- coding: utf-8 -*-

o=Omega() 
o.engines=[
	ForceResetter(),
	InsertionSortCollider([Bo1_Sphere_Aabb(),]),
	IGeomDispatcher([Ig2_Sphere_Sphere_Dem3DofGeom()]),
	IPhysDispatcher([Ip2_2xFrictMat_CSPhys()]),
	LawDispatcher([Law2_Dem3Dof_CSPhys_CundallStrack()]),
	GravityEngine(gravity=[0,0,-9.81]),
	NewtonIntegrator(damping = 0.01)
]

from woo import utils

o.bodies.append(utils.sphere([0,0,6],1,dynamic=True, color=[0,1,0]))
o.bodies.append(utils.sphere([0,0,0],1,dynamic = False, color=[0,0,1]))
o.dt=.2*utils.PWaveTimeStep()

from woo import qt
qt.Controller()
qt.View()
