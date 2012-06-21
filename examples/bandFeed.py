from __future__ import division
from woo import *
from miniEigen import *
from woo.dem import *
from woo.core import *
from woo import utils,pack,plot
import numpy
import woo.pre.Roro_

yade.pre.Roro_.makeBandFeedPack(dim=(.5,.4,.2),psd=((.02,.0),(.03,.2),(.04,.3),(.05,.7)),material=utils.defaultMaterial(),gravity=(0,0,-10),porosity=.5,dontBlock=True)
yade.collider.verletDist=0
O.saveTmp()


