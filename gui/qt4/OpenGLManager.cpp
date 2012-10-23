#ifdef WOO_OPENGL
#include<woo/gui/qt4/OpenGLManager.hpp>

CREATE_LOGGER(OpenGLManager);

OpenGLManager* OpenGLManager::self=NULL;

OpenGLManager::OpenGLManager(QObject* parent): QObject(parent){
	if(self) throw runtime_error("OpenGLManager instance already exists, uses OpenGLManager::self to retrieve it.");
	self=this;
	// Renderer::init(); called automatically when Renderer::render() is called for the first time
	viewsMutexMissed=0;
	connect(this,SIGNAL(createView()),this,SLOT(createViewSlot()));
	connect(this,SIGNAL(resizeView(int,int,int)),this,SLOT(resizeViewSlot(int,int,int)));
	connect(this,SIGNAL(closeView(int)),this,SLOT(closeViewSlot(int)));
	connect(this,SIGNAL(startTimerSignal()),this,SLOT(startTimerSlot()),Qt::QueuedConnection);
}

void OpenGLManager::timerEvent(QTimerEvent* event){
#if 1
	boost::mutex::scoped_lock lock(viewsMutex);
	for(const auto& view: views){ if(view) view->updateGL(); }
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
	startTimer(50);
}

int OpenGLManager::waitForNewView(float timeout,bool center){
	size_t origViewCount=views.size();
	emitCreateView();
	float t=0;
	while(views.size()!=origViewCount+1){
		usleep(50000); t+=.05;
		// wait at most 5 secs
		if(t>=timeout) {
			LOG_ERROR("Timeout waiting for the new view to open, giving up."); return -1;
		}
	}
	if(center)(*views.rbegin())->centerScene();
	return (*views.rbegin())->viewId; 
}

#endif /* WOO_OPENGL */
