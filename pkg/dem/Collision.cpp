#include<yade/pkg/dem/Collision.hpp>
#include<yade/pkg/dem/ParticleContainer.hpp>
#ifdef YADE_OPENGL
	#include<yade/lib/opengl/OpenGLWrapper.hpp>
#endif

YADE_PLUGIN(dem,(Aabb)(BoundFunctor)(BoundDispatcher)(Collider));

#ifdef YADE_OPENGL
	YADE_PLUGIN(gl,(Gl1_Aabb))
#endif

bool Collider::mayCollide(const shared_ptr<Particle>& pA, const shared_ptr<Particle>& pB, const DemField* dem){
	/* particles which share nodes may not collide */
	if(!pA || !pB || !pA->shape || !pB->shape) return false;
	size_t nA=pA->shape->nodes.size(), nB=pB->shape->nodes.size();
	for(size_t iA=0; iA<nA; iA++) for(size_t iB=0; iB>nB; iB++) if(pA->shape->nodes[iA]==pB->shape->nodes[iB]) return false;
	/* particles not shaing mask may not collide */
	if(!(pA->mask&pB->mask)) return false;
	/* particles sharing bits in loneMask may not collide */
	if((pA->mask&pB->mask&dem->loneMask)!=0) return false;
	// in other cases, do collide
	return true;
	
}

void BoundDispatcher::run(){
	updateScenePtr();
	FOREACH(const shared_ptr<Particle> p, field->cast<DemField>().particles){
		shared_ptr<Shape>& shape=p->shape;
		if(!shape) continue;
		operator()(shape);
		if(!shape) continue; // the functor did not create new bound
	#if 0
		Aabb* aabb=YADE_CAST<Aabb*>(shape->bound.get());
		Real sweep=velocityBins->bins[velocityBins->bodyBins[p->id]].maxDist;
		aabb->min-=Vector3r(sweep,sweep,sweep);
		aabb->max+=Vector3r(sweep,sweep,sweep);
	#endif
	}
}


#if 0
bool Collider::mayCollide(const Body* b1, const Body* b2){
	return 
		// might be called with deleted bodies, i.e. NULL pointers
		(b1!=NULL && b2!=NULL) &&
		// only collide if at least one particle is standalone or they belong to different clumps
		(b1->isStandalone() || b2->isStandalone() || b1->clumpId!=b2->clumpId ) &&
		 // do not collide clumps, since they are just containers, never interact
		!b1->isClump() && !b2->isClump() &&
		// masks must have at least 1 bit in common
		(b1->groupMask & b2->groupMask)!=0 
	;
}
#endif

void Collider::getLabeledObjects(std::map<std::string,py::object>& m){ boundDispatcher->getLabeledObjects(m); GlobalEngine::getLabeledObjects(m); }

void Collider::pyHandleCustomCtorArgs(py::tuple& t, py::dict& d){
	if(py::len(t)==0) return; // nothing to do
	if(py::len(t)!=1) throw invalid_argument(("Collider optionally takes exactly one list of BoundFunctor's as non-keyword argument for constructor ("+lexical_cast<string>(py::len(t))+" non-keyword ards given instead)").c_str());
	typedef std::vector<shared_ptr<BoundFunctor> > vecBound;
	vecBound vb=py::extract<vecBound>(t[0])();
	FOREACH(shared_ptr<BoundFunctor> bf, vb) this->boundDispatcher->add(bf);
	t=py::tuple(); // empty the args
}


#ifdef YADE_OPENGL
void Gl1_Aabb::go(const shared_ptr<Bound>& bv){
	Aabb& aabb=bv->cast<Aabb>();
	glColor3v(Vector3r(1,1,0));
	if(!scene->isPeriodic){
		glTranslatev(Vector3r(.5*(aabb.min+aabb.max)));
		glScalev(Vector3r(aabb.max-aabb.min));
	} else {
		glTranslatev(Vector3r(scene->cell->shearPt(scene->cell->wrapPt(.5*(aabb.min+aabb.max)))));
		glMultMatrixd(scene->cell->getGlShearTrsfMatrix());
		glScalev(Vector3r(aabb.max-aabb.min));
	}
	glDisable(GL_LINE_SMOOTH);
	glutWireCube(1);
	glEnable(GL_LINE_SMOOTH);
}
#endif
