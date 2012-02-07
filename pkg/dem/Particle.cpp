#include<yade/pkg/dem/Particle.hpp>
#include<yade/pkg/dem/ParticleContainer.hpp>
#include<yade/pkg/dem/ContactContainer.hpp>
#include<yade/pkg/dem/ContactLoop.hpp>
#include<yade/lib/pyutil/except.hpp>

#ifdef YADE_OPENGL
	#include<yade/pkg/gl/Renderer.hpp>
#endif

YADE_PLUGIN(dem,(CPhys)(CGeom)(CData)(DemField)(Particle)(DemData)(Contact)(Shape)(Material)(Bound)(ContactContainer));

py::dict Particle::pyContacts()const{	py::dict ret; FOREACH(MapParticleContact::value_type i,contacts) ret[i.first]=i.second; return ret;}
py::list Particle::pyCon()const{ py::list ret; FOREACH(MapParticleContact::value_type i,contacts) ret.append(i.first); return ret;}
py::list Particle::pyTacts()const{	py::list ret; FOREACH(MapParticleContact::value_type i,contacts) ret.append(i.second); return ret;}

void Contact::swapOrder(){
	if(geom || phys){ throw std::logic_error("Particles in contact cannot be swapped if they have geom or phys already."); }
	std::swap(pA,pB);
	cellDist*=-1;
}

void Contact::reset(){
	geom=shared_ptr<CGeom>();
	phys=shared_ptr<CPhys>();
	stepMadeReal=-1;
}

std::tuple<Vector3r,Vector3r,Vector3r> Contact::getForceTorqueBranch(const shared_ptr<Particle>& particle, int nodeI, Scene* scene){
	assert(pA==particle || pB==particle);
	assert(geom && phys);
	assert(nodeI>=0 && particle->shape && particle->shape->nodes.size()>(size_t)nodeI);
	bool isPA=(pA==particle);
	int sign=(isPA?1:-1);
	Vector3r F=geom->node->ori.conjugate()*phys->force*sign;
	Vector3r T=(phys->torque==Vector3r::Zero() ? Vector3r::Zero() : geom->node->ori.conjugate()*phys->torque)*sign;
	Vector3r xc=geom->node->pos-(particle->shape->nodes[nodeI]->pos+((!isPA && scene->isPeriodic) ? scene->cell->intrShiftPos(cellDist) : Vector3r::Zero()));
	return std::make_tuple(F,T,xc);
}


Particle::id_t Contact::pyId1() const { return pA->id; }
Particle::id_t Contact::pyId2() const { return pB->id; }
Vector2i Contact::pyIds() const { return Vector2i(min(pA->id,pB->id),max(pA->id,pB->id)); }

void Particle::checkNodes(bool dyn, bool checkOne) const {
	if(!shape || (checkOne  && shape->nodes.size()!=1) || (dyn && !shape->nodes[0]->hasData<DemData>())) yade::AttributeError("Particle #"+lexical_cast<string>(id)+" has no Shape"+(checkOne?string(", or the shape has no/multiple nodes")+string(!dyn?".":", or node.dem is None."):string(".")));
}

Vector3r Shape::avgNodePos(){
	assert(!nodes.empty());
	size_t sz=nodes.size();
	if(sz==1) return nodes[0]->pos;
	Vector3r ret(Vector3r::Zero());
	for(size_t i=0; i<sz; i++) ret+=nodes[i]->pos;
	return ret/sz;
}


void DemData::pyHandleCustomCtorArgs(py::tuple& args, py::dict& kw){
	if(!kw.has_key("blocked")) return;
	py::extract<string> strEx(kw["blocked"]);
	if(!strEx.check()) return;
	blocked_vec_set(strEx());
	py::api::delitem(kw,"blocked");
}


std::string DemData::blocked_vec_get() const {
	std::string ret;
	#define _SET_DOF(DOF_ANY,ch) if((flags & DemData::DOF_ANY)!=0) ret.push_back(ch);
	_SET_DOF(DOF_X,'x'); _SET_DOF(DOF_Y,'y'); _SET_DOF(DOF_Z,'z'); _SET_DOF(DOF_RX,'X'); _SET_DOF(DOF_RY,'Y'); _SET_DOF(DOF_RZ,'Z');
	#undef _SET_DOF
	return ret;
}

void DemData::blocked_vec_set(const std::string& dofs){
	flags&=~DOF_ALL; // reset DOF bits first
	FOREACH(char c, dofs){
		#define _GET_DOF(DOF_ANY,ch) if(c==ch) { flags|=DemData::DOF_ANY; continue; }
		_GET_DOF(DOF_X,'x'); _GET_DOF(DOF_Y,'y'); _GET_DOF(DOF_Z,'z'); _GET_DOF(DOF_RX,'X'); _GET_DOF(DOF_RY,'Y'); _GET_DOF(DOF_RZ,'Z');
		#undef _GET_DOF
		throw std::invalid_argument("Invalid  DOF specification `"+lexical_cast<string>(c)+"' in '"+dofs+"', characters must be ∈{x,y,z,X,Y,Z}.");
	}
}




vector<shared_ptr<Node> > Particle::getNodes(){ checkNodes(false,false); return shape->nodes; }

Vector3r& Particle::getPos() const { checkNodes(false); return shape->nodes[0]->pos; };
void Particle::setPos(const Vector3r& p){ checkNodes(false); shape->nodes[0]->pos=p; }
Quaternionr& Particle::getOri() const { checkNodes(false); return shape->nodes[0]->ori; };
void Particle::setOri(const Quaternionr& p){ checkNodes(false); shape->nodes[0]->ori=p; }

#ifdef YADE_OPENGL
	Vector3r& Particle::getRefPos() {
		checkNodes(false);
		if(!shape->nodes[0]->hasData<GlData>()) setRefPos(getPos());
		return shape->nodes[0]->getData<GlData>().refPos;
	};
	void Particle::setRefPos(const Vector3r& p){
		checkNodes(false);
		if(!shape->nodes[0]->hasData<GlData>()) shape->nodes[0]->setData<GlData>(make_shared<GlData>());
		shape->nodes[0]->getData<GlData>().refPos=p;
	}
#else
	Vector3r& Partial::getRefPos() const{ return Vector3r(NaN,NaN,NaN); }
	void Particle::setRefPos(const Vector3r& p){
		yade::RuntimeError("Particle.refPos only supported with YADE_OPENGL.");
	}
#endif

Vector3r& Particle::getVel() const { checkNodes(); return shape->nodes[0]->getData<DemData>().vel; };
void Particle::setVel(const Vector3r& p){ checkNodes(); shape->nodes[0]->getData<DemData>().vel=p; }
Vector3r& Particle::getAngVel() const { checkNodes(); return shape->nodes[0]->getData<DemData>().angVel; };
void Particle::setAngVel(const Vector3r& p){ checkNodes(); shape->nodes[0]->getData<DemData>().angVel=p; }

Real Particle::getMass() const { checkNodes(); return shape->nodes[0]->getData<DemData>().mass; };
Vector3r Particle::getInertia() const { checkNodes(); return shape->nodes[0]->getData<DemData>().inertia; };
Vector3r Particle::getForce() const { checkNodes(); return shape->nodes[0]->getData<DemData>().force; };
Vector3r Particle::getTorque() const { checkNodes(); return shape->nodes[0]->getData<DemData>().torque; };

std::string Particle::getBlocked() const { checkNodes(); return shape->nodes[0]->getData<DemData>().blocked_vec_get(); }
void Particle::setBlocked(const std::string& s){ checkNodes(); shape->nodes[0]->getData<DemData>().blocked_vec_set(s); }

Real Particle::getEk_any(bool trans, bool rot) const {
	Real ret=0;
	checkNodes();
	const DemData& dyn=shape->nodes[0]->getData<DemData>();
	Scene* scene=Omega::instance().getScene().get();
	if(trans){
		Vector3r fluctVel=scene->isPeriodic?scene->cell->pprevFluctVel(shape->nodes[0]->pos,dyn.vel,scene->dt):dyn.vel;
		Real Etrans=.5*(dyn.mass*(fluctVel.dot(fluctVel.transpose())));
		ret+=Etrans;
	}
	if(rot){
		Matrix3r T(shape->nodes[0]->ori);
		Matrix3r mI(dyn.inertia.asDiagonal());
		Vector3r fluctAngVel=scene->isPeriodic?scene->cell->pprevFluctAngVel(dyn.angVel):dyn.angVel;
		Real Erot=.5*fluctAngVel.transpose().dot((T.transpose()*mI*T)*fluctAngVel);	
		ret+=Erot;
	}
	return ret;
}

Vector3r Contact::dPos(const Scene* scene) const{
	pA->checkNodes(/*dyn*/false,/*checkUninodal*/true); pB->checkNodes(false,true);
	Vector3r rawDx=pB->shape->nodes[0]->pos-pA->shape->nodes[0]->pos;
	if(!scene->isPeriodic || cellDist==Vector3i::Zero()) return rawDx;
	return rawDx+scene->cell->intrShiftPos(cellDist);
}

int DemField::collectNodes(){
	std::set<void*> seen;
	// add regular nodes and clumps
	for(const auto& n: nodes) seen.insert((void*)n.get());
	//not this: for(const auto& n: clumps) seen.insert((void*)n.get());
	int added=0;
	// from particles
	for(const auto& p: particles){
		if(!p || !p->shape || p->shape->nodes.empty()) continue;
		FOREACH(const shared_ptr<Node>& n, p->shape->nodes){
			if(seen.count((void*)n.get())!=0) continue; // node already seen
			seen.insert((void*)n.get());
			nodes.push_back(n);
			added++;
		};
	};
	// from clumps
	for(const auto& n: clumps){
		if(seen.count((void*)n.get())!=0) continue; // already seen
		seen.insert((void*)n.get());
		nodes.push_back(n);
		added++;
	}
	return added;
}


