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
//#include "httptunnel/tunnel.h"

typedef UINT32 HCHANNEL;
#define MUX_HTTP  0 
#define MUX_SOCKS 1

#ifndef PROT2
	#define DATA_SIZE 1000 // Size of Data in a single Mux Packet

	typedef struct t_MuxPacket
		{
			HCHANNEL channel;
			UINT16	len;
			UINT8		type;
			UINT8		reserved;
			UINT8		data[DATA_SIZE];
		} MUXPACKET;

#else

	#define DATA_SIZE 			992
	#define PAYLOAD_SIZE 		989
	#define MUXPACKET_SIZE 	998

	#if defined(WIN32) ||defined(__sgi)
		#pragma pack( push, t_MuxPacket )
		#pragma pack(1)
		typedef struct t_MuxPacket
			{
				HCHANNEL channel;
				UINT16  flags;
				union
					{
						UINT8		data[DATA_SIZE];
						struct t_MuxPacketPayload
							{
								UINT16 len;
								UINT8 type;
								UINT8 data[PAYLOAD_SIZE];
						} payload;
					};
			} MUXPACKET;
		#pragma pack( pop, t_MuxPacket )
	#else
		typedef struct t_MuxPacket
			{
				HCHANNEL channel;
				UINT16  flags;
				union
					{
						UINT8		data[DATA_SIZE];
						struct t_MuxPacketPayload
							{
								UINT16 len;
								UINT8 type;
								UINT8 data[PAYLOAD_SIZE];
						} payload;
					};
			} __attribute__ ((__packed__)) MUXPACKET __attribute__ ((__packed__));
	#endif //WIN32 
#endif //PROT2

class CAMuxSocket
	{
		public:
			CAMuxSocket();
			~CAMuxSocket(){}
//			int useTunnel(char* proxyhost,UINT16 proxyport);
			int accept(UINT16 port);
			SINT32 connect(LPCASOCKETADDR psa);
			SINT32 connect(LPCASOCKETADDR psa,UINT retry,UINT32 time);
			int close();
			int send(MUXPACKET *pPacket);
			int receive(MUXPACKET *pPacket);
			int close(HCHANNEL channel_id);
			operator CASocket*(){return &m_Socket;}
			operator SOCKET(){//if(!bIsTunneld)
														return (SOCKET)m_Socket;
												//else
													//return tunnel_pollin_fd(m_pTunnel);
													}
			private:
				CASocket m_Socket;
	//		bool bIsTunneld;
	//		Tunnel* m_pTunnel;
	//		char *m_szTunnelHost;
	//		UINT16 m_uTunnelPort;
	};
#endif
