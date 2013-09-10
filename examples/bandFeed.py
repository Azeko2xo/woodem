from __future__ import division
import woo.utils, woo.dem, woo.core, woo.pack, woo.plot
import numpy

mat=woo.utils.defaultMaterial()
mat.density=3800

S=woo.master.scene=woo.pack.makeBandFeedPack(dim=(.2,.4,.2),
	psd=[(0.0063, 0.0263),(0.008, 0.0993),(0.009, 0.1958),(0.0095, 27.55),(0.0112, 58.65),(0.0125, 76.87),(0.014, 88.09),(0.016, 96.37),(0.018, 100.0)],
	#psd=((.02,.0),(.03,.2),(.04,.3),(.05,.7)),
	mat=mat,gravity=(0,0,-10),porosity=.5,dontBlock=True
)
S.saveTmp()
#for i in range(0,100):
#	print i
#	S.run(2000,True)
#	

