import yade.log
#yade.log.setLevel('FacetTopologyAnalyzer',yade.log.TRACE)

# Note: FacetTopologyAnalyzer is normally run as an initializer;
# it is only for testing sake that it is in O.engines here.
O.engines=[FacetTopologyAnalyzer(projectionAxis=(1,1,1),label='topo'),]

# most simple case: no touch at all
if 1:
	O.bodies.append([
		utils.facet([(0,0,0),(1,0,0),(0,1,0)]),
		utils.facet([(0,0,1),(1,0,1),(0,1,1)]),
	])
	O.step();
	assert(topo['commonEdgesFound']==0)
if 1:
	O.bodies.clear()
	O.bodies.append([
		utils.facet([(0,0,0),(1,0,0),(0,1,0)]),
		utils.facet([(1,1,0),(1,0,0),(0,1,0)]),
	])
	O.step()
	assert(O.bodies[0].phys['edgeAdjIds'][1]==1 and O.bodies[1].phys['edgeAdjIds'][0]==1)
	assert(topo['commonEdgesFound']==1)
if 1:
	O.bodies.clear()
	r=.5 # radius of the sphere
	nPoly=12; # try 128, it is still quite fast
	def sphPt(i,j):
		if i==0: return (0,0,-r)
		if i==nPoly/2: return (0,0,r)
		assert(i>0 and i<nPoly/2)
		assert(j>=0 and j<=nPoly)
		theta=i*2*pi/nPoly
		rr=r*sin(theta)
		phi=j*2*pi/nPoly
		return rr*cos(phi),rr*sin(phi),-r*cos(theta)
	for i in range(0,nPoly/2):
		for j in range(0,nPoly):
			if i!=0: O.bodies.append(utils.facet([sphPt(i,j),sphPt(i,j+1),sphPt(i+1,j)]))
			if i!=nPoly/2-1: O.bodies.append(utils.facet([sphPt(i+1,j),sphPt(i,j+1),sphPt(i+1,j+1)]))
	print 'Sphere created, has',len(O.bodies),'facets'
	O.step()
	assert(topo['commonVerticesFound']==nPoly*(nPoly/2-1)+2)
	assert(topo['commonEdgesFound']==nPoly*((nPoly/2-1)+(nPoly/2-2)*2+2))
	print topo['commonVerticesFound'],'vertices; ',topo['commonEdgesFound'],'edges'
#quit()
