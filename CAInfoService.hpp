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
#ifndef __CAINFOSERVICE__
#define __CAINFOSERVICE__
#include "CASignature.hpp"
#include "CAFirstMix.hpp"
#include "CAThread.hpp"
#include "CAMutex.hpp"
#include "CACmdLnOptions.hpp"
#include "CAXMLBI.hpp"

#define REQUEST_TYPE_POST 0
#define REQUEST_TYPE_GET	1

#define REQUEST_COMMAND_CONFIGURE 0
#define REQUEST_COMMAND_HELO			1
#define REQUEST_COMMAND_MIXINFO		2

class CAInfoService
	{
		public:
			CAInfoService();
			CAInfoService(CAMix* pMix);
			~CAInfoService();
			SINT32 sendMixHelo(SINT32 requestCommand=-1,const UINT8* param=NULL);
			SINT32 sendMixInfo(const UINT8* pMixID);
			SINT32 sendCascadeHelo();
			SINT32 sendStatus(bool bIncludeCerts);
			SINT32 start();
			SINT32 stop();
			SINT32 getLevel(SINT32* puser,SINT32* prisk,SINT32* ptraffic);
			SINT32 getMixedPackets(UINT64& ppackets);
#ifdef PAYMENT			
			SINT32 getPaymentInstance(const UINT8* a_pstrPIID,CAXMLBI** pXMLBI);
#endif
			bool isRunning()
				{
					return m_bRun;
				}
			SINT32 setSignature(CASignature* pSignature,CACertificate* ownCert);

			// added by ronin <ronin2@web.de>
			bool isConfiguring()
			{
					return m_bConfiguring;
			}
			
			void setConfiguring(bool a_configuring)
			{
					m_bConfiguring = a_configuring;
			}

		private:
			// added by ronin <ronin2@web.de>
			SINT32 handleConfigEvent(DOM_Document& doc);

			volatile bool m_bRun;
			CASignature*	m_pSignature;
			CACertStore*	m_pcertstoreOwnCerts;
			CAMix*				m_pMix;
			CAThread*			m_pthreadRunLoop;
			UINT64				m_lastMixedPackets;
			UINT32				m_minuts;
			SINT32				m_expectedMixRelPos;
			bool					m_bConfiguring;
};
#endif
