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
		const int& ax(cyl.axis);
		glLineWidth(1);
		assert(cyl.nodes.size()==1);
		Vector3r A(Vector3r::Zero()),B(Vector3r::Zero());
		if(isnan(cyl.glAB[0])||isnan(cyl.glAB[1])){
			Vector3r mid=cyl.nodes[0]->pos; mid[ax]=viewInfo.sceneCenter[ax];
			GLUtils::setLocalCoords(mid,cyl.nodes[0]->ori);
			A[ax]=-viewInfo.sceneRadius;
			B[ax]=+viewInfo.sceneRadius;
		} else {
			GLUtils::setLocalCoords(cyl.nodes[0]->pos,cyl.nodes[0]->ori);
			A[ax]=cyl.nodes[0]->pos[ax]+cyl.glAB[0];
			B[ax]=cyl.nodes[0]->pos[ax]+cyl.glAB[1];
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

