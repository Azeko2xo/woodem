#include<woo/pkg/dem/FrictMat.hpp>
#include<woo/core/MatchMaker.hpp>
#include<woo/pkg/dem/L6Geom.hpp>
#include<woo/pkg/dem/G3Geom.hpp>

WOO_PLUGIN(dem,(ElastMat)(FrictMat)(FrictPhys)(Cp2_FrictMat_FrictPhys));

WOO_IMPL__CLASS_BASE_DOC_ATTRS_CTOR(woo_dem_ElastMat__CLASS_BASE_DOC_ATTRS_CTOR);
WOO_IMPL__CLASS_BASE_DOC_ATTRS_CTOR(woo_dem_FrictMat__CLASS_BASE_DOC_ATTRS_CTOR);
WOO_IMPL__CLASS_BASE_DOC_ATTRS_CTOR(woo_dem_FrictPhys__CLASS_BASE_DOC_ATTRS_CTOR);
WOO_IMPL__CLASS_BASE_DOC_ATTRS(woo_dem_Cp2_FrictMat_FrictPhys__CLASS_BASE_DOC_ATTRS);

void Cp2_FrictMat_FrictPhys::go(const shared_ptr<Material>& m1, const shared_ptr<Material>& m2, const shared_ptr<Contact>& C){
	if(!C->phys) C->phys=make_shared<FrictPhys>();
	updateFrictPhys(m1->cast<FrictMat>(),m2->cast<FrictMat>(),C->phys->cast<FrictPhys>(),C);
	//FrictPhys& ph=C->phys->cast<FrictPhys>();
	//FrictMat& mat1=m1->cast<FrictMat>(); FrictMat& mat2=m2->cast<FrictMat>();
}

void Cp2_FrictMat_FrictPhys::updateFrictPhys(FrictMat& mat1, FrictMat& mat2, FrictPhys& ph, const shared_ptr<Contact>& C){
	// fast variant, for L6Geom only
	#if 1
		assert(dynamic_cast<L6Geom*>(C->geom.get()));
		const auto& l6g=C->geom->cast<L6Geom>();
		Real l0=l6g.lens[0],l1=l6g.lens[1], A=l6g.contA;
	#else
		Real l0,l1,A;
		L6Geom* l6g=dynamic_cast<L6Geom*>(C->geom.get());
		if(l6g){ l0=std::abs(l6g->lens[0]); l1=std::abs(l6g->lens[1]); A=l6g->contA; }
		else{
			assert(dynamic_pointer_cast<Sphere>(C->leakPB()->shape));
			l1=std::abs(C->leakPB()->shape->cast<Sphere>().radius);
			if(!dynamic_pointer_cast<Sphere>(C->leakPA()->shape)) l0=l1; // wall-sphere contact, for instance
			else l0=std::abs(C->leakPA()->shape->cast<Sphere>().radius);
			A=M_PI*pow(min(l0,l1),2);
		}
	#endif
	//ph.kn=1/(1/(mat1.young*2*std::abs(l0))+1/(mat2.young*2*l1));
	ph.kn=1/(1/(mat1.young*A/l0)+1/(mat2.young*A/l1));
	// cerr<<"l0="<<l0<<", l1="<<l1<<", A="<<A<<", E1="<<mat1.young<<", E2="<<mat2.young<<", kN="<<ph.kn<<endl;
	//ph.kt=ktDivKn*ph.kn;
	ph.kt=.5*(mat1.ktDivKn+mat2.ktDivKn)*ph.kn;
	ph.tanPhi=(!tanPhi)?std::min(mat1.tanPhi,mat2.tanPhi):(*tanPhi)(mat1.id,mat2.id,mat1.tanPhi,mat2.tanPhi);
};

