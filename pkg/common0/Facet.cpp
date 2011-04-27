/*************************************************************************
*  Copyright (C) 2008 by Sergei Dorofeenko				 *
*  sega@users.berlios.de                                                 *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/
#include<yade/pkg/common/Facet.hpp>
#include<yade/pkg/common/Aabb.hpp>
YADE_PLUGIN(dem,(Facet)(Bo1_Facet_Aabb));
#ifdef YADE_OPENGL
YADE_PLUGIN(gl,(Gl1_Facet));
#endif

CREATE_LOGGER(Facet);

Facet::~Facet(){}

void Facet::postLoad(Facet&)
{
	// if this fails, it means someone did vertices push_back, but they are resized to 3 at Facet initialization already
	// in the future, a fixed-size array should be used instead of vector<Vector3r> for vertices
	// this is prevented by yade::serialization now IIRC
	if(vertices.size()!=3){ throw runtime_error(("Facet must have exactly 3 vertices (not "+lexical_cast<string>(vertices.size())+")").c_str()); }
	if(isnan(vertices[0][0])) return;  // not initialized, nothing to do
	Vector3r e[3] = {vertices[1]-vertices[0] ,vertices[2]-vertices[1] ,vertices[0]-vertices[2]};
	#define CHECK_EDGE(i) if(e[i].squaredNorm()==0){LOG_FATAL("Facet has coincident vertices "<<i<<" ("<<vertices[i]<<") and "<<(i+1)%3<<" ("<<vertices[(i+1)%3]<<")!");}
		CHECK_EDGE(0); CHECK_EDGE(1);CHECK_EDGE(2);
	#undef CHECK_EDGE
	normal = e[0].cross(e[1]); normal.normalize();
	for(int i=0; i<3; ++i){
		ne[i]=e[i].cross(normal); ne[i].normalize();
		vl[i]=vertices[i].norm();
		vu[i]=vertices[i]/vl[i];
	}
	Real p = e[0].norm()+e[1].norm()+e[2].norm();
	icr = e[0].norm()*ne[0].dot(e[2])/p;
}

boost::tuple<Vector3r,Quaternionr> Facet::updateGlobalVertices(const Vector3r& v0, const Vector3r& v1, const Vector3r& v2){
	Vector3r inscribedCenter(v0+((v2-v0)*(v1-v0).norm()+(v1-v0)*(v2-v0).norm())/((v1-v0).norm()+(v2-v1).norm()+(v0-v2).norm()));
	vertices[0]=v0-inscribedCenter;
	vertices[1]=v1-inscribedCenter;
	vertices[2]=v2-inscribedCenter;
	postLoad(*this);
	return boost::make_tuple(inscribedCenter,Quaternionr::Identity());
};



void Bo1_Facet_Aabb::go(const shared_ptr<Shape>& cm, shared_ptr<Bound>& bv, const Se3r& se3, const Body* b){
	if(!bv){ bv=shared_ptr<Bound>(new Aabb); }
	Aabb* aabb=static_cast<Aabb*>(bv.get());
	Facet* facet = static_cast<Facet*>(cm.get());
	const Vector3r& O = se3.position;
	Matrix3r facetAxisT=se3.orientation.toRotationMatrix();
	const vector<Vector3r>& vertices=facet->vertices;
	if(!scene->isPeriodic){
		aabb->min=aabb->max = O + facetAxisT * vertices[0];
		for (int i=1;i<3;++i)
		{
			Vector3r v = O + facetAxisT * vertices[i];
			aabb->min = aabb->min.cwise().min(v);
			aabb->max = aabb->max.cwise().max(v);
		}
	} else {
		Real inf=std::numeric_limits<Real>::infinity();
		aabb->min=Vector3r(inf,inf,inf); aabb->max=Vector3r(-inf,-inf,-inf);
		for(int i=0; i<3; i++){
			Vector3r v=scene->cell->unshearPt(O+facetAxisT*vertices[i]);
			aabb->min=aabb->min.cwise().min(v);
			aabb->max=aabb->max.cwise().max(v);
		}
	}
}



#ifdef YADE_OPENGL
#include<yade/lib/opengl/OpenGLWrapper.hpp>

bool Gl1_Facet::normals=false;

void Gl1_Facet::go(const shared_ptr<Shape>& cm, const shared_ptr<State>& ,bool wire,const GLViewInfo&)
{   
	Facet* facet = static_cast<Facet*>(cm.get());
	const vector<Vector3r>& vertices = facet->vertices;
	const Vector3r* ne = facet->ne;
	const Real& icr = facet->icr;

	if(cm->wire || wire){
		// facet
		glBegin(GL_LINE_LOOP);
			glColor3v(normals ? Vector3r(1,0,0): cm->color);
		   glVertex3v(vertices[0]);
		   glVertex3v(vertices[1]);
		   glVertex3v(vertices[2]);
	    glEnd();
		if(!normals) return;
		// facet's normal 
		glBegin(GL_LINES);
			glColor3(0.0,0.0,1.0); 
			glVertex3(0.0,0.0,0.0);
			glVertex3v(facet->normal);
		glEnd();
		// normal of edges
		glColor3(0.0,0.0,1.0); 
		glBegin(GL_LINES);
			glVertex3(0.0,0.0,0.0); glVertex3v(Vector3r(icr*ne[0]));
			glVertex3(0.0,0.0,0.0);	glVertex3v(Vector3r(icr*ne[1]));
			glVertex3(0.0,0.0,0.0);	glVertex3v(Vector3r(icr*ne[2]));
		glEnd();
	} else {
		glDisable(GL_CULL_FACE); 
		Vector3r normal=(facet->vertices[1]-facet->vertices[0]).cross(facet->vertices[2]-facet->vertices[1]); normal.normalize();
		glColor3v(cm->color);
		glBegin(GL_TRIANGLES);
			glNormal3v(normal); // this makes every triangle different WRT the light direction; important!
			glVertex3v(facet->vertices[0]);
			glVertex3v(facet->vertices[1]);
			glVertex3v(facet->vertices[2]);
		glEnd();
	}
}

#endif /* YADE_OPENGL */

