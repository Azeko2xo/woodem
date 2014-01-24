#include<woo/pkg/dem/Collision.hpp>
#include<woo/pkg/dem/ParticleContainer.hpp>
#ifdef WOO_OPENGL
	#include<woo/lib/opengl/OpenGLWrapper.hpp>
#endif

WOO_PLUGIN(dem,(Aabb)(BoundFunctor)(BoundDispatcher)(Collider));

#ifdef WOO_OPENGL
	WOO_PLUGIN(gl,(Gl1_Aabb))
#endif

bool Collider::mayCollide(const DemField* dem, const shared_ptr<Particle>& pA, const shared_ptr<Particle>& pB){
	/* particles which share nodes may not collide */
	if(!pA || !pB || !pA->shape || !pB->shape || pA.get()==pB.get()) return false;
	/* particles not shaing mask may not collide */
	if(!(pA->mask&pB->mask)) return false;
	/* particles sharing bits in loneMask may not collide */
	if((pA->mask&pB->mask&dem->loneMask)!=0) return false;
	/* mix and match all nodes with each other (this may be expensive, hence after easier checks above) */
	const auto& nnA(pA->shape->nodes); const auto& nnB(pB->shape->nodes);
	for(const auto& nA: nnA) for(const auto& nB: nnB) {
		// particles share a node
		if(nA.get()==nB.get()) return false; 
		// particles are inside the same clump?
		assert(nA->hasData<DemData>() && nB->hasData<DemData>());
		const auto& dA(nA->getData<DemData>()); const auto& dB(nB->getData<DemData>());
		if(dA.isClumped() && dB.isClumped()){
			assert(dA.master.lock() && dB.master.lock());
			if(dA.master.lock().get()==dB.master.lock().get()) return false;
		}
	}
	// in other cases, do collide
	return true;
}

void BoundDispatcher::run(){
	updateScenePtr();
	FOREACH(const shared_ptr<Particle> p, *(field->cast<DemField>().particles)){
		shared_ptr<Shape>& shape=p->shape;
		if(!shape) continue;
		operator()(shape);
		if(!shape) continue; // the functor did not create new bound
	#if 0
		Aabb* aabb=WOO_CAST<Aabb*>(shape->bound.get());
		Real sweep=velocityBins->bins[velocityBins->bodyBins[p->id]].maxDist;
		aabb->min-=Vector3r(sweep,sweep,sweep);
		aabb->max+=Vector3r(sweep,sweep,sweep);
	#endif
	}
}

#ifdef WOO_OPENGL
void Gl1_Aabb::go(const shared_ptr<Bound>& bv){
	Aabb& aabb=bv->cast<Aabb>();
	glColor3v(Vector3r(1,1,0));
	if(!scene->isPeriodic){
		glTranslatev(Vector3r(.5*(aabb.min+aabb.max)));
		glScalev(Vector3r(aabb.max-aabb.min));
	} else {
		Vector3r mn=aabb.min, mx=aabb.max;
		// fit infinite bboxes into the cell, if needed
		for(int ax:{0,1,2}){ if(isinf(mn[ax])) mn[ax]=0; if(isinf(mx[ax])) mx[ax]=scene->cell->getHSize().diagonal()[ax]; }
		glTranslatev(Vector3r(scene->cell->shearPt(scene->cell->wrapPt(.5*(mn+mx)))));
		glMultMatrixd(scene->cell->getGlShearTrsfMatrix());
		glScalev(Vector3r(mx-mn));
	}
	glDisable(GL_LINE_SMOOTH);
	glutWireCube(1);
	glEnable(GL_LINE_SMOOTH);
}
#endif
