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

#ifndef __CAMUXSOCKET__
#define __CAMUXSOCKET__
#include "CASocket.hpp"
#include "CASymCipher.hpp"

typedef UINT32 HCHANNEL;
#define MIX_PAYLOAD_HTTP  0 
#define MIX_PAYLOAD_SOCKS 1


#define DATA_SIZE 			992
#define PAYLOAD_SIZE 		989
#define MIXPACKET_SIZE 	998

#define CHANNEL_DATA		0x00
#define CHANNEL_OPEN		0x00
#define CHANNEL_CLOSE		0x01
#define CHANNEL_SUSPEND 0x02
#define	CHANNEL_RESUME	0x04

#if defined(WIN32) ||defined(__sgi)
	#pragma pack( push, t_MixPacket )
	#pragma pack(1)
	struct t_MixPacket
		{
			HCHANNEL channel;
			UINT16  flags;
			union
				{
					UINT8		data[DATA_SIZE];
					struct t_MixPacketPayload
						{
							UINT16 len;
							UINT8 type;
							UINT8 data[PAYLOAD_SIZE];
					} payload;
				};
		};
	#pragma pack( pop, t_MixPacket )
#else
	struct t_MixPacket
		{
			HCHANNEL channel;
			UINT16  flags;
			union
				{
					UINT8		data[DATA_SIZE];
					struct t_MixPacketPayload
						{
							UINT16 len;
							UINT8 type;
							UINT8 data[PAYLOAD_SIZE];
					} payload;
				};
		} __attribute__ ((__packed__)); // MUXPACKET __attribute__ ((__packed__));
#endif //WIN32 

typedef t_MixPacket MIXPACKET;

class CAMuxSocket
	{
		public:
			CAMuxSocket();
			~CAMuxSocket()
				{
					delete m_Buff;
					DeleteCriticalSection(&csSend);
					DeleteCriticalSection(&csReceive);
				}
			SINT32 accept(UINT16 port);
			SINT32 accept(CASocketAddr & oAddr);
			SINT32 connect(CASocketAddr& psa);
			SINT32 connect(CASocketAddr& psa,UINT retry,UINT32 time);
			int close();
			int send(MIXPACKET *pPacket);
			SINT32 receive(MIXPACKET *pPacket);
			SINT32 receive(MIXPACKET *pPacket,UINT32 timeout);
			int close(HCHANNEL channel_id);
			operator CASocket*(){return &m_Socket;}
			operator SOCKET(){return (SOCKET)m_Socket;}
			
			SINT32 getSendSpace()
				{
					SINT32 s=m_Socket.getSendSpace();
					if(s<0)
						return E_UNKNOWN;
					return s/MIXPACKET_SIZE;
				}
													
			SINT32 setCrypt(bool b);
		private:
				CASocket m_Socket;
				UINT32 m_aktBuffPos;
				UINT8* m_Buff;
				CASymCipher m_oCipherIn;
				CASymCipher m_oCipherOut;
				bool m_bIsCrypted;
				CRITICAL_SECTION csSend;
				CRITICAL_SECTION csReceive;
	};
#endif
