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

#ifndef __CACMDLNOPTIONS__
#define __CACMDLNOPTIONS__
#include "CASocketAddrINet.hpp"
#ifdef HAVE_UNIX_DOMAIN_PROTOCOL
	#include "CASocketAddrUnix.hpp"
#endif
#include "CASignature.hpp"
#include "CACertificate.hpp"
#include "CAThread.hpp"
#include "CAMix.hpp"
#include "CAListenerInterface.hpp"
#include "CAXMLBI.hpp"
#ifdef LOG_CRIME
	#include "tre/regex.h"
#endif


#define TARGET_MIX					1
#define TARGET_HTTP_PROXY		2
#define TARGET_SOCKS_PROXY	3

struct t_TargetInterface
	{
		UINT32 target_type;
		NetworkType net_type;
		CASocketAddr* addr;
	};

typedef struct t_TargetInterface TargetInterface;

THREAD_RETURN threadReConfigure(void *param);

class CACmdLnOptions
    {
	public:
	    CACmdLnOptions();
	    ~CACmdLnOptions();
			SINT32 cleanup();
			void clean();
			SINT32 parse(int argc,const char** arg);
	    bool getDaemon();
      //bool getProxySupport();

			SINT32 getMixId(UINT8* id,UINT32 len);


//			UINT16 getServerPort();
			/*For IP (Host) AND Unix Domain Sockets*/
//	    SINT32 getServerHost(UINT8* path,UINT32 len);

//			SINT32 getServerRTTPort();
			UINT16 getSOCKSServerPort();


			UINT32 getListenerInterfaceCount(){return m_cnListenerInterfaces;}
			CAListenerInterface* getListenerInterface(UINT32 nr)
				{
					if(nr>0&&nr<=m_cnListenerInterfaces&&m_arListenerInterfaces[nr-1]!=NULL)
						return new CAListenerInterface(*m_arListenerInterfaces[nr-1]);
					return NULL;
				};

			//this is only for the local proxy
			UINT16 getMixPort();
			SINT32 getMixHost(UINT8* host,UINT32 len);

			//if we have more than one Target (currently only Caches are possible...)
			UINT32 getTargetInterfaceCount(){return m_cnTargets;}

			/** Fills a \c TargetInterface struct with the values which belongs to
				* the target interface \c nr.
				* This is actual a copy of all values, so the caller is responsible
				* for destroying them after use!
				*
				* @param oTargetInterface \c TargetInterface struct, which gets filles with
				*															the values of target interface \c nr
				*	@param nr the index of the target interface, for whcih information
				*					is request (starting with 1 for the first interface)
				* @retval E_SUCCESS if successful
				* @retval E_UNKNOWN if \c nr is out of range
				*/
			SINT32 getTargetInterface(TargetInterface& oTargetInterface, UINT32 nr)
				{
					if(nr>0&&nr<=m_cnTargets)
						{
							oTargetInterface.net_type=m_arTargetInterfaces[nr-1].net_type;
							oTargetInterface.target_type=m_arTargetInterfaces[nr-1].target_type;
							oTargetInterface.addr=m_arTargetInterfaces[nr-1].addr->clone();
							return E_SUCCESS;
						}
					else
						return E_UNKNOWN;
				};

#ifndef ONLY_LOCAL_PROXY
			//for last Mixes: number of outside visible addresses
			UINT32 getVisibleAddressesCount(){return m_cnVisibleAddresses;}

			/** Fills \c strAddressBuff with a outside visible adress.
				@param strAddressBuff buffer for adress information (either hostname or IP string)
				@param len size of strAddressBuff
				@param nr the number of the adress we request information about (starting with 1 for the first address)
			* @retval E_SUCCESS if successful
			@retval E_SPACE if buffer is to small for the requested address
				* @retval E_UNKNOWN if \c nr is out of range
			*/
			SINT32 getVisibleAddress(UINT8* strAddressBuff, UINT32 len,UINT32 nr);

			UINT16 getSOCKSPort();
	    SINT32 getSOCKSHost(UINT8* host,UINT32 len);
			CAListenerInterface** getInfoServices(UINT32& r_size);
#endif //ONLY_LOCAL_PROXY

			SINT32 getMaxOpenFiles()
				{
					return m_nrOfOpenFiles;
				}


#ifndef ONLY_LOCAL_PROXY
			CASignature* getSignKey()
				{
					if(m_pSignKey!=NULL)
						return m_pSignKey->clone();
					return NULL;
				}
			/** Returns a COPY of the public test certifcate for that mix.
				* @retval a COPY of the mix test certifcate.
				*/
			CACertificate* getOwnCertificate() const
				{
					if(m_pOwnCertificate!=NULL)
					{
						return m_pOwnCertificate->clone();
					}
					return NULL;
				}
				
			/** Returns a COPY of the Operator Certificate that mix.
				* @return opCerts
				*/
			CACertificate** getOpCertificates(UINT32& r_length) const
			{
				if(m_OpCerts!=NULL && m_OpCertsLength > 0)
				{
					CACertificate** opCerts = new CACertificate*[m_OpCertsLength];
					for (UINT32 i = 0; i < m_OpCertsLength; i++)
					{
						opCerts[i] = m_OpCerts[i]->clone();
					}
					r_length = m_OpCertsLength;
					return opCerts;
				}
				r_length = 0;
				return NULL;
			}

			bool hasPrevMixTestCertificate()
				{
					return m_pPrevMixCertificate!=NULL;
				}

			CACertificate* getPrevMixTestCertificate()
				{
					if(m_pPrevMixCertificate!=NULL)
						return m_pPrevMixCertificate->clone();
					return NULL;
				}


			bool hasNextMixTestCertificate()
				{
					return m_pNextMixCertificate!=NULL;
				}

			CACertificate* getNextMixTestCertificate()
				{
					if(m_pNextMixCertificate!=NULL)
						return m_pNextMixCertificate->clone();
					return NULL;
				}
     /** Returns if the encrpyted Log could/should be used**/
			bool isEncryptedLogEnabled()
			{
				return m_bIsEncryptedLogEnabled;
			}

			/** Set to true if the encrpyted log could/should be used**/
			SINT32 enableEncryptedLog(bool b)
			{
				m_bIsEncryptedLogEnabled=b;
				return E_SUCCESS;
			}

			/** Returns a certificate which contains a key which could be used for log encryption*/
			CACertificate* getLogEncryptionKey()
				{
					if(m_pLogEncryptionCertificate!=NULL)
						return m_pLogEncryptionCertificate->clone();
					return NULL;
				}

			DOM_Element& getCascadeXML()
			{
					return m_oCascadeXML;
			}

			SINT32 getCascadeName(UINT8* name,UINT32 len);
    
			// added by ronin <ronin2@web.de>
			SINT32 setCascadeName(char* name)
			{
    		if(m_strCascadeName!=NULL)
					delete m_strCascadeName;
				m_strCascadeName = new char[strlen(name)+1];
				strcpy(m_strCascadeName,name);
				return E_SUCCESS;
			}

			SINT32 reread(CAMix* pMix);


			SINT32 getEncryptedLogDir(UINT8* name,UINT32 len);

		/** Get the XML describing the Mix. this is not a string!*/
			//SINT32 getMixXml(UINT8* strxml,UINT32* len);
			SINT32 getMixXml(DOM_Document &docMixInfo);
	
#endif //ONLY_LOCAL_PROXY			
			bool getCompressLogs()
				{
					return m_bCompressedLogs;
				}
			SINT32 getLogDir(UINT8* name,UINT32 len);
			SINT32 getUser(UINT8* user,UINT32 len);
			SINT32 getPidFile(UINT8* pidfile,UINT32 len);

			bool isLocalProxy();
			bool isFirstMix();
			bool isMiddleMix();
			bool isLastMix();
			bool isInfoServiceEnabled()
				{
					return m_addrInfoServicesSize;
				}

			bool getAutoReconnect()
				{
					return m_bAutoReconnect;
				}
#ifdef LOG_CRIME
			regex_t* getCrimeRegExps(UINT32* len)
				{
					*len=m_nCrimeRegExps;
					return m_arCrimeRegExps;
				}
#endif

#if defined(DELAY_CHANNELS)||defined(DELAY_USERS)
			UINT32 getDelayChannelUnlimitTraffic()
				{
					return m_u32DelayChannelUnlimitTraffic;
				}	
			UINT32 getDelayChannelBucketGrow()
				{
					return m_u32DelayChannelBucketGrow;
				}
			UINT32 getDelayChannelBucketGrowIntervall()
				{
					return m_u32DelayChannelBucketGrowIntervall;
				}
#endif

#if defined(DELAY_CHANNELS_LATENCY)
				/** Channel Latency in ms*/
			UINT32 getDelayChannelLatency()
				{
					return m_u32DelayChannelLatency;
				}	
#endif


#ifdef PAYMENT
// added by Bastian Voigt:
// getter functions for the payment config options
/*			SINT32 getJPIHost(UINT8* host,UINT32 len);
			UINT16 getJPIPort();*/
			
			// accounting database
			SINT32 getDatabaseHost(UINT8 * host, UINT32 len);
			UINT16 getDatabasePort();
			SINT32 getDatabaseName(UINT8 * name, UINT32 len);
			SINT32 getDatabaseUsername(UINT8 * user, UINT32 len);
			SINT32 getDatabasePassword(UINT8 * pass, UINT32 len);
			SINT32 getAiID(UINT8 * id, UINT32 len);
			CAXMLBI* getBI();
			SINT32 getPaymentHardLimit(UINT32 *pHardLimit);
			SINT32 getPaymentSoftLimit(UINT32 *pSoftLimit);
			SINT32 getPaymentSettleInterval(UINT32 *pInterval);
			
#endif	


#ifndef ONLY_LOCAL_PROXY
			// added by ronin <ronin2@web.de>
    // needed for autoconfiguration
			SINT32 setNextMix(DOM_Document&);
			SINT32 setPrevMix(DOM_Document&);
			bool acceptReconfiguration() { return m_bAcceptReconfiguration; }

			friend THREAD_RETURN threadReConfigure(void *param);
			
			/** Writes a default configuration file into the file named by filename*/
			static SINT32 createMixOnCDConfiguration(const UINT8* strFileName);
#endif //only_LOCAL_PROXY
		private:
			UINT8*	m_strConfigFile; //the filename of the config file
			bool		m_bDaemon;
	    UINT16	m_iSOCKSServerPort;
	    UINT16	m_iTargetPort; //only for the local proxy...
			char*		m_strTargetHost; //only for the local proxy...
			char*		m_strSOCKSHost;
	    UINT16	m_iSOCKSPort;
#ifndef ONLY_LOCAL_PROXY
			bool		m_bIsRunReConfigure; //true, if an async reconfigure is under way
	    CAMutex* m_pcsReConfigure; //Ensures that reconfigure is running only once at the same time;
			CAThread m_threadReConfigure; //Thread, that does the actual reconfigure work
	    CAListenerInterface**	m_addrInfoServices;

			CASignature*		m_pSignKey;
			CACertificate*	m_pOwnCertificate;
			
			CACertificate** m_OpCerts;
			UINT32 m_OpCertsLength;

			CACertificate*	m_pPrevMixCertificate;
			CACertificate*	m_pNextMixCertificate;
			CACertificate*	m_pLogEncryptionCertificate;

			// added by ronin <ronin2@web.de>
			DOM_Element m_oCascadeXML;
			bool m_bAcceptReconfiguration;
			DOM_Document m_docMixInfo;
			DOM_Document m_docMixXml;
#endif //ONLY_LOCAL_PROXY

			UINT32 m_addrInfoServicesSize;
			bool		m_bLocalProxy,m_bFirstMix,m_bMiddleMix,m_bLastMix;
			bool		m_bAutoReconnect; //auto reconnect if connection to first mix lost ??
			char*		m_strCascadeName;
			char*		m_strLogDir;
			char*		m_strEncryptedLogDir;
			bool		m_bCompressedLogs;
			char*		m_strUser;
			char*		m_strPidFile;
			SINT32	m_nrOfOpenFiles; //How many open files (sockets) should we use
    
			//char*		m_strMixXml;
			char*		m_strMixID;

			bool m_bIsEncryptedLogEnabled;

			TargetInterface*			m_arTargetInterfaces;
			UINT32								m_cnTargets;
			CAListenerInterface**	m_arListenerInterfaces;
			UINT32								m_cnListenerInterfaces;
			UINT8**								m_arStrVisibleAddresses;
			UINT32								m_cnVisibleAddresses;
	

#ifdef LOG_CRIME
			regex_t* m_arCrimeRegExps;
			UINT32 m_nCrimeRegExps;
#endif
#if defined (DELAY_CHANNELS) ||defined(DELAY_USERS)
		UINT32 m_u32DelayChannelUnlimitTraffic;	
		UINT32 m_u32DelayChannelBucketGrow;	
		UINT32 m_u32DelayChannelBucketGrowIntervall;	
#endif

#if defined (DELAY_CHANNELS_LATENCY)
		UINT32 m_u32DelayChannelLatency;	
#endif

#ifdef PAYMENT
// added by Bastian Voigt:
// getter functions for the payment config options
		private:
			CAXMLBI * m_pBI;
			UINT8 * m_strDatabaseHost;
			UINT8 * m_strDatabaseName;
			UINT8 * m_strDatabaseUser;
			UINT8 * m_strDatabasePassword;
			UINT8* m_strAiID;
			UINT16 m_iDatabasePort;
			UINT32 m_iPaymentHardLimit;
			UINT32 m_iPaymentSoftLimit;
			UINT32 m_iPaymentSettleInterval;
#endif

		private:
			SINT32 setNewValues(CACmdLnOptions& newOptions);
#ifndef ONLY_LOCAL_PROXY
			SINT32 readXmlConfiguration(DOM_Document& docConfig,const UINT8* const configFileName);
			SINT32 readXmlConfiguration(DOM_Document& docConfig,const UINT8* const buf, UINT32 len);
			SINT32 processXmlConfiguration(DOM_Document& docConfig);
			SINT32 clearVisibleAddresses();
			SINT32 addVisibleAddresses(DOM_Node& nodeProxy);
#endif
			SINT32 clearTargetInterfaces();
			SINT32 clearListenerInterfaces();
	};
#endif
