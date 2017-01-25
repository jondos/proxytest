/*
Copyright (c) 2000, The JAP-Team
All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.

- Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

- Neither the name of the University of Technology Dresden, Germany nor the names of its contributors
may be used to endorse or promote products derived from this software without specific
prior written permission.


THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS
OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE
*/
#include "StdAfx.h"
#include "CAFileSystemDirectory.hpp"

CAFileSystemDirectory::CAFileSystemDirectory(UINT8* strPath)
	{
		m_strPath = new UINT8[strlen((char*)strPath) + 1];
		strcpy((char*)m_strPath,(char*) strPath);
#ifdef _WIN32
		m_hSearch = -1;
#else
		m_hSearch = NULL;
		m_strPattern = NULL;
#endif
	}

CAFileSystemDirectory::~CAFileSystemDirectory()
	{
#ifdef _WIN32
		if (m_hSearch >= 0)
			{
				_findclose(m_hSearch);
			}
#else
	if (m_hSearch != NULL)
		{
		closedir(m_hSearch);
			}
	delete m_strPattern;
#endif
		delete m_strPath;
	}

SINT32 CAFileSystemDirectory::find(UINT8* strPattern)
	{
#ifdef _WIN32
		UINT8* strSearchFor = new UINT8[strlen((char*)m_strPath) + strlen((char*)strPattern )+ 10];
		strcpy((char*)strSearchFor, (char*)m_strPath);
		strcat((char*)strSearchFor, (char*)strPattern);
		m_hSearch = _findfirst( (char*)strSearchFor, &m_finddataFoundFile);
		delete strSearchFor;
		if (m_hSearch < 0)
			{
				m_hSearch = -1;
				return E_UNKNOWN;
			}
		return E_SUCCESS;
#else
	m_hSearch=opendir((char*)m_strPath);
	if(m_hSearch==NULL)
		return E_UNKNOWN;
	m_strPattern = new UINT8[strlen((char*)strPattern) + 1];
	strcpy((char*)m_strPattern,(char*) strPattern);
	return E_SUCCESS;
#endif
	}

SINT32 CAFileSystemDirectory::getNextSearchResult(UINT8* strResult, UINT32 sizeResult)
	{
#ifdef _WIN32
	if (m_hSearch > 0)
		{
			strcpy((char*)strResult, (char*)m_strPath);
			strcat((char*)strResult, (char*)m_finddataFoundFile.name);
			SINT32 ret = _findnext(m_hSearch, &m_finddataFoundFile);
			if (ret < 0)
				{
					_findclose(m_hSearch);
					m_hSearch = -1;
				}
			return E_SUCCESS;
		}
	else
		return E_UNKNOWN;
#else
	if (m_hSearch !=NULL)
		{
		struct dirent * pEntry = NULL;
		while ((pEntry = readdir(m_hSearch)) != NULL)
			{
			if (fnmatch((char*)m_strPattern, pEntry->d_name, FNM_PATHNAME) == 0)
				{

				strcpy((char*)strResult, (char*)m_strPath);
				strcat((char*)strResult, (char*)pEntry->d_name);
				return E_SUCCESS;
				}
			}
			closedir(m_hSearch);
			m_hSearch = NULL;
			return E_UNKNOWN;
		}
	else
		return E_UNKNOWN;
#endif
	}