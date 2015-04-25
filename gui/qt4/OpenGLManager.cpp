#ifdef WOO_OPENGL
// use local includes, since MOC generated files do that as well
#include"OpenGLManager.hpp"
#include<woo/core/Timing.hpp>

WOO_IMPL_LOGGER(OpenGLManager);

OpenGLManager* OpenGLManager::self=NULL;

OpenGLManager::OpenGLManager(QObject* parent): QObject(parent){
	if(self) throw runtime_error("OpenGLManager instance already exists, uses OpenGLManager::self to retrieve it.");
	self=this;
	// move the whole rendering thing to a different thread
	// this should make the UI responsive regardless of how long the rendering takes
	// BROKEN!!
	#if 0
		renderThread=new QThread(); // is the thread going to be deleted later automatically?!
		this->moveToThread(renderThread);
		renderThread->start();
	#endif
	// Renderer::init(); called automatically when Renderer::render() is called for the first time
	viewsMutexMissed=0;
	frameMeasureTime=0;
	connect(this,SIGNAL(createView()),this,SLOT(createViewSlot()));
	connect(this,SIGNAL(resizeView(int,int,int)),this,SLOT(resizeViewSlot(int,int,int)));
	connect(this,SIGNAL(closeView(int)),this,SLOT(closeViewSlot(int)));
	connect(this,SIGNAL(startTimerSignal()),this,SLOT(startTimerSlot()),Qt::QueuedConnection);
}

void OpenGLManager::timerEvent(QTimerEvent* event){
#if 1
	if(views.empty() || !views[0]) return;
	boost::mutex::scoped_lock lock(viewsMutex,boost::try_to_lock);
	if(!lock) return;
	if(views.size()>1) LOG_WARN("Only one (primary) view will be rendered.");
	Real t;
	bool measure=(frameMeasureTime>=Renderer::maxFps);
	if(measure){ t=woo::TimingInfo::getNow(/*evenIfDisabled*/true); }
	views[0]->updateGL();
	if(measure){
		frameMeasureTime=0;
		t=1e-9*(woo::TimingInfo::getNow(/*evenIfDisabled*/true)-t); // in seconds
		Real& dt(Renderer::fastDraw?Renderer::fastRenderTime:Renderer::renderTime);
		if(isnan(dt)) dt=t;
		dt=.6*dt+.4*(t);
	} else {
		frameMeasureTime++;
	}

	if(maxFps!=Renderer::maxFps){
		killTimer(renderTimerId);
		maxFps=Renderer::maxFps;
		renderTimerId=startTimer(1000/Renderer::maxFps);
	}
	//for(const auto& view: views){
	//	if(view) view->updateGL();
	//}
#else
	// this implementation makes the GL idle on subsequent timers, if the rednering took longer than one timer shot
	// as many tim ers as the waiting took are the idle
	// this should improve ipython-responsivity under heavy GL loads
	boost::mutex::scoped_try_lock lock(viewsMutex);
	if(lock.owns_lock()){
		if(viewsMutexMissed>1) viewsMutexMissed-=1;
		else{
			viewsMutexMissed=0;
			for(const auto& view: views){ if(view) view->updateGL(); }
		}
	} else {
		viewsMutexMissed+=1;
	}
#endif
}

void OpenGLManager::createViewSlot(){
	boost::mutex::scoped_lock lock(viewsMutex);
	if(views.size()==0){
		views.push_back(shared_ptr<GLViewer>(new GLViewer(0,/*shareWidget*/(QGLWidget*)0)));
	} else {
		throw runtime_error("Secondary views not supported");
		//views.push_back(shared_ptr<GLViewer>(new GLViewer(views.size(),renderer,views[0].get())));
	}	
}

void OpenGLManager::resizeViewSlot(int id, int wd, int ht){
	views[id]->resize(wd,ht);
}

void OpenGLManager::closeViewSlot(int id){
	boost::mutex::scoped_lock lock(viewsMutex);
	for(long i=(long)views.size()-1; (!views[i]) && i>=0; i--){ views.resize(i); } // delete empty views from the end
	if(id<0){ // close the last one existing
		assert(*views.rbegin()); // this should have been sanitized by the loop above
		views.resize(views.size()-1); // releases the pointer as well
	}
	if(id==0){
		LOG_DEBUG("Closing primary view.");
		if(views.size()==1) views.clear();
		else{ LOG_INFO("Cannot close primary view, secondary views still exist."); }
	}
}
void OpenGLManager::centerAllViews(){
	boost::mutex::scoped_lock lock(viewsMutex);
	FOREACH(const shared_ptr<GLViewer>& g, views){ if(!g) continue; g->centerScene(); }
}
void OpenGLManager::startTimerSlot(){
	maxFps=Renderer::maxFps; // remember the last value
	renderTimerId=startTimer(1000/Renderer::maxFps);
}

int OpenGLManager::waitForNewView(float timeout,bool center){
	size_t origViewCount=views.size();
	emitCreateView();
	float t=0;
	while(views.size()!=origViewCount+1){
		boost::this_thread::sleep(boost::posix_time::milliseconds(50));
		t+=.05;
		// wait at most 5 secs
		if(t>=timeout) {
			LOG_ERROR("Timeout waiting for the new view to open, giving up."); return -1;
		}
	}
	if(center)(*views.rbegin())->centerScene();
	return (*views.rbegin())->viewId; 
}

#endif /* WOO_OPENGL */
