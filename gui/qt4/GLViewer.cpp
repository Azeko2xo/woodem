/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*  Copyright (C) 2005 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include"GLViewer.hpp"
#include"OpenGLManager.hpp"

#include<woo/lib/opengl/OpenGLWrapper.hpp>
#include<woo/core/Field.hpp>
#include<woo/core/DisplayParameters.hpp>
#include<boost/filesystem/operations.hpp>
#include<boost/algorithm/string.hpp>
#include<boost/version.hpp>
#include<boost/python.hpp>
#include<boost/make_shared.hpp>
#include<sstream>
#include<iomanip>
#include<boost/algorithm/string/case_conv.hpp>
#include<woo/lib/object/ObjectIO.hpp>
#include<woo/lib/pyutil/gil.hpp>
#include<woo/lib/base/CompUtils.hpp>
#include<woo/lib/opengl/GLUtils.hpp>


#include<QtGui/qevent.h>

#ifdef WOO_GL2PS
#include<gl2ps.h>
#endif

WOO_PLUGIN(/*unused*/qt,(SnapshotEngine));

/*****************************************************************************
*********************************** SnapshotEngine ***************************
*****************************************************************************/

CREATE_LOGGER(SnapshotEngine);

void SnapshotEngine::pyHandleCustomCtorArgs(py::tuple& t, py::dict& d){
	if(py::len(t)==0) return;
	if(py::len(t)!=2) throw std::invalid_argument(("SnapshotEngine takes exactly 2 unnamed arguments iterPeriod,fileBase ("+lexical_cast<string>(py::len(t))+" given)").c_str());
	py::extract<int> ii(t[0]);
	py::extract<string> ff(t[1]);
	if(!(ii.check()&&ff.check())) throw std::invalid_argument("TypeError: SnapshotEngine takes 2 unnamed argument of type int, string (iterPeriod, fileBase)");
	stepPeriod=ii();
	fileBase=ff();
	t=py::tuple();
};

void SnapshotEngine::run(){
	if(!OpenGLManager::self) throw logic_error("No OpenGLManager instance?!");
	if(OpenGLManager::self->views.size()==0){
		int viewNo=OpenGLManager::self->waitForNewView(deadTimeout);
		if(viewNo<0){
			if(!ignoreErrors) throw runtime_error("SnapshotEngine: Timeout waiting for new 3d view.");
			else {
				LOG_WARN("Making myself Engine::dead, as I can not live without a 3d view (timeout)."); dead=true; return;
			}
		}
	}
	const shared_ptr<GLViewer>& glv=OpenGLManager::self->views[0];
	std::ostringstream fss; fss<<fileBase<<std::setw(5)<<std::setfill('0')<<counter++<<"."<<boost::algorithm::to_lower_copy(format);
	LOG_DEBUG("GL view → "<<fss.str())
	glv->setSnapshotFormat(QString(format.c_str()));
	glv->nextFrameSnapshotFilename=fss.str();
	// wait for the renderer to save the frame (will happen at next postDraw)
	timespec t1,t2; t1.tv_sec=0; t1.tv_nsec=10000000; /* 10 ms */
	long waiting=0;
	while(!glv->nextFrameSnapshotFilename.empty()){
		nanosleep(&t1,&t2); waiting++;
		if(((waiting) % 1000)==0) LOG_WARN("Already waiting "<<waiting/100<<"s for snapshot to be saved. Something went wrong?");
		if(waiting/100.>deadTimeout){
			if(ignoreErrors){ LOG_WARN("Timeout waiting for snapshot to be saved, making myself Engine::dead"); dead=true; return; }
			else throw runtime_error("SnapshotEngine: Timeout waiting for snapshot to be saved.");
		}
	}
	snapshots.push_back(fss.str());
	usleep((long)(msecSleep*1000));
	if(!plot.empty()){ pyRunString("import yade.plot; yade.plot.addImgData("+plot+"='"+fss.str()+"')"); }
}

CREATE_LOGGER(GLViewer);

GLLock::GLLock(GLViewer* _glv): boost::try_mutex::scoped_lock(Master::instance().renderMutex), glv(_glv){
	glv->makeCurrent();
}
GLLock::~GLLock(){ glv->doneCurrent(); }


GLViewer::~GLViewer(){ /* get the GL mutex when closing */ GLLock lock(this); /* cerr<<"Destructing view #"<<viewId<<endl;*/ }

void GLViewer::closeEvent(QCloseEvent *e){
	LOG_DEBUG("Will emit closeView for view #"<<viewId);
	OpenGLManager::self->emitCloseView(viewId);
	e->accept();
}

GLViewer::GLViewer(int _viewId, const shared_ptr<Renderer>& _renderer, QGLWidget* shareWidget): QGLViewer(/*parent*/(QWidget*)NULL,shareWidget), renderer(_renderer), viewId(_viewId) {
	isMoving=false;
	drawGrid=0;
	drawScale=true;
	timeDispMask=TIME_REAL|TIME_VIRT|TIME_ITER;
	cut_plane = 0;
	cut_plane_delta = -2;
	gridSubdivide = false;
	prevSize=Vector2i(550,550);
	resize(prevSize[0],prevSize[1]);

	if(viewId==0) setWindowTitle("Primary view");
	else setWindowTitle(("Secondary view #"+lexical_cast<string>(viewId)).c_str());

	show();

	mouseMovesCamera();
	manipulatedClipPlane=-1;

	if(manipulatedFrame()==0) setManipulatedFrame(new qglviewer::ManipulatedFrame());

	xyPlaneConstraint=shared_ptr<qglviewer::LocalConstraint>(new qglviewer::LocalConstraint());
	//xyPlaneConstraint->setTranslationConstraint(qglviewer::AxisPlaneConstraint::AXIS,qglviewer::Vec(0,0,1));
	//xyPlaneConstraint->setRotationConstraint(qglviewer::AxisPlaneConstraint::FORBIDDEN,qglviewer::Vec(0,0,1));
	manipulatedFrame()->setConstraint(NULL);

	setKeyDescription(Qt::Key_A,"Toggle visibility of global axes.");
	setKeyDescription(Qt::Key_C,"Set scene center so that all bodies are visible; if a body is selected, center around this body.");
	setKeyDescription(Qt::Key_C & Qt::AltModifier,"Set scene center to median body position (same as space)");
	setKeyDescription(Qt::Key_D,"Toggle time display mask");
	setKeyDescription(Qt::Key_G,"Cycle through visible grid planes");
	setKeyDescription(Qt::Key_G & Qt::ShiftModifier ,"Toggle grid visibility.");
	setKeyDescription(Qt::Key_X,"Show the xz [shift: xy] (up-right) plane (clip plane: align normal with +x)");
	setKeyDescription(Qt::Key_Y,"Show the yx [shift: yz] (up-right) plane (clip plane: align normal with +y)");
	setKeyDescription(Qt::Key_Z,"Show the zy [shift: zx] (up-right) plane (clip plane: align normal with +z)");
	setKeyDescription(Qt::Key_Period,"Toggle grid subdivision by 10");
	setKeyDescription(Qt::Key_S,"Toggle displacement and rotation scaling (Renderer.scaleOn)");
	setKeyDescription(Qt::Key_S & Qt::AltModifier,"Save QGLViewer state to /tmp/qglviewerState.xml");
	setKeyDescription(Qt::Key_T,"Switch orthographic / perspective camera");
	setKeyDescription(Qt::Key_O,"Set narrower field of view");
	setKeyDescription(Qt::Key_P,"Set wider field of view");
	setKeyDescription(Qt::Key_R,"Revolve around scene center");
	setKeyDescription(Qt::Key_V,"Save PDF of the current view to /tmp/yade-snapshot-0001.pdf (whichever number is available first). (Must be compiled with the gl2ps feature.)");
	#if 0
		setKeyDescription(Qt::Key_Plus,    "Cut plane increase");
		setKeyDescription(Qt::Key_Minus,   "Cut plane decrease");
		setKeyDescription(Qt::Key_Slash,   "Cut plane step decrease");
		setKeyDescription(Qt::Key_Asterisk,"Cut plane step increase");
	#endif
	setPathKey(-Qt::Key_F1);
	setPathKey(-Qt::Key_F2);
	setKeyDescription(Qt::Key_Escape,"Manipulate scene (default); cancel selection (when manipulating scene)");
	setKeyDescription(Qt::Key_F1,"Manipulate clipping plane #1");
	setKeyDescription(Qt::Key_F2,"Manipulate clipping plane #2");
	setKeyDescription(Qt::Key_F3,"Manipulate clipping plane #3");
	setKeyDescription(Qt::Key_1,"Make the manipulated clipping plane parallel with plane #1");
	setKeyDescription(Qt::Key_2,"Make the manipulated clipping plane parallel with plane #2");
	setKeyDescription(Qt::Key_2,"Make the manipulated clipping plane parallel with plane #3");
	setKeyDescription(Qt::Key_1 & Qt::AltModifier,"Add/remove plane #1 to/from the bound group");
	setKeyDescription(Qt::Key_2 & Qt::AltModifier,"Add/remove plane #2 to/from the bound group");
	setKeyDescription(Qt::Key_3 & Qt::AltModifier,"Add/remove plane #3 to/from the bound group");
	setKeyDescription(Qt::Key_0,"Clear the bound group");
	setKeyDescription(Qt::Key_7,"Load [Alt: save] view configuration #0");
	setKeyDescription(Qt::Key_8,"Load [Alt: save] view configuration #1");
	setKeyDescription(Qt::Key_9,"Load [Alt: save] view configuration #2");
	setKeyDescription(Qt::Key_Space,"Center scene (same as Alt-C); clip plane: activate/deactivate");


	// needed for movable scales
	setMouseTracking(true);

	// set initial orientation, z up
	camera()->setViewDirection(qglviewer::Vec(-1,0,0));
	camera()->setUpVector(qglviewer::Vec(0,0,1));
	centerMedianQuartile();

	//connect(&GLGlobals::redrawTimer,SIGNAL(timeout()),this,SLOT(updateGL()));

}

void GLViewer::mouseMovesCamera(){
	camera()->frame()->setWheelSensitivity(-1.0f);

	setMouseBinding(Qt::SHIFT + Qt::LeftButton, SELECT);
	//setMouseBinding(Qt::RightButton, NO_CLICK_ACTION);
	setMouseBinding(Qt::SHIFT + Qt::LeftButton + Qt::RightButton, FRAME, ZOOM);
	setMouseBinding(Qt::SHIFT + Qt::MidButton, FRAME, TRANSLATE);
	setMouseBinding(Qt::SHIFT + Qt::RightButton, FRAME, ROTATE);
	setWheelBinding(Qt::ShiftModifier , FRAME, ZOOM);

	setMouseBinding(Qt::LeftButton + Qt::RightButton, CAMERA, ZOOM);
	setMouseBinding(Qt::MidButton, CAMERA, ZOOM);
	setMouseBinding(Qt::LeftButton, CAMERA, ROTATE);
	setMouseBinding(Qt::RightButton, CAMERA, TRANSLATE);
	setWheelBinding(Qt::NoModifier, CAMERA, ZOOM);
	};

	void GLViewer::mouseMovesManipulatedFrame(qglviewer::Constraint* c){
	setMouseBinding(Qt::LeftButton + Qt::RightButton, FRAME, ZOOM);
	setMouseBinding(Qt::MidButton, FRAME, ZOOM);
	setMouseBinding(Qt::LeftButton, FRAME, ROTATE);
	setMouseBinding(Qt::RightButton, FRAME, TRANSLATE);
	setWheelBinding(Qt::NoModifier , FRAME, ZOOM);
	manipulatedFrame()->setConstraint(c);
}

bool GLViewer::isManipulating(){
	return isMoving || manipulatedClipPlane>=0;
}

void GLViewer::resetManipulation(){
	mouseMovesCamera();
	setSelectedName(-1);
	isMoving=false;
	manipulatedClipPlane=-1;
}

void GLViewer::startClipPlaneManipulation(int planeNo){
	assert(planeNo<renderer->numClipPlanes);
	resetManipulation();
	mouseMovesManipulatedFrame(xyPlaneConstraint.get());
	manipulatedClipPlane=planeNo;
	const Vector3r& pos(renderer->clipPlanePos[planeNo]); const Quaternionr& ori(renderer->clipPlaneOri[planeNo]);
	manipulatedFrame()->setPositionAndOrientation(qglviewer::Vec(pos[0],pos[1],pos[2]),qglviewer::Quaternion(ori.x(),ori.y(),ori.z(),ori.w()));
	string grp=strBoundGroup();
	displayMessage("Manipulating clip plane #"+lexical_cast<string>(planeNo+1)+(grp.empty()?grp:" (bound planes:"+grp+")"));
}

void GLViewer::useDisplayParameters(size_t n, bool fromHandler){
	/* when build without WOO_XMLSERIALIZATION, serialize to binary; otherwise, prefer xml for readability */
	LOG_DEBUG("Loading display parameters from #"<<n);
	vector<shared_ptr<DisplayParameters> >& dispParams=Master::instance().getScene()->dispParams;
	if(dispParams.size()<=(size_t)n){
		string msg("Display parameters #"+lexical_cast<string>(n)+" don't exist (number of entries "+lexical_cast<string>(dispParams.size())+")");
		if(fromHandler) displayMessage(msg.c_str());
		else throw std::runtime_error(msg.c_str());
		return;
	}
	const shared_ptr<DisplayParameters>& dp=dispParams[n];
	string val;
	if(dp->getValue("Renderer",val)){ std::istringstream oglre(val);
		yade::ObjectIO::load<decltype(renderer),
			#ifdef WOO_XMLSERIALIZATION
				boost::archive::xml_iarchive
			#else
				boost::archive::binary_iarchive
		#endif
		>(oglre,"renderer",renderer);
	}
	else { LOG_WARN("Renderer configuration not found in display parameters, skipped.");}
	if(dp->getValue("GLViewer",val)){ GLViewer::setState(val); displayMessage("Loaded view configuration #"+lexical_cast<string>(n)); }
	else { LOG_WARN("GLViewer configuration not found in display parameters, skipped."); }
	}

	void GLViewer::saveDisplayParameters(size_t n){
	LOG_DEBUG("Saving display parameters to #"<<n);
	vector<shared_ptr<DisplayParameters> >& dispParams=Master::instance().getScene()->dispParams;
	if(dispParams.size()<=n){while(dispParams.size()<=n) dispParams.push_back(shared_ptr<DisplayParameters>(new DisplayParameters));} assert(n<dispParams.size());
	shared_ptr<DisplayParameters>& dp=dispParams[n];
	std::ostringstream oglre;
	yade::ObjectIO::save<decltype(renderer),
		#ifdef WOO_XMLSERIALIZATION
			boost::archive::xml_oarchive
		#else
			boost::archive::binary_oarchive
		#endif
			>(oglre,"renderer",renderer);	
	dp->setValue("Renderer",oglre.str());
	dp->setValue("GLViewer",GLViewer::getState());
	displayMessage("Saved view configuration ot #"+lexical_cast<string>(n));
}

string GLViewer::getState(){
	QString origStateFileName=stateFileName();
	string tmpFile=Master::instance().tmpFilename();
	setStateFileName(QString(tmpFile.c_str())); saveStateToFile(); setStateFileName(origStateFileName);
	LOG_DEBUG("State saved to temp file "<<tmpFile);
	// read tmp file contents and return it as string
	// this will replace all whitespace by space (nowlines will disappear, which is what we want)
	std::ifstream in(tmpFile.c_str()); string ret; while(!in.eof()){string ss; in>>ss; ret+=" "+ss;}; in.close();
	boost::filesystem::remove(boost::filesystem::path(tmpFile));
	return ret;
}

void GLViewer::setState(string state){
	string tmpFile=Master::instance().tmpFilename();
	std::ofstream out(tmpFile.c_str());
	if(!out.good()){ LOG_ERROR("Error opening temp file `"<<tmpFile<<"', loading aborted."); return; }
	out<<state; out.close();
	LOG_DEBUG("Will load state from temp file "<<tmpFile);
	QString origStateFileName=stateFileName(); setStateFileName(QString(tmpFile.c_str())); restoreStateFromFile(); setStateFileName(origStateFileName);
	boost::filesystem::remove(boost::filesystem::path(tmpFile));
}

void GLViewer::keyPressEvent(QKeyEvent *e)
{
	last_user_event = boost::posix_time::second_clock::local_time();

	if(false){}
	/* special keys: Escape and Space */
	else if(e->key()==Qt::Key_A){ toggleAxisIsDrawn(); return; }
	else if(e->key()==Qt::Key_S){ renderer->scaleOn=!renderer->scaleOn;
		displayMessage("Scaling is "+(renderer->scaleOn?string("on (displacements ")+lexical_cast<string>(renderer->dispScale.transpose())+", rotations "+lexical_cast<string>(renderer->rotScale)+")":string("off")));
		return;
	}
	else if(e->key()==Qt::Key_Escape){
		if(!isManipulating()){ 
			// reset selection
			renderer->selObj=shared_ptr<Object>(); renderer->selObjNode=shared_ptr<Node>();
			LOG_INFO("Calling onSelection with None to deselect");
			pyRunString("import yade.qt; onSelection(None);");
		}
		else { resetManipulation(); displayMessage("Manipulating scene."); }
	}
	else if(e->key()==Qt::Key_Space){
		if(manipulatedClipPlane>=0) {displayMessage("Clip plane #"+lexical_cast<string>(manipulatedClipPlane+1)+(renderer->clipPlaneActive[manipulatedClipPlane]?" de":" ")+"activated"); renderer->clipPlaneActive[manipulatedClipPlane]=!renderer->clipPlaneActive[manipulatedClipPlane]; }
		else{ centerMedianQuartile(); }
	}
	/* function keys */
	else if(e->key()==Qt::Key_F1 || e->key()==Qt::Key_F2 || e->key()==Qt::Key_F3 /* || ... */ ){
		int n=0; if(e->key()==Qt::Key_F1) n=1; else if(e->key()==Qt::Key_F2) n=2; else if(e->key()==Qt::Key_F3) n=3; assert(n>0); int planeId=n-1;
		if(planeId>=renderer->numClipPlanes) return;
		if(planeId!=manipulatedClipPlane) startClipPlaneManipulation(planeId);
	}
	/* numbers */
	else if(e->key()==Qt::Key_0 && (e->modifiers() & Qt::AltModifier)) { boundClipPlanes.clear(); displayMessage("Cleared bound planes group.");}
	else if(e->key()==Qt::Key_1 || e->key()==Qt::Key_2 || e->key()==Qt::Key_3 /* || ... */ ){
		int n=0; if(e->key()==Qt::Key_1) n=1; else if(e->key()==Qt::Key_2) n=2; else if(e->key()==Qt::Key_3) n=3; assert(n>0); int planeId=n-1;
		if(planeId>=renderer->numClipPlanes) return; // no such clipping plane
		if(e->modifiers() & Qt::AltModifier){
			if(boundClipPlanes.count(planeId)==0) {boundClipPlanes.insert(planeId); displayMessage("Added plane #"+lexical_cast<string>(planeId+1)+" to the bound group: "+strBoundGroup());}
			else {boundClipPlanes.erase(planeId); displayMessage("Removed plane #"+lexical_cast<string>(planeId+1)+" from the bound group: "+strBoundGroup());}
		}
		else if(manipulatedClipPlane>=0 && manipulatedClipPlane!=planeId) {
			const Quaternionr& o=renderer->clipPlaneOri[planeId];
			manipulatedFrame()->setOrientation(qglviewer::Quaternion(o.x(),o.y(),o.z(),o.w()));
			displayMessage("Copied orientation from plane #1");
		}
	}
	else if(e->key()==Qt::Key_7 || e->key()==Qt::Key_8 || e->key()==Qt::Key_9){
		int nn=-1; if(e->key()==Qt::Key_7)nn=0; else if(e->key()==Qt::Key_8)nn=1; else if(e->key()==Qt::Key_9)nn=2; assert(nn>=0); size_t n=(size_t)nn;
		if(e->modifiers() & Qt::AltModifier) saveDisplayParameters(n);
		else useDisplayParameters(n,/*fromHandler*/ true);
	}
	/* letters alphabetically */
	else if(e->key()==Qt::Key_C && (e->modifiers() & Qt::AltModifier)){ displayMessage("Median centering"); centerMedianQuartile(); }
	else if(e->key()==Qt::Key_C){
		// center around selected body
		// if(selectedName() >= 0 && (*(Master::instance().getScene()->bodies)).exists(selectedName())) setSceneCenter(manipulatedFrame()->position());
		// make all bodies visible
		//else
		centerScene();
	}
	#if 0
		else if(e->key()==Qt::Key_D &&(e->modifiers() & Qt::AltModifier)){ /*Body::id_t id; if((id=Master::instance().getScene()->selection)>=0){ const shared_ptr<Body>& b=Body::byId(id); b->setDynamic(!b->isDynamic()); LOG_INFO("Body #"<<id<<" now "<<(b->isDynamic()?"":"NOT")<<" dynamic"); }*/ LOG_INFO("Selection not supported!!"); }
	#endif
	else if(e->key()==Qt::Key_D) {timeDispMask+=1; if(timeDispMask>(TIME_REAL|TIME_VIRT|TIME_ITER))timeDispMask=0; }
	else if(e->key()==Qt::Key_G) { if(e->modifiers() & Qt::ShiftModifier){ drawGrid=(drawGrid>0?0:7); return; } else drawGrid++; if(drawGrid>=8) drawGrid=0; }
	else if (e->key()==Qt::Key_M && selectedName() >= 0){ 
		if(!(isMoving=!isMoving)){displayMessage("Moving done."); mouseMovesCamera();}
		else{ displayMessage("Moving selected object"); mouseMovesManipulatedFrame();}
	}
	else if (e->key() == Qt::Key_T) camera()->setType(camera()->type()==qglviewer::Camera::ORTHOGRAPHIC ? qglviewer::Camera::PERSPECTIVE : qglviewer::Camera::ORTHOGRAPHIC);
	else if(e->key()==Qt::Key_O) camera()->setFieldOfView(camera()->fieldOfView()*0.9);
	else if(e->key()==Qt::Key_P) camera()->setFieldOfView(camera()->fieldOfView()*1.1);
	else if(e->key()==Qt::Key_R){ // reverse the clipping plane; revolve around scene center if no clipping plane selected
		if(manipulatedClipPlane>=0 && manipulatedClipPlane<renderer->numClipPlanes){
			/* here, we must update both manipulatedFrame orientation and renderer->clipPlaneOri in the same way */
			Quaternionr& ori=renderer->clipPlaneOri[manipulatedClipPlane];
			ori=Quaternionr(AngleAxisr(Mathr::PI,Vector3r(0,1,0)))*ori; 
			manipulatedFrame()->setOrientation(qglviewer::Quaternion(qglviewer::Vec(0,1,0),Mathr::PI)*manipulatedFrame()->orientation());
			displayMessage("Plane #"+lexical_cast<string>(manipulatedClipPlane+1)+" reversed.");
		}
		else {
			camera()->setRevolveAroundPoint(sceneCenter());
		}
	}
	else if(e->key()==Qt::Key_S){
		LOG_INFO("Saving QGLViewer state to /tmp/qglviewerState.xml");
		setStateFileName("/tmp/qglviewerState.xml"); saveStateToFile(); setStateFileName(QString::null);
	}
	else if(e->key()==Qt::Key_X || e->key()==Qt::Key_Y || e->key()==Qt::Key_Z){
		int axisIdx=(e->key()==Qt::Key_X?0:(e->key()==Qt::Key_Y?1:2));
		if(manipulatedClipPlane<0){
			qglviewer::Vec up(0,0,0), vDir(0,0,0);
			bool alt=(e->modifiers() && Qt::ShiftModifier);
			up[axisIdx]=1; vDir[(axisIdx+(alt?2:1))%3]=alt?1:-1;
			camera()->setViewDirection(vDir);
			camera()->setUpVector(up);
			centerMedianQuartile();
		}
		else{ // align clipping normal plane with world axis
			// x: (0,1,0),pi/2; y: (0,0,1),pi/2; z: (1,0,0),0
			qglviewer::Vec axis(0,0,0); axis[(axisIdx+1)%3]=1; Real angle=axisIdx==2?0:Mathr::PI/2;
			manipulatedFrame()->setOrientation(qglviewer::Quaternion(axis,angle));
		}
	}
	else if(e->key()==Qt::Key_Period) gridSubdivide = !gridSubdivide;
#ifdef WOO_GL2PS
	else if(e->key()==Qt::Key_V){
		for(int i=0; ;i++){
			std::ostringstream fss; fss<<"/tmp/yade-snapshot-"<<setw(4)<<setfill('0')<<i<<".pdf";
			if(!boost::filesystem::exists(fss.str())){ nextFrameSnapshotFilename=fss.str(); break; }
		}
		LOG_INFO("Will save snapshot to "<<nextFrameSnapshotFilename);
	}
#endif
#if 0
	else if( e->key()==Qt::Key_Plus ){
			cut_plane = std::min(1.0, cut_plane + std::pow(10.0,(double)cut_plane_delta));
			static_cast<YadeCamera*>(camera())->setCuttingDistance(cut_plane);
			displayMessage("Cut plane: "+lexical_cast<std::string>(cut_plane));
	}else if( e->key()==Qt::Key_Minus ){
			cut_plane = std::max(0.0, cut_plane - std::pow(10.0,(double)cut_plane_delta));
			static_cast<YadeCamera*>(camera())->setCuttingDistance(cut_plane);
			displayMessage("Cut plane: "+lexical_cast<std::string>(cut_plane));
	}else if( e->key()==Qt::Key_Slash ){
			cut_plane_delta -= 1;
			displayMessage("Cut plane increment: 1e"+(cut_plane_delta>0?std::string("+"):std::string(""))+lexical_cast<std::string>(cut_plane_delta));
	}else if( e->key()==Qt::Key_Asterisk ){
			cut_plane_delta = std::min(1+cut_plane_delta,-1);
			displayMessage("Cut plane increment: 1e"+(cut_plane_delta>0?std::string("+"):std::string(""))+lexical_cast<std::string>(cut_plane_delta));
	}
#endif

	else if(e->key()!=Qt::Key_Escape && e->key()!=Qt::Key_Space) QGLViewer::keyPressEvent(e);
	updateGL();
}
/* Center the scene such that periodic cell is contained in the view */
void GLViewer::centerPeriodic(){
	Scene* scene=Master::instance().getScene().get();
	assert(scene->isPeriodic);
	Vector3r center=.5*scene->cell->getSize();
	Vector3r halfSize=.5*scene->cell->getSize();
	float radius=std::max(halfSize[0],std::max(halfSize[1],halfSize[2]));
	LOG_DEBUG("Periodic scene center="<<center<<", halfSize="<<halfSize<<", radius="<<radius);
	setSceneCenter(qglviewer::Vec(center[0],center[1],center[2]));
	setSceneRadius(radius*1.5);
	showEntireScene();
}

/* Calculate medians for x, y and z coordinates of all bodies;
 *then set scene center to median position and scene radius to 2*inter-quartile distance.
 *
 * This function eliminates the effect of lonely bodies that went nuts and enlarge
 * the scene's Aabb in such a way that fitting the scene to see the Aabb makes the
 * "central" (where most bodies is) part very small or even invisible.
 */
void GLViewer::centerMedianQuartile(){
	Scene* scene=Master::instance().getScene().get();
	if(scene->isPeriodic){ centerPeriodic(); return; }
	std::vector<Real> coords[3];
	FOREACH(const shared_ptr<Field>& f, scene->fields){
		for(int i=0; i<3; i++) coords[i].reserve(coords[i].size()+f->nodes.size());
		FOREACH(const shared_ptr<Node>& b, f->nodes){
			for(int i=0; i<3; i++) coords[i].push_back(b->pos[i]);
		}
	}
	long nNodes=coords[0].size();
	if(nNodes<4){
		LOG_DEBUG("Less than 4 nodes, median makes no sense; calling centerScene() instead.");
		return centerScene();
	}

	Vector3r median,interQuart;
	for(int i=0;i<3;i++){
		sort(coords[i].begin(),coords[i].end());
		median[i]=*(coords[i].begin()+nNodes/2);
		interQuart[i]=*(coords[i].begin()+3*nNodes/4)-*(coords[i].begin()+nNodes/4);
	}
	LOG_DEBUG("Median position is"<<median<<", inter-quartile distance is "<<interQuart);
	setSceneCenter(qglviewer::Vec(median[0],median[1],median[2]));
	setSceneRadius(2*(interQuart[0]+interQuart[1]+interQuart[2])/3.);
	showEntireScene();
}

void GLViewer::centerScene(){
	Scene* scene=Master::instance().getScene().get();
	if (!scene) return;
	if(scene->isPeriodic){ centerPeriodic(); return; }

	LOG_INFO("Select with shift, press 'm' to move.");
	Vector3r min,max;	
	min=scene->loHint; max=scene->hiHint;
	bool hasNan=(isnan(min[0])||isnan(min[1])||isnan(min[2])||isnan(max[0])||isnan(max[1])||isnan(max[2]));
	Real minDim=std::min(max[0]-min[0],std::min(max[1]-min[1],max[2]-min[2]));
	if(minDim<=0 || hasNan){
		// Aabb is not yet calculated...
		LOG_DEBUG("scene's bound not yet calculated or has zero or nan dimension(s), attempt get that from fields (nodes)");
		Real inf=std::numeric_limits<Real>::infinity();
		min=Vector3r(inf,inf,inf); max=Vector3r(-inf,-inf,-inf);
		FOREACH(const shared_ptr<Field>& f, scene->fields){
			Vector3r a,b;
			if(!f->renderingBbox(a,b)) return;
			max=max.array().max(b.array()).matrix();
			min=min.array().min(a.array()).matrix();
		}
		if(isinf(min[0])||isinf(min[1])||isinf(min[2])||isinf(max[0])||isinf(max[1])||isinf(max[2])){ LOG_DEBUG("No min/max computed from bodies either, setting cube (-1,-1,-1)×(1,1,1)"); min=-Vector3r::Ones(); max=Vector3r::Ones(); }
	} else {LOG_DEBUG("Using scene's Aabb");}
	LOG_DEBUG("Got scene box min="<<min<<" and max="<<max);
	Vector3r center = (max+min)*0.5;
	Vector3r halfSize = (max-min)*0.5;
	float radius=std::max(halfSize[0],std::max(halfSize[1],halfSize[2])); if(radius<=0) radius=1;
	LOG_DEBUG("Scene center="<<center<<", halfSize="<<halfSize<<", radius="<<radius);
	setSceneCenter(qglviewer::Vec(center[0],center[1],center[2]));
	setSceneRadius(radius*1.5);
	showEntireScene();
}

void GLViewer::draw(bool withNames)
{
#ifdef WOO_GL2PS
	if(!nextFrameSnapshotFilename.empty() && boost::algorithm::ends_with(nextFrameSnapshotFilename,".pdf")){
		gl2psStream=fopen(nextFrameSnapshotFilename.c_str(),"wb");
		if(!gl2psStream){ int err=errno; throw runtime_error(string("Error opening file ")+nextFrameSnapshotFilename+": "+strerror(err)); }
		LOG_DEBUG("Start saving snapshot to "<<nextFrameSnapshotFilename);
		size_t nBodies=Master::instance().getScene()->bodies->size();
		int sortAlgo=(nBodies<100 ? GL2PS_BSP_SORT : GL2PS_SIMPLE_SORT);
		gl2psBeginPage(/*const char *title*/"Some title", /*const char *producer*/ "Yade",
			/*GLint viewport[4]*/ NULL,
			/*GLint format*/ GL2PS_PDF, /*GLint sort*/ sortAlgo, /*GLint options*/GL2PS_SIMPLE_LINE_OFFSET|GL2PS_USE_CURRENT_VIEWPORT|GL2PS_TIGHT_BOUNDING_BOX|GL2PS_COMPRESS|GL2PS_OCCLUSION_CULL|GL2PS_NO_BLENDING, 
			/*GLint colormode*/ GL_RGBA, /*GLint colorsize*/0, 
			/*GL2PSrgba *colortable*/NULL, 
			/*GLint nr*/0, /*GLint ng*/0, /*GLint nb*/0, 
			/*GLint buffersize*/4096*4096 /* 16MB */, /*FILE *stream*/ gl2psStream,
			/*const char *filename*/NULL);
	}
#endif

	qglviewer::Vec vd=camera()->viewDirection(); renderer->viewDirection=Vector3r(vd[0],vd[1],vd[2]);
	if(Master::instance().getScene()){
		// int selection = selectedName();
		#if 0
		if(selection!=-1 && (*(Master::instance().getScene()->bodies)).exists(selection) && isMoving){
			static int last(-1);
			if(last == selection) // delay by one redraw, so the body will not jump into 0,0,0 coords
			{
				Quaternionr& q = (*(Master::instance().getScene()->bodies))[selection]->state->ori;
				Vector3r&    v = (*(Master::instance().getScene()->bodies))[selection]->state->pos;
				float v0,v1,v2; manipulatedFrame()->getPosition(v0,v1,v2);v[0]=v0;v[1]=v1;v[2]=v2;
				double q0,q1,q2,q3; manipulatedFrame()->getOrientation(q0,q1,q2,q3);	q.x()=q0;q.y()=q1;q.z()=q2;q.w()=q3;
			}
			(*(Master::instance().getScene()->bodies))[selection]->userForcedDisplacementRedrawHook();	
			last=selection;
		}
#endif
		if(manipulatedClipPlane>=0){
			assert(manipulatedClipPlane<renderer->numClipPlanes);
			float v0,v1,v2; manipulatedFrame()->getPosition(v0,v1,v2);
			double q0,q1,q2,q3; manipulatedFrame()->getOrientation(q0,q1,q2,q3);
			Vector3r newPos(v0,v1,v2); Quaternionr newOri(q0,q1,q2,q3);
			const Vector3r& oldPos(renderer->clipPlanePos[manipulatedClipPlane]);
			const Quaternionr& oldOri(renderer->clipPlaneOri[manipulatedClipPlane]);
			FOREACH(int planeId, boundClipPlanes){
				if(planeId>=renderer->numClipPlanes || !renderer->clipPlaneActive[planeId] || planeId==manipulatedClipPlane) continue;
				Vector3r& boundPos(renderer->clipPlanePos[planeId]); Quaternionr& boundOri(renderer->clipPlaneOri[planeId]);
				Quaternionr relOrient=oldOri.conjugate()*boundOri; relOrient.normalize();
				Vector3r relPos=oldOri.conjugate()*(boundPos-oldPos);
				boundPos=newPos+newOri*relPos;
				boundOri=newOri*relOrient;
				boundOri.normalize();
			}
			renderer->clipPlanePos[manipulatedClipPlane]=newPos;
			renderer->clipPlaneOri[manipulatedClipPlane]=newOri;
		}
		const shared_ptr<Scene>& scene=Master::instance().getScene();
		scene->renderer=renderer;
		renderer->render(scene,withNames);
	}
}

// new object selected.
// set frame coordinates, and isDynamic=false;
void GLViewer::postSelection(const QPoint& point) 
{
	LOG_DEBUG("Selection is "<<selectedName());
	cerr<<"Selection is "<<selectedName()<<endl;
	int selection=selectedName();
	if(selection<0 || selection>=(int)renderer->glNamedObjects.size()) return;

	renderer->selObj=renderer->glNamedObjects[selection];
	renderer->selObjNode=renderer->glNamedNodes[selection];
	renderer->glNamedObjects.clear(); renderer->glNamedNodes.clear();
	Vector3r pos=renderer->selObjNode->pos;
	if(renderer->scene->isPeriodic) pos=renderer->scene->cell->canonicalizePt(pos);
	setSceneCenter(qglviewer::Vec(pos[0],pos[1],pos[2]));

	cerr<<"Selected object #"<<selection<<" is a "<<renderer->selObj->getClassName()<<endl;
	pyRunString("import yade.qt; onSelection(yade.qt.getSel());");
	
	/*
	gilLock lock(); // needed, since we call in python API from c++ here
	py::scope yade=py::import("yade");
	cerr<<"Selected "<<renderer->selObj->getClassName()<<" @ "<<lexical_cast<string>(renderer->selObj)<<endl;
	if(!PyObject_HasAttrString(yade.ptr(),"onSelect")) return;
	yade.attr("onSelect")(py::object(renderer->selObj));
	*/


#if 0
	if(selection>=0 && (*(Master::instance().getScene()->bodies)).exists(selection)){
		resetManipulation();
		/*if(Body::byId(Body::id_t(selection))->isClumpMember()){ // select clump (invisible) instead of its member
			LOG_DEBUG("Clump member #"<<selection<<" selected, selecting clump instead.");
			selection=Body::byId(Body::id_t(selection))->clumpId;
		}
		LOG_DEBUG("New selection "<<selection);
		displayMessage("Selected body #"+lexical_cast<string>(selection)+(Body::byId(selection)->isClump()?" (clump)":""));
		*/
		setSelectedName(selection);
		Quaternionr& q = Body::byId(selection)->state->ori;
		Vector3r&    v = Body::byId(selection)->state->pos;
		manipulatedFrame()->setPositionAndOrientation(qglviewer::Vec(v[0],v[1],v[2]),qglviewer::Quaternion(q.x(),q.y(),q.z(),q.w()));
		Master::instance().getScene()->selection = selection;
			PyGILState_STATE gstate;
			gstate = PyGILState_Ensure();
				python::object main=python::import("__main__");
				python::object global=main.attr("__dict__");
				// the try/catch block must be properly nested inside PyGILState_Ensure and PyGILState_Release
				try{
					python::eval(string("onBodySelect("+lexical_cast<string>(selection)+")").c_str(),global,global);
				} catch (python::error_already_set const &) {
					LOG_DEBUG("unable to call onBodySelect. Not defined?");
				}
			PyGILState_Release(gstate);
			// see https://svn.boost.org/trac/boost/ticket/2781 for exception handling
	}
#endif
}

// maybe new object will be selected.
// if so, then set isDynamic of previous selection, to old value
void GLViewer::endSelection(const QPoint &point){
	manipulatedClipPlane=-1;
	QGLViewer::endSelection(point);
}

qglviewer::Vec GLViewer::displayedSceneCenter(){
	return camera()->unprojectedCoordinatesOf(qglviewer::Vec(width()/2 /* pixels */ ,height()/2 /* pixels */, /*middle between near plane and far plane*/ .5));
}

float GLViewer::displayedSceneRadius(){
	return (camera()->unprojectedCoordinatesOf(qglviewer::Vec(width()/2,height()/2,.5))-camera()->unprojectedCoordinatesOf(qglviewer::Vec(0,0,.5))).norm();
}

void GLViewer::postDraw(){
	Real wholeDiameter=QGLViewer::camera()->sceneRadius()*2;

	renderer->viewInfo.sceneRadius=QGLViewer::camera()->sceneRadius();
	qglviewer::Vec c=QGLViewer::camera()->sceneCenter();
	renderer->viewInfo.sceneCenter=Vector3r(c[0],c[1],c[2]);

	Real dispDiameter=min(wholeDiameter,max((Real)displayedSceneRadius()*2,wholeDiameter/1e3)); // limit to avoid drawing 1e5 lines with big zoom level
	//qglviewer::Vec center=QGLViewer::camera()->sceneCenter();
	Real gridStep=pow(10,(floor(log10(dispDiameter)-.7)));
	Real scaleStep=pow(10,(floor(log10(displayedSceneRadius()*2)-.7))); // unconstrained
	int nSegments=((int)(wholeDiameter/gridStep))+1;
	Real realSize=nSegments*gridStep;
	//LOG_TRACE("nSegments="<<nSegments<<",gridStep="<<gridStep<<",realSize="<<realSize);
	glPushMatrix();

	nSegments *= 2; // there's an error in QGLViewer::drawGrid(), so we need to mitigate it by '* 2'
	// XYZ grids
	glLineWidth(.5);
	if(drawGrid & 1) {glColor3f(0.6,0.3,0.3); glPushMatrix(); glRotated(90.,0.,1.,0.); QGLViewer::drawGrid(realSize,nSegments); glPopMatrix();}
	if(drawGrid & 2) {glColor3f(0.3,0.6,0.3); glPushMatrix(); glRotated(90.,1.,0.,0.); QGLViewer::drawGrid(realSize,nSegments); glPopMatrix();}
	if(drawGrid & 4) {glColor3f(0.3,0.3,0.6); glPushMatrix(); /*glRotated(90.,0.,1.,0.);*/ QGLViewer::drawGrid(realSize,nSegments); glPopMatrix();}
	if(gridSubdivide){
		if(drawGrid & 1) {glColor3f(0.4,0.1,0.1); glPushMatrix(); glRotated(90.,0.,1.,0.); QGLViewer::drawGrid(realSize,nSegments*10); glPopMatrix();}
		if(drawGrid & 2) {glColor3f(0.1,0.4,0.1); glPushMatrix(); glRotated(90.,1.,0.,0.); QGLViewer::drawGrid(realSize,nSegments*10); glPopMatrix();}
		if(drawGrid & 4) {glColor3f(0.1,0.1,0.4); glPushMatrix(); /*glRotated(90.,0.,1.,0.);*/ QGLViewer::drawGrid(realSize,nSegments*10); glPopMatrix();}
	}
	
	// scale
	if(drawScale){
		Real segmentSize=scaleStep;
		qglviewer::Vec screenDxDy[3]; // dx,dy for x,y,z scale segments
		int extremalDxDy[2]={0,0};
		for(int axis=0; axis<3; axis++){
			qglviewer::Vec delta(0,0,0); delta[axis]=segmentSize;
			qglviewer::Vec center=displayedSceneCenter();
			screenDxDy[axis]=camera()->projectedCoordinatesOf(center+delta)-camera()->projectedCoordinatesOf(center);
			for(int xy=0;xy<2;xy++)extremalDxDy[xy]=(axis>0 ? min(extremalDxDy[xy],(int)screenDxDy[axis][xy]) : screenDxDy[axis][xy]);
		}
		//LOG_DEBUG("Screen offsets for axes: "<<" x("<<screenDxDy[0][0]<<","<<screenDxDy[0][1]<<") y("<<screenDxDy[1][0]<<","<<screenDxDy[1][1]<<") z("<<screenDxDy[2][0]<<","<<screenDxDy[2][1]<<")");
		int margin=10; // screen pixels
		int scaleCenter[2]; scaleCenter[0]=abs(extremalDxDy[0])+margin; scaleCenter[1]=abs(extremalDxDy[1])+margin;
		//LOG_DEBUG("Center of scale "<<scaleCenter[0]<<","<<scaleCenter[1]);
		//displayMessage(QString().sprintf("displayed scene radius %g",displayedSceneRadius()));
		startScreenCoordinatesSystem();
			glDisable(GL_LIGHTING);
			glDisable(GL_DEPTH_TEST);
			glLineWidth(3.0);
			for(int axis=0; axis<3; axis++){
				Vector3r color(.4,.4,.4); color[axis]=.9;
				glColor3v(color);
				glBegin(GL_LINES);
				glVertex2f(scaleCenter[0],scaleCenter[1]);
				glVertex2f(scaleCenter[0]+screenDxDy[axis][0],scaleCenter[1]+screenDxDy[axis][1]);
				glEnd();
			}
			glLineWidth(1.);
			QGLViewer::drawText(scaleCenter[0],scaleCenter[1],QString().sprintf("%.3g",(double)scaleStep));
			glEnable(GL_DEPTH_TEST);
			glEnable(GL_LIGHTING);
		stopScreenCoordinatesSystem();
	}

	// cutting planes (should be moved to Renderer perhaps?)
	// only painted if one of those is being manipulated
	if(manipulatedClipPlane>=0){
		for(int planeId=0; planeId<renderer->numClipPlanes; planeId++){
			if(!renderer->clipPlaneActive[planeId] && planeId!=manipulatedClipPlane) continue;
			glPushMatrix();
				const Vector3r& pos=renderer->clipPlanePos[planeId];
				const Quaternionr& ori=renderer->clipPlaneOri[planeId];
				AngleAxisr aa(ori);	
				glTranslatef(pos[0],pos[1],pos[2]);
				glRotated(aa.angle()*Mathr::RAD_TO_DEG,aa.axis()[0],aa.axis()[1],aa.axis()[2]);
				Real cff=1;
				if(!renderer->clipPlaneActive[planeId]) cff=.4;
				glColor3f(max((Real)0.,cff*cos(planeId)),max((Real)0.,cff*sin(planeId)),planeId==manipulatedClipPlane); // variable colors
				QGLViewer::drawGrid(realSize,2*nSegments);
				drawArrow(wholeDiameter/6);
			glPopMatrix();
		}
	}
	
	Scene* scene=Master::instance().getScene().get();
	#define _W3 std::setw(3)<<std::setfill('0')
	#define _W2 std::setw(2)<<std::setfill('0')
	if(timeDispMask!=0){
		const int lineHt=13;
		unsigned x=10,y=height()-3-lineHt*2;
		glColor3v(Vector3r(1,1,1));
		if(timeDispMask & GLViewer::TIME_VIRT){
			std::ostringstream oss;
			const Real& t=Master::instance().getScene()->time;
			unsigned min=((unsigned)t/60),sec=(((unsigned)t)%60),msec=((unsigned)(1e3*t))%1000,usec=((unsigned long)(1e6*t))%1000,nsec=((unsigned long)(1e9*t))%1000;
			if(min>0) oss<<_W2<<min<<":"<<_W2<<sec<<"."<<_W3<<msec<<"m"<<_W3<<usec<<"u"<<_W3<<nsec<<"n";
			else if (sec>0) oss<<_W2<<sec<<"."<<_W3<<msec<<"m"<<_W3<<usec<<"u"<<_W3<<nsec<<"n";
			else if (msec>0) oss<<_W3<<msec<<"m"<<_W3<<usec<<"u"<<_W3<<nsec<<"n";
			else if (usec>0) oss<<_W3<<usec<<"u"<<_W3<<nsec<<"n";
			else oss<<_W3<<nsec<<"ns";
			QGLViewer::drawText(x,y,oss.str().c_str());
			y-=lineHt;
		}
		glColor3v(Vector3r(0,.5,.5));
		if(timeDispMask & GLViewer::TIME_REAL){
			QGLViewer::drawText(x,y,getRealTimeString().c_str() /* virtual, since player gets that from db */);
			y-=lineHt;
		}
		if(timeDispMask & GLViewer::TIME_ITER){
			std::ostringstream oss;
			oss<<"#"<<scene->step;
			if(scene->stopAtStep>scene->step) oss<<" ("<<std::setiosflags(std::ios::fixed)<<std::setw(3)<<std::setprecision(1)<<std::setfill('0')<<(100.*scene->step)/scene->stopAtStep<<"%)";
			QGLViewer::drawText(x,y,oss.str().c_str());
			y-=lineHt;
		}
		if(drawGrid){
			glColor3v(Vector3r(1,1,0));
			std::ostringstream oss;
			oss<<"grid: "<<std::setprecision(4)<<gridStep;
			if(gridSubdivide) oss<<" (minor "<<std::setprecision(3)<<gridStep*.1<<")";
			QGLViewer::drawText(x,y,oss.str().c_str());
			y-=lineHt;
		}
	}
	QGLViewer::postDraw();
	if(!nextFrameSnapshotFilename.empty()){
		#ifdef WOO_GL2PS
			if(boost::algorithm::ends_with(nextFrameSnapshotFilename,".pdf")){
				gl2psEndPage();
				LOG_DEBUG("Finished saving snapshot to "<<nextFrameSnapshotFilename);
				fclose(gl2psStream);
			} else
	#endif
		{
			// save the snapshot
			saveSnapshot(QString(nextFrameSnapshotFilename.c_str()),/*overwrite*/ true);
		}
		// notify the caller that it is done already (probably not an atomic op :-|, though)
		nextFrameSnapshotFilename.clear();
	}

#if 1
	/* draw colormapped ranges, on the right */
	if(prevSize[0]!=width() || prevSize[1]!=height()){
		Vector2i curr(width(),height());
		FOREACH(const shared_ptr<ScalarRange>& r, scene->ranges){
			// positions normalized to window size
			r->dispPos=Vector2i(curr[0]*(r->dispPos[0]*1./prevSize[0]),curr[1]*(r->dispPos[1]*1./prevSize[1]));
			if(r->movablePtr) r->movablePtr->pos=QPoint(r->dispPos[0],r->dispPos[1]);
		};
		prevSize=curr;
	}
	if(scene->ranges.size()>0){
		glDisable(GL_LIGHTING);
		const int nDiv=20; 
		const int scaleWd=20; // width in pixels
		const Real relHt=.8; 
		int ht0=relHt*height();
		int yDef=((1-relHt)/2.)*height(); // default y position, if current is not valid
		glLineWidth(scaleWd);
		for(size_t i=0; i<scene->ranges.size(); i++){
			ScalarRange& range(*scene->ranges[i]);
			if(!range.isOk()) continue;
			int xDef=width()-50-i*70; /* 70px / scale horizontally */ // default x position, if current not valid
			if(!range.movablePtr){ range.movablePtr=make_shared<QglMovableObject>(xDef,yDef);  }
			QglMovableObject& mov(*range.movablePtr);
			if(mov.reset){ mov.reset=false; range.reset(); continue; }
			// adjust if off-screen
			if(mov.pos.x()<0 || mov.pos.y()<0 || mov.pos.x()>width() || mov.pos.y()>(height()-/*leave 20px on the lower edge*/20)){ mov.pos.setX(xDef); mov.pos.setY(yDef); cerr<<"$"; }
			int ht=ht0; // perhaps adjust in the future, if scales might have different heights
			int yStep=ht/nDiv;
			// update dimensions of the grabber object
			mov.dim=QPoint(scaleWd,ht);
			int y0=mov.pos.y(); // upper edge y-position
			int x=mov.pos.x()+scaleWd/2; // upper-edge center x-position
			range.dispPos=Vector2i(x,y0); // for loading & saving
			startScreenCoordinatesSystem();
			glBegin(GL_LINE_STRIP);
				for(int j=0; j<=nDiv; j++){
					glColor3v(CompUtils::mapColor((nDiv-j)*(1./nDiv),range.cmap));
					glVertex2f(x,y0+yStep*j);
				};
			glEnd();
			stopScreenCoordinatesSystem();
			// show some numbers
			int nNum=int(relHt*height())/100; // every 100px approx
			for(int j=0; j<=nNum; j++){
				// static void GLDrawText(const std::string& txt, const Vector3r& pos, const Vector3r& color=Vector3r(1,1,1), bool center=false, void* font=NULL, const Vector3r& bgColor=Vector3r(-1,-1,-1));
				startScreenCoordinatesSystem();
					Vector3r pos=Vector3r(x,y0+((nNum-j)*ht/nNum)-6/*lower baseline*/,0.);
					GLUtils::GLDrawText((boost::format("%.2g")%range.normInv(j*1./nNum)).str(),pos,/*color*/Vector3r::Ones(),/*center*/true,/*font*/(j>0&&j<nNum)?NULL:GLUT_BITMAP_9_BY_15,Vector3r::Zero());
				stopScreenCoordinatesSystem();
			}
			// show label, if any
			if(!range.label.empty()){
				startScreenCoordinatesSystem();
					GLUtils::GLDrawText(range.label,Vector3r(x,y0-20,0),/*color*/Vector3r::Ones(),/*center*/true,/*font*/GLUT_BITMAP_9_BY_15,Vector3r::Zero());
				stopScreenCoordinatesSystem();
			}
		}
		glLineWidth(1);
		glEnable(GL_LIGHTING);
	};
#endif
}

string GLViewer::getRealTimeString(){
	std::ostringstream oss;
	boost::posix_time::time_duration t=Master::instance().getRealTime_duration();
	unsigned d=t.hours()/24,h=t.hours()%24,m=t.minutes(),s=t.seconds();
	oss<<"clock ";
	if(d>0) oss<<d<<"days "<<_W2<<h<<":"<<_W2<<m<<":"<<_W2<<s;
	else if(h>0) oss<<_W2<<h<<":"<<_W2<<m<<":"<<_W2<<s;
	else oss<<_W2<<m<<":"<<_W2<<s;
	return oss.str();
}
#undef _W2
#undef _W3

void GLViewer::mouseMoveEvent(QMouseEvent *e){
	last_user_event = boost::posix_time::second_clock::local_time();
	QGLViewer::mouseMoveEvent(e);
}

void GLViewer::mousePressEvent(QMouseEvent *e){
	last_user_event = boost::posix_time::second_clock::local_time();
	QGLViewer::mousePressEvent(e);
}

/* Handle double-click event; if clipping plane is manipulated, align it with the global coordinate system.
 * Otherwise pass the event to QGLViewer to handle it normally.
 *
 * mostly copied over from ManipulatedFrame::mouseDoubleClickEvent
 */
void GLViewer::mouseDoubleClickEvent(QMouseEvent *event){
	last_user_event = boost::posix_time::second_clock::local_time();

	if(manipulatedClipPlane<0) { /* LOG_DEBUG("Double click not on clipping plane"); */ QGLViewer::mouseDoubleClickEvent(event); return; }
#if QT_VERSION >= 0x040000
	if (event->modifiers() == Qt::NoModifier)
#else
	if (event->state() == Qt::NoButton)
#endif
	switch (event->button()){
		case Qt::LeftButton:  manipulatedFrame()->alignWithFrame(NULL,true); LOG_DEBUG("Aligning cutting plane"); break;
		// case Qt::RightButton: projectOnLine(camera->position(), camera->viewDirection()); break;
		default: break; // avoid warning
	}
}

void GLViewer::wheelEvent(QWheelEvent* event){
	last_user_event = boost::posix_time::second_clock::local_time();

	if(manipulatedClipPlane<0){ QGLViewer::wheelEvent(event); return; }
	assert(manipulatedClipPlane<renderer->numClipPlanes);
	float distStep=1e-3*sceneRadius();
	//const float wheelSensitivityCoef = 8E-4f;
	//Vec trans(0.0, 0.0, -event->delta()*wheelSensitivity()*wheelSensitivityCoef*(camera->position()-position()).norm());
	float dist=event->delta()*manipulatedFrame()->wheelSensitivity()*distStep;
	Vector3r normal=renderer->clipPlaneOri[manipulatedClipPlane]*Vector3r(0,0,1);
	qglviewer::Vec newPos=manipulatedFrame()->position()+qglviewer::Vec(normal[0],normal[1],normal[2])*dist;
	manipulatedFrame()->setPosition(newPos);
	renderer->clipPlanePos[manipulatedClipPlane]=Vector3r(newPos[0],newPos[1],newPos[2]);
	updateGL();
	/* in draw, bound cutting planes will be moved as well */
}

// cut&paste from QGLViewer::domElement documentation
QDomElement GLViewer::domElement(const QString& name, QDomDocument& document) const{
	QDomElement de=document.createElement("grid");
	string val; if(drawGrid & 1) val+="x"; if(drawGrid & 2)val+="y"; if(drawGrid & 4)val+="z";
	de.setAttribute("normals",val.c_str());
	QDomElement de2=document.createElement("timeDisplay"); de2.setAttribute("mask",timeDispMask);
	QDomElement res=QGLViewer::domElement(name,document);
	res.appendChild(de);
	res.appendChild(de2);
	return res;
}

// cut&paste from QGLViewer::initFromDomElement documentation
void GLViewer::initFromDOMElement(const QDomElement& element){
	QGLViewer::initFromDOMElement(element);
	QDomElement child=element.firstChild().toElement();
	while (!child.isNull()){
		if (child.tagName()=="gridXYZ" && child.hasAttribute("normals")){
			string val=child.attribute("normals").toLower().toStdString();
			drawGrid=0;
			if(val.find("x")!=string::npos) drawGrid+=1; if(val.find("y")!=string::npos)drawGrid+=2; if(val.find("z")!=string::npos)drawGrid+=4;
		}
		if(child.tagName()=="timeDisplay" && child.hasAttribute("mask")) timeDispMask=atoi(child.attribute("mask").toAscii());
		child = child.nextSibling().toElement();
	}
}

boost::posix_time::ptime GLViewer::getLastUserEvent(){return last_user_event;};


float YadeCamera::zNear() const
{
  float z = distanceToSceneCenter() - zClippingCoefficient()*sceneRadius()*(1.f-2*cuttingDistance);

  // Prevents negative or null zNear values.
  const float zMin = zNearCoefficient() * zClippingCoefficient() * sceneRadius();
  if (z < zMin)
/*    switch (type())
      {
      case Camera::PERSPECTIVE  :*/ z = zMin; /*break;
      case Camera::ORTHOGRAPHIC : z = 0.0;  break;
      }*/
  return z;
}


