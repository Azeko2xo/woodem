// © 2009 Václav Šmilauer <eudoxos@arcig.cz>
#include<woo/pkg/dem/InfCylinder.hpp>
#include<woo/pkg/dem/ParticleContainer.hpp>
#include<woo/pkg/dem/Sphere.hpp>
#include<limits>

WOO_PLUGIN(dem,(InfCylinder)(Bo1_InfCylinder_Aabb)(Cg2_InfCylinder_Sphere_L6Geom));
#ifdef WOO_OPENGL
	WOO_PLUGIN(gl,(Gl1_InfCylinder))
#endif
WOO_IMPL__CLASS_BASE_DOC_ATTRS_CTOR(woo_dem_InfCylinder__CLASS_BASE_DOC_ATTRS_CTOR);
WOO_IMPL__CLASS_BASE_DOC(woo_dem_Cg2_InfCylinder_Sphere_L6Geom__CLASS_BASE_DOC);

void InfCylinder::updateMassInertia(const Real& density) const {
	checkNodesHaveDemData();
	auto& dyn(nodes[0]->getData<DemData>());
	dyn.mass=0;
	dyn.inertia=Vector3r(Inf,Inf,Inf);
}


void Bo1_InfCylinder_Aabb::go(const shared_ptr<Shape>& sh){
	InfCylinder& cyl=sh->cast<InfCylinder>();
	assert(cyl.axis>=0 && cyl.axis<3);
	if(!cyl.bound){ cyl.bound=make_shared<Aabb>(); /* ignore node rotation*/ cyl.bound->cast<Aabb>().maxRot=-1;  }
	assert(cyl.numNodesOk());
	Aabb& aabb=cyl.bound->cast<Aabb>();
	if(scene->isPeriodic && scene->cell->hasShear()) throw logic_error(__FILE__ ": InfCylinder not supported in periodic cell with skew (Scene.cell.trsf is not diagonal).");
	Vector3r& pos=cyl.nodes[0]->pos;
	short ax0=cyl.axis, ax1=(cyl.axis+1)%3, ax2=(cyl.axis+2)%3;
	aabb.min[ax0]=-Inf; aabb.max[ax0]=Inf;
	aabb.min[ax1]=pos[ax1]-cyl.radius; aabb.max[ax1]=pos[ax1]+cyl.radius;
	aabb.min[ax2]=pos[ax2]-cyl.radius; aabb.max[ax2]=pos[ax2]+cyl.radius;
	return;
}


WOO_IMPL_LOGGER(Cg2_InfCylinder_Sphere_L6Geom);

bool Cg2_InfCylinder_Sphere_L6Geom::go(const shared_ptr<Shape>& sh1, const shared_ptr<Shape>& sh2, const Vector3r& shift2, const bool& force, const shared_ptr<Contact>& C){
	if(scene->isPeriodic && scene->cell->hasShear()) throw std::logic_error("Cg2_InfCylinder_Sphere_L6Geom does not handle periodic boundary conditions with skew (Scene.cell.trsf is not diagonal).");
	const InfCylinder& cyl=sh1->cast<InfCylinder>(); const Sphere& sphere=sh2->cast<Sphere>();
	assert(cyl.numNodesOk()); assert(sphere.numNodesOk());
	const Real& sphRad=sphere.radius;
	const Real& cylRad=cyl.radius; const int& ax=cyl.axis;
	const Vector3r& cylPos=cyl.nodes[0]->pos; Vector3r sphPos=sphere.nodes[0]->pos+shift2;
	Vector3r relPos=sphPos-cylPos;
	relPos[ax]=0.;
	if(!C->isReal() && relPos.squaredNorm()>pow(cylRad+sphRad,2) && !force){ return false; }
	Real dist=relPos.norm();
	#define CATCH_NAN_FACET_INFCYLINDER
	#ifdef CATCH_NAN_FACET_INFCYLINDER
		if(dist==0.) LOG_FATAL("dist==0.0 between InfCylinder #"<<C->leakPA()->id<<" @ "<<cyl.nodes[0]->pos.transpose()<<", r="<<cylRad<<" and Sphere #"<<C->leakPB()->id<<" @ "<<sphere.nodes[0]->pos.transpose()<<", r="<<sphere.radius);
	#else
		#error You forgot to implement dist==0 handler with InfCylinder
	#endif
	Real uN=dist-(cylRad+sphRad);
	Vector3r normal=relPos/dist;
	Vector3r cylPosAx=cylPos; cylPosAx[ax]=sphPos[ax]; // projected
	Vector3r contPt=cylPosAx+(cylRad+0.5*uN)*normal;

	const DemData& cylDyn(cyl.nodes[0]->getData<DemData>());	const DemData& sphDyn(sphere.nodes[0]->getData<DemData>());
	// check impossible rotations of the infinite cylinder
	assert(cylDyn.angVel[(ax+1)%3]==0. && cylDyn.angVel[(ax+2)%3]==0.);
	handleSpheresLikeContact(C,cylPos,cylDyn.vel,cylDyn.angVel,sphPos,sphDyn.vel,sphDyn.angVel,normal,contPt,uN,cylRad,sphRad);
	return true;
};


#ifdef WOO_OPENGL
	#include<woo/lib/opengl/OpenGLWrapper.hpp>
	#include<woo/pkg/gl/Renderer.hpp>
	#include<woo/lib/base/CompUtils.hpp>
	#include<woo/lib/opengl/GLUtils.hpp>


	bool Gl1_InfCylinder::wire;
	bool Gl1_InfCylinder::spokes;
	int  Gl1_InfCylinder::slices;
	int  Gl1_InfCylinder::stacks;

	void Gl1_InfCylinder::go(const shared_ptr<Shape>& shape, const Vector3r& shift, bool wire2, const GLViewInfo& viewInfo){
		const InfCylinder& cyl=shape->cast<InfCylinder>();
		const int& ax(cyl.axis);
		glLineWidth(1);
		assert(cyl.nodes.size()==1);
		Vector3r A(Vector3r::Zero()),B(Vector3r::Zero());
		if(scene->isPeriodic){
			assert(!scene->cell->hasShear());
			const Vector3r& hSize=scene->cell->getHSize().diagonal();
			Vector3r mid=scene->cell->wrapPt(cyl.nodes[0]->pos); mid[ax]=.5*hSize[ax];
			GLUtils::setLocalCoords(mid,cyl.nodes[0]->ori);
			A[ax]=-.5*hSize[ax];
			B[ax]=+.5*hSize[ax];
		}
		else if(isnan(cyl.glAB.maxCoeff())){
			Vector3r mid=cyl.nodes[0]->pos; mid[ax]=viewInfo.sceneCenter[ax];
			GLUtils::setLocalCoords(mid,cyl.nodes[0]->ori);
			A[ax]=-viewInfo.sceneRadius;
			B[ax]=+viewInfo.sceneRadius;
		} else {
			GLUtils::setLocalCoords(cyl.nodes[0]->pos,cyl.nodes[0]->ori);
			A[ax]=cyl.nodes[0]->pos[ax]+cyl.glAB[0];
			B[ax]=cyl.nodes[0]->pos[ax]+cyl.glAB[1];
		}
		// fast drawing
		if(Renderer::fastDraw){
			glBegin(GL_LINES);
			glVertex3v(A); glVertex3v(B);
			glEnd();
			return;
		}
		glDisable(GL_LINE_SMOOTH);
		GLUtils::Cylinder(A,B,cyl.radius,/*keep current color*/Vector3r(NaN,NaN,NaN),/*wire*/wire||wire2,/*caps*/false,/*rad2*/-1,slices);
		if(spokes){
			int ax1((ax+1)%3),ax2((ax+2)%3);
			for(Real axCoord:{A[ax],B[ax]}){
				// only lines
				glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
				glBegin(GL_TRIANGLE_FAN);
					Vector3r v=Vector3r::Zero(); v[ax]=axCoord;
					glVertex3v(v);
					for(int i=0; i<=slices; i++){
						v[ax1]=cyl.radius*cos(i*(2.*M_PI/slices));
						v[ax2]=cyl.radius*sin(i*(2.*M_PI/slices));
						glVertex3v(v);
					}
				glEnd();
				glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
			}
		}
		glEnable(GL_LINE_SMOOTH);
	}
#endif

