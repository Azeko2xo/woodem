#!/usr/bin/python
# -*- coding: utf-8 -*-
"""This example demonstrates GTS (http://gts.sourceforge.net/) opportunities for creating surfaces
VTU-files are created in /tmp directory after simulation. If you open those with paraview
(or other VTK-based) program, you can create video, make screenshots etc."""
import woo
from woo import *
from woo.dem import *
from woo.core import *
from numpy import linspace
from woo import pack,qt,utils
from minieigen import *
from math import *

S=woo.master.scene=Scene(fields=[DemField(gravity=(0,0,-9.81))])

thetas=linspace(0,2*pi,num=16,endpoint=True)
meridians=pack.revolutionSurfaceMeridians([[Vector2(3+rad*sin(th),10*rad+rad*cos(th)) for th in thetas] for rad in linspace(1,2,num=10)],linspace(0,pi,num=10))
surf=pack.sweptPolylines2gtsSurface(meridians+[[Vector3(5*sin(-th),-10+5*cos(-th),30) for th in thetas]])
S.dem.par.add(pack.gtsSurface2Facets(surf))

sp=pack.SpherePack()
sp.makeCloud(Vector3(-1,-9,30),Vector3(1,-13,32),.2,rRelFuzz=.3)
S.dem.par.add([utils.sphere(c,r) for c,r in sp])

S.engines=utils.defaultEngines()+[VtkExport(stepPeriod=100,what=VtkExport.spheres|VtkExport.mesh,out='/tmp/p1-')]
S.dt=utils.pWaveDt(S)

qt.Controller()
qt.View()
S.saveTmp()
S.run(8500)
