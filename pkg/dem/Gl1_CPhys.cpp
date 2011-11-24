#include<yade/pkg/dem/Gl1_CPhys.hpp>
#include<yade/pkg/gl/Renderer.hpp> // for GlData
#include<yade/lib/opengl/GLUtils.hpp>

YADE_PLUGIN(gl,(Gl1_CPhys));

GLUquadric* Gl1_CPhys::gluQuadric=NULL;
shared_ptr<ScalarRange> Gl1_CPhys::range;
shared_ptr<ScalarRange> Gl1_CPhys::shearRange;
bool Gl1_CPhys::shearColor;
int Gl1_CPhys::signFilter;
int Gl1_CPhys::slices;
Vector2i Gl1_CPhys::slices_range;
Real Gl1_CPhys::relMaxRad;

void Gl1_CPhys::go(const shared_ptr<CPhys>& cp, const shared_ptr<Contact>& C, const GLViewInfo& viewInfo){
	if(!range) return;
	if(shearColor && !shearRange) shearColor=false;
	// shared_ptr<CGeom> cg(C->geom); if(!cg) return; // changed meanwhile?
	Real fn=cp->force[0];
	if(isnan(fn)) return;
	if((signFilter>0 && fn<0) || (signFilter<0 && fn>0)) return;
	range->norm(fn); // adjust range, return value discarded
	Real r=relMaxRad*viewInfo.sceneRadius*(abs(fn)/(max(abs(range->mnmx[0]),abs(range->mnmx[1]))));
	Vector3r color=shearColor?shearRange->color(Vector2r(cp->force[1],cp->force[2]).norm()):range->color(fn);
	Vector3r A=C->pA->shape->avgNodePos(), B=C->pB->shape->avgNodePos()+((scene->isPeriodic)?(scene->cell->intrShiftPos(C->cellDist)):Vector3r::Zero());
	if(C->pA->shape->nodes[0]->hasData<GlData>() && C->pB->shape->nodes[0]->hasData<GlData>()){
		const GlData &glA=C->pA->shape->nodes[0]->getData<GlData>(), &glB=C->pB->shape->nodes[0]->getData<GlData>();
		A+=glA.dGlPos; B+=glB.dGlPos;
		 // dGlPos already contains cell shift, cancel it here
		if(scene->isPeriodic){
			A+=scene->cell->intrShiftPos(glA.dCellDist);
			B+=scene->cell->intrShiftPos(glB.dCellDist);
		}
	}
	GLUtils::Cylinder(A,B,r,color,/*wire*/false,/*caps*/false,/*rad2*/-1,slices);
}
