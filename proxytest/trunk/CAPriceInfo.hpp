#ifndef __CAPRICEINFO__
#define __CAPRICEINFO__
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

/**
CAXMLPriceCertificate uses an array of CAPriceInfo as a "poor man's hashtable"
represents one PriceCertificate element as contained in a CC
(Subjectkeyidentifier of a Mix plus either a PriceCert Hash (for pay Mixes)
or null (for non-pay Mixes) )

@author Elmar Schraml
*/
#ifdef PAYMENT
#include "StdAfx.h"
class CAPriceInfo
{
public:
	CAPriceInfo(UINT8* strMixId, UINT8* strPriceCertHash, SINT32 a_postition);
	
	virtual ~CAPriceInfo();
	
	UINT8* getMixId() 
	{
		UINT8* pTmpStr = NULL;
		if(m_pStrMixId!=NULL)
			{
				pTmpStr = new UINT8[strlen((char*)m_pStrMixId)+1];
				strcpy( (char*)pTmpStr, (char*)m_pStrMixId);
			}
		return pTmpStr;
	}
	
	UINT8* getPriceCertHash() 
	{
		UINT8* pTmpStr = NULL;
		if(m_pStrPriceCertHash!=NULL)
			{
				pTmpStr = new UINT8[strlen((char*)m_pStrPriceCertHash)+1];
				strcpy((char*)pTmpStr, (char*)m_pStrPriceCertHash);
			}
		return pTmpStr;
	}
	
	SINT32 getPosition()
	{
		return m_postition;
	}

private:
	
	UINT8* m_pStrMixId; //SubjectKeyIdentifier of the Mix
	UINT8* m_pStrPriceCertHash; 
	SINT32 m_postition;
};
#endif //PAYMENT
#endif /*CAPRICEINFO_H_*/
