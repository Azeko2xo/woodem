// © 2009 Václav Šmilauer <eudoxos@arcig.cz>


#include<woo/lib/sphere-pack/SpherePack.hpp>

#include<boost/random/linear_congruential.hpp>
#include<boost/random/uniform_real.hpp>
#include<boost/random/variate_generator.hpp>

#include<boost/filesystem/convenience.hpp>
#include<boost/tokenizer.hpp>
#include<boost/algorithm/string.hpp>

#include<boost/chrono/chrono.hpp>


#include<iostream>
#include<fstream>

#include<map>
#include<list>
#include<vector>

#include<time.h>

CREATE_LOGGER(SpherePack);

using boost::lexical_cast;
using std::string;
using std::invalid_argument;

// seed for random numbers
unsigned long long getNow(){
	return boost::chrono::duration_cast<boost::chrono::nanoseconds>(boost::chrono::steady_clock::now().time_since_epoch()).count();
}


void SpherePack::fromList(const py::list& l){
	pack.clear();
	size_t len=py::len(l);
	for(size_t i=0; i<len; i++){
		const py::tuple& t=py::extract<py::tuple>(l[i]);
		py::extract<Vector3r> vec(t[0]);
		if(vec.check()) { pack.push_back(Sph(vec(),py::extract<double>(t[1]),(py::len(t)>2?py::extract<int>(t[2]):-1))); continue; }
		woo::TypeError("List elements must be (Vector3, float) or (Vector3, float, int)!");
	}
};

void SpherePack::fromLists(const vector<Vector3r>& centers, const vector<Real>& radii){
	pack.clear();
	if(centers.size()!=radii.size()) throw std::invalid_argument(("The same number of centers and radii must be given (is "+lexical_cast<string>(centers.size())+", "+lexical_cast<string>(radii.size())+")").c_str());
	size_t l=centers.size();
	for(size_t i=0; i<l; i++){
		add(centers[i],radii[i]);
	}
	cellSize=Vector3r::Zero();
}

py::list SpherePack::toList() const {
	py::list ret; for(const Sph& s: pack) ret.append(s.asTuple()); return ret;
};

py::tuple SpherePack::toCcRr() const {
	py::list cc, rr;
	for(size_t i=0; i<pack.size(); i++){
		const auto& s(pack[i]);
		if(s.clumpId>=0) throw std::runtime_error("SpherePack.toCcRr(): Sphere "+to_string(i)+" is in clump "+to_string(i)+", refusing to lose information.");
		cc.append(s.c); rr.append(s.r);
	}
	return py::make_tuple(cc,rr);
}

void SpherePack::fromFile(const string& fname) {
	if(!boost::filesystem::exists(fname)) {
		throw std::invalid_argument(string("File with spheres `")+fname+"' doesn't exist.");
	}
	std::ifstream sphereFile(fname.c_str());
	if(!sphereFile.good()) throw std::runtime_error("File with spheres `"+fname+"' couldn't be opened.");
	pack.clear();
	cellSize=Vector3r::Zero();

	Vector3r C;
	Real r=0;
	string line;
	size_t lineNo=0;
	size_t nCols=0;
	while(std::getline(sphereFile,line,'\n')){
		lineNo++;
		if(boost::algorithm::starts_with(line,"##USER-DATA:: ")){
			userData=line.substr(14,string::npos); // strip the line header
			continue;
		}
		boost::tokenizer<boost::char_separator<char> > toks(line,boost::char_separator<char>(" \t"));
		vector<string> tokens; FOREACH(const string& s, toks) tokens.push_back(s);
		if(tokens.empty()) continue;
		if(tokens[0]=="##PERIODIC::"){
			if(tokens.size()!=4) throw std::invalid_argument(("Spheres file "+fname+":"+lexical_cast<string>(lineNo)+" contains ##PERIODIC::, but the line is malformed.").c_str());
			cellSize=Vector3r(lexical_cast<Real>(tokens[1]),lexical_cast<Real>(tokens[2]),lexical_cast<Real>(tokens[3]));
			continue;
		}
		// 4 or 5 columns, but all lines must have the same
		// it is the clump number
		if(tokens.size()!=4 && tokens.size()!=5) throw std::invalid_argument(fname+":"+to_string(lineNo)+": line has "+to_string(tokens.size())+" columns (must be 4 or 5).");
		if(nCols==0) nCols=tokens.size(); // set the first time
		if(nCols!=tokens.size()) throw std::invalid_argument(fname+":"+to_string(lineNo)+": all line must have the same number of columns (previous had "+to_string(nCols)+", this one has "+to_string(tokens.size())+")");
		C=Vector3r(lexical_cast<Real>(tokens[0]),lexical_cast<Real>(tokens[1]),lexical_cast<Real>(tokens[2]));
		r=lexical_cast<Real>(tokens[3]);
		pack.push_back(Sph(C,r));
		// if clumpId was specified, set it now
		if(tokens.size()==5) pack.rbegin()->clumpId=lexical_cast<int>(tokens[4]);
	}
}

void SpherePack::toFile(const string& fname) const {
	std::ofstream f(fname.c_str());
	if(!f.good()) throw std::runtime_error("Unable to open file `"+fname+"'");
	if(cellSize!=Vector3r::Zero()){ f<<"##PERIODIC:: "<<cellSize[0]<<" "<<cellSize[1]<<" "<<cellSize[2]<<std::endl; }
	if(!userData.empty()) {
		if(userData.find('\n')!=string::npos) throw std::runtime_error("SpherePack.userData must not contain newline.");
		f<<"##USER-DATA:: "<<userData<<std::endl;
	}
	bool clumps=hasClumps();
	FOREACH(const Sph& s, pack){
		f<<s.c[0]<<" "<<s.c[1]<<" "<<s.c[2]<<" "<<s.r;
		if(clumps) f<<" "<<s.clumpId;
		f<<std::endl;
	}
	f.close();
};

long SpherePack::makeCloud(Vector3r mn, Vector3r mx, Real rMean, Real rRelFuzz, int num, bool periodic, Real porosity, const vector<Real>& psdSizes, const vector<Real>& psdCumm, bool distributeMass, int seed, Matrix3r hSize){
	static boost::minstd_rand randGen(seed!=0?seed:(int)getNow());
	static boost::variate_generator<boost::minstd_rand&, boost::uniform_real<Real> > rnd(randGen, boost::uniform_real<Real>(0,1));
	vector<Real> psdRadii; // holds plain radii (rather than diameters), scaled down in some situations to get the target number
	vector<Real> psdCumm2; // psdCumm but dimensionally transformed to match mass distribution	
	Vector3r size;
	bool hSizeFound =(hSize!=Matrix3r::Zero());//is hSize passed to the function?
	if (!hSizeFound) {size=mx-mn; hSize=size.asDiagonal();}
	if (hSizeFound && !periodic) LOG_WARN("hSize can be defined only for periodic cells.");
	Matrix3r invHsize =hSize.inverse();
	Real volume=hSize.determinant();
	if (!volume) throw std::invalid_argument("The box defined as null volume. Define at least maxCorner of the box, or hSize if periodic.");
	int mode=-1; bool err=false;
	// determine the way we generate radii
	if(porosity<=0) {LOG_WARN("porosity must be >0, changing it for you. It will be ineffective if rMean>0."); porosity=0.5;}
	//If rMean is not defined, then in will be defined in RDIST_NUM
	if(rMean>0) mode=RDIST_RMEAN;
	else if(num>0 && psdSizes.size()==0) {
		mode=RDIST_NUM;
		// the term (1+rRelFuzz²) comes from the mean volume for uniform distribution : Vmean = 4/3*pi*Rmean*(1+rRelFuzz²) 
		rMean=pow(volume*(1-porosity)/(M_PI*(4/3.)*(1+rRelFuzz*rRelFuzz)*num),1/3.);}
	if(psdSizes.size()>0){
		err=(mode>=0); mode=RDIST_PSD;
		if(psdSizes.size()!=psdCumm.size()) throw std::invalid_argument(("SpherePack.makeCloud: psdSizes and psdCumm must have same dimensions ("+lexical_cast<string>(psdSizes.size())+"!="+lexical_cast<string>(psdCumm.size())).c_str());
		if(psdSizes.size()<=1) throw invalid_argument("SpherePack.makeCloud: psdSizes must have at least 2 items");
		if((*psdCumm.begin())!=0. && (*psdCumm.rbegin())!=1.) throw invalid_argument("SpherePack.makeCloud: first and last items of psdCumm *must* be exactly 0 and 1.");
		psdRadii.reserve(psdSizes.size());
		for(size_t i=0; i<psdSizes.size(); i++) {
			psdRadii.push_back(/* radius, not diameter */ .5*psdSizes[i]);
			if(distributeMass) {
				//psdCumm2 is first obtained by integrating the number of particles over the volumic PSD (dN/dSize = totV*(dPassing/dSize)*1/(4/3πr³)). I suspect similar reasoning behind pow3Interp, since the expressions are a bit similar. The total cumulated number will be the number of spheres in volume*(1-porosity), it is used to decide if the PSD will be scaled down. psdCumm2 is normalized below in order to fit in [0,1]. (B.C.)
				if (i==0) psdCumm2.push_back(0);
				else psdCumm2.push_back(psdCumm2[i-1] + 3.0*volume*(1-porosity)/M_PI*(psdCumm[i]-psdCumm[i-1])/(psdSizes[i]-psdSizes[i-1])*(pow(psdSizes[i-1],-2)-pow(psdSizes[i],-2)));
			}
			LOG_DEBUG(i<<". "<<psdRadii[i]<<", cdf="<<psdCumm[i]<<", cdf2="<<(distributeMass?lexical_cast<string>(psdCumm2[i]):string("--")));
			// check monotonicity
			if(i>0 && (psdSizes[i-1]>psdSizes[i] || psdCumm[i-1]>psdCumm[i])) throw invalid_argument("SpherePack:makeCloud: psdSizes and psdCumm must be both non-decreasing.");
		}
		if(distributeMass) {
			if (num>0) {
				//if target number will not fit in (1-poro)*volume, scale down particles size
				if (psdCumm2[psdSizes.size()-1]<num){
					appliedPsdScaling=pow(psdCumm2[psdSizes.size()-1]/num,1./3.);
					for(size_t i=0; i<psdSizes.size(); i++) psdRadii[i]*=appliedPsdScaling;}
			}
			//Normalize psdCumm2 so it's between 0 and 1
			for(size_t i=1; i<psdSizes.size(); i++) psdCumm2[i]/=psdCumm2[psdSizes.size()-1];
		}
	}
	if(err || mode<0) throw invalid_argument("SpherePack.makeCloud: at least one of rMean, porosity, psdSizes & psdCumm arguments must be specified. rMean can't be combined with psdSizes.");
	// adjust uniform distribution parameters with distributeMass; rMean has the meaning (dimensionally) of _volume_
	const int maxTry=1000;
	if(periodic)(cellSize=size);
	Real r=0;
	for(int i=0; (i<num) || (num<0); i++) {
		Real norm, rand;
		//Determine radius of the next sphere we will attempt to place in space. If (num>0), generate radii the deterministic way, in decreasing order, else radii are stochastic since we don't know what the final number will be
		if (num>0) rand = ((Real)num-(Real)i+0.5)/((Real)num+1.);
		else rand = rnd();
		int t;
		switch(mode){
			case RDIST_RMEAN:
				//FIXME : r is never defined, it will be zero at first iteration, but it will have values in the next ones. Some magic?
			case RDIST_NUM:
				if(distributeMass) r=pow3Interp(rand,rMean*(1-rRelFuzz),rMean*(1+rRelFuzz));
				else r=rMean*(2*(rand-.5)*rRelFuzz+1); // uniform distribution in rMean*(1±rRelFuzz)
				break;
			case RDIST_PSD:
				if(distributeMass){
					int piece=psdGetPiece(rand,psdCumm2,norm);
					r=pow3Interp(norm,psdRadii[piece],psdRadii[piece+1]);
				} else {
					int piece=psdGetPiece(rand,psdCumm,norm);
					r=psdRadii[piece]+norm*(psdRadii[piece+1]-psdRadii[piece]);}
		}
		// try to put the sphere into a free spot
		for(t=0; t<maxTry; ++t){
			Vector3r c;
			if(!periodic) { for(int axis=0; axis<3; axis++) c[axis]=mn[axis]+r+(size[axis]-2*r)*rnd(); }
			else { 	for(int axis=0; axis<3; axis++) c[axis]=rnd();//coordinates in [0,1]
				c=mn+hSize*c;}//coordinates in reference frame (inside the base cell)
			size_t packSize=pack.size(); bool overlap=false;
			if(!periodic) for(size_t j=0;j<packSize;j++) {if(pow(pack[j].r+r,2)>=(pack[j].c-c).squaredNorm()) {overlap=true; break;}}
			else {
				for(size_t j=0; j<packSize; j++){
					Vector3r dr=Vector3r::Zero();
					if (!hSizeFound) {//The box is axis-aligned, use the wrap methods
						for(int axis=0; axis<3; axis++) dr[axis]=min(cellWrapRel(c[axis],pack[j].c[axis],pack[j].c[axis]+size[axis]),cellWrapRel(pack[j].c[axis],c[axis],c[axis]+size[axis]));
					} else {//not aligned, find closest neighbor in a cube of size 1, then transform distance to cartesian coordinates
						Vector3r c1c2=invHsize*(pack[j].c-c);
						for(int axis=0; axis<3; axis++){
							if (abs(c1c2[axis])<abs(c1c2[axis] - Mathr::Sign(c1c2[axis]))) dr[axis]=c1c2[axis];
							else dr[axis] = c1c2[axis] - Mathr::Sign(c1c2[axis]);}
						dr=hSize*dr;//now in cartesian coordinates
					}
					if(pow(pack[j].r+r,2)>= dr.squaredNorm()){ overlap=true; break; }
				}
			}
			if(!overlap) { pack.push_back(Sph(c,r)); break; }
		}
		if (t==maxTry) {
			if(num>0) {
				if (mode!=RDIST_RMEAN) {
					Real nextPoro = porosity+(1-porosity)/10.;
					LOG_WARN("Exceeded "<<maxTry<<" tries to insert non-overlapping sphere to packing. Only "<<i<<" spheres was added, although you requested "<<num<<". Trying again with porosity "<<nextPoro<<". The size distribution is being scaled down");
					pack.clear();
					return makeCloud(mn, mx, -1., rRelFuzz, num, periodic, nextPoro, psdSizes, psdCumm, distributeMass,seed,hSizeFound?hSize:Matrix3r::Zero());}
				else LOG_WARN("Exceeded "<<maxTry<<" tries to insert non-overlapping sphere to packing. Only "<<i<<" spheres was added, although you requested "<<num<<".");
			}
			return i;}
	}
	if (appliedPsdScaling<1) LOG_WARN("The size distribution has been scaled down by a factor pack.appliedPsdScaling="<<appliedPsdScaling);
	return pack.size();
}

void SpherePack::cellFill(Vector3r vol){
	Vector3i count;
	for(int i=0; i<3; i++) count[i]=(int)(ceil(vol[i]/cellSize[i]));
	LOG_DEBUG("Filling volume "<<vol<<" with cell "<<cellSize<<", repeat counts are "<<count);
	cellRepeat(count);
}

void SpherePack::cellRepeat(Vector3i count){
	if(cellSize==Vector3r::Zero()){ throw std::runtime_error("cellRepeat cannot be used on non-periodic packing."); }
	if(count[0]<=0 || count[1]<=0 || count[2]<=0){ throw std::invalid_argument("Repeat count components must be positive."); }
	size_t origSize=pack.size();
	pack.reserve(origSize*count[0]*count[1]*count[2]);
	for(int i=0; i<count[0]; i++){
		for(int j=0; j<count[1]; j++){
			for(int k=0; k<count[2]; k++){
				if((i==0) && (j==0) && (k==0)) continue; // original cell
				Vector3r off(cellSize[0]*i,cellSize[1]*j,cellSize[2]*k);
				for(size_t l=0; l<origSize; l++){
					const Sph& s=pack[l]; pack.push_back(Sph(s.c+off,s.r));
				}
			}
		}
	}
	cellSize=Vector3r(cellSize[0]*count[0],cellSize[1]*count[1],cellSize[2]*count[2]);
}

int SpherePack::psdGetPiece(Real x, const vector<Real>& cumm, Real& norm){
	int sz=cumm.size(); int i=0;
	while(i<sz && cumm[i]<=x) i++; // upper interval limit index
	if((i==sz-1) && cumm[i]<=x){ i=sz-2; norm=1.; return i;}
	i--; // lower interval limit intex
	norm=(x-cumm[i])/(cumm[i+1]-cumm[i]);
	//LOG_TRACE("count="<<sz<<", x="<<x<<", piece="<<i<<" in "<<cumm[i]<<"…"<<cumm[i+1]<<", norm="<<norm);
	return i;
}

py::tuple SpherePack::psd(int bins, bool mass) const {
	if(pack.size()==0) return py::make_tuple(py::list(),py::list()); // empty packing
	// find extrema
	Real minD=std::numeric_limits<Real>::infinity(); Real maxD=-minD;
	// volume, but divided by π*4/3
	Real vol=0; long N=pack.size();
	FOREACH(const Sph& s, pack){ maxD=max(2*s.r,maxD); minD=min(2*s.r,minD); vol+=pow(s.r,3); }
	if(minD==maxD){ minD-=.5; maxD+=.5; } // emulates what numpy.histogram does
	// create bins and bin edges
	vector<Real> hist(bins,0); vector<Real> cumm(bins+1,0); /* cummulative values compute from hist at the end */
	vector<Real> edges(bins+1); for(int i=0; i<=bins; i++){ edges[i]=minD+i*(maxD-minD)/bins; }
	// weight each grain by its "volume" relative to overall "volume"
	FOREACH(const Sph& s, pack){
		int bin=int(bins*(2*s.r-minD)/(maxD-minD)); bin=min(bin,bins-1); // to make sure
		if (mass) hist[bin]+=pow(s.r,3)/vol; else hist[bin]+=1./N;
	}
	for(int i=0; i<bins; i++) cumm[i+1]=min(1.,cumm[i]+hist[i]); // cumm[i+1] is OK, cumm.size()==bins+1
	return py::make_tuple(edges,cumm);
}

bool SpherePack::hasClumps() const { for(const Sph& s: pack){ if(s.clumpId>=0) return true; } return false; }

py::tuple SpherePack::getClumps() const{
	std::map<int,list<int>> clumps;
	py::list standalone; size_t packSize=pack.size();
	for(size_t i=0; i<packSize; i++){
		const Sph& s(pack[i]);
		if(s.clumpId<0) { standalone.append(i); continue; }
		clumps[s.clumpId].push_back(i);
	}
	py::list clumpList;
	for(const auto& idList: clumps){
		py::list ids;
		for(const int& id: idList.second){
			ids.append(id);
		}
		clumpList.append(ids);
	}
	return py::make_tuple(standalone,clumpList); 
}


int SpherePack::addShadows(){
	size_t sz0=pack.size();
	if(cellSize==Vector3r::Zero()) throw std::runtime_error("SpherePack.addShadows is not meaningful on aperiodic packing.");
	// check there are no shadows yet
	for(size_t i=0; i<sz0; i++){ if (pack[i].shadowOf>=0) throw std::runtime_error((boost::format("SpherePack.addShadows: %d is a shadow of %d; remove shadows (SpherePack.removeShadows) before calling addShadows.")%i%pack[i].shadowOf).str()); }
	int ret=0;
	for(size_t i=0; i<sz0; i++){
		Sph& s=pack[i];
		// check that points are in canonical positions
		for(int j=0;j<3;j++){
			if(s.c[j]<0 || s.c[j]>cellSize[j]){
				if(s.clumpId>=0) throw std::runtime_error((boost::format("SpherePack.addShadows: %d in non-canonical position %s, but cannot be canonicalized as it is member of clump %d")%i%s.c%s.clumpId).str());
				Real norm=s.c[j]/cellSize[j]; s.c[j]=(norm-floor(norm))*cellSize[j];
			}
		}
		Vector3i mn, mx;
		for(int j:{0,1,2}){ mn[j]=(s.c[j]+s.r>cellSize[j]?-1:0); mx[j]=(s.c[j]-s.r<0?1:0); }
		Vector3i n;
		for(n[0]=mn[0]; n[0]<=mx[0]; n[0]++) for(n[1]=mn[1]; n[1]<=mx[1]; n[1]++) for(n[2]=mn[2]; n[2]<=mx[2]; n[2]++){
			if(n==Vector3i::Zero()) continue; // in the middle
			pack.push_back(Sph(s.c+Vector3r(n[0]*cellSize[0],n[1]*cellSize[1],n[2]*cellSize[2]),s.r,/*clumpId*/-1,/*shadowOf*/i));
			ret++;
		}
	};
	return ret;
}

void SpherePack::canonicalize(){
	if(cellSize==Vector3r::Zero()) throw std::runtime_error("SpherePack.canonicalize: only meaningful on periodic packings");
	if(!hasClumps()){
		for(Sph& s: pack){
			for(int i:{0,1,2}) s.c[i]=cellWrapRel(s.c[i],0,cellSize[i]);
		}
	} else {
		// canonicalize mean positions instead
		std::map<int,std::list<int>> cl;
		for(size_t i=0; i<pack.size(); i++){
			const Sph& s(pack[i]);
			if(s.clumpId>=0) cl[s.clumpId].push_back(i);
		}
		// move clumps
		for(const auto& cidId: cl){
			// compute mean position
			Vector3r x=Vector3r::Zero();
			for(const auto& id: cidId.second) x+=pack[id].c;
			x/=cidId.second.size();
			Vector3r x2(x); 
			for(int i:{0,1,2}) x2[i]=cellWrapRel(x2[i],0,cellSize[i]);
			Vector3r dx=x2-x;
			for(const auto& id: cidId.second) pack[id].c+=dx;
		}
		// move standalone spheres
		for(Sph& s: pack){
			if(s.clumpId<0){ for(int i:{0,1,2}) s.c[i]=cellWrapRel(s.c[i],0,cellSize[i]); }
		}
	}
}



int SpherePack::removeShadows(){
	int ret=0;
	for(long i=(long)pack.size(); i>=0; i--){
		if(pack[i].shadowOf>=0){ pack.erase(pack.begin()+i); ret++; }
	}
	return ret;
};

void SpherePack::scale(Real scale, bool keepRadius){
	bool periodic=(cellSize!=Vector3r::Zero());
	Vector3r mid=periodic?Vector3r::Zero():midPt();
	cellSize*=scale;
	FOREACH(Sph& s, pack){
		s.c=scale*(s.c-mid)+mid;
		if(!keepRadius) s.r*=abs(scale);
	}
}

Real SpherePack::maxRelOverlap(){
	/*
	For current distance d₀ and overlap z=r₁+r₂-d₀, determine scaling coeff s=(1+relOverlap) such that d₀s=r₁+r₂ (i.e. scaling packing by s will result in no overlap).
	With s=z/d₀,  we have d₀s=d₀(z/d₀+1)=z+d₀ which is r₁+r₂.
	Therefore this function returns max(z/d₀)=max((r₁+r₂-d₀)/d₀).
	*/
	size_t sz=pack.size();
	bool peri=(cellSize!=Vector3r::Zero());
	Real ret=0.;
	for(size_t i=0; i<sz; i++){
		for(size_t j=i+1; j<sz; j++){
			const Sph &s1=pack[i]; const Sph& s2=pack[j];
			// squared distance - normal or wrapped, if periodic
			Real sqDist=peri?periPtDistSq(s1.c,s2.c):(s1.c-s2.c).squaredNorm();
			// sphere in a clump, and within the same clump
			if(s1.clumpId>=0 && s1.clumpId==s2.clumpId) continue;
			// too far from each other
			if(sqDist>pow(s1.r+s2.r,2)) continue;
			// compute relative overlap
			Real dist=sqrt(sqDist);
			ret=max(ret,(s1.r+s2.r-dist)/dist);
		}
	}
	return ret;
}
