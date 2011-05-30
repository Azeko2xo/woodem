#include<yade/pkg/dem/Collision.hpp>
#include<yade/pkg/dem/ParticleContainer.hpp>
#ifdef YADE_OPENGL
	#include<yade/lib/opengl/OpenGLWrapper.hpp>
#endif

YADE_PLUGIN(dem,(Aabb)(BoundFunctor)(BoundDispatcher)(Collider));

#ifdef YADE_OPENGL
	YADE_PLUGIN(gl,(Gl1_Aabb))
#endif

bool Collider::mayCollide(const shared_ptr<Particle>& pA, const shared_ptr<Particle>& pB){
	/* particles which share nodes may not collide */
	if(!pA->shape || !pB->shape) return false;
	size_t nA=pA->shape->nodes.size(), nB=pB->shape->nodes.size();
	for(size_t iA=0; iA<nA; iA++) for(size_t iB=0; iB>nB; iB++) if(pA->shape->nodes[iA]==pB->shape->nodes[iB]) return false;
	// in other cases, do collide
	return true;
	
}

CREATE_LOGGER(BoundDispatcher);
void BoundDispatcher::run(){
	updateScenePtr();
	FOREACH(const shared_ptr<Particle> p, field->cast<DemField>().particles){
		shared_ptr<Shape>& shape=p->shape;
		//if(!shape || !b->isBounded()) continue;
		if(!shape) continue;
		operator()(shape);
		if(!shape) continue; // the functor did not create new bound
		if(sweepDist>0){
			Aabb* aabb=YADE_CAST<Aabb*>(shape->bound.get());
			aabb->min-=Vector3r(sweepDist,sweepDist,sweepDist);
			aabb->max+=Vector3r(sweepDist,sweepDist,sweepDist);
		}
	}
	// scene->updateBound();
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

void Collider::pyHandleCustomCtorArgs(python::tuple& t, python::dict& d){
	if(python::len(t)==0) return; // nothing to do
	if(python::len(t)!=1) throw invalid_argument(("Collider optionally takes exactly one list of BoundFunctor's as non-keyword argument for constructor ("+lexical_cast<string>(python::len(t))+" non-keyword ards given instead)").c_str());
	typedef std::vector<shared_ptr<BoundFunctor> > vecBound;
	vecBound vb=python::extract<vecBound>(t[0])();
	FOREACH(shared_ptr<BoundFunctor> bf, vb) this->boundDispatcher->add(bf);
	t=python::tuple(); // empty the args
}


#ifdef YADE_OPENGL
void Gl1_Aabb::go(const shared_ptr<Bound>& bv){
	Aabb& aabb=bv->cast<Aabb>();
	glColor3v(Vector3r(1,1,0));
	// glDisable(GL_LIGHTING);
	if(!scene->isPeriodic){
		glTranslatev(Vector3r(.5*(aabb.min+aabb.max)));
		glScalev(Vector3r(aabb.max-aabb.min));
	} else {
		glTranslatev(Vector3r(scene->cell->shearPt(scene->cell->wrapPt(.5*(aabb.min+aabb.max)))));
		glMultMatrixd(scene->cell->getGlShearTrsfMatrix());
		glScalev(Vector3r(aabb.max-aabb.min));
	}
	glutWireCube(1);
}
#endif
