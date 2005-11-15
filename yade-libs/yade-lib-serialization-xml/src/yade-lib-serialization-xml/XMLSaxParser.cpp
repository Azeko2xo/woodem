/*************************************************************************
*  Copyright (C) 2004 by Olivier Galizzi                                 *
*  olivier.galizzi@imag.fr                                               *
*                                                                        *
*  This program is free software; it is licensed under the terms of the  *
*  GNU General Public License v2 or later. See file LICENSE for details. *
*************************************************************************/

#include "XMLSaxParser.hpp"

XmlSaxParser::XmlSaxParser()
{
	lineNumber = 0;
}


XmlSaxParser::~XmlSaxParser ()
{
	basicAttributes.clear();
}


unsigned long int XmlSaxParser::getLineNumber()
{
	return (lineNumber-1);
}


bool XmlSaxParser::readAndParseNextXmlLine(istream& stream)
{
	if (readNextXmlLine(stream))
	{
		parseCurrentXmlLine();
		return true;
	}
	else
		return false;

}


bool XmlSaxParser::isWhiteSpace(char c)
{
	return (c==' ' || c=='\n' || c=='\t');
}


string XmlSaxParser::readNextFundamentalStringValue(istream& stream)
{
	string line="";

	if (!stream.eof())
	{
		while (!stream.eof() && isWhiteSpace(stream.peek()))
			stream.get();
		while (!stream.eof() && stream.peek()!='<')
			line.push_back(stream.get());
	}
	return line;
}


bool XmlSaxParser::readNextXmlLine(istream& stream)
{
	int i;
	string line;
	bool comment=true;

	while (comment)
	{
		line.resize(1);
		if (!stream.eof())
		{
			line[0] = stream.get();
			while (!stream.eof() && line[0]!='<')
			{
				if(line[0]=='\n') ++lineNumber;
				line[0] = stream.get();
			}
			i=0;
			while (!stream.eof() && line[i]!='>')
			{
				i++;
				line.push_back(stream.get());
				if(line[line.size()-1]=='\n') ++lineNumber;
			}

			if (line.size()==1)
				return false;
			else
			{
				currentXmlLine = line;
				comment = isComment();
			}
		}
		else
			return false;
	}

	return true;
}


void XmlSaxParser::parseCurrentXmlLine()
{
	int i;
	string argName;
	int argValueStart,argValueStop,argNameStart,argNameStop;

	basicAttributes.clear();

	currentLineCopy = currentXmlLine;

	if (isOpeningTag())
	{
		i=1;
		while (currentXmlLine[i]!=' ' && currentXmlLine[i]!='\n' && currentXmlLine[i]!='\t' && currentXmlLine[i]!='>')
			i++;
		tagName = currentXmlLine.substr(1,i-1);
	}
	else
	{
		i=2;
		while (currentXmlLine[i]!='>')
			i++;
		tagName = currentXmlLine.substr(2,i-2);
	}

	argNameStop = findCar(0,'=')-1;
	while (argNameStop>0 && currentXmlLine[argNameStop]==' ' && currentXmlLine[i]!='\n' && currentXmlLine[i]!='\t')
		argNameStop--;
	argNameStart = argNameStop;

	while (argNameStop>0) // car findCar retourne -1 si non trouve
	{
		while (currentXmlLine[argNameStart]!=' ' && currentXmlLine[i]!='\n' && currentXmlLine[i]!='\t')
			argNameStart--;
		argNameStart++; // on est sur le premier car de l'argname

		argName = currentLineCopy.substr(argNameStart,argNameStop-argNameStart+1);
		argValueStart = findCar(argNameStop,'"')+1;
		argValueStop = findCar(argValueStart,'"')-1;

		basicAttributes[argName] = currentLineCopy.substr(argValueStart,argValueStop-argValueStart+1);

		argNameStop = findCar(argValueStop,'=') - 1;
		while (argNameStop>0 && currentXmlLine[argNameStop]==' ')
			argNameStop--;
		argNameStart = argNameStop;
	}
}


int XmlSaxParser::findCar(int i,char c)
{
	int j;
	j=i;
	while (static_cast<unsigned int>(j)<=currentXmlLine.length() && currentXmlLine[j]!=c)
		j++;

	if (static_cast<unsigned int>(j)>currentXmlLine.length())
		j=-1;

	return j;
}


bool XmlSaxParser::isFullTag()
{
	return (isOpeningTag() && currentXmlLine[currentXmlLine.length()-2]=='/');
}


bool XmlSaxParser::isOpeningTag()
{
	return (currentXmlLine[1]!='/');
}


bool XmlSaxParser::isClosingTag()
{
	return (!isOpeningTag());
}


bool XmlSaxParser::isComment()
{
	if (currentXmlLine.length()<7)
		return false;

	return (currentXmlLine[1]=='!' && currentXmlLine[2]=='-' && currentXmlLine[3]=='-');
}


string XmlSaxParser::getTagName()
{
	return tagName;
}


string XmlSaxParser::getArgumentValue(const string& argName)
{
	map<string,string>::iterator ita = basicAttributes.find(argName);

	if (ita == basicAttributes.end())
		return NULL;

	return basicAttributes[argName];

}


const map<string,string>& XmlSaxParser::getBasicAttributes()
{
	return basicAttributes;
}


void XmlSaxParser::deleteBasicAttribute(const string& name)
{
	basicAttributes.erase(name);
}

