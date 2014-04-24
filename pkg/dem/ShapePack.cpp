#include<woo/pkg/dem/ShapePack.hpp>
#include<woo/pkg/dem/Clump.hpp>

#include<woo/pkg/dem/Sphere.hpp>
#include<woo/pkg/dem/Capsule.hpp>
#include<woo/pkg/dem/Ellipsoid.hpp>

#ifdef WOO_OPENGL
	#include<woo/pkg/gl/Renderer.hpp>
#endif

#include<boost/filesystem/convenience.hpp>
#include<boost/tokenizer.hpp>


WOO_PLUGIN(dem,(RawShape)(ShapePack));
WOO_IMPL__CLASS_BASE_DOC_ATTRS_PY(woo_dem_ShapePack__CLASS_BASE_DOC_ATTRS_PY);
WOO_IMPL__CLASS_BASE_DOC_ATTRS(woo_dem_RawShape__CLASS_BASE_DOC_ATTRS);

RawShape::RawShape(const shared_ptr<Shape>& sh){
	sh->asRaw(center,radius,raw);
	className=sh->getClassName();
}

shared_ptr<Shape> RawShape::toShape(Real density) const {
	shared_ptr<Shape> ret;
	if(className=="Sphere") ret=make_shared<Sphere>();
	else if(className=="Ellipsoid") ret=make_shared<Ellipsoid>();
	else if(className=="Capsule") ret=make_shared<Capsule>();
	else throw std::invalid_argument("RawShape.toShape: className '"+className+"' is not supported.");
	ret->setFromRaw(center,radius,raw);
	if(!isnan(density)){
		for(const auto& n: ret->nodes){
			n->setData<DemData>(make_shared<DemData>());
			#ifdef WOO_OPENGL
				// to avoid crashes if renderer must resize the node's data array and reallocates it while other thread accesses those data
				n->setData<GlData>(make_shared<GlData>());
			#endif
		}
		ret->updateMassInertia(density);
	}
	return ret;
}

#if 0
void ShapePack::sort(const int& ax, const Real& trimVol){
	std::sort(rawShapes.begin(),shapes.end(),[](const shared_ptr<Shape>& a, const shared_ptr<Shape>& b)->bool{ return a.avgNodePos()[ax]<b.avgNodesPos()[ax]; });
	if(!trimVol>0) return;
	Real vol=0;
	for(size_t i=0; i<shapes.size(); i++){
		vol+=shapes[i]->volume();
		if(vol>trimVol){
			// discard remaining particles above this level
			shapes.resize(i+1);
			return;
		}
	}
};
#endif

void ShapePack::fromDem(const shared_ptr<Scene>& scene, const shared_ptr<DemField>& dem, int mask){
	rawShapes.clear();
	for(const auto& n: dem->nodes){
		assert(n->hasData<DemData>());
		const auto& dyn(n->getData<DemData>());
		if(!dyn.isA<ClumpData>()){
			for(const Particle* p: dyn.parRef){
				if(mask!=0 && (mask&p->mask)==0) continue;
				rawShapes.push_back({make_shared<RawShape>(p->shape)});
			}
		} else {
			const auto& clump=static_pointer_cast<ClumpData>(n->getDataPtr<DemData>());
			vector<shared_ptr<RawShape>> clumpSS;
			for(const auto& n2: clump->nodes){
				const auto& dyn2(n2->getData<DemData>());
				for(const Particle* p: dyn2.parRef){
					if(mask!=0 && (mask&p->mask)==0) continue;
					clumpSS.push_back(make_shared<RawShape>(p->shape));
				}
			}
			rawShapes.push_back(clumpSS);
		}
	}
}

void ShapePack::toDem(const shared_ptr<Scene>& scene, const shared_ptr<DemField>& dem, const shared_ptr<Material>& mat, int mask, Real color){
	if(cellSize!=Vector3r::Zero()) throw std::runtime_error("ShapePack.toDem: cellSize (PBC) not supported yet.");
	for(const auto& ss: rawShapes){
		if(ss.empty()) continue; // ignore no-shape entries, just to make sure
		Real _color=(isnan(color)?Mathr::UnitRandom():color);
		vector<shared_ptr<Particle>> pp; pp.reserve(ss.size());
		for(size_t i=0; i<ss.size(); i++){
			auto p=make_shared<Particle>();
			p->shape=ss[i]->toShape(/*density*/mat->density);
			p->shape->color=_color;
			p->material=mat;
			p->mask=mask;
			pp.push_back(p);
			for(const auto& n: p->shape->nodes) n->getData<DemData>().parRef.push_back(p.get());
		}
		if(pp.size()==1){ // insert a solo particle
			const auto& p(pp[0]);
			dem->particles->pyAppend(p); // node not appended here
			for(const auto& n: p->shape->nodes) dem->pyNodesAppend(n);
		}
		// insert clump
		else dem->particles->pyAppendClumped(pp); // this appends node automatically
	}
}

void ShapePack::postLoad(ShapePack&,void* attr){
	if(!loadFrom.empty()){
		if(attr==NULL){
			loadTxt(loadFrom);
			loadFrom.clear();
		} else if(attr==&loadFrom){ loadFrom.clear(); throw std::runtime_error("ShapePack.loadFrom: may only be specified at construction-time."); }
	};
}

void ShapePack::add(vector<shared_ptr<Shape>>& shsh){
	vector<shared_ptr<RawShape>> ss;
	for(const auto& sh: shsh) ss.push_back(make_shared<RawShape>(sh));
	rawShapes.push_back(ss);
}

void ShapePack::saveTxt(const string& out) const {
	std::ofstream f(out.c_str());
	if(!f.good()) throw std::runtime_error("SpherePack.saveTxt: unable to open '"+out+"'.");
	if(cellSize!=Vector3r::Zero()){ f<<"##PERIODIC:: "<<cellSize[0]<<" "<<cellSize[1]<<" "<<cellSize[2]<<std::endl; }
	if(!userData.empty()) {
		if(userData.find('\n')!=string::npos) throw std::runtime_error("SpherePack.saveTxt: userData must not contain newline.");
		f<<"##USER-DATA:: "<<userData<<std::endl;
	}
	for(size_t i=0; i<rawShapes.size(); i++){
		for(const auto& ss: rawShapes[i]){
			f<<i<<" "<<ss->className<<" "<<ss->center[0]<<" "<<ss->center[1]<<" "<<ss->center[2]<<" "<<ss->radius;
			for(const Real& r: ss->raw) f<<" "<<r;
			f<<endl;
		}
	}
}

void ShapePack::loadTxt(const string& in) {
	if(!boost::filesystem::exists(in)) throw std::invalid_argument("ShapePack.loadTxt: '"+in+"' doesn't exist.");
	std::ifstream f(in);
	if(!f.good()) throw std::runtime_error("ShapePack.loadTxt: '"+in+"' couldn't be opened (but exists).");
	rawShapes.clear();
	movable=false;
	cellSize=Vector3r::Zero();

	string line;
	size_t lineNo=0;
	int lastId=-1;
	while(std::getline(f,line,'\n')){
		lineNo++;
		if(boost::algorithm::starts_with(line,"##USER-DATA:: ")){
			userData=line.substr(14,string::npos); // strip the line header
			continue;
		}
		boost::tokenizer<boost::char_separator<char> > toks(line,boost::char_separator<char>(" \t"));
		vector<string> tokens; for(const string& s: toks) tokens.push_back(s);
		if(tokens[0]=="##PERIODIC::"){
			if(tokens.size()!=4) throw std::invalid_argument(in+":"+to_string(lineNo)+": starts with ##PERIODIC:: but the line is malformed.");
			cellSize=Vector3r(lexical_cast<Real>(tokens[1]),lexical_cast<Real>(tokens[2]),lexical_cast<Real>(tokens[3]));
			continue;
		}
		// strip all tokens after first comment
		for(size_t i=0; i<tokens.size(); i++){ if(tokens[i][0]=='#'){ tokens.resize(i); break; }}
		if(tokens.empty()) continue;
		if(tokens.size()<6) throw std::invalid_argument(in+":"+to_string(lineNo)+": insufficient number of columns ("+to_string(tokens.size())+")");
		int id=lexical_cast<int>(tokens[0]);
		if(id!=lastId && id!=rawShapes.size()){
			LOG_WARN(in<<":"<<lineNo<<": shape numbers not contiguous.");
		}
		// creating a new particle
		if(id!=lastId){
			rawShapes.push_back(vector<shared_ptr<RawShape>>());
			lastId=id;
		}
		// read particle data, no checking whatsoever
		auto ss=make_shared<RawShape>();
		ss->className=tokens[1];
		ss->center=Vector3r(lexical_cast<Real>(tokens[2]),lexical_cast<Real>(tokens[3]),lexical_cast<Real>(tokens[4]));
		ss->radius=lexical_cast<Real>(tokens[5]);
		ss->raw.reserve(tokens.size()-6);
		for(size_t r=6; r<tokens.size(); r++) ss->raw.push_back(lexical_cast<Real>(tokens[r]));
		rawShapes.rbegin()->push_back(ss);
	}
}


