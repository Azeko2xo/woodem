#include<woo/pkg/dem/Contact.hpp>
#include<woo/pkg/dem/Particle.hpp>

WOO_IMPL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_CGeom__CLASS_BASE_DOC_ATTRS_PY);
WOO_IMPL__CLASS_BASE_DOC_ATTRS_CTOR_PY(woo_dem_CPhys__CLASS_BASE_DOC_ATTRS_CTOR_PY);
WOO_IMPL__CLASS_BASE_DOC(woo_dem_CData__CLASS_BASE_DOC);
WOO_IMPL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_Contact__CLASS_BASE_DOC_ATTRS_PY);


WOO_PLUGIN(dem,(CPhys)(CGeom)(CData)(Contact));

void Contact::swapOrder(){
	if(geom || phys){ throw std::logic_error("Particles in contact cannot be swapped if they have geom or phys already."); }
	std::swap(pA,pB);
	cellDist*=-1;
}

void Contact::reset(){
	geom=shared_ptr<CGeom>();
	phys=shared_ptr<CPhys>();
	// stepCreated=-1;
}

std::tuple<Vector3r,Vector3r,Vector3r> Contact::getForceTorqueBranch(const Particle* particle, int nodeI, Scene* scene){
	assert(leakPA()==particle || leakPB()==particle);
	assert(geom && phys);
	assert(nodeI>=0 && particle->shape && particle->shape->nodes.size()>(size_t)nodeI);
	bool isPA=(leakPA()==particle);
	int sign=(isPA?1:-1);
	Vector3r F=geom->node->ori*phys->force*sign; // QQQ
	Vector3r T=(phys->torque==Vector3r::Zero() ? Vector3r::Zero() : geom->node->ori*phys->torque)*sign; // QQQ
	Vector3r xc=geom->node->pos-(particle->shape->nodes[nodeI]->pos+((!isPA && scene->isPeriodic) ? scene->cell->intrShiftPos(cellDist) : Vector3r::Zero()));
	return std::make_tuple(F,T,xc);
}


Particle::id_t Contact::pyId1() const { return pA.expired()?-1:leakPA()->id; }
Particle::id_t Contact::pyId2() const { return pB.expired()?-1:leakPB()->id; }
Vector2i Contact::pyIds() const { Vector2i ids(pyId1(),pyId2()); return Vector2i(ids.minCoeff(),ids.maxCoeff()); }

Vector3r Contact::dPos(const Scene* scene) const{
	leakPA()->checkNodes(/*dyn*/false,/*checkUninodal*/true); leakPB()->checkNodes(false,true);
	Vector3r rawDx=leakPB()->shape->nodes[0]->pos-leakPA()->shape->nodes[0]->pos;
	if(!scene->isPeriodic || cellDist==Vector3i::Zero()) return rawDx;
	return rawDx+scene->cell->intrShiftPos(cellDist);
}


