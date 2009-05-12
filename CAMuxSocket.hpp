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
#include "CAMutex.hpp"
#include "CATLSClientSocket.hpp"

struct __t_hash_key_entry__
	{
		struct __t_hash_key_entry__* next;
		SINT32 hashkey;
	};

typedef struct __t_hash_key_entry__ t_hashkeylistEntry;


class CAMuxSocket
	{
		public:
			CAMuxSocket();
			~CAMuxSocket();

			/** Returns a Hashkey which uniquely identifies this socket*/
			SINT32 getHashKey()
				{
					return m_pHashKeyEntry->hashkey;
				}

			SINT32 accept(UINT16 port);
			SINT32 accept(const CASocketAddr& oAddr);
			SINT32 connect(CASocketAddr& psa);
			SINT32 connect(CASocketAddr& psa,UINT retry,UINT32 time);
			SINT32 close();
			SINT32 send(MIXPACKET *pPacket);
			SINT32 send(MIXPACKET *pPacket,UINT8* buff);
			SINT32 prepareForSend(MIXPACKET* inoutPacket);
			SINT32 receive(MIXPACKET *pPacket);
			SINT32 receive(MIXPACKET *pPacket,UINT32 timeout);
			
			/** Receives some "plain" bytes from the underlying socket - just a convinient function...*/
			SINT32 receiveFully(UINT8* buff,UINT32 len)
				{
					return m_Socket.receiveFully(buff,len);
				}
			//int close(HCHANNEL channel_id);
			//int close(HCHANNEL channel_id,UINT8* buff);
#ifdef LOG_CRIME
			UINT32 sigCrime(HCHANNEL channel_id,MIXPACKET* sigPacket);
#endif
			CASocket* getCASocket(){return &m_Socket;}
			SOCKET getSocket(){return m_Socket.getSocket();}

			SINT32 setCrypt(bool b);
			bool getIsEncrypted()
				{
					return m_bIsCrypted;
				}

			/** Sets the symmetric keys used for de-/encrypting the Mux connection
				*
				* @param key buffer conntaining the key bits
				* @param keyLen size of the buffer (keys)
				*					if keylen=16, then the key is used for incomming and outgoing direction (key only)
				*					if keylen=32, then the first bytes are used for incoming and the last bytes are used for outgoing
				*	@retval E_SUCCESS if successful
				*	@retval E_UNKNOWN otherwise
				*/
			SINT32 setKey(UINT8* key,UINT32 keyLen)
				{
					if(keyLen==16)
						{
							m_oCipherIn.setKey(key);
							m_oCipherOut.setKey(key);
						}
					else if(keyLen==32)
						{
							m_oCipherOut.setKey(key);
							m_oCipherIn.setKey(key+16);
						}
				else
						return E_UNKNOWN;
					return E_SUCCESS;
				}

			SINT32 setSendKey(UINT8* key,UINT32 keyLen)
				{
					if(keyLen==16)
						{
							m_oCipherOut.setKey(key);
						}
					else if(keyLen==32)
						{
							m_oCipherOut.setKey(key);
							m_oCipherOut.setIVs(key+16);
						}
					else
						return E_UNKNOWN;
					return E_SUCCESS;
				}

			SINT32 setReceiveKey(UINT8* key,UINT32 keyLen)
			{
				if(keyLen==16)
				{
					m_oCipherIn.setKey(key);
				}
				else if(keyLen==32)
				{
					m_oCipherIn.setKey(key);
					m_oCipherIn.setIVs(key+16);
				}
				else
				{
					return E_UNKNOWN;
				}
				return E_SUCCESS;
			}

		private:
				CASocket		m_Socket;
				UINT32			m_aktBuffPos;
				UINT8*			m_Buff;
				CASymCipher		m_oCipherIn;
				CASymCipher		m_oCipherOut;
				bool			m_bIsCrypted;
				CAMutex			m_csSend;
				CAMutex			m_csReceive;
				t_hashkeylistEntry*			m_pHashKeyEntry;

				static t_hashkeylistEntry* ms_phashkeylistAvailableHashKeys; //stores he avilalbe hashkeys -> if this list is empty new entires are create on the fly
				static SINT32 ms_nMaxHashKeyValue; // the maximum value of a hash key
				static CAMutex* ms_pcsHashKeyList;

	};
#endif
