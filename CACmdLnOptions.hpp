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
#include "CASignature.hpp"
#include "CACertificate.hpp"
class CACmdLnOptions
    {
	public:
	    CACmdLnOptions();
	    ~CACmdLnOptions();
			void clean();
			int parse(int argc,const char** arg);
	    bool getDaemon();
      bool getProxySupport();
	   
			UINT16 getServerPort();
			/*For IP (Host) AND Unix Domain Sockets*/
	    SINT32 getServerHost(UINT8* path,UINT32 len);
			
			SINT32 getMixId(UINT8* id,UINT32 len);
			SINT32 getServerRTTPort();
			UINT16 getSOCKSServerPort();
	    
			//for historic reason gives always the first entry in the possible target list
			UINT16 getTargetPort();
	    SINT32 getTargetRTTPort();
			SINT32 getTargetHost(UINT8* host,UINT32 len);
	   
			//if we have more than one Target (currently only Caches are possible...)
			UINT32 getTargetCount(){return cntTargets;}
			SINT32 getTargetAddr(CASocketAddrINet& oAddr, UINT32 nr)
				{
					if(nr>0&&nr<=cntTargets)
						{
							oAddr=pTargets[nr-1];
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
			SINT32 getUser(UINT8* user,UINT32 len);
			/** Get the XML describing the Mix. this is not a string!*/
			SINT32 getMixXml(UINT8* strxml,UINT32* len);
			bool isLocalProxy();
			bool isFirstMix();
			bool isMiddleMix();
			bool isLastMix();
			bool isInfoServiceEnabled()
				{
					return strInfoServerHost!=NULL;
				}
	protected:
	    bool bDaemon;
      bool m_bHttps;
	    UINT16 iServerPort;
			SINT32 iServerRTTPort;
	    UINT16 iSOCKSServerPort;
	    UINT16 iTargetPort; //only for the first target...
	    SINT32 iTargetRTTPort; //only for the first target
			char* strTargetHost; //only for the first target...
	    char* strServerHost; //Host or Unix Domain Socket
			char* strSOCKSHost;
	    UINT16 iSOCKSPort;
	    char* strInfoServerHost;
	    UINT16 iInfoServerPort;
			bool bLocalProxy,bFirstMix,bMiddleMix,bLastMix;
			char* strCascadeName;
			char* strLogDir;
			char* m_strUser;
			SINT32 m_nrOfOpenFiles; //How many open files (sockets) should we use
			char* m_strMixXml;
			char* m_strMixID;

			CASocketAddrINet* pTargets;
			UINT32 cntTargets;
			
			CASignature* m_pSignKey;
			CACertificate* m_pOwnCertificate;
			CACertificate* m_pPrevMixCertificate;
			CACertificate* m_pNextMixCertificate;
		private:
			SINT32 generateTemplate();
	};
#endif
