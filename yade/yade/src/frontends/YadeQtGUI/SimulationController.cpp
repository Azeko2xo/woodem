#include "SimulationController.hpp"
#include "Omega.hpp"
#include "ThreadSynchronizer.hpp"
#include "Math.hpp"
#include "Threadable.hpp"



#include <qfiledialog.h>
#include <qlcdnumber.h>
#include <qlabel.h>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>


using namespace boost;


SimulationController::SimulationController(QWidget * parent) : QtGeneratedSimulationController(parent,"SimulationController")
{
	setMinimumSize(size());
	setMaximumSize(size());
	updater = shared_ptr<SimulationControllerUpdater>(new SimulationControllerUpdater(this));
}

SimulationController::~SimulationController()
{

}

void SimulationController::pbLoadClicked()
{	
	QString selectedFilter;
	QString fileName = QFileDialog::getOpenFileName("../data", "XML Yade File (*.xml)", this,"Open File","Choose a file to open",&selectedFilter );

	if (!fileName.isEmpty() && selectedFilter == "XML Yade File (*.xml)")
	{
		Omega::instance().setSimulationFileName(fileName);
		Omega::instance().loadSimulation();

		string fullName = string(filesystem::basename(fileName.data()))+string(filesystem::extension(fileName.data()));
		tlCurrentSimulation->setText(fullName);
		
		if (glViews.size()==0)
 		{
			boost::mutex resizeMutex;	
			boost::mutex::scoped_lock lock(resizeMutex);
			
			QGLFormat format;
			QGLFormat::setDefaultFormat( format );
			format.setStencil(TRUE);
			format.setAlpha(TRUE);
			glViews.push_back(new GLViewer(format,this->parentWidget()->parentWidget()));
		}
		
		Omega::instance().createSimulationLoop();
		
		Omega::instance().synchronizer->startAll();
		Omega::instance().stopSimulationLoop();
		
	}
}

void SimulationController::pbNewViewClicked()
{
	boost::mutex resizeMutex;	
	boost::mutex::scoped_lock lock(resizeMutex);

	QGLFormat format;
	QGLFormat::setDefaultFormat( format );
	format.setStencil(TRUE);
	format.setAlpha(TRUE);
	glViews.push_back(new GLViewer(format, this->parentWidget()->parentWidget(), glViews.front()) );
}

void SimulationController::pbStopClicked()
{
	cerr << "stop" << endl;
	Omega::instance().stopSimulationLoop();
}

void SimulationController::pbStartClicked()
{
	cerr << "start" << endl;
	Omega::instance().startSimulationLoop();
}

void SimulationController::pbResetClicked()
{

}



SimulationControllerUpdater::SimulationControllerUpdater(SimulationController * sc) : Threadable<SimulationControllerUpdater>()
{
	controller = sc;
	createThread(Omega::instance().synchronizer,true);
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
