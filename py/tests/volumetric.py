# encoding: utf-8
# 2013 © Václav Šmilauer <eu@doxos.eu>

import unittest
from minieigen import *
import woo._customConverters
import woo.core
import woo.dem
import woo.fem
import woo.utils
import math
from math import sqrt
from woo.dem import *

class TestVolumetric(unittest.TestCase):
	'Volumetric properties'
	def testTetraCanon(self):
		'Tetra: mass, centrer, inertia of canonical tetrahedron'
		# analytical result:
		# covariance matrix of canonical tetrahedron -- see 
		# http://pyffi.sourceforge.net/apidocs/pyffi.utils.inertia-pysrc.html
		# http://number-none.com/blow/inertia/bb_inertia.doc
		Ca=(1/120.)*Matrix3(2,1,1, 1,2,1, 1,1,2)
		Ia=Matrix3.Identity*Ca.trace()-Ca
		# numerical result:
		A,B,C,D=[Vector3(0,0,0),Vector3(1,0,0),Vector3(0,1,0),Vector3(0,0,1)]
		In=woo.comp.tetraInertia(A,B,C,D)
		for ix in [(0,0),(0,1),(0,2),(1,0),(1,1),(1,2),(2,0),(2,1),(2,2)]:
			self.assertAlmostEqual(Ia[ix],In[ix],delta=1e-9)
 		# volume
		Va=(A-D).dot((B-D).cross(C-D))/6.
		Vn=woo.comp.tetraVolume(A,B,C,D)
		self.assertAlmostEqual(Va,Vn,delta=1e-9)
	#def testTetraGeneric(self):
	#	'Tetra: mass, center, inertia of generic tetrahedron'
	def testTriCanon(self):
		'Triangle: area, inertia of canonical triangle'
		# http://en.wikipedia.org/wiki/Inertia_tensor_of_triangle#Covariance_of_a_canonical_triangle
		# inertia
		Ca=(1/24.)*Matrix3(2,1,0, 1,2,0, 0,0,0)
		Ia=Matrix3.Identity*Ca.trace()-Ca
		A,B,C=(0,0,0),(1,0,0),(0,1,0)
		In=woo.comp.triangleInertia(A,B,C)
		for ix in [(0,0),(0,1),(0,2),(1,0),(1,1),(1,2),(2,0),(2,1),(2,2)]:
			self.assertAlmostEqual(Ia[ix],In[ix],delta=1e-9)
		# area
		Aa=.5
		An=woo.comp.triangleArea(A,B,C)
		self.assertAlmostEqual(Aa,An,delta=1e-9)
	def testTetraUnit(self):
		'Tetra: principal axes of unit tetrahedron'
		vv=[Vector3(1,0,-1/sqrt(2)),Vector3(-1,0,-1/sqrt(2)),Vector3(0,1,1/sqrt(2)),Vector3(0,-1,1/sqrt(2))]
		a=2
		V=a**3/(6*(sqrt(2)))
		Ia=V*a**2/20 # analytical moment of inertia for all axes
		print
		for q in [Quaternion.Identity,Quaternion((0,1,0),1),Quaternion((.5,.5,0),3)]:
			q.normalize()
			vvR=[q*v for v in vv] # add some rotation
			#print vvR
			print 'vertices',vvR
			print 'rotation',q
			print 'volume',woo.comp.tetraVolume(*vvR)
			print 'inertia',woo.comp.tetraInertia_cov(*vvR)
			p,o,ii=woo.comp.computePrincipalAxes(woo.comp.tetraVolume(*vvR),Vector3.Zero,woo.comp.tetraInertia_cov(*vvR))
			for i in 0,1,2: self.assertAlmostEqual(ii[i],Ia,delta=1e-6)
			print q,p,o,ii
			## FIXME: always gives unit orientation?!
			# check that we yield back the original, aligned with global axes
			for i in 0,1,2:
				# should yield (some) global axis
				ax=o.conjugate()*q*Vector3.Unit(i)
				e=Vector3.Unit(i)
				print 'ax',ax
				print 'e',e
				#for j in 0,1,2:
					# self.assertAlmostEqual(ax[i],e[i],delta=1e-9)
	def testTetraGeneric(self):
		'Tetra: inertia and principal axes of generic tetrahedron'
		# example from
		# https://books.google.com/books?id=2b8-79CuIkAC&pg=PA185
		vvG=Vector3(0,0,0),Vector3(.2,0,0),Vector3(0,.4,0),Vector3(0,0,.6)
		vvC=[vG-.25*(sum(vvG,Vector3.Zero)) for vG in vvG]
		dens=60./abs(woo.comp.tetraVolume(*vvC))
		# inertia tensor
		In=dens*woo.comp.tetraInertia_cov(*vvC)
		# book value
		Ib=Matrix3(1.17,.06,.09, .06,.9,.18, .09,.18,.45)
		for i in (0,1,2):
			for j in (0,1,2):
				self.assertAlmostEqual(In[i,j],Ib[i,j],delta=1e-9)
		# principal axes
		pn,on,ii=woo.comp.computePrincipalAxes(woo.comp.tetraVolume(*vvC),Vector3.Zero,In)
		# book values: principal inertia
		ib=(1.20592,.93268,.38140) 
		for i in 0,1,2: self.assertAlmostEqual(ib[i],ii[2-i],delta=1e-4) # book orders decreasing, we order increasing
		# book values: principal axes (limited precision)
		eeb=[Vector3(.9392,.2908,.1811),Vector3(-.3322,.9023,.2746),Vector3(-.0836,-.3181,.9444)]
		# comparison (axes may be arbitrarily ordered and reverse-oriented)
		# check dot-product of basis vectors with each other:
		#   must be perpendicular or parallel, therefore all elements must be (approx) -1,0,1 
		mm=Matrix3(*eeb)*Matrix3(on*Vector3.UnitX,on*Vector3.UnitY,on*Vector3.UnitZ).transpose()
		for i in 0,1,2:
			for j in 0,1,2:
				if abs(mm[i,j])<.5: self.assertAlmostEqual(mm[i,j],0,delta=1e-3)
				else: self.assertAlmostEqual(abs(mm[i,j]),1,delta=1e-3)


	@unittest.expectedFailure
	def testTetraGenericTonon(self):
		'Tetra: inertia of generic tetrahedron (FIXME: deviatoric components inverted, investigation ongoing)'
		# example from http://www.scipub.org/fulltext/jms2/jms2118-11.pdf (Tonon 2005)
		# centroid
		vv=[Vector3(8.33220,-11.86875,0.93355),Vector3(0.75523,5.,16.37072),Vector3(52.61236,5.,-5.38580),Vector3(2.,5.,3.)]
		Ca=(15.92492, 0.78281, 3.72962)
		Cn=.25*sum(vv,Vector3.Zero)
		#print Cn,Ca
		for i in 0,1,2: self.assertAlmostEqual(Ca[i],Cn[i],delta=1e-3) #article's precision rather limited
		# inertia; article gives diagonal + symmetric non-diagonal elements
		Iad=Vector3(43520.33257,194711.28938,191168.76173);
		Iao=Vector3(4417.66150,-46343.16662,11996.20119) 
		## Iao*=-1 ## FIXME: is it possible that the article has all deviatoric parts inversed??!
		Ia=Matrix3(Iad); Ia[0,1]=Ia[1,0]=Iao[1]; Ia[0,2]=Ia[2,0]=Iao[2]; Ia[1,2]=Ia[2,1]=Iao[0]
		vvC=[v-Cn for v in vv]
		In=woo.comp.tetraInertia(*vvC)
		#print Ia
		#print In
		for i in 0,1,2:
			for j in 0,1,2:
				self.assertAlmostEqual(Ia[i,j],In[i,j],delta=1e-2)
		#print Ia,In
