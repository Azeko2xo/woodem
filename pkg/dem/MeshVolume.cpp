#ifdef WOO_GTS

#include<woo/pkg/dem/MeshVolume.hpp>

WOO_PLUGIN(dem,(MeshVolume));
CREATE_LOGGER(MeshVolume);

void MeshVolume::init(){
	surface.reset();
	//faces.clear();
	edges.clear();
	vertices.clear();
	nodes.clear();
	thickVol=0.;
	// index of any node in the nodes array
	std::map<Node*,size_t> nodeIx;
	// index of any node couple in the edges array
	// the pair of pointers is ordered: smaller is always the first one!
	std::map<std::pair<Node*,Node*>,size_t> edgeIx;
	surface=std::unique_ptr<GtsSurface>(gts_surface_new(gts_surface_class(),gts_face_class(),gts_edge_class(),gts_vertex_class()));
	FOREACH(const shared_ptr<Particle>& p, *(field->cast<DemField>().particles)){
		assert(p);
		if(mask!=0 && (p->mask & mask)==0) continue;
		if(!p->shape) throw std::runtime_error("MeshVolume: #"+to_string(p->id)+": shape==None.");
		if(!dynamic_pointer_cast<Facet>(p->shape)) throw std::runtime_error("MeshVolume: #"+to_string(p->id)+": shape must be a Facet (not a "+p->shape->getClassName()+")");
		const Facet& f=p->shape->cast<Facet>();
		assert(f.numNodesOk()); // 3 nodes
		LOG_DEBUG("Facet #"<<p->id<<", mask="<<p->mask<<" (mesh mask "<<mask);
		if(f.halfThick>0){ thickVol+=f.halfThick*f.getArea(); }
		Vector3i nIxs; // node indices in nodes == vertex indices in vertices
		for(int i:{0,1,2}){
			const auto& n(f.nodes[i]);
			auto nI=nodeIx.find(n.get());
			if(nI==nodeIx.end()){
				LOG_TRACE("Node at "<<n->pos.transpose()<<" not yet seen in this surface, adding to ["<<nodes.size()<<"]");
				nodeIx[n.get()]=nodes.size();
				nIxs[i]=nodes.size();
				nodes.push_back(n);
				vertices.push_back(std::unique_ptr<GtsVertex>(gts_vertex_new(gts_vertex_class(),n->pos[0],n->pos[1],n->pos[2])));
			} else {
				LOG_TRACE("Node at "<<n->pos.transpose()<<" already known, at ["<<nI->second<<"]");
				nIxs[i]=nI->second;
				assert(nodes[nI->second].get()==n.get());
			}
		}
		// ixs contains indices of nodes/vertices for this facet/face
		Vector3i eIxs;
		for(int i:{0,1,2}){
			Node *ni(f.nodes[i].get()), *ni1(f.nodes[(i+1)%3].get());
			const std::pair<Node*,Node*> nn=std::make_pair(min(ni,ni1),max(ni,ni1));
			auto nnI=edgeIx.find(nn);
			if(nnI==edgeIx.end()){ // edge not created yet
				LOG_TRACE("Edge between "<<ni->pos.transpose()<<" and "<<ni1->pos.transpose()<<" not yet seen, adding to "<<edges.size()<<"]");
				edgeIx[nn]=edges.size();
				eIxs[i]=edges.size();
				edges.push_back(std::unique_ptr<GtsEdge>(gts_edge_new(gts_edge_class(),vertices[nIxs[i]].get(),vertices[nIxs[(i+1)%3]].get())));
			} else {
				LOG_TRACE("Edge between "<<ni->pos.transpose()<<" and "<<ni1->pos.transpose()<<" already known, at ["<<nnI->second<<"]");
				eIxs[i]=nnI->second;
			}
		}
		LOG_TRACE("New face with edges "<<eIxs.transpose());
		GtsFace* face=gts_face_new(gts_face_class(),edges[eIxs[0]].get(),edges[eIxs[1]].get(),edges[eIxs[2]].get());
		// surface takes ownership of face
		gts_surface_add_face(surface.get(),face);
	}
	LOG_INFO("Create surface with "<<gts_surface_vertex_number(surface.get())<<" vertices, "<<gts_surface_edge_number(surface.get())<<" edges, "<<gts_surface_face_number(surface.get())<<" faces. The surface is"<<(gts_surface_is_orientable(surface.get())?"":" NOT")<<" orientable, is"<<(gts_surface_is_closed(surface.get())?"":" NOT")<<" closed.");
	if(!gts_surface_is_orientable(surface.get()) || !gts_surface_is_closed(surface.get())){
		surface.reset();
		throw std::runtime_error("Surface is not orientable or not closed.");
	}
}

void MeshVolume::updateVertices(){
	assert(surface);
	assert(nodes.size()==vertices.size());
	for(size_t i=0; i<nodes.size(); i++){
		const Vector3r& pos=(nodes[i]->pos);
		gts_point_set(GTS_POINT(vertices[i].get()),pos[0],pos[1],pos[2]);
	}
}

void MeshVolume::run(){
	if(!surface || reinit) init();
	reinit=false;
	updateVertices();
	vol=gts_surface_volume(surface.get());
}

#endif
