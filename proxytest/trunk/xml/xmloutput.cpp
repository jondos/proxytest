/*
	xmloutput.cpp
	Utility class for outputting XML data files.

	Copyright (c) 2000 Paul T. Miller

	LGPL DISCLAIMER
	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Library General Public
	License as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.

	This library is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Library General Public License for more details.

	You should have received a copy of the GNU Library General Public
	License along with this library; if not, write to the
	Free Software Foundation, Inc., 59 Temple Place - Suite 330,
	Boston, MA  02111-1307, USA.

	http://www.gnu.org/copyleft/lgpl.html
*/
#include "../StdAfx.h"
#include "xmloutput.h"
//#include <stdio.h>
//#include <assert.h>

//XML_BEGIN_NAMESPACE

XMLOutput::XMLOutput(XMLOutputStream &stream) : mStream(stream)
{
	mLevel = 0;
	mAttributes = false;
}

void XMLOutput::write(const char *str, size_t len)
{
	mStream.write(str, len);
}

void XMLOutput::writeString(const char *str)
{
	assert(str);
	write(str, strlen(str));
}

void XMLOutput::writeLine(const char *str)
{
	assert(str);
	write(str, strlen(str));
	write("\n", 1);
}

/*
XMLOutput &XMLOutput::operator<<(const std::string &str)
{
	write(str.c_str(), str.size());
	return *this;
}
*/

XMLOutput &XMLOutput::operator<<(const char *str)
{
	assert(str);
	writeString(str);
	return *this;
}

XMLOutput &XMLOutput::operator<<(int value)
{
	char tmp[50];
	sprintf(tmp, "%d", value);
	writeString(tmp);
	return *this;
}

XMLOutput &XMLOutput::operator<<(unsigned int value)
{
	char tmp[50];
	sprintf(tmp, "%d", value);
	writeString(tmp);
	return *this;
}

XMLOutput &XMLOutput::operator<<(double value)
{
	char tmp[50];
	sprintf(tmp, "%g", value);
	writeString(tmp);
	return *this;
}

XMLOutput &XMLOutput::operator<<(bool value)
{
	writeString(value ? "True" : "False");
	return *this;
}

void XMLOutput::BeginDocument(const char *version, const char *encoding, bool standalone)
{
	assert(version);
	assert(encoding);

	(*this) << "<?xml version=\"" << version << "\" encoding=\"" << encoding << "\"";
	(*this) << " standalone=\"" << (standalone ? "yes" : "no") << "\"?>\n";
}

void XMLOutput::EndDocument()
{
	assert(!mAttributes);
	assert(mElements.empty());
}

void XMLOutput::Indent()
{
	for (int i = 0; i < mLevel; i++)
		(*this) << "\t";
}

void XMLOutput::BeginElement(const char *name, Mode mode)
{
	assert(name);
	assert(!mAttributes);
	Indent();
	mLevel++;
	(*this) << "<" << name << ">";
	if (mode != terse)
		(*this) << "\n";

	mElements.push_back(name);
}

void XMLOutput::BeginElementAttrs(const char *name)
{
	assert(name);
	assert(!mAttributes);
	Indent();
	mLevel++;
	(*this) << "<" << name;
	mAttributes = true;

	mElements.push_back(name);
}

void XMLOutput::EndAttrs(Mode mode)
{
	assert(mAttributes);
	mAttributes = false;
	(*this) << ">";
	if (mode != terse)
		(*this) << "\n";
}

void XMLOutput::EndElement(Mode mode)
{
	assert(mElements.size() > 0);
	assert(!mAttributes);
	assert(mLevel > 0);
	--mLevel;

	if (mode != terse)
		Indent();

	const char *name = mElements.back();
	mElements.pop_back();

	(*this) << "</" << name << ">" << "\n";
}
/*
void XMLOutput::WriteElement(const char *name, const std::string &value)
{
	assert(name);
	BeginElement(name, terse);
	(*this) << value;
	EndElement(terse);
}
*/
void XMLOutput::WriteElement(const char *name, const char *value)
{
	assert(name);
	assert(value);
	BeginElement(name, terse);
	(*this) << value;
	EndElement(terse);
}

void XMLOutput::WriteElement(const char *name, int value)
{
	assert(name);
	BeginElement(name, terse);
	(*this) << value;
	EndElement(terse);
}

void XMLOutput::WriteElement(const char *name, unsigned int value)
{
	assert(name);
	BeginElement(name, terse);
	(*this) << value;
	EndElement(terse);
}

void XMLOutput::WriteElement(const char *name, double value)
{
	assert(name);
	BeginElement(name, terse);
	(*this) << value;
	EndElement(terse);
}

void XMLOutput::WriteElement(const char *name, bool value)
{
	assert(name);
	BeginElement(name, terse);
	(*this) << value;
	EndElement(terse);
}

/*
void XMLOutput::WriteAttr(const char *name, const std::string &value)
{
	assert(mAttributes);
	assert(name);
	(*this) << " " << name << "=\"" << value << "\"";
}
*/
void XMLOutput::WriteAttr(const char *name, const char *value)
{
	assert(mAttributes);
	assert(name);
	assert(value);

	(*this) << " " << name << "=\"" << value << "\"";
}

void XMLOutput::WriteAttr(const char *name, int value)
{
	assert(mAttributes);
	assert(name);

	(*this) << " " << name << "=\"" << value << "\"";
}

void XMLOutput::WriteAttr(const char *name, double value)
{
	assert(mAttributes);
	assert(name);

	(*this) << " " << name << "=\"" << value << "\"";
}

void XMLOutput::WriteAttr(const char *name, bool value)
{
	assert(mAttributes);
	assert(name);
	
	(*this) << " " << name << "=\"" << value << "\"";
}

//XML_END_NAMESPACE

