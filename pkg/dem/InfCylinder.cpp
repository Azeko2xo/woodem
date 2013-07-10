// © 2009 Václav Šmilauer <eudoxos@arcig.cz>
#include<woo/pkg/dem/InfCylinder.hpp>
#include<woo/pkg/dem/ParticleContainer.hpp>
#include<limits>

WOO_PLUGIN(dem,(InfCylinder)(Bo1_InfCylinder_Aabb));
#ifdef WOO_OPENGL
	WOO_PLUGIN(gl,(Gl1_InfCylinder))
#endif

void Bo1_InfCylinder_Aabb::go(const shared_ptr<Shape>& sh){
	InfCylinder& cyl=sh->cast<InfCylinder>();
	assert(cyl.axis>=0 && cyl.axis<3);
	if(!cyl.bound){ cyl.bound=make_shared<Aabb>(); }
	assert(cyl.numNodesOk());
	Aabb& aabb=cyl.bound->cast<Aabb>();
	if(scene->isPeriodic) throw logic_error(__FILE__ "InfCylinder not supported in periodic cell.");
	Vector3r& pos=cyl.nodes[0]->pos;
	short ax0=cyl.axis, ax1=(cyl.axis+1)%3, ax2=(cyl.axis+2)%3;
	aabb.min[ax0]=-Inf; aabb.max[ax0]=Inf;
	aabb.min[ax1]=pos[ax1]-cyl.radius; aabb.max[ax1]=pos[ax1]+cyl.radius;
	aabb.min[ax2]=pos[ax2]-cyl.radius; aabb.max[ax2]=pos[ax2]+cyl.radius;
	return;
}

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
		glLineWidth(1);
		assert(cyl.nodes.size()==1);
		Vector3r A(Vector3r::Zero()),B(Vector3r::Zero());
		if(isnan(cyl.glAB[0])||isnan(cyl.glAB[1])){
			Vector3r mid=cyl.nodes[0]->pos; mid[cyl.axis]=viewInfo.sceneCenter[cyl.axis];
			GLUtils::setLocalCoords(mid,cyl.nodes[0]->ori);
			A[cyl.axis]=-viewInfo.sceneRadius;
			B[cyl.axis]=+viewInfo.sceneRadius;
		} else {
			GLUtils::setLocalCoords(cyl.nodes[0]->pos,cyl.nodes[0]->ori);
			A[cyl.axis]=cyl.nodes[0]->pos[cyl.axis]+cyl.glAB[0];
			B[cyl.axis]=cyl.nodes[0]->pos[cyl.axis]+cyl.glAB[1];
		}
		glDisable(GL_LINE_SMOOTH);
		GLUtils::Cylinder(A,B,cyl.radius,/*keep current color*/Vector3r(NaN,NaN,NaN),/*wire*/wire||wire2,/*caps*/false,/*rad2*/-1,slices);
		if(spokes){
			glBegin(GL_LINES);
				int ax1((cyl.axis+1)%3),ax2((cyl.axis+2)%3);
				for(int i=0; i<slices; i++){
					Vector3r A1=A, B1=B;
					A1[ax1]=B1[ax1]=cyl.radius*cos(i*(2.*M_PI/slices));
					A1[ax2]=B1[ax2]=cyl.radius*sin(i*(2.*M_PI/slices));
					glVertex3v(A); glVertex3v(A1);
					glVertex3v(B); glVertex3v(B1);
				}
			glEnd();
		}
		glEnable(GL_LINE_SMOOTH);
	}
#endif

