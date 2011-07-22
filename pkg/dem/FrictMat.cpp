#include<yade/pkg/dem/FrictMat.hpp>
#include<yade/core/MatchMaker.hpp>
#include<yade/pkg/dem/L6Geom.hpp>

YADE_PLUGIN(dem,(ElastMat)(FrictMat)(FrictPhys)(Cp2_FrictMat_FrictPhys));

void Cp2_FrictMat_FrictPhys::go(const shared_ptr<Material>& m1, const shared_ptr<Material>& m2, const shared_ptr<Contact>& C){
	if(C->phys) return;
	C->phys=shared_ptr<CPhys>(new FrictPhys);
	FrictPhys& ph=C->phys->cast<FrictPhys>();
	FrictMat& mat1=m1->cast<FrictMat>(); FrictMat& mat2=m2->cast<FrictMat>();
	assert(dynamic_cast<L6Geom*>(C->geom.get()));
	const Vector2r& len=C->geom->cast<L6Geom>().lens;
	ph.kn=1/(1/(mat1.young*2*std::abs(len[0]))+1/(mat2.young*2*len[1]));
	ph.kt=ktDivKn*ph.kn;
	ph.tanPhi=(!tanPhi)?std::min(mat1.tanPhi,mat2.tanPhi):(*tanPhi)(mat1.id,mat2.id,mat1.tanPhi,mat2.tanPhi);
};

