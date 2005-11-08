/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include "QtPreferencesEditor.hpp"
#include "FileDialog.hpp"
#include <yade/yade-core/Omega.hpp>
#include <yade/yade-core/Preferences.hpp>
#include <qlineedit.h>
#include <qlistview.h>
#include <qlistbox.h>
#include <qwidgetstack.h>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>

QtPreferencesEditor::QtPreferencesEditor ( QWidget * parent , const char * name) : QtGeneratedPreferencesEditor(parent,name)
{
	loadPreferences();

	buildPluginsListView();
}


QtPreferencesEditor::~QtPreferencesEditor ()
{

}


void QtPreferencesEditor::closeEvent(QCloseEvent *evt)
{

	savePreferences();	

	QtGeneratedPreferencesEditor::closeEvent(evt);
}

/// List box

void QtPreferencesEditor::lbPreferencesListHighlighted(int i)
{
	wsPreferences->raiseWidget(i);
}

/// Include Folder Page
	
void QtPreferencesEditor::pbAddIncludeFolderClicked()
{
	if (!leIncludeFolder->text().isEmpty())
	{
		if (testDirectory(leIncludeFolder->text().data()))
			lvIncludeFolders->insertItem(new QListViewItem(lvIncludeFolders,leIncludeFolder->text()));
	}
}


void QtPreferencesEditor::pbDeleteIncludeFolderClicked()
{
	lvIncludeFolders->takeItem(lvIncludeFolders->currentItem());
}


void QtPreferencesEditor::pbIncludePathClicked()
{
	leIncludeFolder->setText(FileDialog::getExistingDirectory ( "./","Choose A New Inlude Folder",this->parentWidget()->parentWidget()).c_str());
}


void QtPreferencesEditor::lvIncludeFoldersSelectionChanged(QListViewItem* lvi)
{
	leIncludeFolder->setText(lvi->text(0));
}


void QtPreferencesEditor::leIncludeFolderReturnPressed()
{
	if (!leIncludeFolder->text().isEmpty() && testDirectory(leIncludeFolder->text().data()))
	{
			
		if (lvIncludeFolders->childCount()==0)
			lvIncludeFolders->insertItem(new QListViewItem(lvIncludeFolders,leIncludeFolder->text()));
		else if (lvIncludeFolders->selectedItem())
			lvIncludeFolders->selectedItem()->setText(0,leIncludeFolder->text());
	}	
}

/// Plugin Folder Page

void QtPreferencesEditor::pbAddPluginFolderClicked()
{
	if (!lePluginFolder->text().isEmpty())
	{
		if (testDirectory(lePluginFolder->text().data()))
			lvPluginFolders->insertItem(new QListViewItem(lvPluginFolders,lePluginFolder->text()));
	}
}


void QtPreferencesEditor::pbDeletePluginFolderClicked()
{
	lvPluginFolders->takeItem(lvPluginFolders->currentItem());
}



void QtPreferencesEditor::pbPluginPathClicked()
{
	lePluginFolder->setText(FileDialog::getExistingDirectory ( "./","Choose A New Inlude Folder",this->parentWidget()->parentWidget()).c_str());
}


void QtPreferencesEditor::lvPluginFoldersSelectionChanged(QListViewItem* lvi)
{
	lePluginFolder->setText(lvi->text(0));
}


void QtPreferencesEditor::lePluginFolderReturnPressed()
{
	if (!lePluginFolder->text().isEmpty() && testDirectory(lePluginFolder->text().data()))
	{
			
		if (lvPluginFolders->childCount()==0)
			lvPluginFolders->insertItem(new QListViewItem(lvPluginFolders,lePluginFolder->text()));
		else if (lvPluginFolders->selectedItem())
			lvPluginFolders->selectedItem()->setText(0,lePluginFolder->text());
	}	
}

/// Plugins Page

void QtPreferencesEditor::pbRescanPluginsClicked()
{
	scanPlugins();
}

/// Misc

bool QtPreferencesEditor::testDirectory(const string& dirName)
{
	filesystem::path path = filesystem::path(dirName, filesystem::native);
	return filesystem::exists( path );
}



void QtPreferencesEditor::loadPreferences()
{
	filesystem::path yadeConfigPath = filesystem::path(string(getenv("HOME")) + string("/.yade"), filesystem::native);
	IOFormatManager::loadFromFile("XMLFormatManager",yadeConfigPath.string()+"/preferences.xml","preferences",Omega::instance().preferences);

	vector<string>::iterator idi    = Omega::instance().preferences->includeDirectories.begin();
	vector<string>::iterator idiEnd = Omega::instance().preferences->includeDirectories.end();
	for( ; idi!=idiEnd ; ++idi)
		lvIncludeFolders->insertItem(new QListViewItem(lvIncludeFolders,(*idi).c_str()));

	vector<string>::iterator dldi    = Omega::instance().preferences->dynlibDirectories.begin();
	vector<string>::iterator dldiEnd = Omega::instance().preferences->dynlibDirectories.end();
	for( ; dldi!=dldiEnd ; ++dldi)
		lvPluginFolders->insertItem(new QListViewItem(lvPluginFolders,(*dldi).c_str()));
}


void QtPreferencesEditor::savePreferences()
{
	filesystem::path yadeConfigPath = filesystem::path(string(getenv("HOME")) + string("/.yade"), filesystem::native);

	Omega::instance().preferences->dynlibDirectories.clear();
	Omega::instance().preferences->includeDirectories.clear();

	QListViewItem * currentItem = lvIncludeFolders->firstChild();
	while (currentItem)
	{
		Omega::instance().preferences->includeDirectories.push_back(currentItem->text(0).data());
		currentItem=currentItem->nextSibling();	
	}

	currentItem = lvPluginFolders->firstChild();
	while (currentItem)
	{
		Omega::instance().preferences->dynlibDirectories.push_back(currentItem->text(0).data());
		currentItem=currentItem->nextSibling();	
	}

	IOFormatManager::saveToFile("XMLFormatManager",yadeConfigPath.string()+"/preferences.xml","preferences",Omega::instance().preferences);
}


void QtPreferencesEditor::scanPlugins()
{
	savePreferences();
	
	Omega::instance().scanPlugins();
	
	buildPluginsListView();
}


#include <qpixmap.h>

void QtPreferencesEditor::buildPluginsListView()
{
	lvPluginsList->clear();

// 	QCheckListItem * data		= new QCheckListItem(lvPluginsList,"Data",);
// 	QCheckListItem * engine		= new QCheckListItem(lvPluginsList,"Engine");
// 	QCheckListItem * container	= new QCheckListItem(lvPluginsList,"Container");
// 
// 	QCheckListItem * state			= new QCheckListItem(data,"State");
// 	QCheckListItem * physicalParameters	= new QCheckListItem(data,"PhysicalParameters");
// 	QCheckListItem * geometricalModel	= new QCheckListItem(data,"GeometricalModel");
// 	QCheckListItem * interactingGeometry	= new QCheckListItem(data,"InteractingGeometry");
// 	QCheckListItem * boundingVolume		= new QCheckListItem(data,"BoundingVolume");
// 	QCheckListItem * interactionGeometry	= new QCheckListItem(data,"InteractionGeometry");
// 	QCheckListItem * interactionPhysics	= new QCheckListItem(data,"InteractionPhysics");
// 	QCheckListItem * physicalAction 	= new QCheckListItem(data,"PhysicalAction");

	//QCheckListItem * engine = new QCheckListItem(lvPluginsList,"Engine")
	//QCheckListItem * container = new QCheckListItem(lvPluginsList,"Container")

	map<string,DynlibDescriptor>::const_iterator di    = Omega::instance().getDynlibsDescriptor().begin();
	map<string,DynlibDescriptor>::const_iterator diEnd = Omega::instance().getDynlibsDescriptor().end();
	for(;di!=diEnd;++di)
		lvPluginsList->insertItem(new QListViewItem(lvPluginsList,(*di).first));

}
