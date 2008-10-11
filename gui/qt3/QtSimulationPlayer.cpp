/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include "QtSimulationPlayer.hpp"
#include "FileDialog.hpp"
#include "GLSimulationPlayerViewer.hpp"
#include<yade/core/FileGenerator.hpp>
#include<yade/pkg-common/FilterEngine.hpp>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include<qlabel.h>
#include<qstring.h>
CREATE_LOGGER(QtSimulationPlayer);

void QtSimulationPlayer::keyPressEvent(QKeyEvent* e){
	assert(glSimulationPlayerViewer);
	if(e->key()==Qt::Key_H && (e->state() & AltButton)){ glSimulationPlayerViewer->raise(); this->hide(); }
	else QtGeneratedSimulationPlayer::keyPressEvent(e);
}

QtSimulationPlayer::QtSimulationPlayer() : QtGeneratedSimulationPlayer(){
	glSimulationPlayerViewer=new GLSimulationPlayerViewer(NULL);
	glSimulationPlayerViewer->simPlayer=this;
	leInputConfigFile->setText(Omega::instance().getSimulationFileName());
	enableControls(false);
}
QtSimulationPlayer::~QtSimulationPlayer(){
	if(glSimulationPlayerViewer) delete glSimulationPlayerViewer;
	}
void QtSimulationPlayer::pbPlayClicked(){	setParameters(); glSimulationPlayerViewer->startAnimation();}
void QtSimulationPlayer::pbPauseClicked(){ glSimulationPlayerViewer->stopAnimation();}
void QtSimulationPlayer::pbStepClicked(){ glSimulationPlayerViewer->stopAnimation(); glSimulationPlayerViewer->doOneStep(); }
void QtSimulationPlayer::pbResetClicked(){ setParameters();	glSimulationPlayerViewer->reset();}
void QtSimulationPlayer::cbSaveSnapShotsToggled(bool b){	glSimulationPlayerViewer->saveSnapShots=b;}
void QtSimulationPlayer::cbAllowFiltrationToggled(bool b){	FilterEngine::isFiltrationActivated=b; }
void QtSimulationPlayer::cbBodyWireToggled(bool b){	glSimulationPlayerViewer->bodyWire(b); }
void QtSimulationPlayer::closeEvent(QCloseEvent *e){ QtGeneratedSimulationPlayer::closeEvent(e); emit closeSignal(); }


void QtSimulationPlayer::pbInputConfigFileClicked(){
	string selectedFilter;
	std::vector<string> filters;
	filters.push_back("XML Yade File (*.xml *.xml.gz *.xml.bz2)");
	filters.push_back("SQLite simulation states (*)");
	string fileName = FileDialog::getOpenFileName(".", filters, "Choose a file to load",NULL,selectedFilter );
	if (!fileName.empty()) // && selectedFilter== "XML Yade File (*.xml *.xml.gz *.xml.bz2)")
		leInputConfigFile->setText(fileName);
}
void QtSimulationPlayer::pbInputDirectoryClicked(){
	string directory = FileDialog::getExistingDirectory ( ".","Choose the directory where the recorded file are", NULL );
	if (!directory.empty()) leInputDirectory->setText(directory);
}
void QtSimulationPlayer::pbLoadClicked(){
	glSimulationPlayerViewer->stride=sbStride->value();
	glSimulationPlayerViewer->load(leInputConfigFile->text());
	leInputBaseName->setText(glSimulationPlayerViewer->inputBaseName);
	leInputDirectory->setText(glSimulationPlayerViewer->inputBaseDirectory);
}
void QtSimulationPlayer::pbOutputDirectoryClicked(){
	string directory = FileDialog::getExistingDirectory ( ".","Choose the directory where to save the snapshots", NULL );
	if (!directory.empty()) leOutputDirectory->setText(directory.c_str());
}
void QtSimulationPlayer::setParameters(){
	glSimulationPlayerViewer->inputBaseName=leInputBaseName->text().ascii();
	glSimulationPlayerViewer->inputBaseDirectory=leInputDirectory->text().ascii();
	glSimulationPlayerViewer->outputBaseName=leOutputBaseName->text().ascii();
	glSimulationPlayerViewer->outputBaseDirectory=leOutputDirectory->text().ascii();
	glSimulationPlayerViewer->snapshotsBase=string(leOutputDirectory->text().ascii())+"/"+leOutputBaseName->text().ascii();
	glSimulationPlayerViewer->stride=sbStride->value();
}

void QtSimulationPlayer::enableControls(bool enable=true){
	pbPlay->setEnabled(enable);
	pbStep->setEnabled(enable);
	pbPause->setEnabled(enable);
	pbReset->setEnabled(enable);
}

void QtSimulationPlayer::pushMessage(string msg){
	LOG_INFO(msg)
	messages.push_back(msg);
	if(messages.size()>15) messages.pop_front();
	QString disp;
	FOREACH(string m, messages){
		disp+=m+"\n";
	}
	tlStatus->setText(disp);
	tlStatus->adjustSize();
}
