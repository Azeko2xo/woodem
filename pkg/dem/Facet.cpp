#include<yade/pkg/dem/Facet.hpp>
#include<yade/lib/base/CompUtils.hpp>
YADE_PLUGIN(dem,(Facet)(Bo1_Facet_Aabb));
#ifdef YADE_OPENGL
YADE_PLUGIN(gl,(Gl1_Facet));
#endif

Vector3r Facet::getNormal() const {
	assert(numNodesOk());
	return ((nodes[1]->pos-nodes[0]->pos).cross(nodes[2]->pos-nodes[0]->pos)).normalized();
}

std::tuple<Vector3r,Vector3r,Vector3r> Facet::getOuterVectors() const {
	assert(numNodesOk());
	// is not normalized
	Vector3r nn=(nodes[1]->pos-nodes[0]->pos).cross(nodes[2]->pos-nodes[0]->pos);
	return std::make_tuple((nodes[1]->pos-nodes[0]->pos).cross(nn),(nodes[2]->pos-nodes[1]->pos).cross(nn),(nodes[0]->pos-nodes[2]->pos).cross(nn));
}

std::tuple<Vector3r,Vector3r> Facet::interpolatePtLinAngVel(const Vector3r& x) const {
	assert(numNodesOk());
	Vector3r a=CompUtils::facetBarycentrics(x,nodes[0]->pos,nodes[1]->pos,nodes[2]->pos);
	Vector3r vv[3]={(nodes[0]->getData<DemData>().vel,nodes[1]->getData<DemData>().vel,nodes[2]->getData<DemData>().vel)};
	Vector3r linVel=a[0]*vv[0]+a[1]*vv[1]+a[2]*vv[2];
	Vector3r angVel=(nodes[0]->pos-x).cross(vv[0])+(nodes[1]->pos-x).cross(vv[1])+(nodes[2]->pos-x).cross(vv[2]);
	return std::make_tuple(linVel,angVel);
}


void Bo1_Facet_Aabb::go(const shared_ptr<Shape>& sh){
	Facet& f=sh->cast<Facet>();
	if(!f.bound){ f.bound=make_shared<Aabb>(); }
	Aabb& aabb=f.bound->cast<Aabb>();
	if(!scene->isPeriodic){
		aabb.min=aabb.max=f.nodes[0]->pos;
		for(int i:{1,2}){
			aabb.min=aabb.min.array().min(f.nodes[i]->pos.array()).matrix();
			aabb.max=aabb.max.array().max(f.nodes[i]->pos.array()).matrix();
		}
	} else {
		// periodic cell: unshear everything
		aabb.min=aabb.max=scene->cell->unshearPt(f.nodes[0]->pos);
		for(int i:{1,2}){
			Vector3r v=scene->cell->unshearPt(f.nodes[i]->pos);
			aabb.min=aabb.min.array().min(v.array()).matrix();
			aabb.max=aabb.max.array().max(v.array()).matrix();
		}
	}
}


#ifdef YADE_OPENGL
#include<yade/lib/opengl/OpenGLWrapper.hpp>
#include<yade/lib/opengl/GLUtils.hpp>
#include<yade/pkg/gl/Renderer.hpp>
#include<yade/lib/base/CompUtils.hpp>

bool Gl1_Facet::wire;

void Gl1_Facet::go(const shared_ptr<Shape>& sh, const Vector3r& shift, bool wire2, const GLViewInfo&){   
	Facet& f=sh->cast<Facet>();
	glColor3v(CompUtils::mapColor(f.getBaseColor()));

	if(wire || wire2){
		glBegin(GL_LINE_LOOP);
			for(int i:{0,1,2}) glVertex3v(f.nodes[i]->pos);
	    glEnd();
	} else {
		glDisable(GL_CULL_FACE); 
		Vector3r normal=f.getNormal();
		glBegin(GL_TRIANGLES);
			 // this makes every triangle different WRT the light direction; important for shading
			glNormal3v(normal);
			for(int i:{0,1,2}) glVertex3v(f.nodes[i]->pos);
		glEnd();
	}
}

#endif /* YADE_OPENGL */

