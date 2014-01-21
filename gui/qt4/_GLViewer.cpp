#ifdef WOO_OPENGL

// use local includes, since MOC does the same
#include"GLViewer.hpp"
#include"OpenGLManager.hpp"

#include<boost/python.hpp>
#include<woo/pkg/gl/Renderer.hpp>
#include<woo/lib/pyutil/doc_opts.hpp>

#include<QApplication>
#include<QCloseEvent>

#ifdef WOO_LOG4CXX
static log4cxx::LoggerPtr logger=log4cxx::Logger::getLogger("woo.qt4");
#endif

namespace py=boost::python;


//#define POST_SYNTH_EVENT(EVT,checker) void evt##EVT(){QApplication::postEvent(OpenGLManager::self,new QCustomEvent(YadeQtMainWindow::EVENT_##EVT)); bool wait=true; if(wait){while(!(bool)(OpenGLManager::self->checker)) usleep(50000);} }
//POST_SYNTH_EVENT(CONTROLLER,controller);
//POST_SYNTH_EVENT(GENERATOR,generator);
//#undef POST_SYNT_EVENT

#if 0
// event associated data will be deleted in the event handler
void restoreGLViewerState_str(string str){string* s=new string(str); QApplication::postEvent(OpenGLManager::self,new 
	QCustomEvent((QEvent::Type)YadeQtMainWindow::EVENT_RESTORE_GLVIEWER_STR,(void*)s));}
void restoreGLViewerState_num(int dispStateNo){
		int* i=new int(dispStateNo);
		QApplication::postEvent(OpenGLManager::self,new QCustomEvent((QEvent::Type)YadeQtMainWindow::EVENT_RESTORE_GLVIEWER_NUM,(void*)i));
}
#endif


qglviewer::Vec tuple2vec(py::tuple t){ qglviewer::Vec ret; for(int i=0;i<3;i++){py::extract<Real> e(t[i]); if(!e.check()) throw invalid_argument("Element #"+lexical_cast<string>(i)+" is not a number"); ret[i]=e();} return ret;};
py::tuple vec2tuple(qglviewer::Vec v){return py::make_tuple(v[0],v[1],v[2]);};

class pyGLViewer{
	const size_t viewNo;
	public:
		#define GLV if((OpenGLManager::self->views.size()<=viewNo) || !(OpenGLManager::self->views[viewNo])) throw runtime_error("No view #"+lexical_cast<string>(viewNo)); GLViewer* glv=OpenGLManager::self->views[viewNo].get();
		pyGLViewer(size_t _viewNo=0): viewNo(_viewNo){}
		void close(){ GLV; QCloseEvent* e(new QCloseEvent); QApplication::postEvent(glv,e); }
		#define VEC_GET_SET(property,getter,setter) Vector3r get_##property(){GLV; qglviewer::Vec v=getter(); return Vector3r(v[0],v[1],v[2]); } void set_##property(const Vector3r& t){GLV;  setter(qglviewer::Vec(t[0],t[1],t[2]));}
		VEC_GET_SET(upVector,glv->camera()->upVector,glv->camera()->setUpVector);
		VEC_GET_SET(lookAt,glv->camera()->position()+glv->camera()->viewDirection,glv->camera()->lookAt);
		VEC_GET_SET(viewDir,glv->camera()->viewDirection,glv->camera()->setViewDirection);
		VEC_GET_SET(eyePosition,glv->camera()->position,glv->camera()->setPosition);
		#define BOOL_GET_SET(property,getter,setter)void set_##property(bool b){GLV; setter(b);} bool get_##property(){GLV; return getter();}
		BOOL_GET_SET(axes,glv->axisIsDrawn,glv->setAxisIsDrawn);
		BOOL_GET_SET(fps,glv->FPSIsDisplayed,glv->setFPSIsDisplayed);
		static bool get_rotCursorFreeze(){ return GLViewer::rotCursorFreeze; }
		static void set_rotCursorFreeze(bool b){ GLViewer::rotCursorFreeze=b; }
		bool get_orthographic(){GLV; return glv->camera()->type()==qglviewer::Camera::ORTHOGRAPHIC;}
		void set_orthographic(bool b){GLV; return glv->camera()->setType(b ? qglviewer::Camera::ORTHOGRAPHIC : qglviewer::Camera::PERSPECTIVE);}
		int get_selection(void){ GLV; return glv->selectedName(); } void set_selection(int s){ GLV; glv->setSelectedName(s); }
		#define FLOAT_GET_SET(property,getter,setter)void set_##property(Real r){GLV; setter(r);} Real get_##property(){GLV; return getter();}
		FLOAT_GET_SET(sceneRadius,glv->sceneRadius,glv->setSceneRadius);
		void fitAABB(const Vector3r& min, const Vector3r& max){GLV;  glv->camera()->fitBoundingBox(qglviewer::Vec(min[0],min[1],min[2]),qglviewer::Vec(max[0],max[1],max[2]));}
		void fitSphere(const Vector3r& center,Real radius){GLV;  glv->camera()->fitSphere(qglviewer::Vec(center[0],center[1],center[2]),radius);}
		void showEntireScene(){GLV;  glv->camera()->showEntireScene();}
		void center(bool median){GLV;  if(median)glv->centerMedianQuartile(); else glv->centerScene();}
		Vector2i get_screenSize(){GLV;  return Vector2i(glv->width(),glv->height());}
		void set_screenSize(Vector2i t){ /*GLV;*/ OpenGLManager::self->emitResizeView(viewNo,t[0],t[1]);}
		string pyStr(){return string("<GLViewer for view #")+lexical_cast<string>(viewNo)+">";}
		void saveDisplayParameters(size_t n){GLV;  glv->saveDisplayParameters(n);}
		void useDisplayParameters(size_t n){GLV;  glv->useDisplayParameters(n);}
		#undef GLV
		#undef VEC_GET_SET
		#undef BOOL_GET_SET
		#undef FLOAT_GET_SET
};

// ask to create a new view and wait till it exists
pyGLViewer createView(){ 
	int id=OpenGLManager::self->waitForNewView();
	if(id<0) throw std::runtime_error("Unable to open new 3d view.");
	return pyGLViewer((*OpenGLManager::self->views.rbegin())->viewId);
}

py::list getAllViews(){ py::list ret; FOREACH(const shared_ptr<GLViewer>& v, OpenGLManager::self->views){ if(v) ret.append(pyGLViewer(v->viewId)); } return ret; };
void centerViews(void){ OpenGLManager::self->centerAllViews(); }

WOO_PYTHON_MODULE(_qt._GLViewer);
BOOST_PYTHON_MODULE(_GLViewer){
	WOO_SET_DOCSTRING_OPTS;
	
	OpenGLManager* glm=new OpenGLManager(); // keep this singleton object forever
	glm->emitStartTimer();

	py::def("View",createView,"Create a new 3d view.");
	py::def("center",centerViews,"Center all views.");
	py::def("views",getAllViews,"Return list of all open :obj:`woo.qt.GLViewer` objects");
	
	py::class_<pyGLViewer>("GLViewer",py::no_init)
		.add_property("upVector",&pyGLViewer::get_upVector,&pyGLViewer::set_upVector,"Vector that will be shown oriented up on the screen.")
		.add_property("lookAt",&pyGLViewer::get_lookAt,&pyGLViewer::set_lookAt,"Point at which camera is directed.")
		.add_property("viewDir",&pyGLViewer::get_viewDir,&pyGLViewer::set_viewDir,"Camera orientation (as vector).")
		.add_property("eyePosition",&pyGLViewer::get_eyePosition,&pyGLViewer::set_eyePosition,"Camera position.")
		.add_property("fps",&pyGLViewer::get_fps,&pyGLViewer::set_fps,"Show frames per second indicator.")
		.add_property("axes",&pyGLViewer::get_axes,&pyGLViewer::set_axes,"Show arrows for axes.")
		.add_property("sceneRadius",&pyGLViewer::get_sceneRadius,&pyGLViewer::set_sceneRadius,"Visible scene radius.")
		.add_property("ortho",&pyGLViewer::get_orthographic,&pyGLViewer::set_orthographic,"Whether orthographic projection is used; if false, use perspective projection.")
		.add_static_property("rotCursorFreeze",&pyGLViewer::get_rotCursorFreeze,&pyGLViewer::set_rotCursorFreeze)
		.add_property("screenSize",&pyGLViewer::get_screenSize,&pyGLViewer::set_screenSize,"Size of the viewer's window, in scree pixels")
		.def("fitAABB",&pyGLViewer::fitAABB,(py::arg("mn"),py::arg("mx")),"Adjust scene bounds so that Axis-aligned bounding box given by its lower and upper corners *mn*, *mx* fits in.")
		.def("fitSphere",&pyGLViewer::fitSphere,(py::arg("center"),py::arg("radius")),"Adjust scene bounds so that sphere given by *center* and *radius* fits in.")
		.def("showEntireScene",&pyGLViewer::showEntireScene)
		.def("center",&pyGLViewer::center,(py::arg("median")=true),"Center view. View is centered either so that all bodies fit inside (``median=False``), or so that 75\\% of bodies fit inside (``median=True``).")
		.def("saveState",&pyGLViewer::saveDisplayParameters,(py::arg("slot")),"Save display parameters into numbered memory slot. Saves state for both :obj:`GLViewer` and associated :obj:`Renderer`.")
		.def("loadState",&pyGLViewer::useDisplayParameters,(py::arg("slot")),"Load display parameters from slot saved previously into, identified by its number.")
		.def("__repr__",&pyGLViewer::pyStr).def("__str__",&pyGLViewer::pyStr)
		.def("close",&pyGLViewer::close)
		.add_property("selection",&pyGLViewer::get_selection,&pyGLViewer::set_selection)
		;
}
#endif /* WOO_OPENGL */
