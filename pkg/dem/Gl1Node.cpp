

#ifdef YADE_OPENGL
#include<yade/pkg/dem/Gl1Node.hpp>
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
			// if(axesLabels) GLUtils::GLDrawText(string(i==0?"x":(i==1?"y":"z")),pt,color);
		}
	}
	glLineWidth(1);
};

#endif

