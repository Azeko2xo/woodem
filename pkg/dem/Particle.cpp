#include<woo/pkg/dem/Particle.hpp>
#include<woo/pkg/dem/ParticleContainer.hpp>
#include<woo/pkg/dem/Contact.hpp>
#include<woo/lib/pyutil/except.hpp>
#include<woo/pkg/dem/Clump.hpp>
#include<woo/pkg/dem/Funcs.hpp>

#ifdef WOO_OPENGL
	#include<woo/pkg/gl/GlData.hpp>
#endif

#include<boost/range/algorithm/count_if.hpp>
#include<boost/range/algorithm/find_if.hpp>

WOO_PLUGIN(dem,(DemField)(Particle)(MatState)(DemData)(Impose)(Shape)(Material)(Bound));

WOO_IMPL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_Particle__CLASS_BASE_DOC_ATTRS_PY);
WOO_IMPL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_DemData__CLASS_BASE_DOC_ATTRS_PY);
WOO_IMPL__CLASS_BASE_DOC_ATTRS_CTOR_PY(woo_dem_DemField__CLASS_BASE_DOC_ATTRS_CTOR_PY);
WOO_IMPL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_Impose__CLASS_BASE_DOC_ATTRS_PY);
WOO_IMPL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_MatState__CLASS_BASE_DOC_ATTRS_PY);
WOO_IMPL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_Shape__CLASS_BASE_DOC_ATTRS_PY);
WOO_IMPL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_Material__CLASS_BASE_DOC_ATTRS_PY);
WOO_IMPL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_Bound__CLASS_BASE_DOC_ATTRS_PY);



CREATE_LOGGER(DemField);
CREATE_LOGGER(DemData);
CREATE_LOGGER(Particle);

py::dict Particle::pyContacts()const{	py::dict ret; FOREACH(MapParticleContact::value_type i,contacts) ret[i.first]=i.second; return ret;}
py::list Particle::pyCon()const{ py::list ret; FOREACH(MapParticleContact::value_type i,contacts) ret.append(i.first); return ret;}
py::list Particle::pyTacts()const{	py::list ret; FOREACH(MapParticleContact::value_type i,contacts) ret.append(i.second); return ret;}

void Particle::checkNodes(bool dyn, bool checkOne) const {
	if(!shape || (checkOne  && shape->nodes.size()!=1) || (dyn && !shape->nodes[0]->hasData<DemData>())) woo::AttributeError("Particle #"+lexical_cast<string>(id)+" has no Shape"+(checkOne?string(", or the shape has no/multiple nodes")+string(!dyn?".":", or node.dem is None."):string(".")));
}

void Particle::selfTest(){
	if(shape) shape->selfTest(static_pointer_cast<Particle>(shared_from_this()));
	// test other things here as necessary
}


shared_ptr<Particle> Particle::make(const shared_ptr<Shape>& shape, const shared_ptr<Material>& mat, bool fixed){
	auto par=make_shared<Particle>();
	par->shape=shape;
	par->material=mat;
	auto& nodes(shape->nodes);
	nodes.resize(shape->numNodes());
	for(size_t i=0; i<shape->nodes.size(); i++){
		if(!nodes[i]) nodes[i]=make_shared<Node>();
		if(!nodes[i]->hasData<DemData>()) nodes[i]->setData<DemData>(make_shared<DemData>());
		if(fixed) nodes[i]->getData<DemData>().setBlockedAll();
		nodes[i]->getData<DemData>().addParRef(par);
		#ifdef WOO_OPENGL
			// to avoid crashes if renderer must resize the node's data array and reallocates it while other thread accesses those data
			nodes[i]->setData<GlData>(make_shared<GlData>());
		#endif
	}
	shape->updateMassInertia(mat->density);
	return par;
}


Vector3r Shape::avgNodePos(){
	assert(!nodes.empty());
	size_t sz=nodes.size();
	if(sz==1) return nodes[0]->pos;
	Vector3r ret(Vector3r::Zero());
	for(size_t i=0; i<sz; i++) ret+=nodes[i]->pos;
	return ret/sz;
}

void Particle::updateMassInertia() const {
	if(!shape) throw std::runtime_error("Particle.shape==None");
	if(!material) throw std::runtime_error("Particle.material==None");
	shape->updateMassInertia(material->density);
}

void Shape::checkNodesHaveDemData() const{
	if(!numNodesOk()) woo::AttributeError(pyStr()+": mismatch of Shape and number of nodes.");
	for(const auto& n: nodes){ if(!n->hasData<DemData>()) woo::AttributeError(pyStr()+": Node.dem==None."); } 
}

void Shape::updateMassInertia(const Real& density) const{
	throw std::runtime_error(getClassName()+"::updateMassInertia: not implemented for this shape.");
}


int Particle::countRealContacts() const{
	return boost::range::count_if(contacts,[&](const MapParticleContact::value_type& C)->bool{ return C.second->isReal(); });
}

void Shape::setFromRaw_helper_checkRaw_makeNodes(const vector<Real>& raw, size_t numRaw){
	if(raw.size()!=numRaw) throw std::runtime_error("Error setting "+pyStr()+" from raw data: "+to_string(numRaw)+" numbers expected, "+to_string(raw.size())+" given.");
	// add/remove nodes as necessary
	while((int)nodes.size()<numNodes()) nodes.push_back(make_shared<Node>());
	nodes.resize(numNodes());
}

py::tuple Shape::pyAsRaw() const {
	Vector3r center; Real radius; vector<Real> raw;
	asRaw(center,radius,raw);
	return py::make_tuple(center,radius,raw);
}

void Shape::asRaw(Vector3r& center, Real& radius, vector<Real>& raw) const { throw std::runtime_error(pyStr()+" does not implement Shape.asRaw."); }
void Shape::setFromRaw(const Vector3r& center, const Real& radius, const vector<Real>& raw){	throw std::runtime_error(pyStr()+" does not implement Shape.setFromRaw."); }
bool Shape::isInside(const Vector3r& pt) const { throw std::runtime_error(pyStr()+" does not implement Shape.isInside."); }
AlignedBox3r Shape::alignedBox() const { throw std::runtime_error(pyStr()+" does not implement Shape.alignedBox."); }
void Shape::applyScale(Real s) { throw std::runtime_error(pyStr()+" does not implement Shape.applyScale."); }

#if 0
shared_ptr<Particle> Shape::make(const shared_ptr<Shape>& shape, const shared_ptr<Material>& mat, py::dict kwargs){
	int mask=1;
	if(kwargs.haskey("mask")) mask=py::extract<int>(kwargs["mask"]);
	bool fixed=false;
}
#endif



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


void Particle::postLoad(Particle&,void* attr){
	if(attr==NULL){ // only do this when really deserializing
		if(!shape) return;
		for(const auto& n: shape->nodes){
			if(!n->hasData<DemData>()) continue;
			LOG_TRACE("Adding "<<this->pyStr()<<" to "<<n->pyStr()<<".dem.parRef [linIx="<<n->getData<DemData>().linIx<<"] ");
			n->getData<DemData>().addParRef_raw(this);
		}
	}
}


void DemData::setAngVel(const Vector3r& av){ angVel=av; angMom=Vector3r(NaN,NaN,NaN); };
void DemData::postLoad(DemData&,void*attr){
	if(attr==&angVel) angMom=Vector3r(NaN,NaN,NaN);
}

py::list DemData::pyParRef_get(){
	py::list ret;
	for(const auto& p: parRef) ret.append(static_pointer_cast<Particle>(p->shared_from_this()));
	return ret;
}

void DemData::addParRef(const shared_ptr<Particle>& p){
	addParRef_raw(p.get());
}

void DemData::addParRef_raw(Particle* p){
	if(boost::range::find_if(parRef,[&p](const Particle* i)->bool{ return p==i; })!=parRef.end()){
		LOG_TRACE(p->pyStr()<<": already in parRef, skipping.");
		return;
	}
	parRef.push_back(p);
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
void Particle::setAngVel(const Vector3r& p){ checkNodes(); shape->nodes[0]->getData<DemData>().setAngVel(p); }
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


	
void DemField::pyHandleCustomCtorArgs(py::tuple& t, py::dict& d){
	if(d.has_key("par")){
		py::extract<vector<shared_ptr<Particle>>> ex(d["par"]);
		if(!ex.check()) throw std::runtime_error("DemField(par=...) must be a sequence of Particles.");
		for(const auto& p: ex()) particles->pyAppend(p);
		py::api::delitem(d,"par");
		this->collectNodes();
	}
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


Real DemField::critDt() {
	// fake shared_ptr's to work around issues with enable_from_this (crash, perhaps related to boost::python?)
	shared_ptr<Scene> s(scene,woo::Object::null_deleter());
	shared_ptr<DemField> f(this,woo::Object::null_deleter());
	return DemFuncs::pWaveDt(f,/*noClumps*/true);
}


vector<shared_ptr<Node>> DemField::splitNode(const shared_ptr<Node>& node, const vector<shared_ptr<Particle>>& pp, const Real massMult, const Real inertiaMult){
	auto& dyn=node->getData<DemData>();
	if(dyn.isClump()) throw std::runtime_error("Node may not be a clump.");
	if(dyn.isClumped()) throw std::runtime_error("Node may not be a clumped.");
	vector<shared_ptr<Node>> ret(2); ret[0]=node;
	shared_ptr<Node> clone=static_pointer_cast<Node>(Master::instance().deepcopy(node));
	ret[1]=clone;
	auto& dyn2=clone->getData<DemData>();
	dyn2.linIx=-1; // not in DemField.nodes yet

	for(const auto& p: pp){
		for(std::list<Particle*>::iterator I=dyn.parRef.begin(); I!=dyn.parRef.end(); I++){
			Particle* ref=*I;
			if(ref!=p.get()) continue;
			// swap node of the particle itself (do this first, as it may fail due to bad user data, and we don't mess particle references uselessly in that case)
			auto N=boost::range::find_if(p->shape->nodes,[&node](shared_ptr<Node>& n){ return n.get()==node.get(); });
			if(N==p->shape->nodes.end()) throw std::runtime_error("Node "+node->pyStr()+" not in shape.nodes of "+p->pyStr());
			*N=clone;
			// remove from dyn.parRef
			dyn.parRef.erase(I);
			// add to dyn2.parRef
			dyn2.parRef.push_back(ref);
			break; // don't iterate further, the iterator is invalidated by deletion
		}
	}
	if(!isnan(massMult)){ dyn.mass*=massMult; dyn2.mass*=massMult; }
	if(!isnan(inertiaMult)){ dyn.inertia*=inertiaMult; dyn2.inertia*=inertiaMult; }
	pyNodesAppend(clone); // add to DemField.nodes
	return ret;
}


int DemField::collectNodes(){
	std::set<void*> seen;
	// ack nodes which are already there
	for(const auto& n: nodes) seen.insert((void*)n.get());
	int added=0;
	// from particles
	for(const auto& p: *particles){
		if(!p || !p->shape || p->shape->nodes.empty()) continue;
		for(const shared_ptr<Node>& n: p->shape->nodes){
			if(seen.count((void*)n.get())!=0) continue; // node already seen
			seen.insert((void*)n.get());
			n->getData<DemData>().linIx=nodes.size();
			nodes.push_back(n);
			added++;
		};
	};
	return added;
}

void DemField::pyNodesAppend(const shared_ptr<Node>& n){
	if(!n) throw std::runtime_error("DemField.nodesAppend: Node to be added may not be None.");
	if(!n->hasData<DemData>()) throw std::runtime_error("DemField.nodesAppend: Node must define Node.dem (DemData)");
	auto& dyn=n->getData<DemData>();
	if(dyn.linIx>=0 && dyn.linIx<(int)nodes.size() && (n.get()==nodes[dyn.linIx].get())) throw std::runtime_error("Node already in DemField.nodes["+to_string(dyn.linIx)+"], refusing to add it again.");
	n->getData<DemData>().linIx=nodes.size();
	nodes.push_back(n);
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
		// remove particle back-reference from each node
		DemData& dyn=n->getData<DemData>();
		if(dyn.parRef.empty()) throw std::runtime_error("#"+to_string(id)+" has node which back-references no particle!");
		auto I=std::find_if(dyn.parRef.begin(),dyn.parRef.end(),[&p](const Particle* w)->bool{ return w==p.get(); });
		if(I==dyn.parRef.end()) throw std::runtime_error("#"+to_string(id)+": node does not back-reference its own particle!");
		// remove parRef to this particle
		dyn.parRef.erase(I); 
		// no particle left, delete the node itself as well
		if(dyn.parRef.empty()){
			if(dyn.linIx<0) continue; // node not in DemField.nodes
			if(dyn.linIx>(int)nodes.size() || nodes[dyn.linIx].get()!=n.get()) throw std::runtime_error("Node in #"+to_string(id)+" has invalid linIx entry!");
			LOG_DEBUG("Removing #"<<id<<" / DemField::nodes["<<dyn.linIx<<"]"<<" (not used anymore)");
			boost::mutex::scoped_lock lock(nodesMutex);
			if(saveDeadNodes) deadNodes.push_back(n);
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

void DemField::removeClump(size_t linIx){
	if(linIx>nodes.size()) throw std::runtime_error("DemField.removeClump("+to_string(linIx)+"): invalid index.");
	if(!nodes[linIx]) throw std::runtime_error("DemField.removeClump: DemField.nodes["+to_string(linIx)+"]=None.");
	const auto& node=nodes[linIx];
	assert(node->hasData<DemData>());
	assert(dynamic_pointer_cast<ClumpData>(node->getDataPtr<DemData>()));
	ClumpData& cd=node->getData<DemData>().cast<ClumpData>();
	std::set<Particle::id_t> delPar;
	for(const auto& n: cd.nodes){
		for(Particle* p: n->getData<DemData>().parRef){
			assert(p && p->shape && p->shape->nodes.size()>0);
			for(auto& n: p->shape->nodes){
				#ifdef WOO_DEBUG
					if(std::find_if(cd.nodes.begin(),cd.nodes.end(),[&n](const shared_ptr<Node>& a)->bool{ return(a.get()==n.get()); })==cd.nodes.end()) throw std::runtime_error("#"+to_string(p->id)+" should contain node at "+lexical_cast<string>(n->pos.transpose()));
				#endif
				n->getData<DemData>().setNoClump(); // fool the test in removeParticle
			}
			// collect particles in a set, in case they share nodes etc, to not delete one multiple times
			delPar.insert(p->id);
		}
	}
	for(const auto& pId: delPar) removeParticle(pId);
	if(saveDeadNodes) deadNodes.push_back(node);
	// remove the clump node here
	boost::mutex::scoped_lock lock(nodesMutex);
	(*nodes.rbegin())->getData<DemData>().linIx=cd.linIx;
	nodes[cd.linIx]=*nodes.rbegin();
	nodes.resize(nodes.size()-1);
}

void DemField::selfTest(){
	// check that particle's nodes reference the particles they belong to
	for(size_t i=0; i<particles->size(); i++){
		const auto& p=(*particles)[i];
		if(!p) continue;
		if(p->id!=(Particle::id_t)i) throw std::logic_error("DemField.par["+to_string(i)+"].id="+to_string(p->id)+", should be "+to_string(i));
		if(!p->shape) throw std::runtime_error("DemField.par["+to_string(i)+"].shape=None.");
		if(!p->material) throw std::runtime_error("DemField.par["+to_string(i)+"].material=None.");
		if(!(p->material->density>0)) throw std::runtime_error("DemField.par["+to_string(i)+"].material.density="+to_string(p->material->density)+", should be positive.");
		if(!p->shape->numNodesOk()) throw std::logic_error("DemField.par["+to_string(i)+"].shape: numNodesOk failed with "+p->shape->pyStr()+".");
		p->selfTest();
		for(size_t j=0; j<p->shape->nodes.size(); j++){
			const auto& n=p->shape->nodes[j];
			if(!n) throw std::logic_error("DemField.par["+to_string(p->id)+"].shape.nodes["+to_string(j)+"]=None.");
			if(!n->hasData<DemData>()) throw std::logic_error("DemField.par["+to_string(p->id)+"].shape.nodes["+to_string(j)+"] does not define DemData.");
			const auto& dyn=n->getData<DemData>();
			if(std::find(dyn.parRef.begin(),dyn.parRef.end(),p.get())==dyn.parRef.end()) throw std::logic_error("DemField.par["+to_string(p->id)+"].shape.nodes["+to_string(j)+"].dem.parRef: does not back-reference the particle "+p->pyStr()+" it belongs to.");
			// clumps
			if(dyn.isClumped() && !dyn.master.lock()) throw std::logic_error("DemField.par["+to_string(p->id)+"].shape.nodes["+to_string(j)+"].dem.master=None, but the node claims to be clumped.");
			if(!dyn.isClumped() && dyn.master.lock()) throw std::logic_error("DemField.par["+to_string(p->id)+"].shape.nodes["+to_string(j)+"].dem.clumped=False, but the node defines a master node (at this point, there is no other valid use of the master node except as clump; this may change in the future).");
		}
	}
	// check that DemField.nodes properly keep their indices
	for(size_t i=0; i<nodes.size(); i++){
		const auto& n=nodes[i];
		if(!n) throw std::logic_error("DemField.nodes["+to_string(i)+"]=None, DemField.nodes should be contiguous");
		if(!n->hasData<DemData>()) throw std::logic_error("DemField.nodes["+to_string(i)+"] does not define DemData.");
		const auto& dyn=n->getData<DemData>();
		if(dyn.linIx!=(int)i) throw std::logic_error("DemField.nodes["+to_string(i)+"].dem.linIx="+to_string(dyn.linIx)+", should be "+to_string(i)+" (use DemField.nodesAppend instead of DemField.nodes.append to have linIx set automatically in python)");
		if(dyn.isClump()){
			if(!dynamic_pointer_cast<ClumpData>(n->getDataPtr<DemData>())) throw std::logic_error("DemField.nodes["+to_string(i)+".dem.clump=True, but does not define ClumpData.");
			const auto& cd=*static_pointer_cast<ClumpData>(n->getDataPtr<DemData>());
			for(size_t j=0; j<cd.nodes.size(); j++){
				const auto& nn=cd.nodes[j];
				if(!nn) throw std::logic_error("DemField.nodes["+to_string(i)+"].dem.nodes["+to_string(j)+"]=None (node is a clump).");
				if(!nn->hasData<DemData>()) throw std::logic_error("DemField.nodes["+to_string(i)+"].dem.nodes["+to_string(j)+"].dem=None (node is a clump).");
				const auto& dyn2=nn->getData<DemData>();
				if(!dyn2.isClumped()) throw std::logic_error("DemField.nodes["+to_string(i)+"].dem.nodes["+to_string(j)+"].clumped=False, should be True (node is a clump).");
				if(!dyn2.master.lock()) throw std::logic_error("DemField.nodes["+to_string(i)+"].dem.nodes["+to_string(j)+"].master=None, but the node claims to be clumped.");
				if(dyn2.master.lock().get()!=n.get()) throw std::logic_error("DemField.nodes["+to_string(i)+"].dem.nodes["+to_string(j)+"].master!=DemField.nodes["+to_string(i)+"].");
			}
		}
		// check parRef
		size_t j=0;
		for(Particle* p: dyn.parRef){
			if(!p) throw std::logic_error("DemField.nodes["+to_string(i)+"].dem.parRef["+to_string(j)+"]==NULL."); //very unlikely
			if(!p->shape) throw std::logic_error("DemField.nodes["+to_string(i)+"].dem.parRef["+to_string(j)+"].shape=None (#"+to_string(p->id)+"): clumps may not meaningfully contain particles without shape.");
			if(boost::range::find_if(
				p->shape->nodes,
				[&n](const shared_ptr<Node>& a){return a.get()==n.get();}
			)==p->shape->nodes.end()) throw std::logic_error("DemField.nodes["+to_string(i)+"].dem.parRef["+to_string(j)+"].shape.nodes does not contain DemField.nodes["+to_string(i)+"] (the node back-references the particle, but the particle does not reference the node).");
		}
	}
}

