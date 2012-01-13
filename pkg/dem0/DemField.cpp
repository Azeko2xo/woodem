#include<yade/pkg/dem/DemField.hpp>
#include<yade/pkg/common/Facet.hpp>
#include<yade/pkg/common/Sphere.hpp>
YADE_PLUGIN(dem,(DemField)(DemNodeData)(StateNodeXCompat));

void StateNodeXCompat::action(){
	FOREACH(const shared_ptr<Body>& b, *scene->bodies){
		// only update spheres, facets just follow them
		if(!((bool)dynamic_cast<Sphere*>(b->shape.get()))) continue;
		for(size_t i=0; i<b->nodes.size(); i++){
			b->nodes[i]->pos+=(b->state->pos-b->state->refPos);
			b->nodes[i]->ori=(b->state->refOri.conjugate()*b->state->ori)*b->nodes[i]->ori;
		}
		b->state->refPos=b->state->pos;
		b->state->refOri=b->state->refOri;
	}
	// now cycle again, to update Shape of deformed particles (facets), so that corners coincide with nodes
	FOREACH(const shared_ptr<Body>& b, *scene->bodies){
		Facet* fac=dynamic_cast<Facet*>(b->shape.get());
		if(!fac) continue;
		std::tie(b->state->pos,b->state->ori)=fac->updateGlobalVertices(b->nodes[0]->pos,b->nodes[1]->pos,b->nodes[2]->pos);
	}
}


#ifdef YADE_OPENGL

#include<yade/lib/opengl/GLUtils.hpp>

YADE_PLUGIN(gl,(Gl1_Node));

int Gl1_Node::wd;
Vector2i Gl1_Node::wd_range;
Real Gl1_Node::len;
Vector2r Gl1_Node::len_range;

void Gl1_Node::go(const shared_ptr<Node>& node, const GLViewInfo& viewInfo){
	if(wd<=0) return;
	glLineWidth(wd);
	if(len<=0){
		glPointSize(wd);
		glBegin(GL_POINTS); glVertex3f(0,0,0); glEnd();
	} else {
		for(int i=0; i<3; i++){
			Vector3r pt=Vector3r::Zero(); pt[i]=len*viewInfo.sceneRadius; Vector3r color=.3*Vector3r::Ones(); color[i]=1;
			GLUtils::GLDrawLine(Vector3r::Zero(),pt,color);
			//if(axesLabels) GLUtils::GLDrawText(string(i==0?"x":(i==1?"y":"z")),pt,color);
		}
	}
};


#endif
