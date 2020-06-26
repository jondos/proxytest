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
#ifndef __CAUDPMIX__
#define __CAUDPMIX__

#define UDPMIXPACKET_SIZE 304

typedef UINT16 HUDPCHANNEL;
typedef UINT32 HUDPPACKET;

#define UDPMIXPACKET_LINKHEADER_TYPE_SIZE 27
#define UDPMIXPACKET_LINKHEADER_TOTAL_SIZE 27
#define UDPMIXPACKET_LINKHEADER_MAC_SIZE 16
#define UDPMIXPACKET_LINKHEADER_COUNTER_SIZE 4
#define UDPMIXPACKET_LINKHEADER_HEADERFIELDS_SIZE 7


#define UDPMIXPACKET_INIT_ECC_PUB_ELEMENT_SIZE 29

#if (defined(WIN32) || defined(__sgi)) && !defined(__GNUC__)
#pragma pack(push, t_UDPMixPacket)
#pragma pack(1)
struct UDPMIXPACKET_LINKHEADER_t
{
	union
	{
		UINT32 u32_LinkCounterPrefix;
		UINT8 aru8__LinkCounterPrefix[UDPMIXPACKET_LINKHEADER_COUNTER_SIZE];
	};
	HUDPCHANNEL channelID;
	HUDPPACKET packetID;
	UINT8 u8_type;
	UINT8 MAC[UDPMIXPACKET_LINKHEADER_MAC_SIZE];
};

struct UDPMIXPACKET_INIT_HEADER_t
{
	UINT8 eccPubElement[UDPMIXPACKET_INIT_ECC_PUB_ELEMENT_SIZE];
};

#pragma pack(pop, t_UDPMixPacket)
#else
struct UDPMIXPACKET_LINKHEADER_t
{
	union
	{
		UINT32 u32_LinkCounterPrefix;
		UINT8 aru8__LinkCounterPrefix[UDPMIXPACKET_LINKHEADER_COUNTER_SIZE];
	};
	HUDPCHANNEL channelID;
	HUDPPACKET packetID;
	UINT8 u8_type;
	UINT8 MAC[UDPMIXPACKET_LINKHEADER_MAC_SIZE];
} __attribute__((__packed__));

#endif //WIN32




typedef struct UDPMIXPACKET_LINKHEADER_t UDPMIXPACKET_LINKHEADER;
typedef struct UDPMIXPACKET_INIT_HEADER_t UDPMIXPACKET_INIT_HEADER;
	
struct UDPMIXPACKET_t
	{
		union
		{
			UINT8 rawBytes[UDPMIXPACKET_SIZE];
			struct
			{
				UDPMIXPACKET_LINKHEADER linkHeader;
				UDPMIXPACKET_INIT_HEADER initHeader;
			};
		};
	};

typedef struct UDPMIXPACKET_t UDPMIXPACKET;

#define UDPMIXPACKET_TYPE_DATA 1 
#define UDPMIXPACKET_TYPE_INIT 2
#define UDPMIXPACKET_TYPE_INIT_CONFIRM 3

class CAUDPMix
	{
		public:
	CAUDPMix()
	{
		m_bLoop = false;
		m_bShutDown = false;
	}
	SINT32 start();

	virtual void shutDown()
	{
		m_bShutDown = true;
		m_bLoop = false;
	}

	virtual bool isShutDown()
	{
		return m_bShutDown;
	}

		private:
	volatile bool m_bShutDown;
	volatile bool m_bLoop;
	virtual SINT32 clean() = 0;
	virtual SINT32 initOnce()=0;
	virtual SINT32 init() = 0;
	virtual SINT32 loop() = 0;
	SINT32 setReceivePort(UINT16 port);
	SINT32 setSendPort(UINT16 port);
};
#endif //__CAUDPMIX__
