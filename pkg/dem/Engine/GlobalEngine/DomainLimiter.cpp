#include<yade/pkg-dem/DomainLimiter.hpp>

YADE_PLUGIN((DomainLimiter)(LawTester)
	#ifdef YADE_OPENGL
		(GlExtra_LawTester)
	#endif
);

void DomainLimiter::action(){
	std::list<Body::id_t> out;
	FOREACH(const shared_ptr<Body>& b, *scene->bodies){
		if(!b) continue;
		const Vector3r& p(b->state->pos);
		if(p[0]<lo[0] || p[0]>hi[0] || p[1]<lo[1] || p[1]>hi[1] || p[2]<lo[2] || p[2]>hi[2]) out.push_back(b->id);
	}
	FOREACH(Body::id_t id, out){
		scene->bodies->erase(id);
		nDeleted++;
	}
}

#include<yade/pkg-dem/DemXDofGeom.hpp>
#include<yade/pkg-dem/ScGeom.hpp>
#include<yade/pkg-common/NormShearPhys.hpp>
#include<yade/pkg-common/LinearInterpolate.hpp>
#include<yade/lib-pyutil/gil.hpp>

CREATE_LOGGER(LawTester);

void LawTester::postLoad(LawTester&){
	if(ids.size()!=2) throw std::invalid_argument("LawTester.ids: exactly two values must be given.");
	if(path.empty()) throw invalid_argument("LawTester.path: at least one point must be given.");
	if(pathSteps.empty()) throw invalid_argument("LawTester.pathSteps: at least one value must be given.");
	// update path points
	_pathV.clear(); _pathV.push_back(Vector3r::Zero());
	for(size_t i=0; i<path.size(); i++) _pathV.push_back(path[i]);
	// update time points from distances, repeat last distance if shorter than path
	_pathT.clear(); _pathT.push_back(0);
	for(size_t i=0; i<pathSteps.size(); i++) _pathT.push_back(_pathT[i]+pathSteps[i]);
	int lastDist=pathSteps[pathSteps.size()-1];
	for(size_t i=pathSteps.size(); i<path.size(); i++) _pathT.push_back(*(_pathT.rbegin())+lastDist);
}

void LawTester::action(){
	LOG_DEBUG("=================== LawTester step "<<step<<" ========================");
	const shared_ptr<Interaction> Inew=scene->interactions->find(ids[0],ids[1]);
	string strIds("##"+lexical_cast<string>(ids[0])+"+"+lexical_cast<string>(ids[1]));
	// interaction not found at initialization
	if(!I && (!Inew || !Inew->isReal())) throw std::runtime_error("LawTester: interaction "+strIds+" does not exist"+(Inew?" (to be honest, it does exist, but it is not real).":"."));
	// interaction was deleted meanwhile
	if(I && (!Inew || !Inew->isReal())) throw std::runtime_error("LawTester: interaction "+strIds+" was deleted"+(Inew?" (is not real anymore).":"."));
	// different interaction object
	if(I && Inew && I!=Inew) throw std::logic_error("LawTester: interacion "+strIds+" is a different object now?!");
	assert(Inew);
	bool doInit=(!I);
	if(doInit) I=Inew;

	id1=I->getId1(); id2=I->getId2();
	// test object types
	GenericSpheresContact* gsc=dynamic_cast<GenericSpheresContact*>(I->geom.get());
	ScGeom* scGeom=dynamic_cast<ScGeom*>(I->geom.get());
	Dem3DofGeom* d3dGeom=dynamic_cast<Dem3DofGeom*>(I->geom.get());
	NormShearPhys* phys=dynamic_cast<NormShearPhys*>(I->phys.get());
	if(!gsc) throw std::invalid_argument("LawTester: IGeom of "+strIds+" not a GenericSpheresContact.");
	if(!scGeom && !d3dGeom) throw std::invalid_argument("LawTester: IGeom of "+strIds+" is neither ScGeom, nor Dem3DofGeom.");
	assert(!((bool)scGeom && (bool)d3dGeom)); // nonsense
	// get body objects
	State *state1=Body::byId(id1,scene)->state.get(), *state2=Body::byId(id2,scene)->state.get();
	if(state1->blockedDOFs!=State::DOF_ALL) { LOG_INFO("Blocking all DOFs for #"<<id1); state1->blockedDOFs=State::DOF_ALL;}
	if(state2->blockedDOFs!=State::DOF_ALL) { LOG_INFO("Blocking all DOFs for #"<<id2); state2->blockedDOFs=State::DOF_ALL;}

	if(step>*(_pathT.rbegin())){
		LOG_INFO("Last step done, setting zero velocities on #"<<id1<<", #"<<id2<<".");
		state1->vel=state1->angVel=state2->vel=state2->angVel=Vector3r::Zero();
		if(doneHook.empty()){ LOG_INFO("No doneHook set, dying."); dead=true; }
		else{ LOG_INFO("Running doneHook: "<<doneHook);	pyRunString(doneHook);}
		return;
	}

	axX=gsc->normal; /* just in case */ axX.normalize();
	if(doInit){ // initialization of the new interaction
		// take vector in the y or z direction, depending on its length; arbitrary, but one of them is sure to be non-zero
		axY=axX.cross(axX[1]>axX[2]?Vector3r::UnitY():Vector3r::UnitZ());
		axY.normalize();
		axZ=axX.cross(axY);
		LOG_DEBUG("Initial axes x="<<axX<<", y="<<axY<<", z="<<axZ);
		renderLength=.5*(gsc->refR1+gsc->refR2);
	} else { // udpate of an existing interaction
		if(scGeom){
			scGeom->rotate(axY); scGeom->rotate(axZ);
			scGeom->rotate(shearTot);
			shearTot-=scGeom->shearIncrement();
			ptGeom=Vector3r(-scGeom->penetrationDepth,shearTot.dot(axY),shearTot.dot(axZ));
		}
		else{ // d3dGeom
			throw runtime_error("LawTester: Dem3DofGeom not yet supported.");
			// essentially copies code from ScGeom, which is not very nice indeed; oh well…
			Vector3r vRel=(state2->vel+state2->angVel.cross(-gsc->refR2*gsc->normal))-(state1->vel+state1->angVel.cross(gsc->refR1*gsc->normal));
		}
	}
	// update the transformation
	// the matrix is orthonormal, since axX, axY are normalized and and axZ is their corss-product
	trsf.row(0)=axX; trsf.row(1)=axY; trsf.row(2)=axZ;
	trsfQ=Quaternionr(trsf);
	contPt=gsc->contactPoint;
	
	// here we go ahead, finally
	Vector3r val=linearInterpolate<Vector3r,int>(step,_pathT,_pathV,_interpPos);
	Vector3r valDiff=val-prevVal; prevVal=val;
	if(pathIsRel){
		LOG_DEBUG("Relative diff is "<<valDiff<<" (will be normalized by "<<gsc->refR1+gsc->refR2<<")");
		valDiff*=gsc->refR1+gsc->refR2;
	}
	LOG_DEBUG("Absolute diff is "<<valDiff);
	ptOurs+=valDiff;

	#if 0
		// we compute displacement for the 2nd particles; cff1 and cff2 determine how to apply it on both particles, depending on sense:
		// 0 is the symmetric case (-.5,.5); -1 applies only to the first particle, i.e. (-1,0), +1 only to the second one (0,+1)
		Real r1_r2=gsc->radius1/gsc->radius2; // ratio of radii
		Real cff1=(sense<0?-1:(sense>0?0:-.5)), cff2=(sense<0?0:(sense<0?0:.5));
	#endif
	// shear is applied as rotation of id2: dε=r₁dθ → dθ=dε/r₁;
	Real rot2y=valDiff[1]/gsc->refR2, rot2z=valDiff[2]/gsc->refR2;
	Vector3r angVel2=(rot2y*axY+rot2z*axZ)/scene->dt;
	Vector3r linVel2=axX*valDiff[0]/scene->dt;
	state2->angVel=angVel2;
	state2->vel=linVel2;
	LOG_DEBUG("Body #"<<id2<<", setting vel="<<linVel2<<", angVel="<<angVel2);

	step++;
}

#ifdef YADE_OPENGL
#include<yade/lib-opengl/OpenGLWrapper.hpp>
#include<yade/lib-opengl/GLUtils.hpp>
#include<yade/pkg-common/GLDrawFunctors.hpp>
#include<GL/glu.h>

CREATE_LOGGER(GlExtra_LawTester);

void GlExtra_LawTester::render(){
	if(!tester){ FOREACH(shared_ptr<Engine> e, scene->engines){ tester=dynamic_pointer_cast<LawTester>(e); if(tester) break; } }
	if(!tester){ LOG_ERROR("No LawTester in O.engines, killing myself."); dead=true; return; }
	//if(tester->renderLength<=0) return;
	glColor3v(Vector3r(0,1,0));
	glBegin(GL_LINES); glVertex3v(Vector3r::Zero()); glVertex3v(.1*Vector3r::Ones()); glEnd(); 
	//cerr<<".";
	return; 

	glBegin(GL_LINES);
		glVertex3v(tester->contPt);
		glVertex3v(tester->contPt+tester->renderLength*tester->axX);
		glVertex3v(tester->contPt);
		glVertex3v(tester->contPt+tester->renderLength*tester->axY);
		glVertex3v(tester->contPt);
		glVertex3v(tester->contPt+tester->renderLength*tester->axZ);
		glVertex3v(tester->contPt);
		glVertex3v(tester->contPt+tester->ptOurs);
	glEnd();
	#if 0
	GLUtils::GLDrawArrow(tester->contPt,tester->contPt+tester->renderLength*tester->axX,Vector3r(1,0,0));
	GLUtils::GLDrawArrow(tester->contPt,tester->contPt+tester->renderLength*tester->axY,Vector3r(0,1,0));
	GLUtils::GLDrawArrow(tester->contPt,tester->contPt+tester->renderLength*tester->axZ,Vector3r(0,0,1));
	GLUtils::GLDrawArrow(tester->contPt,tester->contPt+tester->ptOurs,Vector3r(1,1,0));
	GLUtils::GLDrawArrow(Vector3r::Zero(),Vector3r::Ones(),Vector3r(.5,.5,1));
	#endif
}
#endif
