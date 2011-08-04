#include<yade/pkg/dem/FrictMat.hpp>
#include<yade/core/MatchMaker.hpp>
#include<yade/pkg/dem/L6Geom.hpp>
#include<yade/pkg/dem/G3Geom.hpp>

YADE_PLUGIN(dem,(ElastMat)(FrictMat)(FrictPhys)(Cp2_FrictMat_FrictPhys));

void Cp2_FrictMat_FrictPhys::go(const shared_ptr<Material>& m1, const shared_ptr<Material>& m2, const shared_ptr<Contact>& C){
	if(C->phys) return;
	C->phys=shared_ptr<CPhys>(new FrictPhys);
	FrictPhys& ph=C->phys->cast<FrictPhys>();
	FrictMat& mat1=m1->cast<FrictMat>(); FrictMat& mat2=m2->cast<FrictMat>();
	// fast avriant, for L6Geom only
	#if 0
		assert(dynamic_cast<L6Geom*>(C->geom.get()));
		const Vector2r& len=C->geom->cast<L6Geom>().lens;
		Real l0=len[0],l1=len[1];
	#else
		// slower variant, to be compiled if G3Geom or similar is used
		Real l0,l1;
		L6Geom* l6g=dynamic_cast<L6Geom*>(C->geom.get());
		if(l6g){ l0=l6g->lens[0]; l1=l6g->lens[1]; }
		else{
			assert(dynamic_pointer_cast<Sphere>(C->pA->shape));
			assert(dynamic_pointer_cast<Sphere>(C->pB->shape));
			l0=C->pA->shape->cast<Sphere>().radius;
			l1=C->pA->shape->cast<Sphere>().radius;
		}
	#endif
	ph.kn=1/(1/(mat1.young*2*std::abs(l0))+1/(mat2.young*2*l1));
	ph.kt=ktDivKn*ph.kn;
	ph.tanPhi=(!tanPhi)?std::min(mat1.tanPhi,mat2.tanPhi):(*tanPhi)(mat1.id,mat2.id,mat1.tanPhi,mat2.tanPhi);
};

