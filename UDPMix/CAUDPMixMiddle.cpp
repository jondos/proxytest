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
#include "../StdAfx.h"
#include "CAUDPMixMiddle.hpp"
#include "CAUDPMixLinkEncryption.hpp"
#include "CAUDPMixECCrypto.hpp"
#include "../CADatagramSocket.hpp"
#include "../CASocketAddrINet.hpp"
#include "../CAMsg.hpp"

SINT32 CAUDPMiddleMix::loop()
{
	CAUDPMixECCrypto *eccCrypto = new CAUDPMixECCrypto(); 


	CADatagramSocket *psocketIn = new CADatagramSocket();
	CADatagramSocket *psocketOut = new CADatagramSocket();
	CASocketAddrINet *psockaddrFrom = new CASocketAddrINet();
	CASocketAddrINet addrNextMix;
	addrNextMix.setAddr((UINT8*)"172.20.46.105", 20003);
	UDPMIXPACKET *pUDPMixPacketIn = new UDPMIXPACKET;
	UDPMIXPACKET *pUDPMixPacketOut = new UDPMIXPACKET;
	CAUDPMixLinkEncryption *pLinkEncryption = new CAUDPMixLinkEncryption();
	UINT8 *sharedSecret = new UINT8[100];
	UINT8 k[16];
	memset(k, 0, 16);
	pLinkEncryption->setKeys(k, k);
	psocketIn->bind(20002);
	psocketOut->create();
	CAMsg::printMsg(LOG_DEBUG, "Starting UDP Middle Mix\n");
	while (true)
		{
			SINT32 len = psocketIn->receive(pUDPMixPacketIn->rawBytes, UDPMIXPACKET_SIZE, psockaddrFrom);
			if (len<0)
				{
					CAMsg::printMsg(LOG_ERR, "Error receving UDP MixPacket - errno: %i\n", GET_NET_ERROR);
					return E_UNKNOWN;
				}
			CAMsg::printMsg(LOG_DEBUG, "Received UDP MixPacket - size: %i\n", len);
			pLinkEncryption->decrypt(pUDPMixPacketIn, pUDPMixPacketOut);
			if (pUDPMixPacketOut->linkHeader.u8_type == UDPMIXPACKET_TYPE_INIT)
				{
					eccCrypto->getSharedSecretAndBlindedPublicKey(pUDPMixPacketIn, pUDPMixPacketOut, sharedSecret);
				}

			pLinkEncryption->encrypt(pUDPMixPacketOut, pUDPMixPacketIn);
			SINT32 ret=psocketOut->send(pUDPMixPacketIn->rawBytes, UDPMIXPACKET_SIZE, addrNextMix);
			if (ret < 0)
				{
					CAMsg::printMsg(LOG_ERR, "Error send UDP MixPacket - errno: %i\n", GET_NET_ERROR);
					return E_UNKNOWN;
				}
		}
	return E_SUCCESS;
}