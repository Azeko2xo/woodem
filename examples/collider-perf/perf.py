
utils.readParamsFromTable(nSpheres=1000,collider='PersistentSAPCollider',noTableOk=True)
# name of file containing sphere packing with given number of spheres
spheresFile="packing-%dk.spheres"%(nSpheres/1000)

import os
if not os.path.exists(spheresFile):
	print "Generating packing"
	p=Preprocessor('TriaxialTest',{'numberOfGrains':nSpheres,'radiusMean':1e-3,'lowerCorner':[0,0,0],'upperCorner':[1,1,1]})
	p.load()
	utils.spheresToFile(spheresFile)
	O.reset()
	print "Packing %s done"%spheresFile
else: print "Packing found (%s), using it."%spheresFile

from yade import timing
O.timingEnabled=True

p=Preprocessor('TriaxialTest',{'importFilename':spheresFile}).load()
utils.replaceCollider(StandAloneEngine(collider))

O.step()
timing.stats()
timing.reset()
O.run(100,True)
timing.stats()
quit()
