/*
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
#include "xmlinputp.h"
#include "xmlstream.h"


struct InputImpl
{
	::XML_InputStream adapter;
	XMLInputStream *stream;
	::XML_Input *input;
	void *userData;
};

// convert XML_Element to XML::XMLElement and handle exceptions
::XML_Error XMLInput::elementHandler(::XML_Input *input, ::XML_Element *elem, const ::XML_Handler *handler, void *userData)
{
	XMLElement e(elem);
	HandlerProc proc = (HandlerProc)handler->info.Element.proc;
	try {
		(*proc)(e, handler->info.Element.userData ? handler->info.Element.userData : userData);
	}
	catch (const XMLParseException &e)
	{
		return e.GetError();
	}
	return ::XML_Error_None;
}

::XML_Error XMLInput::dataHandler(::XML_Input *input, const ::XML_Char *data, size_t len, const ::XML_Handler *handler, void *userData)
{
	DataProc proc = (DataProc)handler->info.Data.proc;
	(*proc)(data, len, userData);
	return ::XML_Error_None;
}

static int sInputReadProc(::XML_InputStream *str, ::XML_Char *buf, size_t bufLen)
{
	InputImpl *impl = (InputImpl *)str;
	assert(impl);
	assert(buf);
	return impl->stream->read(buf, bufLen);
}

XMLInput::XMLInput(XMLInputStream &stream) : impl(new InputImpl)
{
	impl->stream = &stream;
	impl->adapter.readProc = sInputReadProc;
	impl->input = ::XML_InputCreate(&impl->adapter);
	impl->input->elementHandler = elementHandler;
	impl->input->dataHandler = dataHandler;
	impl->input->userData = this;
	impl->userData = NULL;
}

XMLInput::~XMLInput()
{
	::XML_InputFree(impl->input);
	delete impl;
}

void XMLInput::Process(const XMLHandler handlers[], void *userData)
{
	::XML_Error error = ::XML_InputProcess(impl->input, (const ::XML_Handler *)handlers, userData);
	if (error != ::XML_Error_None)
		throw XMLParseException(*this);
}

int XMLInput::GetLine() const
{
	return ::XML_InputGetLine(impl->input);
}

int XMLInput::GetColumn() const
{
	return ::XML_InputGetColumn(impl->input);
}

int XMLInput::GetOffset() const
{
	return ::XML_InputGetOffset(impl->input);
}

void XMLInput::SetUserData(void *userData)
{
	impl->userData = userData;
}

void *XMLInput::GetUserData() const
{
	return impl->userData;
}

::XML_Error XMLInput::GetError() const
{
	return ::XML_InputGetError(impl->input);
}

//
// Handler
//

const XMLHandler XMLHandler::END;

XMLHandler::XMLHandler()
{
	name = NULL;
	type = ::XML_Handler_None;
}

XMLHandler::XMLHandler(const ::XML_Char *elemName, HandlerProc proc, void *userData)
{
	assert(elemName);
	assert(proc);

	name = elemName;
	type = ::XML_Handler_Element;
	offset = size = 0;
	info.Element.proc = (::XML_HandlerProc)proc;
	info.Element.userData = userData;
}

XMLHandler::XMLHandler(HandlerProc proc, void *userData)
{
	assert(proc);

	name = NULL;
	type = ::XML_Handler_Element;
	offset = size = 0;
	info.Element.proc = (::XML_HandlerProc)proc;
	info.Element.userData = userData;
}

XMLHandler::XMLHandler(DataProc proc, void *userData)
{
	assert(proc);

	name = NULL;
	type = ::XML_Handler_Data;
	offset = size = 0;
	info.Data.proc = (::XML_DataProc)proc;
	info.Data.userData = userData;
}

XMLHandler::XMLHandler(XML_HandlerType type, DataProc proc, void *userData)
{
	assert(proc);
	assert(type == XML_Handler_Data || type == XML_Handler_CDATA || type == XML_Handler_Comment);

	name = NULL;
	this->type = type;
	offset = size = 0;
	info.Data.proc = (::XML_DataProc)proc;
	info.Data.userData = userData;
}

XMLHandler::XMLHandler(const XMLHandler handlers[], void *userData)
{
	assert(handlers);

	name = NULL;
	type = ::XML_Handler_Chain;
	offset = size = 0;
	info.Chain.handlers = handlers;
	info.Chain.userData = userData;
}

XMLHandler::XMLHandler(const ::XML_Char *elemName, int *value, int minVal, int maxVal)
{
	assert(elemName);
	assert(value);

	name = elemName;
	type = ::XML_Handler_Int;
	offset = 0;
	size = sizeof(int);
	info.Int.result = value;
	info.Int.minVal = minVal;
	info.Int.maxVal = maxVal;
}

XMLHandler::XMLHandler(const ::XML_Char *elemName, unsigned int *value, unsigned int minVal, unsigned int maxVal)
{
	assert(elemName);
	assert(value);

	name = elemName;
	type = ::XML_Handler_UInt;
	offset = 0;
	size = sizeof(unsigned int);
	info.UInt.result = value;
	info.UInt.minVal = minVal;
	info.UInt.maxVal = maxVal;
}

XMLHandler::XMLHandler(const ::XML_Char *elemName, float *value, float minVal, float maxVal)
{
	assert(elemName);
	assert(value);

	name = elemName;
	type = ::XML_Handler_Float;
	offset = 0;
	size = sizeof(float);
	info.Float.result = value;
	info.Float.minVal = minVal;
	info.Float.maxVal = maxVal;
}

XMLHandler::XMLHandler(const ::XML_Char *elemName, double *value, double *minVal, double *maxVal)
{
	assert(elemName);
	assert(value);

	name = elemName;
	type = ::XML_Handler_Double;
	offset = 0;
	size = sizeof(double);
	info.Double.result = value;
	info.Double.minVal = minVal;
	info.Double.maxVal = maxVal;
}

XMLHandler::XMLHandler(const ::XML_Char *elemName, bool *value)
{
	assert(elemName);
	assert(value);

	name = elemName;
	type = ::XML_Handler_Bool;
	offset = 0;
	size = sizeof(bool);
	info.Bool.result = (int *)value;
}

XMLHandler::XMLHandler(const ::XML_Char *elemName, int *value, const ::XML_Char * const*list, size_t size)
{
	assert(elemName);
	assert(value);

	name = elemName;
	type = ::XML_Handler_List;
	offset = 0;
	size = sizeof(int);
	info.List.result = value;
	info.List.list = list;
	info.List.listSize = size;
}

XMLHandler::XMLHandler(const ::XML_Char *elemName, ::XML_Char *value, size_t maxLen)
{
	assert(elemName);
	assert(value);

	name = elemName;
	type = ::XML_Handler_CString;
	offset = 0;
	size = sizeof(::XML_Char);
	info.CString.result = value;
	info.CString.maxLen = maxLen;
}

XMLHandler::XMLHandler(const ::XML_Char *elemName, ::XML_HandlerType type, size_t offset, size_t size)
{
	assert(elemName);

	::memset(this, 0, sizeof(XMLHandler));

	name = elemName;
	this->type = type;
	this->offset = offset;
	this->size = size;
}

XMLHandler::XMLHandler(const ::XML_Char *elemName, const ::XML_Char *const list[], int listSize, size_t offset, size_t size)
{
	assert(elemName);
	assert(list);

	name = elemName;
	this->type = ::XML_Handler_List;
	this->offset = offset;
	this->size = size;
	this->info.List.result = NULL;
	this->info.List.list = list;
	this->info.List.listSize = listSize;
}

const ::XML_Char *XMLHandler::GetName() const
{
	return name;
}

XMLAttribute::XMLAttribute(const ::XML_Attribute *attr) : attr(attr)
{
}

XMLAttribute::XMLAttribute(const XMLAttribute &rhs) : attr(rhs.attr)
{
}

XMLAttribute &XMLAttribute::operator=(const XMLAttribute &rhs)
{
	attr = rhs.attr;
	return *this;
}

XMLAttribute XMLAttribute::GetNext() const
{
	assert(attr);
	return XMLAttribute(::XML_AttrGetNext(attr));
}

const XML_Char *XMLAttribute::GetName() const
{
	assert(attr);
	return ::XML_AttrGetName(attr);
}

const XML_Char *XMLAttribute::GetValue() const
{
	assert(attr);
	return ::XML_AttrGetValue(attr);
}

XMLAttribute::operator bool() const
{
	return attr != NULL;
}


//
// XMLElement
//

XMLElement::XMLElement(::XML_Element *elem)
{
	assert(elem);
	element = elem;
}

const ::XML_Char *XMLElement::GetName() const
{
	return element->name;
}

int XMLElement::NumAttributes() const
{
	return ::XML_ElementGetNumAttrs(element);
}

XMLAttribute XMLElement::GetAttrList() const
{
	return XMLAttribute(::XML_ElementGetAttrList(element));
}

const ::XML_Char *XMLElement::GetAttribute(const ::XML_Char *name, const ::XML_Char *def) const
{
	const ::XML_Attribute *attr = ::XML_ElementFindAttr(element, name);
	return attr ? attr->value : def;
}

void XMLElement::GetAttribute(const ::XML_Char *name, int &value, int defValue) const
{
	value = ::XML_AttrGetInt(::XML_ElementFindAttr(element, name), defValue);
}

void XMLElement::GetAttribute(const ::XML_Char *name, unsigned int &value, unsigned int defValue) const
{
	value = ::XML_AttrGetUInt(::XML_ElementFindAttr(element, name), defValue);
}

void XMLElement::GetAttribute(const ::XML_Char *name, double &value, double defValue) const
{
	value = ::XML_AttrGetDouble(::XML_ElementFindAttr(element, name), defValue);
}

void XMLElement::GetAttribute(const ::XML_Char *name, float &value, float defValue) const
{
	value = ::XML_AttrGetFloat(::XML_ElementFindAttr(element, name), defValue);
}

void XMLElement::GetAttribute(const ::XML_Char *name, bool &value, bool defValue) const
{
	value = ::XML_AttrGetBoolean(::XML_ElementFindAttr(element, name), defValue) != 0;
}

size_t XMLElement::ReadData(::XML_Char *buf, size_t len)
{
	::XML_Error error = ::XML_ElementReadData(element, buf, &len);
	if (error != ::XML_Error_None)
		throw XMLParseException(GetInput());
	return len;
}

void XMLElement::Process(const XMLHandler handlers[], void *userData)
{
	::XML_Error error = ::XML_ElementProcess(element, handlers, userData);
	if (error != ::XML_Error_None)
		throw XMLParseException(GetInput());
}

const XMLInput &XMLElement::GetInput() const
{
	XMLInput *input = (XMLInput *)element->input->userData;
	assert(input);
	return *input;
}

bool XMLElement::IsEmpty() const
{
	return element->empty != 0;
}

//
// Exceptions
//

XMLParseException::XMLParseException(const XMLInput &input)
{
	error = input.GetError();
	line = input.GetLine();
	column = input.GetColumn();
	offset = input.GetOffset();
}

const ::XML_Char *XMLParseException::What() const
{
	return ::XML_InputGetErrorString(error);
}

InvalidValue::InvalidValue(const XMLInput &input) : XMLParseException(input)
{
	// possibly thrown by user code so assign error code here
	this->error = ::XML_Error_InvalidValue;
}

InvalidValue::InvalidValue(const XMLInput &input, int line, int column) : XMLParseException(input)
{
	// possibly thrown by user code so assign error code here
	this->error = ::XML_Error_InvalidValue;
	this->line = line;
	this->column = column;
}


