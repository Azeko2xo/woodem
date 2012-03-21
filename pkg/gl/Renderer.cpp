#ifdef YADE_OPENGL

#include<yade/pkg/gl/Renderer.hpp>
#include<yade/lib/opengl/OpenGLWrapper.hpp>
#include<yade/lib/opengl/GLUtils.hpp>
#include<yade/lib/base/CompUtils.hpp>
#include<yade/core/Timing.hpp>
#include<yade/core/Scene.hpp>
#include<yade/core/Field.hpp>

#include<yade/pkg/dem/Particle.hpp>


#include<yade/pkg/sparc/SparcField.hpp>

#include <GL/glu.h>
#include <GL/gl.h>
#include <GL/glut.h>

YADE_PLUGIN(gl,(Renderer)(GlExtraDrawer)(GlData));
CREATE_LOGGER(Renderer);

void GlExtraDrawer::render(){ throw runtime_error("GlExtraDrawer::render called from class "+getClassName()+". (did you forget to override it in the derived class?)"); }

bool Renderer::initDone=false;
const int Renderer::numClipPlanes;

Renderer* Renderer::self=NULL; // pointer to the only existing instance





void Renderer::init(){
	typedef std::pair<string,DynlibDescriptor> strDldPair; // necessary as FOREACH, being macro, cannot have the "," inside the argument (preprocessor does not parse templates)
	#define _TRY_ADD_FUNCTOR(functorT,dispatcher,className) if(Omega::instance().isInheritingFrom_recursive(className,#functorT)){ shared_ptr<functorT> f(static_pointer_cast<functorT>(ClassFactory::instance().createShared(className))); dispatcher.add(f); continue; }
	FOREACH(const strDldPair& item, Omega::instance().getDynlibsDescriptor()){
		_TRY_ADD_FUNCTOR(GlFieldFunctor,fieldDispatcher,item.first);
		_TRY_ADD_FUNCTOR(GlShapeFunctor,shapeDispatcher,item.first);
		_TRY_ADD_FUNCTOR(GlBoundFunctor,boundDispatcher,item.first);
		_TRY_ADD_FUNCTOR(GlNodeFunctor,nodeDispatcher,item.first);
		_TRY_ADD_FUNCTOR(GlCPhysFunctor,cPhysDispatcher,item.first);
	}
	clipPlaneNormals.resize(numClipPlanes);
	static bool glutInitDone=false;
	if(!glutInitDone){
		char* argv[]={};
		int argc=0;
		glutInit(&argc,argv);
		//glutInitDisplayMode(GLUT_DOUBLE);
		/* transparent spheres (still not working): glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE | GLUT_ALPHA); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE); */
		glutInitDone=true;
	}
	initDone=true;
}

bool Renderer::pointClipped(const Vector3r& p){
	if(numClipPlanes<1) return false;
	for(int i=0;i<numClipPlanes;i++) if(clipPlaneActive[i]&&(p-clipPlaneSe3[i].position).dot(clipPlaneNormals[i])<0) return true;
	return false;
}


void Renderer::setNodeGlData(const shared_ptr<Node>& n){
	bool scaleRotations=(rotScale!=1.0 && scaleOn);
	bool scaleDisplacements=(dispScale!=Vector3r::Ones() && scaleOn);
	const bool isPeriodic=scene->isPeriodic;

	// FOREACH(const shared_ptr<Node>& n, nn){

		if(!n->hasData<GlData>()) n->setData<GlData>(make_shared<GlData>());
		GlData& gld=n->getData<GlData>();
		// inside the cell if periodic, same as pos otherwise
		Vector3r cellPos=(!isPeriodic ? n->pos : scene->cell->canonicalizePt(n->pos,gld.dCellDist)); 
		bool rendered=!pointClipped(cellPos);
		if(!rendered){ gld.dGlPos=Vector3r(NaN,NaN,NaN); return; }
		if(isnan(gld.refPos[0])) gld.refPos=n->pos;
		if(isnan(gld.refOri.x())) gld.refOri=n->ori;
		const Vector3r& pos=n->pos; const Vector3r& refPos=gld.refPos;
		const Quaternionr& ori=n->ori; const Quaternionr& refOri=gld.refOri;
		#ifdef YADE_SUBDOMAINS
			int subDom; Body::id_t localId;
			std::tie(subDom,localId)=scene->bodies->subDomId2domNumLocalId(b->subDomId);
			if(subDomMask!=0 && (((1<<subDom) & subDomMask)==0)) bodyDisp[id].isDisplayed=false;
		#endif
		// if no scaling and no periodic, return quickly
		if(!(scaleDisplacements||scaleRotations||isPeriodic)){ gld.dGlPos=Vector3r::Zero(); gld.dGlOri=Quaternionr::Identity(); return; }
		// apply scaling
		gld.dGlPos=cellPos-n->pos;
		// add scaled translation to the point of reference
		if(scaleDisplacements) gld.dGlPos+=((dispScale-Vector3r::Ones()).array()*Vector3r(pos-refPos).array()).matrix(); 
		if(!scaleRotations) gld.dGlOri=Quaternionr::Identity();
		else{
			Quaternionr relRot=refOri.conjugate()*ori;
			AngleAxisr aa(relRot);
			aa.angle()*=rotScale;
			gld.dGlOri=Quaternionr(aa);
		}

	// }
}

// draw periodic cell, if active
void Renderer::drawPeriodicCell(){
	if(!scene->isPeriodic) return;
	glColor3v(Vector3r(1,1,0));
	glPushMatrix();
		const Matrix3r& hSize=scene->cell->hSize;
		if(dispScale!=Vector3r::Ones()){
			const Matrix3r& refHSize(scene->cell->refHSize);
			Matrix3r scaledHSize;
			for(int i=0; i<3; i++) scaledHSize.col(i)=refHSize.col(i)+((dispScale-Vector3r::Ones()).array()*Vector3r(hSize.col(i)-refHSize.col(i)).array()).matrix();
			GLUtils::Parallelepiped(scaledHSize.col(0),scaledHSize.col(1),scaledHSize.col(2));
		} else 
		{
			GLUtils::Parallelepiped(hSize.col(0),hSize.col(1),hSize.col(2));
		}
	glPopMatrix();
}

void Renderer::setClippingPlanes(){
	// clipping
	assert(clipPlaneNormals.size()==(size_t)numClipPlanes);
	for(size_t i=0;i<(size_t)numClipPlanes; i++){
		// someone could have modified those from python and truncate the vectors; fill those here in that case
		if(i==clipPlaneSe3.size()) clipPlaneSe3.push_back(Se3r(Vector3r::Zero(),Quaternionr::Identity()));
		if(i==clipPlaneActive.size()) clipPlaneActive.push_back(false);
		if(i==clipPlaneNormals.size()) clipPlaneNormals.push_back(Vector3r::UnitX());
		// end filling stuff modified from python
		if(clipPlaneActive[i]) clipPlaneNormals[i]=clipPlaneSe3[i].orientation*Vector3r(0,0,1);
		/* glBegin(GL_LINES);glVertex3v(clipPlaneSe3[i].position);glVertex3v(clipPlaneSe3[i].position+clipPlaneNormals[i]);glEnd(); */
	}
}

void Renderer::resetSpecularEmission(){
	glMateriali(GL_FRONT_AND_BACK, GL_SHININESS, 128);
	const GLfloat glutMatSpecular[4]={.6,.6,.6,1};
	const GLfloat glutMatEmit[4]={.1,.1,.1,.5};
	glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,glutMatSpecular);
	glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,glutMatEmit);
}


void Renderer::setLighting(){
	// recompute emissive light colors for highlighted bodies
	Real now=TimingInfo::getNow(/*even if timing is disabled*/true)*1e-9;
	highlightEmission0[0]=highlightEmission0[1]=highlightEmission0[2]=.8*normSquare(now,1);
	highlightEmission1[0]=highlightEmission1[1]=highlightEmission0[2]=.5*normSaw(now,2);

	glClearColor(bgColor[0],bgColor[1],bgColor[2],1.0);

	// set light sources
	glLightModelf(GL_LIGHT_MODEL_TWO_SIDE,1); // important: do lighting calculations on both sides of polygons

	const GLfloat pos[4]	= {(GLfloat)lightPos[0],(GLfloat)lightPos[1],(GLfloat)lightPos[2],(GLfloat)1.0};
	const GLfloat ambientColor[4]={0.5,0.5,0.5,1.0};
	const GLfloat specularColor[4]={1,1,1,1.};
	const GLfloat diffuseLight[4] = { (GLfloat)lightColor[0], (GLfloat)lightColor[1], (GLfloat)lightColor[2], 1.0f };
	glLightfv(GL_LIGHT0, GL_POSITION,pos);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specularColor);
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambientColor);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
	if (light1) glEnable(GL_LIGHT0); else glDisable(GL_LIGHT0);

	const GLfloat pos2[4]	= {(GLfloat)light2Pos[0],(GLfloat)light2Pos[1],(GLfloat)light2Pos[2],1.0};
	const GLfloat ambientColor2[4]={0.0,0.0,0.0,1.0};
	const GLfloat specularColor2[4]={.8,.8,.8,1.};
	const GLfloat diffuseLight2[4] = { (GLfloat)light2Color[0], (GLfloat)light2Color[1], (GLfloat)light2Color[2], 1.0f };
	glLightfv(GL_LIGHT1, GL_POSITION,pos2);
	glLightfv(GL_LIGHT1, GL_SPECULAR, specularColor2);
	glLightfv(GL_LIGHT1, GL_AMBIENT, ambientColor2);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuseLight2);
	if (light2) glEnable(GL_LIGHT1); else glDisable(GL_LIGHT1);

	glEnable(GL_LIGHTING);

	// show both sides of triangles
	glDisable(GL_CULL_FACE);

	glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER,1);
	glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,1);

	// http://www.sjbaker.org/steve/omniv/opengl_lighting.html
	glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);
	//Shared material settings
	resetSpecularEmission();


	// not sctrictly lighting related
	glEnable(GL_BLEND);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_POLYGON_SMOOTH);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_RESCALE_NORMAL);
	// important for rendering text, to avoid repetivie glDisable(GL_TEXTURE_2D) ... glEnable(GL_TEXTURE_2D)
	// glBindTexture(GL_TEXTURE_2D,0);
};


void Renderer::render(const shared_ptr<Scene>& _scene, bool _withNames){
	if(!initDone) init();
	assert(initDone);
	self=this; // HACK: in case someone creates a new instance in the meantime...

	withNames=_withNames; // used in many methods
	if(withNames) glNamedObjects.clear();

	scene=_scene;
	// smuggle scene and ourselves into GLViewInfo for use with GlRep and field functors
	viewInfo.scene=scene.get();
	viewInfo.renderer=this;

	setClippingPlanes();
	setLighting();
	drawPeriodicCell();

	fieldDispatcher.scene=scene.get(); fieldDispatcher.updateScenePtr();

	FOREACH(shared_ptr<Field> f, scene->fields){
		fieldDispatcher(f,&viewInfo);
	}

	FOREACH(const shared_ptr<GlExtraDrawer> d, extraDrawers){
		if(d->dead) continue;
		glPushMatrix();
			d->scene=scene.get();
			d->render();
		glPopMatrix();
	}

	if(withNames) cerr<<"render(withNames==true) done, "<<glNamedObjects.size()<<" objects inserted"<<endl;

}

void Renderer::setLightHighlighted(int highLev){
	// highLev can be <=0 (for 0), or 1 (for 1)
	const Vector3r& h=(highLev<=0?highlightEmission0:highlightEmission1);
	glMaterialv(GL_FRONT_AND_BACK,GL_EMISSION,h);
	glMaterialv(GL_FRONT_AND_BACK,GL_SPECULAR,h);
}

void Renderer::setLightUnhighlighted(){ resetSpecularEmission(); }	

void Renderer::renderRawNode(shared_ptr<Node> node){
	Vector3r x;
	if(node->hasData<GlData>()){
		x=node->pos+node->getData<GlData>().dGlPos; // pos+dGlPos is already canonicalized, if periodic
		if(isnan(x[0])) return;
	}
	else{ x=(scene->isPeriodic?scene->cell->canonicalizePt(node->pos):node->pos); }
	Quaternionr ori=(node->hasData<GlData>()?node->getData<GlData>().dGlOri:Quaternionr::Identity())*node->ori;
	glPushMatrix();
		glTranslatev(x);
		AngleAxisr aa(ori.conjugate());
		glRotatef(aa.angle()*Mathr::RAD_TO_DEG,aa.axis()[0],aa.axis()[1],aa.axis()[2]);
		nodeDispatcher(node,viewInfo);
	glPopMatrix();
	// if(node->rep){ node->rep->render(node,&viewInfo); }
}

#endif /* YADE_OPENGL */
