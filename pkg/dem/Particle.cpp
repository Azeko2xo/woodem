#include<woo/pkg/dem/Particle.hpp>
#include<woo/pkg/dem/ParticleContainer.hpp>
#include<woo/pkg/dem/Contact.hpp>
#include<woo/lib/pyutil/except.hpp>

#include<woo/pkg/dem/Clump.hpp>

#ifdef WOO_OPENGL
	#include<woo/pkg/gl/Renderer.hpp>
#endif

WOO_PLUGIN(dem,(DemField)(Particle)(MatState)(DemData)(Impose)(Shape)(Material)(Bound));
CREATE_LOGGER(DemField);

py::dict Particle::pyContacts()const{	py::dict ret; FOREACH(MapParticleContact::value_type i,contacts) ret[i.first]=i.second; return ret;}
py::list Particle::pyCon()const{ py::list ret; FOREACH(MapParticleContact::value_type i,contacts) ret.append(i.first); return ret;}
py::list Particle::pyTacts()const{	py::list ret; FOREACH(MapParticleContact::value_type i,contacts) ret.append(i.second); return ret;}

void Particle::checkNodes(bool dyn, bool checkOne) const {
	if(!shape || (checkOne  && shape->nodes.size()!=1) || (dyn && !shape->nodes[0]->hasData<DemData>())) woo::AttributeError("Particle #"+lexical_cast<string>(id)+" has no Shape"+(checkOne?string(", or the shape has no/multiple nodes")+string(!dyn?".":", or node.dem is None."):string(".")));
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


Real DemData::getEk_any(const shared_ptr<Node>& n, bool trans, bool rot, Scene* scene){
	assert(scene!=NULL);
	assert(n->hasData<DemData>());
	const DemData& dyn=n->getData<DemData>();
	Real ret=0.;
	if(trans){
		Vector3r fluctVel=scene->isPeriodic?scene->cell->pprevFluctVel(n->pos,dyn.vel,scene->dt):dyn.vel;
		Real Etrans=.5*(dyn.mass*(fluctVel.dot(fluctVel.transpose())));
		ret+=Etrans;
	}
	if(rot){
		Matrix3r T(n->ori);
		Matrix3r mI(dyn.inertia.asDiagonal());
		Vector3r fluctAngVel=scene->isPeriodic?scene->cell->pprevFluctAngVel(dyn.angVel):dyn.angVel;
		Real Erot=.5*fluctAngVel.transpose().dot((T.transpose()*mI*T)*fluctAngVel);	
		ret+=Erot;
	}
	return ret;
}


vector<shared_ptr<Node> > Particle::getNodes(){ checkNodes(false,false); return shape->nodes; }

Vector3r& Particle::getPos() const { checkNodes(false); return shape->nodes[0]->pos; };
void Particle::setPos(const Vector3r& p){ checkNodes(false); shape->nodes[0]->pos=p; }
Quaternionr& Particle::getOri() const { checkNodes(false); return shape->nodes[0]->ori; };
void Particle::setOri(const Quaternionr& p){ checkNodes(false); shape->nodes[0]->ori=p; }

#ifdef WOO_OPENGL
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
	Vector3r Particle::getRefPos() const { return Vector3r(NaN,NaN,NaN); }
	void Particle::setRefPos(const Vector3r& p){
		woo::RuntimeError("Particle.refPos only supported with WOO_OPENGL.");
	}
#endif

Vector3r& Particle::getVel() const { checkNodes(); return shape->nodes[0]->getData<DemData>().vel; };
void Particle::setVel(const Vector3r& p){ checkNodes(); shape->nodes[0]->getData<DemData>().vel=p; }
Vector3r& Particle::getAngVel() const { checkNodes(); return shape->nodes[0]->getData<DemData>().angVel; };
void Particle::setAngVel(const Vector3r& p){ checkNodes(); shape->nodes[0]->getData<DemData>().angVel=p; }
shared_ptr<Impose> Particle::getImpose() const { checkNodes(); return shape->nodes[0]->getData<DemData>().impose; };
void Particle::setImpose(const shared_ptr<Impose>& i){ checkNodes(); shape->nodes[0]->getData<DemData>().impose=i; }

Real Particle::getMass() const { checkNodes(); return shape->nodes[0]->getData<DemData>().mass; };
Vector3r Particle::getInertia() const { checkNodes(); return shape->nodes[0]->getData<DemData>().inertia; };
Vector3r Particle::getForce() const { checkNodes(); return shape->nodes[0]->getData<DemData>().force; };
Vector3r Particle::getTorque() const { checkNodes(); return shape->nodes[0]->getData<DemData>().torque; };

std::string Particle::getBlocked() const { checkNodes(); return shape->nodes[0]->getData<DemData>().blocked_vec_get(); }
void Particle::setBlocked(const std::string& s){ checkNodes(); shape->nodes[0]->getData<DemData>().blocked_vec_set(s); }

Real Particle::getEk_any(bool trans, bool rot, Scene* scene) const {
	checkNodes();
	if(!scene) scene=Master::instance().getScene().get();
	return DemData::getEk_any(shape->nodes[0],trans,rot,scene);
}


AlignedBox3r DemField::renderingBbox() const{
	AlignedBox3r box;
	for(const auto& p: *particles){
		if(!p || !p->shape) continue;
		for(size_t i=0; i<p->shape->nodes.size(); i++) box.extend(p->shape->nodes[i]->pos);
	}
	for(const auto& n: nodes) box.extend(n->pos);
	return box;
}


void DemField::postLoad(DemField&,void*){
	particles->dem=this;
	contacts->dem=this;
	contacts->particles=particles.get();
	/* recreate particle contact information */
	for(size_t i=0; i<contacts->size(); i++){
		const shared_ptr<Contact>& c((*contacts)[i]);
		(*particles)[c->leakPA()->id]->contacts[c->leakPB()->id]=c;
		(*particles)[c->leakPB()->id]->contacts[c->leakPA()->id]=c;
	}
}


int DemField::collectNodes(){
	std::set<void*> seen;
	// add regular nodes and clumps
	for(const auto& n: nodes) seen.insert((void*)n.get());
	//not this: for(const auto& n: clumps) seen.insert((void*)n.get());
	int added=0;
	// from particles
	for(const auto& p: *particles){
		if(!p || !p->shape || p->shape->nodes.empty()) continue;
		FOREACH(const shared_ptr<Node>& n, p->shape->nodes){
			if(seen.count((void*)n.get())!=0) continue; // node already seen
			seen.insert((void*)n.get());
			n->getData<DemData>().linIx=nodes.size();
			nodes.push_back(n);
			added++;
		};
	};
	// from clumps
	for(const auto& n: clumps){
		if(seen.count((void*)n.get())!=0) continue; // already seen
		seen.insert((void*)n.get());
		assert(n->hasData<DemData>());
		n->getData<DemData>().linIx=nodes.size();
		nodes.push_back(n);
		added++;
	}
	return added;
}

void DemField::removeParticle(Particle::id_t id){
	LOG_DEBUG("Removing #"<<id);
	assert(id>=0);
	assert((int)particles->size()>id);
	// don't actually delete the particle until before returning, so that p is not dangling
	const shared_ptr<Particle>& p((*particles)[id]);
	for(const auto& n: p->shape->nodes){
		if(n->getData<DemData>().isClumped()) throw std::runtime_error("#"+to_string(id)+": a node is clumped, remove the clump itself instead!");
	}
	if(!p->shape || p->shape->nodes.empty()){
		LOG_TRACE("Removing #"<<id<<" without shape or nodes");
		particles->remove(id);
		return;
	}
	// remove particle's nodes, if they are no longer used
	for(const auto& n: p->shape->nodes){
		// decrease parCount for each node
		DemData& dyn=n->getData<DemData>();
		if(dyn.parCount==0) throw std::runtime_error("#"+to_string(id)+" has node which has zero particle count!");
		dyn.parCount-=1;
		// no particle left, delete the node itself as well
		if(dyn.parCount==0){
			if(dyn.linIx<0) continue; // node not in O.dem.nodes
			if(nodes[dyn.linIx].get()!=n.get()) throw std::runtime_error("Node in #"+to_string(id)+" has invalid linIx entry!");
			LOG_DEBUG("Removing #"<<id<<" / DemField::nodes["<<dyn.linIx<<"]"<<" (not used anymore)");
			boost::mutex::scoped_lock lock(nodesMutex);
			if(saveDeadNodes) deadNodes.push_back(nodes[dyn.linIx]);
			(*nodes.rbegin())->getData<DemData>().linIx=dyn.linIx;
			nodes[dyn.linIx]=*nodes.rbegin(); // move the last node to the current position
			nodes.resize(nodes.size()-1);
		}
	}
	// remove all contacts of the particle
	if(!p->contacts.empty()){
		vector<shared_ptr<Contact>> cc; cc.reserve(p->contacts.size());
		for(const auto& idCon: p->contacts) cc.push_back(idCon.second);
		for(const auto& c: cc){
			LOG_DEBUG("Removing #"<<id<<" / ##"<<c->leakPA()->id<<"+"<<c->leakPB()->id);
			contacts->remove(c);
		}
	}
	LOG_TRACE("Actually removing #"<<id<<" now");
	particles->remove(id);
};

void DemField::removeClump(size_t clumpLinIx){
	const auto& node=clumps[clumpLinIx];
	assert(node->hasData<DemData>());
	assert(dynamic_pointer_cast<ClumpData>(node->getDataPtr<DemData>()));
	ClumpData& cd=node->getData<DemData>().cast<ClumpData>();
	if(cd.clumpLinIx!=(long)clumpLinIx) throw std::runtime_error("Clump #"+to_string(clumpLinIx)+": clumpLinIx ("+to_string(cd.clumpLinIx)+") does not match its position ("+to_string(clumpLinIx)+")");
	for(size_t i=0; i<cd.memberIds.size(); i++){
		auto& p=(*particles)[cd.memberIds[i]];
		// make sure that clump nodes are those which the particles have
		assert(p && p->shape && p->shape->nodes.size()>0);
		for(auto& n: p->shape->nodes){
			#ifdef WOO_DEBUG
				if(std::find_if(cd.nodes.begin(),cd.nodes.end(),[&](const shared_ptr<Node>& a)->bool{ return(a.get()==n.get()); })==cd.nodes.end()) throw std::runtime_error("#"+to_string(cd.memberIds[i])+" should contain node at "+lexical_cast<string>(n->pos.transpose()));
			#endif
			n->getData<DemData>().setNoClump(); // fool the test in removeParticle
		}
		removeParticle(i);
	}
	// remove the clump node here
	boost::mutex::scoped_lock lock(nodesMutex);
	(*clumps.rbegin())->getData<DemData>().cast<ClumpData>().clumpLinIx=cd.clumpLinIx;
	clumps[cd.clumpLinIx]=*clumps.rbegin();
	clumps.resize(clumps.size()-1);
}


