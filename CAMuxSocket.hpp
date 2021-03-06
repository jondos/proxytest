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
#include "CASymChannelCipher.hpp"
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
			CAMuxSocket(SYMCHANNELCIPHER_ALGORITHM algCipher);
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
			
			/** Receives some "plain" bytes from the underlying socket - just a convenient function...*/
			SINT32 receiveFully(UINT8* buff,UINT32 len)
			{
				return m_pSocket->receiveFully(buff,len);
			}
				
			SINT32 receiveFully(UINT8* buff,UINT32 len, UINT32 msTimeOut)
			{
				return m_pSocket->receiveFullyT(buff,len, msTimeOut);
			}
				
			//int close(HCHANNEL channel_id);
			//int close(HCHANNEL channel_id,UINT8* buff);
#ifdef LOG_CRIME
			void sigCrime(HCHANNEL channel_id,MIXPACKET* sigPacket);
#endif
			CASocket* getCASocket(){return m_pSocket;}
			/**
			 * @brief This will set the underlying CASocket. Note: The object will be under full controll of CAMuxSocket. 
			 * It will be destroyed by CAMuxSocket, if not longer needed. Therefore it must have be dynamically allocated.
			 * @param pSocket the new CASocket to use, should not be NULL
			 * @return E_SUCCESS, if successful, E_UNKNOWN otherwise
			*/
			SINT32 setCASocket(CASocket *pSocket);
			SOCKET getSocket(){return m_pSocket->getSocket();}


			SINT32 setCrypt(bool b);
			bool getIsEncrypted()
				{
					return m_bIsCrypted;
				}


			/***Sets the cipher algorithm used. Note: the current values for keys and IVs will not be carried over to the new cipher algorithm!
			**/
			SINT32 setCipher(SYMCHANNELCIPHER_ALGORITHM algCipher);

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
							m_pCipherIn->setKey(key);
							m_pCipherOut->setKey(key);
						}
					else if(keyLen==32)
						{
							m_pCipherOut->setKey(key);
							m_pCipherIn->setKey(key+16);
						}
				else
						return E_UNKNOWN;
					return E_SUCCESS;
				}

			SINT32 setSendKey(UINT8* key,UINT32 keyLen)
				{
					if(keyLen==16)
						{
							m_pCipherOut->setKey(key);
						}
					else if(keyLen==32)
						{
							m_pCipherOut->setKey(key);
							m_pCipherOut->setIVs(key+16);
						}
					else
						return E_UNKNOWN;
					return E_SUCCESS;
				}

			SINT32 setReceiveKey(UINT8* key,UINT32 keyLen)
			{
				if(keyLen==16)
				{
					m_pCipherIn->setKey(key);
				}
				else if(keyLen==32)
				{
					m_pCipherIn->setKey(key);
					m_pCipherIn->setIVs(key+16);
				}
				else
				{
					return E_UNKNOWN;
				}
				return E_SUCCESS;
			}

			static SINT32 init()
					{
						ms_pcsHashKeyList=new CAMutex();
						return E_SUCCESS;
					}

			static SINT32 cleanup()
					{
						while(ms_phashkeylistAvailableHashKeys!=NULL)
							{
								t_hashkeylistEntry* tmpEntry = ms_phashkeylistAvailableHashKeys;
								ms_phashkeylistAvailableHashKeys=ms_phashkeylistAvailableHashKeys->next;
								delete tmpEntry;
							}
						delete ms_pcsHashKeyList;
						ms_pcsHashKeyList=NULL;
						return E_SUCCESS;
					}

		private:
				CASocket*		m_pSocket;
				UINT32			m_aktBuffPos;
				UINT8*			m_Buff;
				CASymCipherMuxSocket*		m_pCipherIn;
				CASymCipherMuxSocket*		m_pCipherOut;
				bool				m_bIsCrypted;
				CAMutex			m_csSend;
				CAMutex			m_csReceive;
				t_hashkeylistEntry*			m_pHashKeyEntry;

				static t_hashkeylistEntry* ms_phashkeylistAvailableHashKeys; //stores the available hashkeys -> if this list is empty new entires are created on the fly
				static SINT32 ms_nMaxHashKeyValue; // the maximum value of a hash key
				static CAMutex* ms_pcsHashKeyList;

	};
#endif
