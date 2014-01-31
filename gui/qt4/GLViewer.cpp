#ifdef WOO_OPENGL
// Copyright (C) 2004 by Olivier Galizzi, Janek Kozicki                  *
// © 2008 Václav Šmilauer

// use local includes, since MOC-generated files do the same
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

using boost::algorithm::iends_with;


#include<QtGui/qevent.h>
#include<QtCore/qdir.h>
#include<QtGui/qfiledialog.h>
#include<QtGui/QMessageBox>

WOO_PLUGIN(_qt,(SnapshotEngine));

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
		if(tryOpenView){
			int viewNo=OpenGLManager::self->waitForNewView(deadTimeout);
			if(viewNo<0){
				if(!ignoreErrors) throw runtime_error("SnapshotEngine: Timeout waiting for new 3d view.");
				else {
					LOG_WARN("Making myself Engine::dead, as I can not live without a 3d view (timeout)."); dead=true; return;
				}
			}
		} else {
			static bool warnedNoView=false;
			if(!warnedNoView) LOG_WARN("No 3d view, snapshot not taken (warn once)");
			warnedNoView=true;
			return;
		}
	}
	const shared_ptr<GLViewer>& glv=OpenGLManager::self->views[0];
	std::ostringstream fss; fss<<fileBase<<std::setw(5)<<std::setfill('0')<<counter++<<"."<<boost::algorithm::to_lower_copy(format);
	LOG_DEBUG("GL view → "<<fss.str())
	glv->setSnapshotFormat(QString(format.c_str()));
	glv->nextSnapFile=fss.str();
	glv->nextSnapMsg=false;
	// wait for the renderer to save the frame (will happen at next postDraw)
	long waiting=0;
	while(!glv->nextSnapFile.empty()){
		boost::this_thread::sleep(boost::posix_time::milliseconds(10)); waiting++;
		if(((waiting) % 1000)==0) LOG_WARN("Already waiting "<<waiting/100<<"s for snapshot to be saved. Something went wrong?");
		if(waiting/100.>deadTimeout){
			if(ignoreErrors){ LOG_WARN("Timeout waiting for snapshot to be saved, making myself Engine::dead"); dead=true; return; }
			else throw runtime_error("SnapshotEngine: Timeout waiting for snapshot to be saved.");
		}
	}
	snapshots.push_back(fss.str());
	boost::this_thread::sleep(boost::posix_time::microseconds(msecSleep));
	if(!plot.empty()){ pyRunString("import woo.plot; woo.plot.addImgData("+plot+"='"+fss.str()+"')"); }
}

CREATE_LOGGER(GLViewer);

bool GLViewer::rotCursorFreeze=false;

GLLock::GLLock(GLViewer* _glv): boost::try_mutex::scoped_lock(Master::instance().renderMutex), glv(_glv){
	glv->makeCurrent();
}
GLLock::~GLLock(){ glv->doneCurrent(); }


GLViewer::~GLViewer(){ /* get the GL mutex when closing */ GLLock lock(this); /* cerr<<"Destructing view #"<<viewId<<endl;*/ }

void GLViewer::closeEvent(QCloseEvent *e){
	// if there is an active SnapshotEngine, 
	bool snapshot=false;
	for(const auto& e: Master::instance().scene->engines){ if(!e->dead && dynamic_pointer_cast<SnapshotEngine>(e)) snapshot=true; }
	if(snapshot){
		auto confirm=QMessageBox::warning(this,"Confirmation","There is an active SnapshotEngine in the simulation, closing the 3d view may cause errors. Really close?",QMessageBox::Yes|QMessageBox::No);
	 	if (confirm==QMessageBox::No){ e->ignore(); return; }
	}
	LOG_DEBUG("Will emit closeView for view #"<<viewId);
	OpenGLManager::self->emitCloseView(viewId);
	e->accept();
}

GLViewer::GLViewer(int _viewId, QGLWidget* shareWidget): QGLViewer(/*parent*/(QWidget*)NULL,shareWidget), viewId(_viewId) {
	isMoving=false;
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
	setKeyDescription(Qt::Key_S & Qt::ControlModifier,"Save hardcopy of the image to file.");
	setKeyDescription(Qt::Key_C & Qt::ControlModifier,"Save hardcopy of the image to clipboard.");
	setKeyDescription(Qt::Key_T,"Switch orthographic / perspective camera");
	setKeyDescription(Qt::Key_O,"Set narrower field of view");
	setKeyDescription(Qt::Key_P,"Set wider field of view");
	setKeyDescription(Qt::Key_R,"Revolve around scene center");
	setKeyDescription(Qt::Key_V,"Save PDF of the current view to /tmp/woo-snapshot-0001.pdf (whichever number is available first).");
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

	setInitialView();

	// FIXME: needed for movable scales (why?! we can just detect button pressed, then mouseMoveEvent should be received automatically...?)
	setMouseTracking(true);

}

void GLViewer::setInitialView(){
	// set initial orientation, z up
	Vector3r &u(Renderer::iniUp), &v(Renderer::iniViewDir);
	qglviewer::Vec up(u[0],u[1],u[2]), vDir(v[0],v[1],v[2]);
	camera()->setViewDirection(vDir);
	camera()->setUpVector(up);
	centerScene();
	//centerMedianQuartile();
	//connect(&GLGlobals::redrawTimer,SIGNAL(timeout()),this,SLOT(updateGL()));
}

void GLViewer::mouseMovesCamera(){
	camera()->frame()->setWheelSensitivity(-1.0f);
	// old version - deprecated
#if QGLVIEWER_VERSION<0x020500
	setMouseBinding(Qt::SHIFT + Qt::LeftButton, SELECT);
	//setMouseBinding(Qt::RightButton, NO_CLICK_ACTION);
	setMouseBinding(Qt::SHIFT + Qt::LeftButton + Qt::RightButton, FRAME, ZOOM);
	setMouseBinding(Qt::SHIFT + Qt::MidButton, FRAME, TRANSLATE);
	setMouseBinding(Qt::SHIFT + Qt::RightButton, FRAME, ROTATE);
	setMouseBinding(Qt::LeftButton + Qt::RightButton, CAMERA, ZOOM);
	setMouseBinding(Qt::MidButton, CAMERA, ZOOM);
	setMouseBinding(Qt::LeftButton, CAMERA, ROTATE);
	setMouseBinding(Qt::RightButton, CAMERA, TRANSLATE);
#else
	setMouseBinding(Qt::ShiftModifier, Qt::LeftButton, SELECT);
	setMouseBinding(Qt::ShiftModifier, Qt::LeftButton | Qt::RightButton, FRAME, ZOOM);
	setMouseBinding(Qt::ShiftModifier, Qt::MidButton, FRAME, TRANSLATE);
	setMouseBinding(Qt::ShiftModifier, Qt::RightButton, FRAME, ROTATE);
	setMouseBinding(Qt::NoModifier, Qt::LeftButton|Qt::RightButton, CAMERA, ZOOM);
	setMouseBinding(Qt::NoModifier, Qt::MidButton, CAMERA, ZOOM);
	setMouseBinding(Qt::NoModifier, Qt::LeftButton, CAMERA, ROTATE);
	setMouseBinding(Qt::NoModifier, Qt::RightButton, CAMERA, TRANSLATE);
#endif
	setWheelBinding(Qt::ShiftModifier , FRAME, ZOOM);
	setWheelBinding(Qt::NoModifier, CAMERA, ZOOM);
};

void GLViewer::mouseMovesManipulatedFrame(qglviewer::Constraint* c){
#if QGLVIEWER_VERSION<0x020500
	setMouseBinding(Qt::LeftButton + Qt::RightButton, FRAME, ZOOM);
	setMouseBinding(Qt::MidButton, FRAME, ZOOM);
	setMouseBinding(Qt::LeftButton, FRAME, ROTATE);
	setMouseBinding(Qt::RightButton, FRAME, TRANSLATE);
#else
	setMouseBinding(Qt::NoModifier, Qt::LeftButton | Qt::RightButton, FRAME, ZOOM);
	setMouseBinding(Qt::NoModifier, Qt::MidButton, FRAME, ZOOM);
	setMouseBinding(Qt::NoModifier, Qt::LeftButton, FRAME, ROTATE);
	setMouseBinding(Qt::NoModifier, Qt::RightButton, FRAME, TRANSLATE);
#endif
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
	assert(planeNo<Renderer::numClipPlanes);
	resetManipulation();
	mouseMovesManipulatedFrame(xyPlaneConstraint.get());
	manipulatedClipPlane=planeNo;
	const Vector3r& pos(Renderer::clipPlanePos[planeNo]); const Quaternionr& ori(Renderer::clipPlaneOri[planeNo]);
	manipulatedFrame()->setPositionAndOrientation(qglviewer::Vec(pos[0],pos[1],pos[2]),qglviewer::Quaternion(ori.x(),ori.y(),ori.z(),ori.w()));
	string grp=strBoundGroup();
	displayMessage("Manipulating clip plane #"+lexical_cast<string>(planeNo+1)+(grp.empty()?grp:" (bound planes:"+grp+")"));
}

void GLViewer::useDisplayParameters(size_t n, bool fromHandler){
	/* when build with WOO_NOXML, serialize to binary; otherwise, prefer xml for readability */
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
	if(dp->getValue("Renderer",val)){
		std::istringstream oglre(val);
		Renderer rendererDummyInstance;
		woo::ObjectIO::load<Renderer,
			#ifdef WOO_NOXML
				boost::archive::binary_iarchive
			#else
				boost::archive::xml_iarchive
		#endif
		>(oglre,"renderer",rendererDummyInstance);
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
	Renderer rendererDummyInstance;
	woo::ObjectIO::save<Renderer,
		#ifdef WOO_NOXML
			boost::archive::binary_oarchive
		#else
			boost::archive::xml_oarchive
		#endif
			>(oglre,"renderer",rendererDummyInstance);	
	dp->setValue("Renderer",oglre.str());
	dp->setValue("GLViewer",GLViewer::getState());
	displayMessage("Saved view configuration to #"+lexical_cast<string>(n));
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
	// last_user_event = boost::posix_time::second_clock::local_time();

	if(false){}
	/* special keys: Escape and Space */
	else if(e->key()==Qt::Key_A){
		if(e->modifiers() & Qt::ShiftModifier){ Renderer::oriAxes=!Renderer::oriAxes; }
		else { toggleAxisIsDrawn(); }
		return;
	}
	else if(e->key()==Qt::Key_S){
		if(e->modifiers() & Qt::AltModifier){
			LOG_INFO("Saving QGLViewer state to /tmp/qglviewerState.xml");
			setStateFileName("/tmp/qglviewerState.xml"); saveStateToFile(); setStateFileName(QString::null);
			// return;
		} else if (e->modifiers() & Qt::ControlModifier){
			#if 0
				// popup save dialog
				QString filters("Portable Network Graphics (*.png);; Portable Document Format [unreliable] (*.pdf)");
				// if cancelled, assigns empty string, which means no screenshot will be taken at all
				nextSnapFile=QFileDialog::getSaveFileName(NULL,"Save screenshot",QDir::currentPath(),filters).toStdString();
			#else
				Scene* scene=Master::instance().getScene().get();
				string out=scene->expandTags(Renderer::snapFmt);
				if(boost::algorithm::contains(out,"{#}")){
					for(int i=0; ;i++){
						std::ostringstream fss; fss<<std::setw(4)<<std::setfill('0')<<i;
						string out2=boost::algorithm::replace_all_copy(out,"{#}",fss.str());
						if(!boost::filesystem::exists(out2)){ nextSnapFile=out2; break; }
					}
				} else nextSnapFile=out;
				LOG_INFO("Will save snapshot to "<<nextSnapFile);
			#endif
		} else {
			Renderer::scaleOn=!Renderer::scaleOn;
			displayMessage("Scaling is "+(Renderer::scaleOn?string("on (displacements ")+lexical_cast<string>(Renderer::dispScale.transpose())+", rotations "+lexical_cast<string>(Renderer::rotScale)+")":string("off")));
			return;
		}
	}
	else if(e->key()==Qt::Key_Escape){
		if(!isManipulating()){ 
			// reset selection
			Renderer::selObj=shared_ptr<Object>(); Renderer::selObjNode=shared_ptr<Node>();
			LOG_INFO("Calling onSelection with None to deselect");
			if(!Renderer::selFunc.empty()) pyRunString(Renderer::selFunc+"(None);");
		}
		else { resetManipulation(); displayMessage("Manipulating scene."); }
	}
	else if(e->key()==Qt::Key_Space){
		if(manipulatedClipPlane>=0) {displayMessage("Clip plane #"+lexical_cast<string>(manipulatedClipPlane+1)+(Renderer::clipPlaneActive[manipulatedClipPlane]?" de":" ")+"activated"); Renderer::clipPlaneActive[manipulatedClipPlane]=!Renderer::clipPlaneActive[manipulatedClipPlane]; }
		else{ centerMedianQuartile(); }
	}
	/* function keys */
	else if(e->key()==Qt::Key_F1 || e->key()==Qt::Key_F2 || e->key()==Qt::Key_F3 /* || ... */ ){
		int n=0; if(e->key()==Qt::Key_F1) n=1; else if(e->key()==Qt::Key_F2) n=2; else if(e->key()==Qt::Key_F3) n=3; assert(n>0); int planeId=n-1;
		if(planeId>=Renderer::numClipPlanes) return;
		if(planeId!=manipulatedClipPlane) startClipPlaneManipulation(planeId);
	}
	/* numbers */
	else if(e->key()==Qt::Key_0 && (e->modifiers() & Qt::AltModifier)) { boundClipPlanes.clear(); displayMessage("Cleared bound planes group.");}
	else if(e->key()==Qt::Key_1 || e->key()==Qt::Key_2 || e->key()==Qt::Key_3 /* || ... */ ){
		int n=0; if(e->key()==Qt::Key_1) n=1; else if(e->key()==Qt::Key_2) n=2; else if(e->key()==Qt::Key_3) n=3; assert(n>0); int planeId=n-1;
		if(planeId>=Renderer::numClipPlanes) return; // no such clipping plane
		if(e->modifiers() & Qt::AltModifier){
			if(boundClipPlanes.count(planeId)==0) {boundClipPlanes.insert(planeId); displayMessage("Added plane #"+lexical_cast<string>(planeId+1)+" to the bound group: "+strBoundGroup());}
			else {boundClipPlanes.erase(planeId); displayMessage("Removed plane #"+lexical_cast<string>(planeId+1)+" from the bound group: "+strBoundGroup());}
		}
		else if(manipulatedClipPlane>=0 && manipulatedClipPlane!=planeId) {
			const Quaternionr& o=Renderer::clipPlaneOri[planeId];
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
	else if(e->key()==Qt::Key_C && /*Ctrl-C is handled by qglviewer*/ !(e->modifiers() & Qt::ControlModifier)){
		// center around selected body
		// if(selectedName() >= 0 && (*(Master::instance().getScene()->bodies)).exists(selectedName())) setSceneCenter(manipulatedFrame()->position());
		// make all bodies visible
		//else
		centerScene();
	}
	#if 0
		else if(e->key()==Qt::Key_D &&(e->modifiers() & Qt::AltModifier)){ /*Body::id_t id; if((id=Master::instance().getScene()->selection)>=0){ const shared_ptr<Body>& b=Body::byId(id); b->setDynamic(!b->isDynamic()); LOG_INFO("Body #"<<id<<" now "<<(b->isDynamic()?"":"NOT")<<" dynamic"); }*/ LOG_INFO("Selection not supported!!"); }
	#endif
	else if(e->key()==Qt::Key_D) {Renderer::showTime+=1; if(Renderer::showTime>Renderer::TIME_ALL) Renderer::showTime=Renderer::TIME_NONE; }
	else if(e->key()==Qt::Key_G) { if(e->modifiers() & Qt::ShiftModifier){ Renderer::grid=(Renderer::grid>0?0:7); return; } else Renderer::grid++; if(Renderer::grid>=8) Renderer::grid=0; }
	else if (e->key()==Qt::Key_M && selectedName() >= 0){ 
		if(!(isMoving=!isMoving)){displayMessage("Moving done."); mouseMovesCamera();}
		else{ displayMessage("Moving selected object"); mouseMovesManipulatedFrame();}
	}
	else if (e->key() == Qt::Key_T) camera()->setType(camera()->type()==qglviewer::Camera::ORTHOGRAPHIC ? qglviewer::Camera::PERSPECTIVE : qglviewer::Camera::ORTHOGRAPHIC);
	else if(e->key()==Qt::Key_O) camera()->setFieldOfView(camera()->fieldOfView()*0.9);
	else if(e->key()==Qt::Key_P) camera()->setFieldOfView(camera()->fieldOfView()*1.1);
	else if(e->key()==Qt::Key_R){ // reverse the clipping plane; revolve around scene center if no clipping plane selected
		if(manipulatedClipPlane>=0 && manipulatedClipPlane<Renderer::numClipPlanes){
			/* here, we must update both manipulatedFrame orientation and Renderer::clipPlaneOri in the same way */
			Quaternionr& ori=Renderer::clipPlaneOri[manipulatedClipPlane];
			ori=Quaternionr(AngleAxisr(M_PI,Vector3r(0,1,0)))*ori; 
			manipulatedFrame()->setOrientation(qglviewer::Quaternion(qglviewer::Vec(0,1,0),M_PI)*manipulatedFrame()->orientation());
			displayMessage("Plane #"+lexical_cast<string>(manipulatedClipPlane+1)+" reversed.");
		}
		else {
			camera()->setRevolveAroundPoint(sceneCenter());
		}
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
			qglviewer::Vec axis(0,0,0); axis[(axisIdx+1)%3]=1; Real angle=axisIdx==2?0:M_PI/2;
			manipulatedFrame()->setOrientation(qglviewer::Quaternion(axis,angle));
		}
	}
	else if(e->key()==Qt::Key_Period) gridSubdivide = !gridSubdivide;
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
	AlignedBox3r box;
	if(!scene->boxHint.isEmpty()){
		LOG_DEBUG("Using Scene.boxHint");
		box=scene->boxHint;
	} else {
		LOG_DEBUG("Getting field bboxes...");
		for(const auto& f: scene->fields){ box.extend(f->renderingBbox()); }
		if(box.isEmpty()){
			LOG_DEBUG("Fields did not provide bbox either, using (-1,-1,-1)×(1,1,1) as fallback.");
			box=AlignedBox3r(-Vector3r::Ones(),Vector3r::Ones());
		}
	}
	Vector3r center=box.center();
	Vector3r halfSize=box.sizes()*.5;
	float radius=halfSize.maxCoeff(); if(radius<=0) radius=1;
	LOG_DEBUG("Scene center="<<center<<", halfSize="<<halfSize<<", radius="<<radius);
	setSceneCenter(qglviewer::Vec(center[0],center[1],center[2]));
	setSceneRadius(radius*1.5);
	showEntireScene();
}

void GLViewer::draw(bool withNames)
{

	qglviewer::Vec vd=camera()->viewDirection(); Renderer::viewDirection=Vector3r(vd[0],vd[1],vd[2]);
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
			assert(manipulatedClipPlane<Renderer::numClipPlanes);
			float v0,v1,v2; manipulatedFrame()->getPosition(v0,v1,v2);
			double q0,q1,q2,q3; manipulatedFrame()->getOrientation(q0,q1,q2,q3);
			Vector3r newPos(v0,v1,v2); Quaternionr newOri(q0,q1,q2,q3);
			const Vector3r& oldPos(Renderer::clipPlanePos[manipulatedClipPlane]);
			const Quaternionr& oldOri(Renderer::clipPlaneOri[manipulatedClipPlane]);
			FOREACH(int planeId, boundClipPlanes){
				if(planeId>=Renderer::numClipPlanes || !Renderer::clipPlaneActive[planeId] || planeId==manipulatedClipPlane) continue;
				Vector3r& boundPos(Renderer::clipPlanePos[planeId]); Quaternionr& boundOri(Renderer::clipPlaneOri[planeId]);
				Quaternionr relOrient=oldOri.conjugate()*boundOri; relOrient.normalize();
				Vector3r relPos=oldOri.conjugate()*(boundPos-oldPos);
				boundPos=newPos+newOri*relPos;
				boundOri=newOri*relOrient;
				boundOri.normalize();
			}
			Renderer::clipPlanePos[manipulatedClipPlane]=newPos;
			Renderer::clipPlaneOri[manipulatedClipPlane]=newOri;
		}
		const shared_ptr<Scene>& scene=Master::instance().getScene();
		if(Renderer::scene.get()!=scene.get()) setInitialView();
		Renderer::render(scene,withNames);
	}
}

// new object selected.
// set frame coordinates, and isDynamic=false;
void GLViewer::postSelection(const QPoint& point) 
{
	LOG_DEBUG("Selection is "<<selectedName());
	//cerr<<"Selection is "<<selectedName()<<endl;
	int selection=selectedName();
	if(selection<0 || selection>=(int)Renderer::glNamedObjects.size()) return;

	Renderer::selObj=Renderer::glNamedObjects[selection];

	auto prevSelNode=Renderer::selObjNode;
	Renderer::selObjNode=Renderer::glNamedNodes[selection];
	Renderer::glNamedObjects.clear(); Renderer::glNamedNodes.clear();
	// selection Node can be None
	if(Renderer::selObjNode){
		Vector3r pos=Renderer::selObjNode->pos;
		if(Renderer::scene->isPeriodic) pos=Renderer::scene->cell->canonicalizePt(pos);
		setSceneCenter(qglviewer::Vec(pos[0],pos[1],pos[2]));
		{
			//cerr<<"Selected object #"<<selection<<" is a "<<Renderer::selObj->getClassName()<<endl;
			GilLock lock;
			cerr<<"Selected "<<py::extract<string>(py::str(py::object(Renderer::selObj)))()<<endl;
		}
		cerr<<"\tat "<<Renderer::selObjNode->pos.transpose()<<endl;
		if(prevSelNode){
			Vector3r dPos=Renderer::selObjNode->pos-prevSelNode->pos;
			cerr<<"\tdistance from previous "<<dPos.norm()<<" (dx="<<dPos.transpose()<<")"<<endl;
			qglviewer::Vec vd0=camera()->viewDirection();
			Vector3r vd(vd0[0],vd0[1],vd0[2]);
			displayMessage("distance "+to_string(dPos.norm())+" ("+to_string((dPos-vd*dPos.dot(vd)).norm())+" view perp.)",/*delay*/6000);
		}
	}
	if(!Renderer::selFunc.empty()) pyRunString("import woo.gl\n"+Renderer::selFunc+"(woo.gl.Renderer.selObj);");
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

	Renderer::viewInfo.sceneRadius=QGLViewer::camera()->sceneRadius();
	qglviewer::Vec c=QGLViewer::camera()->sceneCenter();
	Renderer::viewInfo.sceneCenter=Vector3r(c[0],c[1],c[2]);

	Real dispDiameter=min(wholeDiameter,max((Real)displayedSceneRadius()*2,wholeDiameter/1e3)); // limit to avoid drawing 1e5 lines with big zoom level
	//qglviewer::Vec center=QGLViewer::camera()->sceneCenter();
	Real gridStep=pow(10,(floor(log10(dispDiameter)-.7)));
	Real scaleStep=pow(10,(floor(log10(displayedSceneRadius()*2)-.7))); // unconstrained
	int nSegments=((int)(wholeDiameter/gridStep))+1;
	Real realSize=nSegments*gridStep;
	//LOG_TRACE("nSegments="<<nSegments<<",gridStep="<<gridStep<<",realSize="<<realSize);
	glPushMatrix();

	nSegments *= 2; // there's an error in QGLViewer::drawGrid(), fix it by '* 2'
	// XYZ grids
	glLineWidth(.5);
	int& grid(Renderer::grid);
	if(grid & 1) {glColor3f(0.6,0.3,0.3); glPushMatrix(); glRotated(90.,0.,1.,0.); QGLViewer::drawGrid(realSize,nSegments); glPopMatrix();}
	if(grid & 2) {glColor3f(0.3,0.6,0.3); glPushMatrix(); glRotated(90.,1.,0.,0.); QGLViewer::drawGrid(realSize,nSegments); glPopMatrix();}
	if(grid & 4) {glColor3f(0.3,0.3,0.6); glPushMatrix(); /*glRotated(90.,0.,1.,0.);*/ QGLViewer::drawGrid(realSize,nSegments); glPopMatrix();}
	if(gridSubdivide){
		if(grid & 1) {glColor3f(0.4,0.1,0.1); glPushMatrix(); glRotated(90.,0.,1.,0.); QGLViewer::drawGrid(realSize,nSegments*10); glPopMatrix();}
		if(grid & 2) {glColor3f(0.1,0.4,0.1); glPushMatrix(); glRotated(90.,1.,0.,0.); QGLViewer::drawGrid(realSize,nSegments*10); glPopMatrix();}
		if(grid & 4) {glColor3f(0.1,0.1,0.4); glPushMatrix(); /*glRotated(90.,0.,1.,0.);*/ QGLViewer::drawGrid(realSize,nSegments*10); glPopMatrix();}
	}
	
	// scale
	if(Renderer::oriAxes && Renderer::oriAxesPx>10){
		Real segmentSize=10*scaleStep; // we divide every time in the loop
		qglviewer::Vec screenDxDy[3]; // dx,dy for x,y,z scale segments
		int extremalDxDy[2]={0,0};
		qglviewer::Vec center=displayedSceneCenter();
		Real maxSqLen;
		// repeate until the scale is no bigger than 100px
		do{
			maxSqLen=0.;
			segmentSize/=10;
			for(int axis=0; axis<3; axis++){
				qglviewer::Vec delta(0,0,0); delta[axis]=segmentSize;
				screenDxDy[axis]=camera()->projectedCoordinatesOf(center+delta)-camera()->projectedCoordinatesOf(center);
				for(int xy=0;xy<2;xy++)extremalDxDy[xy]=(axis>0 ? min(extremalDxDy[xy],(int)screenDxDy[axis][xy]) : screenDxDy[axis][xy]);
				maxSqLen=max(maxSqLen,1.*screenDxDy[axis].squaredNorm());
			}
		} while(maxSqLen>pow(Renderer::oriAxesPx,2));

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
			QGLViewer::drawText(scaleCenter[0],scaleCenter[1],QString().sprintf("%.3g",(double)segmentSize));
			glEnable(GL_DEPTH_TEST);
			glEnable(GL_LIGHTING);
		stopScreenCoordinatesSystem();
	}

	// cutting planes (should be moved to Renderer perhaps?)
	// only painted if one of those is being manipulated
	if(manipulatedClipPlane>=0){
		for(int planeId=0; planeId<Renderer::numClipPlanes; planeId++){
			if(!Renderer::clipPlaneActive[planeId] && planeId!=manipulatedClipPlane) continue;
			glPushMatrix();
				const Vector3r& pos=Renderer::clipPlanePos[planeId];
				const Quaternionr& ori=Renderer::clipPlaneOri[planeId];
				AngleAxisr aa(ori);	
				glTranslatef(pos[0],pos[1],pos[2]);
				glRotated(aa.angle()*(180./M_PI),aa.axis()[0],aa.axis()[1],aa.axis()[2]);
				Real cff=1;
				if(!Renderer::clipPlaneActive[planeId]) cff=.4;
				glColor3f(max((Real)0.,cff*cos(planeId)),max((Real)0.,cff*sin(planeId)),planeId==manipulatedClipPlane); // variable colors
				QGLViewer::drawGrid(realSize,2*nSegments);
				drawArrow(wholeDiameter/6);
			glPopMatrix();
		}
	}
	
	Scene* scene=Master::instance().getScene().get();
	#define _W3 std::setw(3)<<std::setfill('0')
	#define _W2 std::setw(2)<<std::setfill('0')
	if(Renderer::showTime!=Renderer::TIME_NONE){
		const int lineHt=13;
		unsigned x=10,y=height()-3-lineHt*2;
		if(Renderer::showTime & Renderer::TIME_VIRT){
			glColor3v(Renderer::virtColor);
			std::ostringstream oss;
			const Real& t=Master::instance().getScene()->time;
			const Real& dt=Master::instance().getScene()->dt;
			unsigned min=((unsigned)t/60),sec=(((unsigned)t)%60),msec=((unsigned)(1e3*t))%1000,usec=((unsigned long)(1e6*t))%1000,nsec=((unsigned long)(1e9*t))%1000;
			if(t>0){
				if(min>0) oss<<_W2<<min<<":";
					//<<_W2<<sec<<"."<<_W3<<msec<<"m"<<_W3<<usec<<"u"<<_W3<<nsec<<"n";
				oss<<_W2<<sec<<"s";
				if(dt<10) oss<<_W3<<msec<<"m";
				if(dt<1e-1) oss<<_W3<<usec<<"u";
				if(dt<1e-4) oss<<_W3<<nsec<<"n";
				//else if (msec>0) oss<<_W3<<msec<<"m"<<_W3<<usec<<"u"<<_W3<<nsec<<"n";
				//else if (usec>0) oss<<_W3<<usec<<"u"<<_W3<<nsec<<"n";
				//else oss<<_W3<<nsec<<"ns";
			} else {
				oss<<"0s";
			}
			QGLViewer::drawText(x,y,oss.str().c_str());
			y-=lineHt;
		}
		glColor3v(Vector3r(0,.5,.5));
		if(Renderer::showTime & Renderer::TIME_REAL){
			glColor3v(Renderer::realColor);
			QGLViewer::drawText(x,y,getRealTimeString().c_str() /* virtual, since player gets that from db */);
			y-=lineHt;
		}
		if(Renderer::showTime & Renderer::TIME_STEP){
			glColor3v(Renderer::stepColor);
			std::ostringstream oss;
			oss<<"#"<<scene->step;
			if(scene->stopAtStep>scene->step) oss<<" ("<<std::setiosflags(std::ios::fixed)<<std::setw(3)<<std::setprecision(1)<<std::setfill('0')<<(100.*scene->step)/scene->stopAtStep<<"%)";
			QGLViewer::drawText(x,y,oss.str().c_str());
			y-=lineHt;
		}
		if(Renderer::grid){
			glColor3v(Vector3r(1,1,0));
			std::ostringstream oss;
			oss<<"grid: "<<std::setprecision(4)<<gridStep;
			if(gridSubdivide) oss<<" (minor "<<std::setprecision(3)<<gridStep*.1<<")";
			QGLViewer::drawText(x,y,oss.str().c_str());
			y-=lineHt;
		}
	}


	/* draw colormapped ranges, on the right */
	if(Renderer::ranges && (prevSize[0]!=width() || prevSize[1]!=height())){
		Vector2i curr(width(),height());
		FOREACH(const shared_ptr<ScalarRange>& r, scene->ranges){
			// positions normalized to window size
			r->dispPos=Vector2i(curr[0]*(r->dispPos[0]*1./prevSize[0]),curr[1]*(r->dispPos[1]*1./prevSize[1]));
			if(r->movablePtr) r->movablePtr->pos=QPoint(r->dispPos[0],r->dispPos[1]);
		};
		prevSize=curr;
	}
	if(Renderer::ranges && scene->ranges.size()>0){
		glDisable(GL_LIGHTING);
		const int pixDiv=100; // show number every 100 px approximately
		const int scaleWd=20; // width in pixels
		const int nDiv=60; // draw colorline with this many segments (for gradients)
		int yDef=.2*height(); // default y position, if current is not valid
		glLineWidth(scaleWd);
		for(size_t i=0; i<scene->ranges.size(); i++){
			if(!scene->ranges[i]) continue;
			ScalarRange& range(*scene->ranges[i]);
			if(!range.isOk() || range.isHidden()) continue;
			int xDef=width()-50-i*70; /* 70px / scale horizontally */ // default x position, if current not valid
			if(!range.movablePtr){ range.movablePtr=make_shared<QglMovableObject>(xDef,yDef);  }
			QglMovableObject& mov(*range.movablePtr);
			// reset flag from the UI
			if(mov.reset){ mov.reset=false; range.reset(); continue; }
			// length was changed (mouse wheel), do it here
			if(mov.dL!=0){
				// dL is 120 for one wheel "tick"
				//cerr<<"length was "<<range.length<<", dL="<<mov.dL<<endl;
				if(range.length>0) range.length=max(50.,range.length-mov.dL*.4);
				if(range.length<0) range.length=min(-.05,range.length-mov.dL*5e-4);
				mov.dL=0;
				//cerr<<"new length "<<range.length<<endl;
			}
			// adjust if off-screen

			// adjust if too long/short
			// first value of ht, before adjustments
			int ht=int(range.length<0?abs((!range.landscape?height():width())*range.length):range.length); 

			bool flipped=false;
			// flip range if close to window edge
			if(mov.pos.x()<10 || mov.pos.x()>width()-10-(range.landscape?ht:scaleWd)){ 
				if(range.landscape){ flipped=true; range.landscape=false; }
				if(mov.pos.x()<10) mov.pos.setX(10);
				// 20 instead of 10 avoids orientation-change loop
				else mov.pos.setX(width()-10-(range.landscape?ht:scaleWd));
			}
			if(mov.pos.y()<10 || mov.pos.y()>height()-10-(range.landscape?scaleWd:ht)){
				if(!range.landscape && !flipped){
					flipped=true; range.landscape=true;
					if(mov.pos.x()>width()-10-ht) mov.pos.setX(width()-20-ht);
				}
				if(mov.pos.y()<10) mov.pos.setY(10);
				else mov.pos.setY(height()-10-(range.landscape?scaleWd:ht));
			}
			// 
			// adjust if too long/short
			if(range.length<0) CompUtils::clamp(range.length,-.7,-.1);
			else CompUtils::clamp(range.length,(!range.landscape?height():width())*.1,(!range.landscape?height():width())*.7);
			// length in pixels (second value, after adjustments)
			ht=int(range.length<0?abs((!range.landscape?height():width())*range.length):range.length); 
			Real yStep=ht*1./nDiv;
			// update dimensions of the grabber object
			int y0, x;
			if(!range.landscape){
				mov.dim=QPoint(scaleWd,ht);
				y0=mov.pos.y(); // upper edge y-position
				x=mov.pos.x()+scaleWd/2; // upper-edge center x-position
				range.dispPos=Vector2i(x,y0); // for loading & saving
			} else {
				mov.dim=QPoint(ht,scaleWd);
				y0=mov.pos.x();
				x=mov.pos.y()+scaleWd/2;
				range.dispPos=Vector2i(y0,x);
			}
			startScreenCoordinatesSystem();
			const bool& reversed(range.isReversed());
			glBegin(GL_LINE_STRIP);
				for(int j=0; j<=nDiv; j++){
					Real val=(nDiv-j)*(1./nDiv); // value in 0..1
					if(reversed) val=1-val; // reversed range
					glColor3v(CompUtils::mapColor(val,range.cmap));
					if(!range.landscape) glVertex2f(x,y0+yStep*j);
					else glVertex2f(y0+(ht-yStep*j),x);
					//cerr<<"RG "<<i<<": lin "<<j<<": "<<(range.landscape?Vector2r(y0+(ht-yStep*j),x):Vector2r(x,y0+yStep*j))<<endl;
				};
			glEnd();
			stopScreenCoordinatesSystem();
			// show some numbers
			int nNum=max(1,ht/pixDiv); // label every pixDiv approx, but at least start and end will be labeled
			//cerr<<"RG "<<i<<": ht="<<ht<<" nNum="<<nNum<<" pixDiv="<<pixDiv<<" y0="<<y0<<endl;
			for(int j=0; j<=nNum; j++){
				// static void GLDrawText(const std::string& txt, const Vector3r& pos, const Vector3r& color=Vector3r(1,1,1), bool center=false, void* font=NULL, const Vector3r& bgColor=Vector3r(-1,-1,-1));
				startScreenCoordinatesSystem();
					Real yy=y0+((!range.landscape?nNum-j:j)*ht*1./nNum);
					Vector3r pos=!range.landscape?Vector3r(x,yy-6/*lower baseline*/,0):Vector3r(yy,x-5/*lower baseline*/,0);
					//cerr<<"RG "<<i<<": num "<<j<<": "<<Vector2r(pos[0],pos[1])<<endl;
					GLUtils::GLDrawText((boost::format("%.2g")%range.normInv(j*1./nNum)).str(),pos,/*color*/Vector3r::Ones(),/*center*/true,/*font*/(j>0&&j<nNum)?NULL:GLUT_BITMAP_9_BY_15,/*bgColor*/Vector3r::Zero(),/*shiftIfNeg*/false);
				stopScreenCoordinatesSystem();
			}
			// show label, if any
			if(!range.label.empty()){
				startScreenCoordinatesSystem();
					Vector3r pos=!range.landscape?Vector3r(x,y0-20,0):Vector3r(y0+ht/2,x-25,0);
					GLUtils::GLDrawText(range.label,pos,/*color*/Vector3r::Ones(),/*center*/true,/*font*/GLUT_BITMAP_9_BY_15,Vector3r::Zero(),/*shiftIfNeg*/true);
				stopScreenCoordinatesSystem();
			}
		}
		glLineWidth(1);
		glEnable(GL_LIGHTING);
	};


	QGLViewer::postDraw();

	string _snapTo=nextSnapFile; // make a copy so that it does not change while we use it
	if(!_snapTo.empty()){
		// save the snapshot
		if(iends_with(_snapTo,".png")) setSnapshotFormat("PNG");
		else if(iends_with(_snapTo,".jpg") || iends_with(_snapTo,".jpeg")) setSnapshotFormat("JPEG");
		// using QGLViewer/VRender opens dialogues which make us freeze
		#if 0
			else if(iends_with(_snapTo,".eps")) setSnapshotFormat("EPS");
			else if(iends_with(_snapTo,".ps") ) setSnapshotFormat("PS");
			else if(iends_with(_snapTo,".xfig") || iends_with(_snapTo,".fig")) setSnapshotFormat("XFIG");
			else if(iends_with(_snapTo,".pdf")) setSnapshotFormat("PDF");
			else if(iends_with(_snapTo,".svg")) setSnapshotFormat("SVG");
		#endif
		else {
			LOG_WARN("Unable to deduce raster snapshot format from filename "<<_snapTo<<", or format is not supported; using PNG (recognized extensions are png, jpg, jpeg.");
			setSnapshotFormat("PNG");
		}
		saveSnapshot(QString(_snapTo.c_str()),/*overwrite*/ true);

		if(nextSnapMsg) displayMessage("Saved snapshot to "+_snapTo);
		// notify the caller that it is done already (probably not an atomic op :-|, though)
		nextSnapFile.clear();
		nextSnapMsg=true; // show next message, unless disabled again
	}
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

void GLViewer::mousePressEvent(QMouseEvent *e){
	#if 0
		pressPos=e->globalPos(); // remember this position for blocking the cursor when rotating
		cerr<<"["<<pressPos.x()<<","<<pressPos.y()<<"]";
	#endif
	QGLViewer::mousePressEvent(e);
}

void GLViewer::mouseMoveEvent(QMouseEvent *e){
	// cerr<<(rotCursorFreeze?":":".");
	#if 0
		if(rotCursorFreeze && (e->buttons()&(Qt::LeftButton|Qt::RightButton|Qt::MiddleButton))){
			// this is the event returning us to the initial position, which we need to ignore
			// see http://www.qtcentre.org/threads/16298-free-mouse-pointer
			if(QCursor::pos()==pressPos) { cerr<<"_"; return; }
			cerr<<"#";
			QCursor::setPos(pressPos); // already in global coords
	}
	#endif
	QGLViewer::mouseMoveEvent(e);
}

/* Handle double-click event; if clipping plane is manipulated, align it with the global coordinate system.
 * Otherwise pass the event to QGLViewer to handle it normally.
 *
 * mostly copied over from ManipulatedFrame::mouseDoubleClickEvent
 */
void GLViewer::mouseDoubleClickEvent(QMouseEvent *event){
	// last_user_event = boost::posix_time::second_clock::local_time();

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
	// last_user_event = boost::posix_time::second_clock::local_time();

	if(manipulatedClipPlane<0){
		// with right button pressed (which is normally used for camera rotation), roll the scene
		if(!mouseGrabber() && (event->buttons() & Qt::LeftButton)){
			// hopefully we are really rotating the camera since there is no mouseGrabber
			qglviewer::Vec up(camera()->upVector());
			qglviewer::Vec dir(camera()->viewDirection());
			//  QT5: use angleDelta() instead of delta()
			float angle=manipulatedFrame()->rotationSensitivity()*manipulatedFrame()->wheelSensitivity()*event->delta()*(1/15.)*(M_PI/180);
			qglviewer::Quaternion qRot; qRot.setAxisAngle(dir,angle);
			camera()->setUpVector(qRot.rotate(up));
		} else QGLViewer::wheelEvent(event);
		return;
	}
	assert(manipulatedClipPlane<Renderer::numClipPlanes);
	float distStep=1e-3*sceneRadius();
	//const float wheelSensitivityCoef = 8E-4f;
	//Vec trans(0.0, 0.0, -event->delta()*wheelSensitivity()*wheelSensitivityCoef*(camera->position()-position()).norm());
	float dist=event->delta()*manipulatedFrame()->wheelSensitivity()*distStep;
	Vector3r normal=Renderer::clipPlaneOri[manipulatedClipPlane]*Vector3r(0,0,1);
	qglviewer::Vec newPos=manipulatedFrame()->position()+qglviewer::Vec(normal[0],normal[1],normal[2])*dist;
	manipulatedFrame()->setPosition(newPos);
	Renderer::clipPlanePos[manipulatedClipPlane]=Vector3r(newPos[0],newPos[1],newPos[2]);
	updateGL();
	/* in draw, bound cutting planes will be moved as well */
}

// cut&paste from QGLViewer::domElement documentation
QDomElement GLViewer::domElement(const QString& name, QDomDocument& document) const{
	QDomElement res=QGLViewer::domElement(name,document);
	// not saving any custom attributes anymore
	return res;
}

// cut&paste from QGLViewer::initFromDomElement documentation
void GLViewer::initFromDOMElement(const QDomElement& element){
	QGLViewer::initFromDOMElement(element);
	// we don't save any custom attributes here anymore
	#if 0
	QDomElement child=element.firstChild().toElement();
	while (!child.isNull()){
		if (child.tagName()=="gridXYZ" && child.hasAttribute("normals")){
			string val=child.attribute("normals").toLower().toStdString();
			drawGrid=0;
			if(val.find("x")!=string::npos) drawGrid+=1; if(val.find("y")!=string::npos)drawGrid+=2; if(val.find("z")!=string::npos)drawGrid+=4;
		}
		child = child.nextSibling().toElement();
	}
	#endif
}

// boost::posix_time::ptime GLViewer::getLastUserEvent(){return last_user_event;};

#endif
