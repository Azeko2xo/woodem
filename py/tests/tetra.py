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
from woo.dem import *

class TestTet4(unittest.TestCase):
	'Test :obj:`woo.fem.Tet4`.'
	def testStiffnessMatrix(self):
		'Tet4: stiffness matrix'
		# check values against:
		# Felippa, Linear Tetrahedron: Fig. 9.10 and 9.11
		nodes=[woo.core.Node(pos=p) for p in [(2,3,4),(6,3,2),(2,5,1),(4,3,6)]]
		t=woo.fem.Tet4.make(nodes,mat=ElastMat(young=480))
		t.shape.ensureStiffnessMatrix(young=480,nu=1/3.)
		K=t.shape.KK
		Kok=MatrixX([
			(745, 540, 120, -5, 30, 60, -270, -240, 0, -470, -330, -180),
			(540, 1720, 270, -120, 520, 210, -120, -1080, -60, -300, -1160, -420),
			(120, 270, 565, 0, 150, 175, 0, -120, -270, -120, -300, -470),
			(-5, -120, 0, 145, -90, -60, -90, 120, 0, -50, 90, 60),
			(30, 520, 150, -90, 220, 90, 60, -360, -60, 0, -380, -180),
			(60, 210, 175, -60, 90, 145, 0, -120, -90, 0, -180, -230),
			(-270, -120, 0, -90, 60, 0, 180, 0, 0, 180, 60, 0),
			(-240, -1080, -120, 120, -360, -120, 0, 720, 0, 120, 720, 240),
			(0, -60, -270, 0, -60, -90, 0, 0, 180, 0, 120, 180),
			(-470, -300, -120, -50, 0, 0, 180, 120, 0, 340, 180, 120),
			(-330, -1160, -300, 90, -380, -180, 60, 720, 120, 180, 820, 360),
			(-180, -420, -470, 60, -180, -230, 0, 240, 180, 120, 360, 520)
		])
		self.assert_( (K-Kok).norm()<1e-11 )
		self.assert_( (K-K.transpose()).norm()<1e-11 )
