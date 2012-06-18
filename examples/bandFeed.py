from __future__ import division
from yade import *
from miniEigen import *
from yade.dem import *
from yade.core import *
from yade import utils,pack,plot
import numpy
import yade.pre.Roro_

yade.pre.Roro_.makeBandFeedPack(dim=(.5,.4,.2),psd=((.02,.0),(.03,.2),(.04,.3),(.05,.7)),material=utils.defaultMaterial(),gravity=(0,0,-10),porosity=.5,dontBlock=True)
yade.collider.verletDist=0
O.saveTmp()


