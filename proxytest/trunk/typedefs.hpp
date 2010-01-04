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
enum NetworkType {UNKNOWN_NETWORKTYPE,RAW_TCP,RAW_UNIX,SSL_TCP,SSL_UNIX, HTTP_TCP};

typedef UINT32 HCHANNEL;
#define MIX_PAYLOAD_HTTP  0
#define MIX_PAYLOAD_SOCKS 1


#define MIXPACKET_SIZE 	998

#define CHANNEL_DATA		0x00
#define CHANNEL_OPEN		0x08

#define CHANNEL_TIMESTAMPS_UP	0x60
#define CHANNEL_TIMESTAMPS_DOWN	0x90
#define CHANNEL_CLOSE		0x01
#define CHANNEL_SUSPEND 0x02
#define	CHANNEL_RESUME	0x04
#define	CHANNEL_DUMMY		0x10

#ifdef LOG_CRIME
	#define	CHANNEL_SIG_CRIME 0x20
	#define	CHANNEL_SIG_CRIME_ID_MASK 0x0000FF00
	#define CHANNEL_ALLOWED_FLAGS		(CHANNEL_OPEN|CHANNEL_CLOSE|CHANNEL_SUSPEND|CHANNEL_RESUME|CHANNEL_TIMESTAMPS_UP|CHANNEL_TIMESTAMPS_DOWN|CHANNEL_SIG_CRIME|CHANNEL_SIG_CRIME_ID_MASK)
#else
	#define	CHANNEL_SIG_CRIME 0x0
	#define	CHANNEL_SIG_CRIME_ID_MASK 0x0
#ifdef NEW_FLOW_CONTROL
	#define CHANNEL_ALLOWED_FLAGS		(CHANNEL_OPEN|CHANNEL_CLOSE|CHANNEL_TIMESTAMPS_UP|CHANNEL_TIMESTAMPS_DOWN)
#else
	#define CHANNEL_ALLOWED_FLAGS		(CHANNEL_OPEN|CHANNEL_CLOSE|CHANNEL_SUSPEND|CHANNEL_RESUME|CHANNEL_TIMESTAMPS_UP|CHANNEL_TIMESTAMPS_DOWN)
#endif
#endif

#define NEW_FLOW_CONTROL_FLAG 0x8000

#define CONNECTION_ERROR_FLAG 0x01

#define DATA_SIZE 			992
#define PAYLOAD_SIZE 		989
#define PAYLOAD_LEN_MASK 0x03FF;

#if (defined(WIN32) ||defined(__sgi))&&!defined(__GNUC__)
	#define DO_PACKED
#else
	#define DO_PACKED __attribute__ ((__packed__))
#endif


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

typedef t_MixPacket MIXPACKET;

#ifdef DATA_RETENTION_LOG
#if (defined(WIN32) ||defined(__sgi))&&!defined(__GNUC__)
		#pragma pack( push, t_DataRetentionLogEntry )
		#pragma pack(1)
#endif
struct __t__data_retention_log_entry
	{
		UINT32 t_in;
		UINT32 t_out;
		union t_union_entity
		{
			struct t_first_mix_data_retention_log_entry
				{
					HCHANNEL channelid;
					UINT8 ip_in[4];
					UINT16 port_in;
				} DO_PACKED first;
			struct t_last_mix_data_retention_log_entry
				{
					HCHANNEL channelid;
					UINT8 ip_out[4];
					UINT16 port_out;
				} DO_PACKED last;
		} DO_PACKED entity;
	}
#if (defined(WIN32) ||defined(__sgi))&&!defined(__GNUC__)
	;
	#pragma pack( pop, t_DataRetentionLogEntry )
#else
	DO_PACKED ;
#endif

typedef struct __t__data_retention_log_entry t_dataretentionLogEntry;
#endif //DATA_RETENION_LOG



//For that we store in our packet queue...
//normally this is just the packet
struct t_queue_entry
	{
		MIXPACKET packet;
		#if defined(DATA_RETENTION_LOG)
			t_dataretentionLogEntry dataRetentionLogEntry;
		#endif
		#if defined  (LOG_PACKET_TIMES) || defined (LOG_CHANNEL)
			UINT64 timestamp_proccessing_start;
			UINT64 timestamp_proccessing_start_OP;
			UINT64 timestamp_proccessing_end;
		#endif
		#if defined  (LOG_PACKET_TIMES)
			//without send/receive or queueing times
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
		// stores the local time in seconds since epoch for interval '0' for this mix
		UINT32 m_u32ReplayOffset;
		UINT16 m_u32ReplayBase;
	};
typedef struct t_mix_parameters tMixParameters;


/**
 * These flags are used to represent the state
 * of the payment
 */

/** new user, not yet authenticated */
#define AUTH_NEW 0x0

/** user has sent an account certificate */
#define AUTH_GOT_ACCOUNTCERT 0x1

/** format and signature of all received certificates was OK */
#define AUTH_ACCOUNT_OK 0x2

/** First CC from client has not been settled yet. */
#define AUTH_WAITING_FOR_FIRST_SETTLED_CC 0x4

/** we have sent one or two CC requests */
#define AUTH_SENT_CC_REQUEST 0x20

/** A database error occured (internal or in the BI) */
#define AUTH_DATABASE 0x40

/** Account has been blocked temporarly */
#define AUTH_BLOCKED 0x80

/** we have sent one request for an accountcertificate */
#define AUTH_SENT_ACCOUNT_REQUEST 0x100

#define AUTH_HARD_LIMIT_REACHED 0x200

/** the user tried to fake something */
#define AUTH_FAKE 0x400

/** we have sent a challenge and not yet received the response */
#define AUTH_CHALLENGE_SENT 0x800

/** the account is empty */
#define AUTH_ACCOUNT_EMPTY 0x1000

/** a fatal error occured earlier */
#define AUTH_FATAL_ERROR 0x2000

#define AUTH_OUTDATED_CC 0x4000

/** Account does not exist */
#define AUTH_INVALID_ACCOUNT 0x8000

// AI is waiting for a necessary message from JAP (e.g. response to challenge)
#define AUTH_TIMEOUT_STARTED 0x10000

#define AUTH_MULTIPLE_LOGIN 0x20000

#define AUTH_UNKNOWN 0x40000

// we settled at least one CC for this account in this session
#define AUTH_SETTLED_ONCE 0x80000

/*
 * The user corresponding to this entry has closed the connection.
 * Delete the entry as soon as possible.
 */
#define AUTH_DELETE_ENTRY 0x80000

#define AUTH_LOGIN_NOT_FINISHED 0x100000
#define AUTH_LOGIN_FAILED 0x200000
#define AUTH_LOGIN_SKIP_SETTLEMENT 0x400000

class CASignature;
class CAAccountingControlChannel;
class CAMutex;
struct t_fmhashtableentry;
/**
 * Structure that holds all per-user payment information
 * Included in CAFirstMixChannelList (struct fmHashTableEntry)
 */
struct t_accountinginfo
{
	CAMutex* mutex;

	/** we store the challenge here to verify the response later */
	UINT8 * pChallenge;

	/** the signature verifying instance for this user */
	CASignature * pPublicKey;

	/** The number of packets transfered. */
	UINT64 sessionPackets;

	/** the number of bytes that was transferred (as counted by the AI)  Elmar: since last CC, or in total? */
	UINT64 transferredBytes;

	/** the number of bytes that was confirmed by the account user */
	UINT64 confirmedBytes;

	/** The bytes the user could confirm in the last CC sent to him.  */
	UINT64 bytesToConfirm;

	/** the user's account number */
	UINT64 accountNumber;

	/** The same value as in fmHashTableEntry. */
	UINT64 userID;

	struct t_fmhashtableentry *ownerRef;
	/** a pointer to the user-specific control channel object */
	CAAccountingControlChannel* pControlChannel;

	/** Flags, see above AUTH_* */
	UINT32 authFlags;

	/** timestamp when last HardLimit was reached */
	SINT32 lastHardLimitSeconds;

	/** timestamp when last PayRequest was sent */
	SINT32 challengeSentSeconds;

	/** ID of payment instance belonging to this account */
	UINT8* pstrBIID;

	//time at which the timeout for waiting for the account certificate has been started
	SINT32 authTimeoutStartSeconds;

	// the number of references to this entry in the ai queue
	UINT32 nrInQueue;

	// new JonDo clients will send their version number as during challenge-response.
	UINT8* clientVersion;
};
typedef struct t_accountinginfo tAiAccountingInfo;

#endif
