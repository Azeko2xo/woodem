#ifdef YADE_OPENGL

#include<yade/pkg/gl/Renderer.hpp>
#include<yade/lib/opengl/OpenGLWrapper.hpp>
#include<yade/lib/opengl/GLUtils.hpp>
#include<yade/lib/base/CompUtils.hpp>
#include<yade/core/Timing.hpp>
#include<yade/core/Scene.hpp>
#include<yade/core/Field.hpp>


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
		_TRY_ADD_FUNCTOR(GlShapeFunctor,shapeDispatcher,item.first);
		_TRY_ADD_FUNCTOR(GlBoundFunctor,boundDispatcher,item.first);
		_TRY_ADD_FUNCTOR(GlNodeFunctor,nodeDispatcher,item.first);
		_TRY_ADD_FUNCTOR(GlCPhysFunctor,cPhysDispatcher,item.first);
	}
	clipPlaneNormals.resize(numClipPlanes);
	static bool glutInitDone=false;
	if(!glutInitDone){
		glutInit(&Omega::instance().origArgc,Omega::instance().origArgv);
		/* transparent spheres (still not working): glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH | GLUT_MULTISAMPLE | GLUT_ALPHA); glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE); */
		glutInitDone=true;
	}
	initDone=true;
}

#if 0
void Renderer::setBodiesRefSe3(){
	LOG_DEBUG("(re)initializing reference positions and orientations.");
	FOREACH(const shared_ptr<Body>& b, *scene->bodies) if(b && b->state) { b->state->refPos=b->state->pos; b->state->refOri=b->state->ori; }
	scene->cell->refHSize=scene->cell->hSize;
}
#endif

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
			boost::tie(subDom,localId)=scene->bodies->subDomId2domNumLocalId(b->subDomId);
			if(subDomMask!=0 && (((1<<subDom) & subDomMask)==0)) bodyDisp[id].isDisplayed=false;
		#endif
		// if no scaling and no periodic, return quickly
		if(!(scaleDisplacements||scaleRotations||isPeriodic)){ gld.dGlPos=Vector3r::Zero(); gld.dGlOri=Quaternionr::Identity(); return; }
		// apply scaling
		gld.dGlPos=cellPos-n->pos;
		// add scaled translation to the point of reference
		if(scaleDisplacements) gld.dGlPos+=(dispScale-Vector3r::Ones()).cwise()*Vector3r(pos-refPos); 
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
			for(int i=0; i<3; i++) scaledHSize.col(i)=refHSize.col(i)+(dispScale-Vector3r::Ones()).cwise()*Vector3r(hSize.col(i)-refHSize.col(i));
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
	// smuggle scene in GLViewInfo for use with GlRep (ugly)
	viewInfo.scene=scene.get();

	dem=shared_ptr<DemField>();
	sparc=shared_ptr<SparcField>();
	for(size_t i=0; i<scene->fields.size(); i++){
		if(!dem) dem=dynamic_pointer_cast<DemField>(scene->fields[i]);
		// if(!voro) voro=dynamic_pointer_cast<VoroField>(scene->fields[i]);
		if(!sparc) sparc=dynamic_pointer_cast<SparcField>(scene->fields[i]);
	}


#if 0
	// assign scene inside functors
	geomDispatcher.updateScenePtr();
	physDispatcher.updateScenePtr();
	fieldDispatcher.updateScenePtr();
	// stateDispatcher.updateScenePtr();
#endif

#if 0
	// set displayed Se3 of body (scaling) and isDisplayed (clipping)
	setBodiesDispInfo();
#endif

	setClippingPlanes();
	setLighting();
	drawPeriodicCell();

	if(dem){
		if(shape)renderShape();
		if(bound)renderBound();
		// if(cGeom)renderCGeom();
		renderDemNodes(); // unconditionally, since nodes might have GlRep's
		if(cPhys) renderDemCPhys();
		if(cNodes>=0)renderDemContactNodes();
	}
	//if(voro){ renderVoro();	}
	if(sparc){
		renderSparc();
	}

	#if 0
	if (dof || id) renderDOF_ID();
	if (intrAllWire) renderAllInteractionsWire();
	if (intrGeom) renderIGeom();
	if (intrPhys) renderIPhys();
	#endif

	// if(field) renderField();

	FOREACH(const shared_ptr<GlExtraDrawer> d, extraDrawers){
		if(d->dead) continue;
		glPushMatrix();
			d->scene=scene.get();
			d->render();
		glPopMatrix();
	}

	if(withNames) cerr<<"render(withNames==true) done, "<<glNamedObjects.size()<<" objects inserted"<<endl;

}

#if 0
void Renderer::renderField(){
	// later: loop over all fields
	if(!scene->field) return;
}
#endif

void Renderer::setLightHighlighted(int highLev){
	// highLev can be <=0 (for 0), or 1 (for 1)
	const Vector3r& h=(highLev<=0?highlightEmission0:highlightEmission1);
	glMaterialv(GL_FRONT_AND_BACK,GL_EMISSION,h);
	glMaterialv(GL_FRONT_AND_BACK,GL_SPECULAR,h);
}

void Renderer::setLightUnhighlighted(){ resetSpecularEmission(); }	


void Renderer::renderDemNodes(){
	nodeDispatcher.scene=scene.get(); nodeDispatcher.updateScenePtr();
	FOREACH(shared_ptr<Node> node, dem->nodes){
		setNodeGlData(node);
		glScopedName name(node);
		if(nodes){
			renderRawNode(node);
		}
		if(node->rep){ node->rep->render(node,&viewInfo); }
	}
}

// render nodes of DEM contacts
void Renderer::renderDemContactNodes(){
	nodeDispatcher.scene=scene.get(); nodeDispatcher.updateScenePtr();
	boost::mutex::scoped_lock lock(*dem->contacts.manipMutex);
	FOREACH(const shared_ptr<Contact>& C, dem->contacts){
		shared_ptr<CGeom> geom=C->geom;
		if(!geom) continue;
		shared_ptr<Node> node=geom->node;
		setNodeGlData(node);
		glScopedName name(C,node);
		if(cNodes & 1) renderRawNode(node);
		if(cNodes & 2){ // connect node by lines with particle's positions
			assert(C->pA->shape && C->pB->shape);
			assert(C->pA->shape->nodes.size()>0); assert(C->pB->shape->nodes.size()>0);
			Vector3r x[3]={node->pos,C->pA->shape->avgNodePos(),C->pB->shape->avgNodePos()};
			if(scene->isPeriodic){
				Vector3i cellDist;
				x[0]=scene->cell->canonicalizePt(x[0],cellDist);
				x[1]+=scene->cell->intrShiftPos(cellDist);
				x[2]+=scene->cell->intrShiftPos(cellDist+C->cellDist);
			}
			Vector3r color=.7*CompUtils::mapColor(C->color);
			GLUtils::GLDrawLine(x[0],x[1],color);
			GLUtils::GLDrawLine(x[0],x[2],color);
		}
		if(node->rep){ node->rep->render(node,&viewInfo); }
	}
}

void Renderer::renderDemCPhys(){
	cPhysDispatcher.scene=scene.get(); cPhysDispatcher.updateScenePtr();
	boost::mutex::scoped_lock lock(*dem->contacts.manipMutex);
	FOREACH(const shared_ptr<Contact>& C, dem->contacts){
		#if 1
			shared_ptr<CGeom> geom(C->geom);
			shared_ptr<CPhys> phys(C->phys);
		#else
			// HACK: make fast now
			shared_ptr<CGeom>& geom(C->geom);
			shared_ptr<CPhys>& phys(C->phys);
		#endif
		//assert(C->pA->shape && C->pB->shape);
		//assert(C->pA->shape->nodes.size()>0); assert(C->pB->shape->nodes.size()>0);
		if(!geom || !phys) continue;
		// glScopedName name(C,geom->node);
		cPhysDispatcher(phys,C,viewInfo);
	}
}

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

// this function is called for both rendering as well as
// in the selection mode

// nice reading on OpenGL selection
// http://glprogramming.com/red/chapter13.html

void Renderer::renderShape(){
	shapeDispatcher.scene=scene.get(); shapeDispatcher.updateScenePtr();

	// instead of const shared_ptr&, get proper shared_ptr;
	// Less efficient in terms of performance, since memory has to be written (not measured, though),
	// but it is still better than crashes if the body gets deleted meanwile.
	FOREACH(shared_ptr<Particle> b, dem->particles){
		if(!b->shape || b->shape->nodes.size()==0) continue;
		const shared_ptr<Shape>& sh=b->shape;
		if(!sh->getVisible()) continue;

		//if(!bodyDisp[b->getId()].isDisplayed) continue;
		//Vector3r pos=bodyDisp[b->getId()].pos;
		//Quaternionr ori=bodyDisp[b->getId()].ori;
		//Vector3r pos=sh->nodes[0].pos;
		//Quaternionr ori=sh->nodes[0].ori;
		//if(!b->shape || !((b->getGroupMask()&mask) || b->getGroupMask()==0)) continue;

		// int selId=(dynamic_pointer_cast<Particle>(selObj)?static_pointer_cast<Particle>(selObj)->id:-1);

		// sets highlighted color, if the particle is selected
		// last optional arg can be used to provide additional highlight conditions (unused for now)
		glScopedName name(b,b->shape->nodes[0]);
		// bool highlight=(b->id==selId || (b->clumpId>=0 && b->clumpId==selId) || b->shape->highlight);

		FOREACH(const shared_ptr<Node>& n,b->shape->nodes) setNodeGlData(n);

		glPushMatrix();
			shapeDispatcher(b->shape,/*shift*/Vector3r::Zero(),wire||sh->getWire(),viewInfo);
		glPopMatrix();

		if(name.highlighted){
			const Vector3r& p=sh->nodes[0]->pos;
			if(!sh->bound || wire || sh->getWire()) GLUtils::GLDrawInt(b->id,p);
			else {
				// move the label towards the camera by the bounding box so that it is not hidden inside the body
				const Vector3r& mn=sh->bound->min; const Vector3r& mx=sh->bound->max;
				Vector3r ext(viewDirection[0]>0?p[0]-mn[0]:p[0]-mx[0],viewDirection[1]>0?p[1]-mn[1]:p[1]-mx[1],viewDirection[2]>0?p[2]-mn[2]:p[2]-mx[2]); // signed extents towards the camera
				Vector3r dr=-1.01*(viewDirection.dot(ext)*viewDirection);
				GLUtils::GLDrawInt(b->id,p+dr,Vector3r::Ones());
			}
		}
		// if the body goes over the cell margin, draw it in positions where the bbox overlaps with the cell in wire
		// precondition: pos is inside the cell.
		if(sh->bound && scene->isPeriodic && ghosts){
			const Vector3r& cellSize(scene->cell->getSize());
			//Vector3r pos=scene->cell->unshearPt(.5*(sh->bound->min+sh->bound->max)); // middle of bbox, remove the shear component
			Vector3r pos=.5*(sh->bound->min+sh->bound->max); // middle of bbox, in sheared coords already
			pos-=scene->cell->intrShiftPos(sh->nodes[0]->getData<GlData>().dCellDist); // wrap the same as node[0] was wrapped
			// traverse all periodic cells around the body, to see if any of them touches
			Vector3r halfSize=.5*(sh->bound->max-sh->bound->min);
			Vector3r pmin,pmax;
			Vector3i i;
			// try all positions around, to see if any of them sticks outside the cell boundary
			for(i[0]=-1; i[0]<=1; i[0]++) for(i[1]=-1;i[1]<=1; i[1]++) for(i[2]=-1; i[2]<=1; i[2]++){
				if(i[0]==0 && i[1]==0 && i[2]==0) continue; // middle; already rendered above
				Vector3r pos2=pos+Vector3r(cellSize[0]*i[0],cellSize[1]*i[1],cellSize[2]*i[2]); // shift, but without shear!
				pmin=pos2-halfSize; pmax=pos2+halfSize;
				if(pmin[0]<=cellSize[0] && pmax[0]>=0 &&
					pmin[1]<=cellSize[1] && pmax[1]>=0 &&
					pmin[2]<=cellSize[2] && pmax[2]>=0) {
					Vector3r pt=scene->cell->shearPt(pos2);
					// if(pointClipped(pt)) continue;
					glPushMatrix();
						shapeDispatcher(b->shape,/*shift*/pt-pos,/*wire*/true,viewInfo);
					glPopMatrix();
				}
			}
		}
	}
}

void Renderer::renderBound(){
	boundDispatcher.scene=scene.get(); boundDispatcher.updateScenePtr();
	FOREACH(const shared_ptr<Particle>& b, dem->particles){
		if(!b->shape || !b->shape->bound) continue;
		//if(!bodyDisp[b->getId()].isDisplayed) continue;
		//if(b->bound && ((b->getGroupMask()&mask) || b->getGroupMask()==0)){
		glPushMatrix(); boundDispatcher(b->shape->bound); glPopMatrix();
		//}
	}
#if 0
	// since we remove the functor as Scene doesn't inherit from Body anymore, hardcore the rendering routine here
	// for periodic scene, renderPeriodicCell is called separately
	if(!scene->isPeriodic){
		if(!scene->bound) scene->updateBound();
		glColor3v(Vector3r(0,1,0));
		Vector3r size=scene->bound->max-scene->bound->min;
		Vector3r center=.5*(scene->bound->min+scene->bound->max);
		glPushMatrix();
			glTranslatev(center);
			glScalev(size);
			glutWireCube(1);
		glPopMatrix();
	}
#endif
}


/* FIXME: move to a field-specific renderer */
void Renderer::renderSparc(){
	FOREACH(const shared_ptr<Node>& n, sparc->nodes){
		glScopedName name(n);
		setNodeGlData(n); // assures that GlData is defined
		renderRawNode(n);
		if(n->rep){ n->rep->render(n,&viewInfo); }
		// GLUtils::GLDrawText((boost::format("%d")%n->getData<SparcData>().nid).str(),n->pos,/*color*/Vector3r(1,1,1), /*center*/true,/*font*/NULL);
		int nnid=n->getData<SparcData>().nid;
		const Vector3r& pos=n->pos+n->getData<GlData>().dGlPos;
		if(nid && nnid>=0) GLUtils::GLDrawNum(nnid,pos);
		if(!sparc->showNeighbors && !name.highlighted) continue;
		// show neighbours with lines, with node colors
		Vector3r color=CompUtils::mapColor(n->getData<SparcData>().color);
		FOREACH(const shared_ptr<Node>& neighbor, n->getData<SparcData>().neighbors){
			if(!neighbor->hasData<GlData>()) continue; // neighbor might not have GlData yet, will be ok in next frame
			const Vector3r& np=neighbor->pos+neighbor->getData<GlData>().dGlPos;
			GLUtils::GLDrawLine(pos,pos+.5*(np-pos),color,3);
			GLUtils::GLDrawLine(pos+.5*(np-pos),np,color,1);
		}
	}
}


#if 0
void Renderer::renderCGeom(){
	geomDispatcher.scene=scene.get(); geomDispatcher.updateScenePtr();
	{
		boost::mutex::scoped_lock lock(dem->contacts.manipMutex);
		FOREACH(const shared_ptr<Contact>& C, dem->contacts){
			assert(C->geom); // should be handled by iterator
			shared_ptr<IGeom> cg(C->geom); // keep reference so that ig does not disappear suddenly while being rendered
			//if(!(bodyDisp[I->getId1()].isDisplayed||bodyDisp[I->getId2()].isDisplayed)) continue;
			glPushMatrix(); geomDispatcher(C->geom,C,cWire); glPopMatrix();
		}
	}
}
#endif

#if 0
void Renderer::renderAllInteractionsWire(){
	FOREACH(const shared_ptr<Interaction>& i, *scene->interactions){
		if(!i->functorCache.geomExists) continue;
		glColor3v(i->isReal()? Vector3r(0,1,0) : Vector3r(.5,0,1));
		Vector3r p1=Body::byId(i->getId1(),scene)->state->pos;
		const Vector3r& size=scene->cell->getSize();
		Vector3r shift2(i->cellDist[0]*size[0],i->cellDist[1]*size[1],i->cellDist[2]*size[2]);
		// in sheared cell, apply shear on the mutual position as well
		shift2=scene->cell->shearPt(shift2);
		Vector3r rel=Body::byId(i->getId2(),scene)->state->pos+shift2-p1;
		if(scene->isPeriodic) p1=scene->cell->canonicalizePt(p1);
		glBegin(GL_LINES); glVertex3v(p1);glVertex3v(Vector3r(p1+rel));glEnd();
	}
}



void Renderer::renderIPhys(){
	physDispatcher.scene=scene.get(); physDispatcher.updateScenePtr();
	{
		boost::mutex::scoped_lock lock(scene->interactions->drawloopmutex);
		FOREACH(const shared_ptr<Interaction>& I, *scene->interactions){
			shared_ptr<IPhys> ip(I->phys);
			if(!ip) continue;
			const shared_ptr<Body>& b1=Body::byId(I->getId1(),scene), b2=Body::byId(I->getId2(),scene);
			Body::id_t id1=I->getId1(), id2=I->getId2();
			if(!(bodyDisp[id1].isDisplayed||bodyDisp[id2].isDisplayed)) continue;
			glPushMatrix(); physDispatcher(ip,I,b1,b2,intrWire); glPopMatrix();
		}
	}
}
#endif


#endif /* YADE_OPENGL */
