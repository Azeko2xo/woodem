#include<yade/pkg/dem/Particle.hpp>
#include<yade/pkg/dem/ParticleContainer.hpp>
#include<yade/pkg/dem/ContactContainer.hpp>
#include<yade/pkg/dem/ContactLoop.hpp>
#include<yade/lib/pyutil/except.hpp>

YADE_PLUGIN(dem,(CPhys)(CGeom)(CData)(DemField)(Particle)(DemData)(Contact)(Shape)(Material)(Bound)(ContactContainer));

py::dict Particle::pyContacts(){	py::dict ret; FOREACH(MapParticleContact::value_type i,contacts) ret[i.first]=i.second; return ret;}
py::list Particle::pyCon(){ py::list ret; FOREACH(MapParticleContact::value_type i,contacts) ret.append(i.first); return ret;}
py::list Particle::pyTacts(){	py::list ret; FOREACH(MapParticleContact::value_type i,contacts) ret.append(i.second); return ret;}

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

Particle::id_t Contact::pyId1(){ return pA->id; }
Particle::id_t Contact::pyId2(){ return pB->id; }

void Particle::checkNodes(bool dyn, bool checkOne){
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
	#define _SET_DOF(DOF_ANY,ch) if((blocked & DemData::DOF_ANY)!=0) ret.push_back(ch);
	_SET_DOF(DOF_X,'x'); _SET_DOF(DOF_Y,'y'); _SET_DOF(DOF_Z,'z'); _SET_DOF(DOF_RX,'X'); _SET_DOF(DOF_RY,'Y'); _SET_DOF(DOF_RZ,'Z');
	#undef _SET_DOF
	return ret;
}

void DemData::blocked_vec_set(const std::string& dofs){
	blocked=0;
	FOREACH(char c, dofs){
		#define _GET_DOF(DOF_ANY,ch) if(c==ch) { blocked|=DemData::DOF_ANY; continue; }
		_GET_DOF(DOF_X,'x'); _GET_DOF(DOF_Y,'y'); _GET_DOF(DOF_Z,'z'); _GET_DOF(DOF_RX,'X'); _GET_DOF(DOF_RY,'Y'); _GET_DOF(DOF_RZ,'Z');
		#undef _GET_DOF
		throw std::invalid_argument("Invalid  DOF specification `"+lexical_cast<string>(c)+"' in '"+dofs+"', characters must be ∈{x,y,z,X,Y,Z}.");
	}
}




vector<shared_ptr<Node> > Particle::getNodes(){ checkNodes(false,false); return shape->nodes; }

Vector3r& Particle::getPos(){ checkNodes(false); return shape->nodes[0]->pos; };
void Particle::setPos(const Vector3r& p){ checkNodes(false); shape->nodes[0]->pos=p; }
Quaternionr& Particle::getOri(){ checkNodes(false); return shape->nodes[0]->ori; };
void Particle::setOri(const Quaternionr& p){ checkNodes(false); shape->nodes[0]->ori=p; }

Vector3r& Particle::getVel(){ checkNodes(); return shape->nodes[0]->getData<DemData>().vel; };
void Particle::setVel(const Vector3r& p){ checkNodes(); shape->nodes[0]->getData<DemData>().vel=p; }
Vector3r& Particle::getAngVel(){ checkNodes(); return shape->nodes[0]->getData<DemData>().angVel; };
void Particle::setAngVel(const Vector3r& p){ checkNodes(); shape->nodes[0]->getData<DemData>().angVel=p; }

Vector3r Particle::getForce(){ checkNodes(); return shape->nodes[0]->getData<DemData>().force; };
Vector3r Particle::getTorque(){ checkNodes(); return shape->nodes[0]->getData<DemData>().torque; };

std::string Particle::getBlocked(){ checkNodes(); return shape->nodes[0]->getData<DemData>().blocked_vec_get(); }
void Particle::setBlocked(const std::string& s){ checkNodes(); shape->nodes[0]->getData<DemData>().blocked_vec_set(s); }




int DemField::collectNodes(bool clear, bool dynOnly){
	std::set<void*> seen;
	if(clear) nodes.clear();
	else FOREACH(const shared_ptr<Node>& n, nodes) seen.insert((void*)n.get());
	int added=0;
	FOREACH(const shared_ptr<Particle>& p, particles){
		if(!p || !p->shape || p->shape->nodes.empty()) continue;
		FOREACH(const shared_ptr<Node>& n, p->shape->nodes){
			if(seen.count((void*)n.get())!=0) continue; // node already seen
			if(dynOnly && !n->hasData<DemData>()) continue; // doesn't have DemData
			seen.insert((void*)n.get());
			nodes.push_back(n);
			added++;
		};
	};
	return added;
}


