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

#ifndef __CACMDLNOPTIONS__
#define __CACMDLNOPTIONS__
#include "CASocketAddrINet.hpp"
#include "CASocketAddrUnix.hpp"
#include "CASignature.hpp"
#include "CACertificate.hpp"
#include "CAThread.hpp"
#include "CAMix.hpp"
#ifdef LOG_CRIME
	#include "tre/regex.h"
#endif

#define RAW_TCP		1
#define RAW_UNIX	2
#define SSL_TCP		3
#define SSL_UNIX	4

struct t_ListenerInterface
	{
		UINT32 type;
		CASocketAddr* addr;
		UINT8* hostname; 
	};

typedef struct t_ListenerInterface ListenerInterface;

#define TARGET_MIX					1
#define TARGET_HTTP_PROXY		2	
#define TARGET_SOCKS_PROXY	3

struct t_TargetInterface
	{
		UINT32 target_type;
		UINT32 net_type;
		CASocketAddr* addr;
	};

typedef struct t_TargetInterface TargetInterface;

class CACmdLnOptions
    {
	public:
	    CACmdLnOptions();
	    ~CACmdLnOptions();
			void clean();
			SINT32 parse(int argc,const char** arg);
			SINT32 reread(CAMix* pMix);
	    bool getDaemon();
      //bool getProxySupport();

			SINT32 getMixId(UINT8* id,UINT32 len);
	   

//			UINT16 getServerPort();
			/*For IP (Host) AND Unix Domain Sockets*/
//	    SINT32 getServerHost(UINT8* path,UINT32 len);
			
//			SINT32 getServerRTTPort();
			UINT16 getSOCKSServerPort();
	    
			UINT32 getListenerInterfaceCount(){return m_cnListenerInterfaces;}
			SINT32 getListenerInterface(ListenerInterface& oListenerInterface, UINT32 nr)		
				{
					if(nr>0&&nr<=m_cnListenerInterfaces)
						{
							oListenerInterface.type=m_arListenerInterfaces[nr-1].type;
							oListenerInterface.hostname=m_arListenerInterfaces[nr-1].hostname;
							oListenerInterface.addr=m_arListenerInterfaces[nr-1].addr->clone();
							return E_SUCCESS;
						}
					else
						return E_UNKNOWN;
				};
			
			//this is only for the local proxy
			UINT16 getMixPort();
			SINT32 getMixHost(UINT8* host,UINT32 len);
	   
			//if we have more than one Target (currently only Caches are possible...)
			UINT32 getTargetInterfaceCount(){return m_cnTargets;}

			/** Fills a \c TargetInterface struct with the values which belongs to
				* the target interface \c nr.
				* This is actual a copy of all values, so the caller is responsible 
				* for destroying them after use!
				*
				* @param oTargetInterface \c TargetInterface struct, which gets filles with
				*															the values of target interface \c nr
				*	@param nr the index of the target interface, for whcih information
				*					is request (starting with 1 for the first interface)
				* @retval E_SUCCESS if successful
				* @retval E_UNKNOWN if \c nr is out of range
				*/
			SINT32 getTargetInterface(TargetInterface& oTargetInterface, UINT32 nr)
				{
					if(nr>0&&nr<=m_cnTargets)
						{
							oTargetInterface.net_type=m_arTargetInterfaces[nr-1].net_type;
							oTargetInterface.target_type=m_arTargetInterfaces[nr-1].target_type;
							oTargetInterface.addr=m_arTargetInterfaces[nr-1].addr->clone();
							return E_SUCCESS;
						}
					else
						return E_UNKNOWN;
				};

			
			UINT16 getSOCKSPort();
	    SINT32 getSOCKSHost(UINT8* host,UINT32 len);
	    UINT16 getInfoServerPort();
	    SINT32 getInfoServerHost(UINT8* host,UINT32 len);
			
			SINT32 getMaxOpenFiles()
				{
					return m_nrOfOpenFiles;
				}

			CASignature* getSignKey()
				{
					if(m_pSignKey!=NULL)
						return m_pSignKey->clone(); 
					return NULL;
				}
			
			CACertificate* getOwnCertificate()
				{
					if(m_pOwnCertificate!=NULL)
						return m_pOwnCertificate->clone(); 
					return NULL;
				}

			CACertificate* getPrevMixTestCertificate()
				{
					if(m_pPrevMixCertificate!=NULL)
						return m_pPrevMixCertificate->clone(); 
					return NULL;
				}

			CACertificate* getNextMixTestCertificate()
				{
					if(m_pNextMixCertificate!=NULL)
						return m_pNextMixCertificate->clone(); 
					return NULL;
				}
			
			SINT32 getCascadeName(UINT8* name,UINT32 len);
			SINT32 getLogDir(UINT8* name,UINT32 len);
			SINT32 getSpecialLogDir(UINT8* name,UINT32 len);
			bool getCompressLogs()
				{
					return m_bCompressedLogs;
				}
			SINT32 getUser(UINT8* user,UINT32 len);

			/** Get the XML describing the Mix. this is not a string!*/
			//SINT32 getMixXml(UINT8* strxml,UINT32* len);
			SINT32 getMixXml(DOM_Document &docMixInfo);
			bool isLocalProxy();
			bool isFirstMix();
			bool isMiddleMix();
			bool isLastMix();
			bool isInfoServiceEnabled()
				{
					return m_strInfoServerHost!=NULL;
				}

			bool getAutoReconnect()
				{
					return m_bAutoReconnect;
				}
#ifdef LOG_CRIME
			regex_t* getCrimeRegExps(UINT32* len)
				{
					*len=m_nCrimeRegExps;
					return m_arCrimeRegExps;
				}
#endif
		friend THREAD_RETURN threadReConfigure(void *param);
		private:
			UINT8*	m_strConfigFile; //the filename of the config file
			bool		m_bIsRunReConfigure; //true, if an async reconfigure is under way 
	    CAMutex m_csReConfigure; //Ensures that reconfigure is running only once at the same time;
			CAThread m_threadReConfigure; //Thread, that does the actual reconfigure work
			bool		m_bDaemon;
	    UINT16	m_iSOCKSServerPort;
	    UINT16	m_iTargetPort; //only for the local proxy...
			char*		m_strTargetHost; //only for the local proxy...
			char*		m_strSOCKSHost;
	    UINT16	m_iSOCKSPort;
	    char*		m_strInfoServerHost;
	    UINT16	m_iInfoServerPort;
			bool		m_bLocalProxy,m_bFirstMix,m_bMiddleMix,m_bLastMix;
			bool		m_bAutoReconnect; //auto reconnect if connection to first mix lost ??
			char*		m_strCascadeName;
			char*		m_strLogDir;
			char*		m_strSpecialLogDir;
			bool		m_bCompressedLogs;
			char*		m_strUser;
			SINT32	m_nrOfOpenFiles; //How many open files (sockets) should we use
			DOM_Document m_docMixInfo;
			//char*		m_strMixXml;
			char*		m_strMixID;

			TargetInterface*		m_arTargetInterfaces;
			UINT32							m_cnTargets;
			ListenerInterface*	m_arListenerInterfaces;
			UINT32							m_cnListenerInterfaces;
			
			CASignature*		m_pSignKey;
			CACertificate*	m_pOwnCertificate;
			CACertificate*	m_pPrevMixCertificate;
			CACertificate*	m_pNextMixCertificate;
#ifdef LOG_CRIME
			regex_t* m_arCrimeRegExps;
			UINT32 m_nCrimeRegExps;
#endif
		private:
			SINT32 setNewValues(CACmdLnOptions& newOptions);
			SINT32 readXmlConfiguration(DOM_Document& docConfig,const UINT8* const configFileName);
			SINT32 processXmlConfiguration(DOM_Document& docConfig);
			SINT32 clearTargetInterfaces();
			SINT32 clearListenerInterfaces();
	};
#endif
