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
#ifndef __TYEDEFS__
#define __TYEDEFS__
enum NetworkType {UNKNOWN_NETWORKTYPE,RAW_TCP,RAW_UNIX,SSL_TCP,SSL_UNIX};

typedef UINT32 HCHANNEL;
#define MIX_PAYLOAD_HTTP  0 
#define MIX_PAYLOAD_SOCKS 1


#define MIXPACKET_SIZE 	998

#define CHANNEL_DATA		0x00
#define CHANNEL_OPEN		0x08

#define CHANNEL_CLOSE		0x01
#define CHANNEL_SUSPEND 0x02
#define	CHANNEL_RESUME	0x04
#define	CHANNEL_DUMMY		0x10

#ifdef LOG_CRIME
	#define	CHANNEL_SIG_CRIME 0x20
	#define	CHANNEL_SIG_CRIME_ID_MASK 0x0000FF00
	#define CHANNEL_ALLOWED_FLAGS		(CHANNEL_OPEN|CHANNEL_CLOSE|CHANNEL_SUSPEND|CHANNEL_RESUME|CHANNEL_SIG_CRIME|CHANNEL_SIG_CRIME_ID_MASK)
#else
	#define CHANNEL_ALLOWED_FLAGS		(CHANNEL_OPEN|CHANNEL_CLOSE|CHANNEL_SUSPEND|CHANNEL_RESUME)
#endif

#ifdef NEW_FLOW_CONTROL
	#define NEW_FLOW_CONTROL_FLAG 0x8000
#endif
	
#ifndef NEW_MIX_TYPE

	#define DATA_SIZE 			992
	#define PAYLOAD_SIZE 		989

	#if (defined(WIN32) ||defined(__sgi))&&!defined(__GNUC__)
		#pragma pack( push, t_MixPacket )
		#pragma pack(1)
		struct t_MixPacketPayload
			{
				UINT16 len;
				UINT8 type;
				UINT8 data[PAYLOAD_SIZE];
			};
		struct t_MixPacket
			{
				HCHANNEL channel;
				UINT16  flags;
				union
					{
						UINT8		data[DATA_SIZE];
						struct t_MixPacketPayload payload;
					};
			};
		#pragma pack( pop, t_MixPacket )
	#else
		struct t_MixPacketPayload
			{
				UINT16 len;
				UINT8 type;
				UINT8 data[PAYLOAD_SIZE];
			} __attribute__ ((__packed__));

		struct t_MixPacket
			{
				HCHANNEL channel;
				UINT16  flags;
				union
					{
						UINT8		data[DATA_SIZE];
						struct t_MixPacketPayload payload;
					};
			} __attribute__ ((__packed__)); // MUXPACKET __attribute__ ((__packed__));
	#endif //WIN32 
#else //NEW_MIX_TYPE

	#define DATA_SIZE 			994
	#define PAYLOAD_SIZE 		979

	#if defined(WIN32) ||defined(__sgi)
		#pragma pack( push, t_MixPacket )
		#pragma pack(1)
		struct t_MixPacket
			{
				HCHANNEL channel;
				union
					{
						UINT8		data[DATA_SIZE];
						struct t_MixPacketPayload
							{
								UINT8 linkid[8];
								UINT8 seq[4];
								UINT8 type;
								UINT16 len;
								UINT8 data[PAYLOAD_SIZE];
						} payload;
					};
			};
		#pragma pack( pop, t_MixPacket )
	#else
		struct t_MixPacketPayload
			{
				UINT8 linkid[8];
				UINT8 seq[4];
				UINT8 type;
				UINT16 len;
				UINT8 data[PAYLOAD_SIZE];
			} __attribute__ ((__packed__));

		struct t_MixPacket
			{
				HCHANNEL channel;
				union
					{
						UINT8		data[DATA_SIZE];
						struct t_MixPacketPayload payload;
					};
			} __attribute__ ((__packed__)); // MUXPACKET __attribute__ ((__packed__));
	#endif //WIN32 
#endif //NEW_MIX_TYPE

typedef t_MixPacket MIXPACKET;

//For that we store in our packet queue...
//normally this is just the packet
struct t_queue_entry
	{
		MIXPACKET packet;
		#ifdef LOG_PACKET_TIMES
			UINT64 timestamp_proccessing_start;
			UINT64 timestamp_proccessing_end;
			//without send/receive or queueing times
			UINT64 timestamp_proccessing_start_OP;
			UINT64 timestamp_proccessing_end_OP;
			#ifdef USE_POOL
				UINT64 pool_timestamp_in;
				UINT64 pool_timestamp_out;
			#endif
		#endif
	};
typedef struct t_queue_entry tQueueEntry;

//for that we store in our pool
//normaly this is just the packet
typedef tQueueEntry tPoolEntry; 	

///the Replaytimestamp type
struct t_replay_timestamp
	{
		UINT interval; //the current interval number
		UINT offset; //seconds since start of this interval
	};

typedef struct t_replay_timestamp tReplayTimestamp;

struct t_mix_parameters
	{
		//stores the mix id of the mix
		UINT8* m_strMixID;
		/// stores the local time in seconds since epoch for interval '0' for this mix
		UINT32 m_u32ReplayRefTime;
	};
typedef struct t_mix_parameters tMixParameters;
#endif
