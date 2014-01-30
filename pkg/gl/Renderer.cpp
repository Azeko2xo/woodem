#ifdef WOO_OPENGL

#include<woo/pkg/gl/Renderer.hpp>
#include<woo/lib/opengl/OpenGLWrapper.hpp>
#include<woo/lib/opengl/GLUtils.hpp>
#include<woo/lib/base/CompUtils.hpp>
#include<woo/lib/pyutil/gil.hpp>
#include<woo/core/Timing.hpp>
#include<woo/core/Scene.hpp>
#include<woo/core/Field.hpp>

// #include<woo/pkg/dem/Particle.hpp>

#include <GL/glu.h>
#include <GL/gl.h>
#include <GL/glut.h>

WOO_PLUGIN(gl,(Renderer)(GlExtraDrawer)(GlData));
CREATE_LOGGER(Renderer);

void GlExtraDrawer::render(){ throw runtime_error("GlExtraDrawer::render called from class "+getClassName()+". (did you forget to override it in the derived class?)"); }

vector<Vector3r> Renderer::clipPlaneNormals;
bool Renderer::initDone=false;
Vector3r Renderer::viewDirection; // updated from GLViewer regularly
GLViewInfo Renderer::viewInfo; // update from GLView regularly
Vector3r Renderer::highlightEmission0;
Vector3r Renderer::highlightEmission1;
const int Renderer::numClipPlanes;
string Renderer::snapFmt;

GlFieldDispatcher Renderer::fieldDispatcher;
GlShapeDispatcher Renderer::shapeDispatcher;
GlBoundDispatcher Renderer::boundDispatcher;
GlNodeDispatcher Renderer::nodeDispatcher;
GlCPhysDispatcher Renderer::cPhysDispatcher;
shared_ptr<Scene> Renderer::scene;
bool Renderer::withNames;
vector<shared_ptr<Object>> Renderer::glNamedObjects;
vector<shared_ptr<Node>> Renderer::glNamedNodes;

bool Renderer::scaleOn;
Vector3r Renderer::dispScale;
Real Renderer::rotScale;
Vector3r Renderer::lightPos;
Vector3r Renderer::light2Pos;
Vector3r Renderer::lightColor;
Vector3r Renderer::light2Color;
Vector3r Renderer::bgColor;
bool Renderer::light1;
bool Renderer::light2;
bool Renderer::ghosts;
bool Renderer::cell;
shared_ptr<Object> Renderer::selObj;
shared_ptr<Node> Renderer::selObjNode;
string Renderer::selFunc;
vector<Vector3r> Renderer::clipPlanePos;
vector<Quaternionr> Renderer::clipPlaneOri;
vector<bool> Renderer::clipPlaneActive;
vector<shared_ptr<GlExtraDrawer>> Renderer::extraDrawers;
bool Renderer::engines;
bool Renderer::ranges;
Vector3r Renderer::iniUp;
Vector3r Renderer::iniViewDir;

int Renderer::showTime;
Vector3r Renderer::virtColor;
Vector3r Renderer::realColor;
Vector3r Renderer::stepColor;
int Renderer::grid;
bool Renderer::oriAxes;
int Renderer::oriAxesPx;


void Renderer::init(){
	LOG_DEBUG("Renderer::init()");
	#define _TRY_ADD_FUNCTOR(functorT,dispatcher,className) if(Master::instance().isInheritingFrom_recursive(className,#functorT)){ shared_ptr<functorT> f(static_pointer_cast<functorT>(Master::instance().factorClass(className))); dispatcher.add(f); continue; }
	for(auto& item: Master::instance().getClassBases()){
		_TRY_ADD_FUNCTOR(GlFieldFunctor,fieldDispatcher,item.first);
		_TRY_ADD_FUNCTOR(GlShapeFunctor,shapeDispatcher,item.first);
		_TRY_ADD_FUNCTOR(GlBoundFunctor,boundDispatcher,item.first);
		_TRY_ADD_FUNCTOR(GlNodeFunctor,nodeDispatcher,item.first);
		_TRY_ADD_FUNCTOR(GlCPhysFunctor,cPhysDispatcher,item.first);
	}
	clipPlaneNormals.resize(numClipPlanes);
	static bool glutInitDone=false;
	if(!glutInitDone){
		char* argv[]={NULL};
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
	for(int i=0;i<numClipPlanes;i++) if(clipPlaneActive[i]&&(p-clipPlanePos[i]).dot(clipPlaneNormals[i])<0) return true;
	return false;
}


/* this function is called from field renderers for all field nodes */
void Renderer::setNodeGlData(const shared_ptr<Node>& n, bool updateRefPos){
	bool scaleRotations=(rotScale!=1.0 && scaleOn);
	bool scaleDisplacements=(dispScale!=Vector3r::Ones() && scaleOn);
	const bool isPeriodic=scene->isPeriodic;

	if(!n->hasData<GlData>()) n->setData<GlData>(make_shared<GlData>());
	GlData& gld=n->getData<GlData>();
	// inside the cell if periodic, same as pos otherwise
	Vector3r cellPos=(!isPeriodic ? n->pos : scene->cell->canonicalizePt(n->pos,gld.dCellDist)); 
	bool rendered=!pointClipped(cellPos);
	// this encodes that the node is clipped
	if(!rendered){ gld.dGlPos=Vector3r(NaN,NaN,NaN); return; }
	if(updateRefPos || isnan(gld.refPos[0])) gld.refPos=n->pos;
	if(updateRefPos || isnan(gld.refOri.x())) gld.refOri=n->ori;
	const Vector3r& pos=n->pos; const Vector3r& refPos=gld.refPos;
	const Quaternionr& ori=n->ori; const Quaternionr& refOri=gld.refOri;
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
}

// draw periodic cell, if active
void Renderer::drawPeriodicCell(){
	if(!scene->isPeriodic || !cell) return;
	glColor3v(Vector3r(1,1,0));
	glPushMatrix();
		const Matrix3r& hSize=scene->cell->hSize;
		if(scaleOn && dispScale!=Vector3r::Ones()){
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
		if(i==clipPlanePos.size()) clipPlanePos.push_back(Vector3r::Zero());
		if(i==clipPlaneOri.size()) clipPlaneOri.push_back(Quaternionr::Identity());
		if(i==clipPlaneActive.size()) clipPlaneActive.push_back(false);
		if(i==clipPlaneNormals.size()) clipPlaneNormals.push_back(Vector3r::UnitX());
		// end filling stuff modified from python
		if(clipPlaneActive[i]) clipPlaneNormals[i]=clipPlaneOri[i]*Vector3r(0,0,1);
		/* glBegin(GL_LINES);glVertex3v(clipPlanePos[i]);glVertex3v(clipPlanePos[i]+clipPlaneNormals[i]);glEnd(); */
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
	//glEnable(GL_POLYGON_SMOOTH);
	glShadeModel(GL_SMOOTH);
	#ifdef __MINGW64__ 
		// see http://www.opengl.org/discussion_boards/showthread.php/133406-GL_RESCALE_NORMAL-in-VC
		// why is glext.h not in mingw:
		//    * http://sourceforge.net/projects/mingw-w64/forums/forum/723797/topic/3572810
		glEnable(GL_NORMALIZE);
	#else
		glEnable(GL_RESCALE_NORMAL);
	#endif
	// important for rendering text, to avoid repetivie glDisable(GL_TEXTURE_2D) ... glEnable(GL_TEXTURE_2D)
	// glBindTexture(GL_TEXTURE_2D,0);
};


void Renderer::render(const shared_ptr<Scene>& _scene, bool _withNames){
	if(!initDone) init();
	assert(initDone);

	withNames=_withNames; // used in many methods
	if(withNames) glNamedObjects.clear();
	
	// assign new scene; use GIL lock to avoid crash
	if(scene.get()!=_scene.get()){
		GilLock lock;
		scene=_scene;
	}
	// smuggle scene and ourselves into GLViewInfo for use with GlRep and field functors
	viewInfo.scene=scene.get();

	setClippingPlanes();
	setLighting();
	drawPeriodicCell();

	fieldDispatcher.scene=scene.get(); fieldDispatcher.updateScenePtr();

	FOREACH(shared_ptr<Field> f, scene->fields){
		fieldDispatcher(f,&viewInfo);
	}

	if(engines){
		for(const auto& e: scene->engines){
			if(!e) continue; // should not happen, but...
			glScopedName name(e,shared_ptr<Node>());
			e->render(viewInfo);
		}
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
		GLUtils::setLocalCoords(x,ori.conjugate());
		#if 0
			glTranslatev(x);
			AngleAxisr aa(ori.conjugate());
			glRotatef(aa.angle()*(180/M_PI),aa.axis()[0],aa.axis()[1],aa.axis()[2]);
		#endif
		nodeDispatcher(node,viewInfo);
	glPopMatrix();
	// if(node->rep){ node->rep->render(node,&viewInfo); }
}

#endif /* WOO_OPENGL */
