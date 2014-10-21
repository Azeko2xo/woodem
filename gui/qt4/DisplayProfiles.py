import woo.dem
import woo.gl as gl
from woo.gl import Gl1_DemField
import collections

def predefPublishing():
	predefHighQuality()
	gl.Renderer(bgColor=(81/255.,87/255.,110/255.),showTime=5,fast='never')

def predefHighQuality():
	gl.Gl1_Sphere(quality=3.,wire=False)
	gl.Gl1_Facet(slices=16,wire=False)

def predefLowQuality():
	gl.Gl1_Sphere(quality=0)
	gl.Gl1_Facet(slices=-1,wire=True)

def predefNonspheresSpheres():
	Gl1_DemField(
		shape=Gl1_DemField.shapeNonSpheres,
		colorBy=Gl1_DemField.colorMatState,
		matStateIx=0,
		shape2=True,
		colorBy2=Gl1_DemField.colorVel,
	)

def predefSpheresNonspheres():
	Gl1_DemField(
		shape=Gl1_DemField.shapeSpheres,
		colorBy=Gl1_DemField.colorVel,
		shape2=True,
		colorBy2=Gl1_DemField.colorSolid
	)

predefinedProfiles=collections.OrderedDict([
	('High Quality',predefHighQuality),
	('Low Quality',predefLowQuality),
	('Publishing',predefPublishing),
	('Nonspheres-spheres',predefNonspheresSpheres),
	('Spheres-nonspheres',predefSpheresNonspheres),
])
