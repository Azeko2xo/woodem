from woo.pack import SpherePack

sp=SpherePack()
sp.cellSize=(1,1,1)
# add two weird clumps
sp.add((.1,.1,.1),.2,0)
sp.add((.1,.1,.1),.2,0)
sp.add((.9,.9,.9),.2,1)
sp.add((.9,.9,.9),.2,1)
sp.save('/tmp/aa')
print sp.maxRelOverlap()
# sp.canonicalize()
sp.makeOverlapFree()
print sp.maxRelOverlap()
for s in sp.toList(): print s

