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
#ifndef __CAMIX__
#define __CAMIX__

extern class CACmdLnOptions options;

class CASignature;
class CAInfoService;
//class DOM_Element;

class CAMix
	{
		public:
			CAMix();
			virtual ~CAMix(){}
			SINT32 start();
			virtual SINT32 reconfigure(){return E_SUCCESS;}
			/** Returns the Mix-Cascade info which should be send to the InfoService.
    		* This is NOT a copy!
    		*
    		* @param docMixCascadeInfo where the XML struct would be stored
    		* @retval E_SUCCESS
    		*/
			SINT32 getMixCascadeInfo(DOM_Document& docMixCascadeInfo)
			{
					if(m_docMixCascadeInfo != NULL)
					{
							docMixCascadeInfo=m_docMixCascadeInfo;
							return E_SUCCESS;
					}
					else
					{
							return E_UNKNOWN;
					}
			}

			// added by ronin <ronin2@web.de>
			bool acceptsReconfiguration()
			{
					return m_acceptReconfiguration;
			}

		protected:
			virtual SINT32 clean()=0;
			virtual SINT32 initOnce(){return E_SUCCESS;}
			virtual SINT32 init()=0;
			virtual SINT32 loop()=0;

			// added by ronin <ronin2@web.de>
			virtual SINT32 processKeyExchange()
			{
					return E_SUCCESS;
			}
			// added by ronin <ronin2@web.de>
			virtual SINT32 initMixCascadeInfo(DOM_Element& elemMixes);

			CASignature* m_pSignature;
			CAInfoService* m_pInfoService;

	    bool m_acceptReconfiguration;

			// added by ronin <ronin2@web.de>
			DOM_Document m_docMixCascadeInfo;
		
		private:
			// added by ronin <ronin2@web.de>
			bool needAutoConfig();
	};
#endif
