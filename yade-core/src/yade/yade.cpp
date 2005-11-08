/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*  Copyright (C) 2004 by Janek Kozicki                                   *
*  cosurgi@berlios.de                                                    *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include <iostream>
#include <getopt.h>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/convenience.hpp>
#include <yade/yade-lib-factory/ClassFactory.hpp>
#include "Omega.hpp"
#include "FrontEnd.hpp"
#include "Preferences.hpp"

using namespace std;

bool containOnlyWhiteSpaces(const string& str)
{
	if (str.size()==0)
		return true;

	unsigned int i=0;
	while (i<str.size() && (str[i]==' ' || str[i]=='\t' || str[i]=='\n'))
		i++;

	return (i==str.size());
}


void printHelp()
{
	cout << endl;
	cout << "Yet Another Dynamic Engine, beta. " << endl;
	cout << endl;
	cout << "	-h	: print this help" << endl;
	cout << "	-n	: use NullGUI instead of default one" << endl;
	cout << "	-g	: change default GUI" << endl;
	cout << "	-d	: add plugin directory" << endl;
	cout << "	-r	: reset current preferences" << endl;
	cout << endl;
}


void changeDefaultGUI(shared_ptr<Preferences>& pref)
{
	string guiName;
	string guiDir;
	filesystem::path guiFullPath;
	char tmpStr1[1000];
	char tmpStr2[1000];

	do 
	{
		cout << endl;
		cout << endl;
		cout << "1. Specify default GUI name [QtGUI] : " ;
		cin.getline(tmpStr1, 1000);
		guiName = tmpStr1;
		while (containOnlyWhiteSpaces(guiName))
			guiName = "QtGUI";

		cout << endl;
		cout << "2. Specify the path where to find it [/usr/local/lib/yade/yade-guis] : " ;
		cin.getline(tmpStr2, 1000);
		guiDir = tmpStr2;
		while (containOnlyWhiteSpaces(guiDir))
			guiDir = "/usr/local/lib/yade/yade-guis";
		cout << endl;

		if (guiDir[guiDir.size()-1]!='/')
			guiDir.push_back('/');

		guiFullPath = filesystem::path(guiDir+ClassFactory::instance().libNameToSystemName(guiName), filesystem::native);

		if (!filesystem::exists(guiFullPath))
		{
			cout << "##"<<endl;
			cout << "## Error : could not find file " << guiDir+ClassFactory::instance().libNameToSystemName(guiName) << endl;
			cout << "## Try Again" << endl;
			cout << "##"<<endl;
			cout << endl;
		}

	} while (!filesystem::exists(guiFullPath));
	
	pref->dynlibDirectories.push_back(guiDir.substr(0,guiDir.size()-1));
	pref->defaultGUILibName = guiName;

	string yadeConfigPath = string(getenv("HOME")) + string("/.yade");
	IOFormatManager::saveToFile("XMLFormatManager",yadeConfigPath+"/preferences.xml","preferences",pref);

}


void addPluginDirectory(shared_ptr<Preferences>& pref)
{
	string directory;
	filesystem::path path;
	do 
	{
		cout << endl;
		cout << endl;
		cout << "1. New plugin directory : " ;
		cin >> directory;
		cout << endl;

		if (directory[directory.size()-1]!='/')
			directory.push_back('/');

		path = filesystem::path(directory, filesystem::native);

		if (!filesystem::exists(path))
		{
			cout << "##"<<endl;
			cout << "## Error : could not find directory " << directory << endl;
			cout << "## Try Again" << endl;
			cout << "##"<<endl;
			cout << endl;
		}

	} while (!filesystem::exists(path));
	
	pref->dynlibDirectories.push_back(directory.substr(0,directory.size()-1));

	string yadeConfigPath = string(getenv("HOME")) + string("/.yade");
	IOFormatManager::saveToFile("XMLFormatManager",yadeConfigPath+"/preferences.xml","preferences",pref);

}


int main(int argc, char *argv[])
{
///
/// First time yade is loaded
///
	Omega::instance().preferences = shared_ptr<Preferences>(new Preferences);
	filesystem::path yadeConfigPath = filesystem::path(string(getenv("HOME")) + string("/.yade"), filesystem::native);

	if ( !filesystem::exists( yadeConfigPath ) )
	{
		filesystem::create_directories(yadeConfigPath);

		cout << endl;
		cout << endl;
		cout << "################################################" << endl;
		cout << "#	Welcome to Yade first use wizard	#" << endl;
		cout << "################################################" << endl;

		changeDefaultGUI(Omega::instance().preferences);
	}

	IOFormatManager::loadFromFile("XMLFormatManager",yadeConfigPath.string()+"/preferences.xml","preferences",Omega::instance().preferences);

///
/// Checks for command line argument
///
	int ch;
	bool useNullGUI = false;
	if( ( ch = getopt(argc,argv,"hngdr") ) != -1)
		switch(ch)
		{
			case 'h' :	printHelp();
					return 1;
			case 'n' :	useNullGUI = true;
					break;
			case 'g' :	changeDefaultGUI(Omega::instance().preferences);
					break;
			case 'd' :	addPluginDirectory(Omega::instance().preferences);
					break;
			case 'r' :	filesystem::remove_all(yadeConfigPath);
					break;
			default	 :	printHelp();
					return 1;
		}
	
	shared_ptr<FrontEnd> frontEnd;

	cout << "Please wait while loading plugins.........................." << endl;
	Omega::instance().scanPlugins();
	cout << "Plugins loaded."<<endl;

	Omega::instance().init();

	if (useNullGUI)
		frontEnd = dynamic_pointer_cast<FrontEnd>(ClassFactory::instance().createShared("NullGUI"));
	else
		frontEnd = dynamic_pointer_cast<FrontEnd>(ClassFactory::instance().createShared(Omega::instance().preferences->defaultGUILibName));

 	int ok = frontEnd->run(argc,argv);
	
	cout << "Yade: normal exit." << endl;

	return ok;
}

