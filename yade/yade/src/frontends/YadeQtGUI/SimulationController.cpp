#include "SimulationController.hpp"
#include "Omega.hpp"
#include "ThreadSynchronizer.hpp"
#include "Math.hpp"
#include "Threadable.hpp"



#include <qfiledialog.h>
#include <qlcdnumber.h>
#include <qlabel.h>
#include <boost/lexical_cast.hpp>


using namespace boost;


SimulationController::SimulationController(QWidget * parent) : QtGeneratedSimulationController(parent,"SimulationController")
{
	setMinimumSize(size());
	setMaximumSize(size());
	//updater = shared_ptr<SimulationControllerUpdater>(new SimulationControllerUpdater(this));
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

		tlCurrentSimulation->setText(fileName);
		
		if (glViews.size()==0)
 		{
				boost::mutex resizeMutex;	
				boost::mutex::scoped_lock lock(resizeMutex);

			glViews.push_back(new GLViewer(this->parentWidget()->parentWidget()));
			//glViews.push_back(new GLViewer( this->parentWidget()->parentWidget(), glViews.front()->glView ) );
		}
		Omega::instance().synchronizer->startAll();
		//Omega::instance().stopSimulationLoop();
		
	}
}

void SimulationController::pbNewViewClicked()
{
	boost::mutex resizeMutex;	
	boost::mutex::scoped_lock lock(resizeMutex);

	glViews.push_back(new GLViewer( this->parentWidget()->parentWidget(), glViews.front()/*->glView*/ ) );
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
	createThread(Omega::instance().synchronizer);
}

SimulationControllerUpdater::~SimulationControllerUpdater()
{ 

}
//#define THREAD_DEBUG

void SimulationControllerUpdater::oneLoop()
{
#ifndef THREAD_DEBUG
	controller->lcdCurrentIteration->display(lexical_cast<string>(Omega::instance().getCurrentIteration()));
	double simulationTime = Omega::instance().getSimulationTime();
#else
	ThreadSafe::cerr("mark:  10 " + string(typeid(*this).name()) );
	controller->lcdCurrentIteration->display(lexical_cast<string>(Omega::instance().getCurrentIteration()));
	
	ThreadSafe::cerr("mark:  11 " + string(typeid(*this).name()) );
	double simulationTime = Omega::instance().getSimulationTime();
	
	ThreadSafe::cerr("mark:  12 " + string(typeid(*this).name()) );
#endif
	
	/*long int nsec	= (long int)(simulationTime*1000000000);
	long int misec	= (long int)(simulationTime*1000000000);
	 
	int sec		= (int)((simulationTime));
	cout << sec << endl;
	int min		= (int)(((double)sec/60.0));	
	
	
	
	double infSec	= (simulationTime-sec)*1000;
	int msec	= infSec;
	cout << (simulationTime-sec)*1000 << endl;
	infSec		= (infSec-msec)*1000; 
	int misec	= (int)((infSec));
	cout << misec << endl;
	infSec		= (infSec-misec)*1000;
	int nsec	= (int)((infSec));
	sec		= sec-60*min;

cout << simulationTime << "  ||  " << min << " " << sec << " " << msec << " " << misec << " " << nsec << endl;
	lcdMinutev->display(lexical_cast<string>(min));
	lcdSecondv->display(lexical_cast<string>(sec));
	lcdMSecondv->display(lexical_cast<string>(msec));
	lcdMiSecondv->display(lexical_cast<string>(misec));
	lcdNSecondv->display(lexical_cast<string>(nsec));*/
}

bool SimulationControllerUpdater::notEnd()
{
	return true;
}
