#include "SimulationController.hpp"
#include "Omega.hpp"
#include "ThreadSynchronizer.hpp"
#include "Math.hpp"
#include "Threadable.hpp"

#include "OpenGLRenderingEngine.hpp"


#include <qfiledialog.h>
#include <qlcdnumber.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qgroupbox.h>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <unistd.h>


using namespace boost;


SimulationController::SimulationController(QWidget * parent) : QtGeneratedSimulationController(parent,"SimulationController")
{
	setMinimumSize(size());
	setMaximumSize(size());

//	while(! renderer )
// FIXME - what is going on here? it was crashing rabdomly unless I added these lines...
	shared_ptr<Factorable> tmpRenderer = ClassFactory::instance().createShared("OpenGLRenderingEngine");
	shared_ptr<OpenGLRenderingEngine> tmp2 = dynamic_pointer_cast<OpenGLRenderingEngine>(tmpRenderer);
	renderer = dynamic_pointer_cast<RenderingEngine>(tmp2);
	
	if(renderer)
	{
		guiGen.setResizeHeight(true);
		guiGen.setResizeWidth(false);
		guiGen.setShift(10,30);
		guiGen.setShowButtons(false);
		guiGen.buildGUI(renderer, gbRenderingEngineParameters);
	}
	else
	{
		cerr << "renderer not created - why?!\n";
	}
	
	maxNbViews=0;
	addNewView();
	
	updater = shared_ptr<SimulationControllerUpdater>(new SimulationControllerUpdater(this));
}

SimulationController::~SimulationController()
{
	terminateAllThreads();
	
	map<int,GLViewer*>::iterator gi = glViews.begin();
	map<int,GLViewer*>::iterator giEnd = glViews.end();
	for(;gi!=giEnd;++gi)
		delete (*gi).second;
	glViews.clear();
}

void SimulationController::terminateAllThreads()
{
	updater->finish();
	updater->join();
	
	map<int,GLViewer*>::iterator gi = glViews.begin();
	map<int,GLViewer*>::iterator giEnd = glViews.end();
	for(;gi!=giEnd;++gi)
	{
		(*gi).second->finishRendering();
		(*gi).second->joinRendering();
	}
		
	Omega::instance().finishSimulationLoop();
	Omega::instance().joinSimulationLoop();
	//while (Omega::instance().synchronizer->getNbThreads()!=0);
}

void SimulationController::pbApplyClicked()
{
	guiGen.deserialize(renderer);
}

void SimulationController::pbLoadClicked()
{
	boost::mutex resizeMutex;
	boost::mutex::scoped_lock lock(resizeMutex);

	QString selectedFilter;
	QString fileName = QFileDialog::getOpenFileName("../data", "XML Yade File (*.xml)", this,"Open File","Choose a file to open",&selectedFilter );

	if (!fileName.isEmpty() && selectedFilter == "XML Yade File (*.xml)")
	{
		map<int,GLViewer*>::iterator gi = glViews.begin();
		map<int,GLViewer*>::iterator giEnd = glViews.end();
		//for(;gi!=giEnd;++gi)
		//	(*gi).second->stopRendering();
		
		Omega::instance().setSimulationFileName(fileName);
		Omega::instance().loadSimulation();
		
		gi = glViews.begin();
		for(;gi!=giEnd;++gi)
			(*gi).second->centerScene();
		
		string fullName = string(filesystem::basename(fileName.data()))+string(filesystem::extension(fileName.data()));
		tlCurrentSimulation->setText(fullName);

		Omega::instance().createSimulationLoop();
		//Omega::instance().stopSimulationLoop();

		//gi = glViews.begin();
		//for(;gi!=giEnd;++gi)
		//	(*gi).second->startRendering();
	}
} 

void SimulationController::pbNewViewClicked()
{
	boost::mutex resizeMutex;
	boost::mutex::scoped_lock lock(resizeMutex);

	addNewView();
	
}

void SimulationController::addNewView()
{
	boost::mutex resizeMutex;
	boost::mutex::scoped_lock lock(resizeMutex);

	QGLFormat format;
	QGLFormat::setDefaultFormat( format );
	format.setStencil(TRUE);
	format.setAlpha(TRUE);

	
	if (glViews.size()==0)
	{
			glViews[0] = new GLViewer(0,renderer,format,this->parentWidget()->parentWidget());
			maxNbViews = 0;
	}
	else
	{
		maxNbViews++;
		glViews[maxNbViews] = new GLViewer(maxNbViews,renderer, format, this->parentWidget()->parentWidget(), glViews[0]);
	}
	
	connect( glViews[maxNbViews], SIGNAL( closeSignal(int) ), this, SLOT( closeGLViewEvent(int) ) );
	glViews[maxNbViews]->centerScene();
	glViews[maxNbViews]->startRendering();
}







void SimulationController::closeGLViewEvent(int id)
{
//	int n = Omega::instance().synchronizer->getNbThreads();
	if (id!=0)
	{
		//for(unsigned int i=0;i<glViews.size();i++)
		glViews[id]->finishRendering();
		glViews[id]->joinRendering();
	//	while (Omega::instance().synchronizer->getNbThreads()!=n-1);
		delete glViews[id];
		glViews.erase(id);
		//glViews[id] = 0;
		if (id==maxNbViews)
			maxNbViews--;
	}
}

void SimulationController::pbStopClicked()
{
	boost::mutex resizeMutex;
	boost::mutex::scoped_lock lock(resizeMutex);
	Omega::instance().stopSimulationLoop();
	updater->stop();
}

void SimulationController::pbStartClicked()
{
	boost::mutex resizeMutex;
	boost::mutex::scoped_lock lock(resizeMutex);
	Omega::instance().startSimulationLoop();	
	updater->start();
}

void SimulationController::pbResetClicked()
{
	boost::mutex resizeMutex;
	boost::mutex::scoped_lock lock(resizeMutex);
	Omega::instance().loadSimulation();
	Omega::instance().stopSimulationLoop();
}

void SimulationController::pbCenterSceneClicked()
{
	boost::mutex resizeMutex;
	boost::mutex::scoped_lock lock(resizeMutex);
	map<int,GLViewer*>::iterator gi = glViews.begin();
	map<int,GLViewer*>::iterator giEnd = glViews.end();
	for(;gi!=giEnd;++gi)
		(*gi).second->centerScene();
}

void SimulationController::closeEvent(QCloseEvent *)
{
	emit closeSignal();
}
















SimulationControllerUpdater::SimulationControllerUpdater(SimulationController * sc) : Threadable<SimulationControllerUpdater>()
{
	controller = sc;
	createThread(true,Omega::instance().synchronizer);
}

SimulationControllerUpdater::~SimulationControllerUpdater()
{

}

void SimulationControllerUpdater::oneLoop()
{

	controller->lcdCurrentIteration->display(lexical_cast<string>(Omega::instance().getCurrentIteration()));
	double simulationTime = Omega::instance().getSimulationTime();

	unsigned int sec	= (unsigned int)(simulationTime);
	unsigned int min	= sec/60;
	double time		= (simulationTime-sec)*1000;
	unsigned int msec	= (unsigned int)(time);
	time			= (time-msec)*1000;
	unsigned int misec	= (unsigned int)(time);
	time			= (time-misec)*1000;
	unsigned int nsec	= (unsigned int)(time);
	sec			= sec-60*min;

	controller->lcdMinutev->display(lexical_cast<string>(min));
	controller->lcdSecondv->display(lexical_cast<string>(sec));
	controller->lcdMSecondv->display(lexical_cast<string>(msec));
	controller->lcdMiSecondv->display(lexical_cast<string>(misec));
	controller->lcdNSecondv->display(lexical_cast<string>(nsec));

	time_duration duration = microsec_clock::local_time()-Omega::instance().msStartingSimulationTime;

	unsigned int hours	= duration.hours();
	unsigned int minutes 	= duration.minutes();
	unsigned int seconds	= duration.seconds();
	unsigned int mseconds	= duration.fractional_seconds()/1000;
	unsigned int days 	= hours/24;
	hours			= hours-24*days;

	controller->lcdDay->display(lexical_cast<string>(days));
	controller->lcdHour->display(lexical_cast<string>(hours));
	controller->lcdMinute->display(lexical_cast<string>(minutes));
	controller->lcdSecond->display(lexical_cast<string>(seconds));
	controller->lcdMSecond->display(lexical_cast<string>(mseconds));

}

bool SimulationControllerUpdater::notEnd()
{
	return true;
}
