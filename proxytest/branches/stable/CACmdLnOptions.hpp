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
#include "CAMultiSignature.hpp"
#include "CACertificate.hpp"
#include "CAThread.hpp"
#include "CAMix.hpp"
#include "CAListenerInterface.hpp"
#include "CAXMLBI.hpp"
#include "CAXMLPriceCert.hpp"
//#ifdef LOG_CRIME
	#include "tre/regex.h"
//#endif

#define REGEXP_BUFF_SIZE 4096

#define TARGET_MIX			1
#define TARGET_HTTP_PROXY	2
#define TARGET_SOCKS_PROXY	3

// LERNGRUPPE moved this define from CACmdLnOptions.cpp
#define DEFAULT_TARGET_PORT 6544
#define DEFAULT_CONFIG_FILE "default.xml"
#define MIN_INFOSERVICES 1
// END LERNGRUPPE

#define WITH_SUBTREE true
#define WITHOUT_SUBTREE (!(WITHSUBTREE))


/* General Option definitions */
#define OPTIONS_NODE_GENERAL "General"

#define OPTIONS_NODE_MIX_TYPE "MixType"
#define OPTIONS_NODE_MIX_NAME "MixName"
#define OPTIONS_NODE_MIX_ID	"MixID"
#define OPTIONS_NODE_DYNAMIC_MIX "Dynamic"
#define OPTIONS_NODE_MIN_CASCADE_LENGTH "MinCascadeLength"
#define OPTIONS_NODE_CASCADE_NAME "CascadeName"
#define OPTIONS_NODE_USER_ID "UserID"
#define OPTIONS_NODE_FD_NR "NrOfFileDescriptors"
#define OPTIONS_NODE_DAEMON "Daemon"
#define OPTIONS_NODE_MAX_USERS "MaxUsers"
#define OPTIONS_NODE_LOGGING "Logging"
#define OPTIONS_NODE_LOGGING_FILE "File"
#define OPTIONS_ATTRIBUTE_LOGGING_MAXFILESIZE "MaxFileSize"
#define OPTIONS_ATTRIBUTE_LOGGING_MAXFILES "MaxFiles"
#define LOGGING_MAXFILES_DEFAULT 10
#define OPTIONS_NODE_SYSLOG "Syslog"
#define OPTIONS_NODE_ENCRYPTED_LOG "EncryptedLog"
#define OPTIONS_NODE_LOGGING_KEYINFO "KeyInfo"
#define OPTIONS_NODE_DESCRIPTION "Description"
#define OPTIONS_ATTRIBUTE_NAME_FOR_CASCADE "forCascade"

/* values for the operator OPTIONS_NODE_MIX_NAME */
#define OPTIONS_VALUE_OPERATOR_NAME "Operator"
#define OPTIONS_VALUE_MIX_NAME "Mix"
#define OPTIONS_VALUE_NAMETYPE_DEFAULT OPTIONS_VALUE_MIX_NAME

/* Certificate Option definitions */
#define OPTIONS_NODE_CERTIFICATE_LIST "Certificates"

#define OPTIONS_NODE_OWN_CERTIFICATE "OwnCertificate"
#define OPTIONS_NODE_OWN_OPERATOR_CERTIFICATE "OperatorOwnCertificate"
#define OPTIONS_NODE_NEXT_MIX_CERTIFICATE "NextMixCertificate"
#define OPTIONS_NODE_NEXT_OPERATOR_CERTIFICATE "NextOperatorCertificate"
#define OPTIONS_NODE_PREV_MIX_CERTIFICATE "PrevMixCertificate"
#define OPTIONS_NODE_PREV_OPERATOR_CERTIFICATE "PrevOperatorCertificate"
#define OPTIONS_NODE_X509DATA "X509Data"
#define OPTIONS_NODE_ELEMENT_X509CERT "X509Certificate"
#define OPTIONS_NODE_SIGNATURE "Signature"


/* Accounting Option definitions */
#define OPTIONS_NODE_ACCOUNTING "Accounting"

#define OPTIONS_NODE_PRICE_CERTIFICATE "PriceCertificate"
#define OPTIONS_NODE_PAYMENT_INSTANCE CAXMLBI::getXMLElementName()
#define OPTIONS_NODE_AI_SOFT_LIMIT "SoftLimit"
#define OPTIONS_NODE_AI_HARD_LIMIT "HardLimit"
#define OPTIONS_NODE_SETTLE_IVAL "SettleInterval"
#define OPTIONS_NODE_PREPAID_IVAL "PrepaidInterval"
#define OPTIONS_NODE_PREPAID_IVAL_KB "PrepaidIntervalKbytes"
#define OPTIONS_NODE_AI_DB "Database"
#define OPTIONS_NODE_AI_DB_HOST "Host"
#define OPTIONS_NODE_AI_DB_PORT "Port"
#define OPTIONS_NODE_AI_DB_NAME "DBName"
#define OPTIONS_NODE_AI_DB_USER "Username"
#define OPTIONS_NODE_AI_DB_PASSW "Password"

#define OPTIONS_DEFAULT_PREPAID_IVAL 3000000 //3 MB as safe default if not explicitly set in config file

#define OPTIONS_NODE_NETWORK "Network"

#define OPTIONS_NODE_INFOSERVICE_LIST "InfoServices"
#define OPTIONS_NODE_INFOSERVICE "InfoService"
#define OPTIONS_NODE_ALLOW_AUTO_CONF "AllowAutoConfiguration"
#define OPTIONS_NODE_LISTENER_INTERFACES CAListenerInterface::XML_ELEMENT_CONTAINER_NAME
#define OPTIONS_NODE_NEXT_MIX "NextMix"
#define OPTIONS_NODE_NETWORK_PROTOCOL "NetworkProtocol"
#define OPTIONS_NODE_IP "IP"
#define OPTIONS_NODE_PROXY_LIST "Proxies"
#define OPTIONS_NODE_PROXY "Proxy"
#define OPTIONS_NODE_PROXY_TYPE "ProxyType"
#define OPTIONS_NODE_SERVER_MONITORING "ServerMonitoring"
#define OPTIONS_NODE_VISIBLE_ADDRESS_LIST "VisibleAddresses"
#define OPTIONS_NODE_VISIBLE_ADDRESS "VisibleAddress"
#define OPTIONS_NODE_LISTENER_INTERFACE_LIST CAListenerInterface::XML_ELEMENT_CONTAINER_NAME
#define OPTIONS_NODE_LISTENER_INTERFACE CAListenerInterface::XML_ELEMENT_NAME
#define OPTIONS_NODE_KEEP_ALIVE "KeepAlive"
#define OPTIONS_NODE_KEEP_ALIVE_SEND_IVAL "SendInterval"
#define OPTIONS_NODE_KEEP_ALIVE_RECV_IVAL "ReceiveInterval"
#define OPTIONS_NODE_IP "IP"
#define OPTIONS_NODE_HOST "Host"
#define OPTIONS_NODE_PORT "Port"
#define OPTIONS_NODE_FILE "File"

#define OPTIONS_NODE_RESSOURCES "Ressources"

#define OPTIONS_NODE_UNLIMIT_TRAFFIC "UnlimitTraffic"
#define OPTIONS_NODE_BYTES_PER_IVAL "BytesPerIntervall"
#define OPTIONS_NODE_DELAY_IVAL "Intervall"
#define OPTIONS_NODE_LATENCY "Latency"

#define OPTIONS_NODE_TNCS_OPTS "TermsAndConditionsOptions"
#define OPTIONS_NODE_TNCS_TEMPLATES "Templates"
#define OPTIONS_NODE_TNCS_TEMPLATE "Template"
#define OPTIONS_NODE_TNCS "TermsAndConditions"
#define OPTIONS_NODE_TNCS_TRANSLATION "TCTranslation"
#define OPTIONS_NODE_TNCS_TRANSLATION_IMPORTS "TCTranslationImports"
#define OPTIONS_NODE_TNCS_OPERATOR "Operator"
#define OPTIONS_ATTRIBUTE_TNC_DATE "date"
#define OPTIONS_ATTRIBUTE_TNC_SERIAL "serial"
#define OPTIONS_ATTRIBUTE_TNC_VERSION "version"
#define OPTIONS_ATTRIBUTE_TNC_LOCALE "locale"
#define OPTIONS_ATTRIBUTE_TNC_TEMPLATE_TYPE "type"
#define OPTIONS_ATTRIBUTE_TNC_ID "id"
#define OPTIONS_ATTRIBUTE_TNC_TEMPLATE_REFID "referenceId"
#define OPTIONS_ATTRIBUTE_TNC_DEFAULT_LANG_DEFINED "default"
#define OPTIONS_ATTRIBUTE_TNC_DEFAULT_LANG "defaultLang"

#define OPTIONS_NODE_CRIME_DETECTION "CrimeDetection"

#define OPTIONS_NODE_CRIME_REGEXP_URL "RegExpURL"
#define OPTIONS_NODE_CRIME_REGEXP_PAYLOAD "RegExpPayload"
#define OPTIONS_NODE_CRIME_SURVEILLANCE_IP "SurveillanceIP"
#define OPTIONS_ATTRIBUTE_LOG_PAYLOAD "logPayload"

#define MIXINFO_NODE_PARENT "Mix"
#define MIXINFO_NODE_MIX_NAME "Name"
#define MIXINFO_NODE_SOFTWARE "Software"
#define MIXINFO_NODE_VERSION "Version"

#define MIXINFO_ATTRIBUTE_MIX_ID "id"

#define LOG_NODE_NOT_FOUND(Nodename) \
	CAMsg::printMsg(LOG_CRIT,"No \"%s\" node found in configuration file!\n", (Nodename))

#define LOG_NODE_EMPTY_OR_INVALID(Nodename) \
	CAMsg::printMsg(LOG_CRIT,"Node \"%s\" is empty or has invalid content!\n", (Nodename))

#define LOG_NODE_WRONG_PARENT(Parentname, Childname) \
	CAMsg::printMsg(LOG_CRIT,"\"%s\" is the wrong parent for Node \"%s\"\n", (Parentname), (Childname))


#define ASSERT_PARENT_NODE_NAME(Parentname, NameToMatch, Childname) 	\
	if(!equals((Parentname), (NameToMatch) )) 			\
	{											 		\
		char *parentName = XMLString::transcode(Parentname); \
		LOG_NODE_WRONG_PARENT(parentName, Childname);	\
		XMLString::release(&parentName);				\
		return E_UNKNOWN;								\
	}

#define ASSERT_GENERAL_OPTIONS_PARENT(Parentname, Childname) \
	ASSERT_PARENT_NODE_NAME(Parentname, OPTIONS_NODE_GENERAL, Childname)

#define ASSERT_CERTIFICATES_OPTIONS_PARENT(Parentname, Childname) \
	ASSERT_PARENT_NODE_NAME(Parentname, OPTIONS_NODE_CERTIFICATE_LIST, Childname)

#define ASSERT_ACCOUNTING_OPTIONS_PARENT(Parentname, Childname) \
	ASSERT_PARENT_NODE_NAME(Parentname, OPTIONS_NODE_ACCOUNTING, Childname)

#define ASSERT_NETWORK_OPTIONS_PARENT(Parentname, Childname) \
	ASSERT_PARENT_NODE_NAME(Parentname, OPTIONS_NODE_NETWORK, Childname)

#define ASSERT_CRIME_DETECTION_OPTIONS_PARENT(Parentname, Childname) \
	ASSERT_PARENT_NODE_NAME(Parentname, OPTIONS_NODE_CRIME_DETECTION, Childname)

struct t_TargetInterface
{
	UINT32 target_type;
	NetworkType net_type;
	CASocketAddr* addr;
};

typedef struct t_TargetInterface TargetInterface;

THREAD_RETURN threadReConfigure(void *param);

class CACmdLnOptions;
typedef SINT32 (CACmdLnOptions::*optionSetter_pt)(DOMElement *);

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
		 * @param strAddressBuff buffer for adress information (either hostname or IP string)
		 * @param len size of strAddressBuff
		 * @param nr the number of the adress we request information about (starting with 1 for the first address)
		 * @retval E_SUCCESS if successful
		 * @retval E_SPACE if buffer is to small for the requested address
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
		//TODO maybe clone MultiSignature object!
		CAMultiSignature* getMultiSigner(){ return m_pMultiSignature; }
		/*CASignature* getSignKey()
		{
			if(m_pSignKey!=NULL)
				return m_pSignKey->clone();
			return NULL;
		}*/
		/** Returns a COPY of the public test certifcate for that mix.
		 * @retval a COPY of the mix test certifcate.
		 */
		/*CACertificate* getOwnCertificate() const
		{
			if(m_pOwnCertificate!=NULL)
			{
				return m_pOwnCertificate->clone();
			}
			return NULL;
		}*/
		/** Returns a COPY of the Operator Certificate of that mix.
		 * @return opCerts
		 */
		/*CACertificate* getOpCertificate() const
		{
			if( m_OpCert != NULL )
			{
				return m_OpCert->clone();
			}
			return NULL;
		}*/
		SINT32 getOperatorSubjectKeyIdentifier(UINT8 *buffer, UINT32 *length);
#ifdef PAYMENT
		CAXMLPriceCert* getPriceCertificate() const
		{
			if(m_pPriceCertificate != NULL)
			{
				return m_pPriceCertificate;
			}
			return NULL;
		}
#endif

#ifdef COUNTRY_STATS
		SINT32 getCountryStatsDBConnectionLoginData(char** db_host,char**db_user,char**db_passwd);
#endif
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
		bool isSyslogEnabled()
		{
			return m_bSyslog;
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

		DOMElement* getCascadeXML()
		{
			return m_pCascadeXML;
		}

		SINT32 getCascadeName(UINT8* name,UINT32 len) const;

		// added by ronin <ronin2@web.de>
		SINT32 setCascadeName(const UINT8* name)
		{
			delete[] m_strCascadeName;
			m_strCascadeName = new UINT8[strlen((const char*)name)+1];
			strcpy((char*)m_strCascadeName,(const char*)name);
			return E_SUCCESS;
		}

		SINT32 reread(CAMix* pMix);


		SINT32 getEncryptedLogDir(UINT8* name,UINT32 len);

		/** Get the XML describing the Mix. this is not a string!*/
		//SINT32 getMixXml(UINT8* strxml,UINT32* len);
		SINT32 getMixXml(XERCES_CPP_NAMESPACE::DOMDocument* & docMixInfo);

		UINT32 getNumberOfTermsAndConditionsTemplates();
		XERCES_CPP_NAMESPACE::DOMDocument **getAllTermsAndConditionsTemplates();
		XERCES_CPP_NAMESPACE::DOMElement *getTermsAndConditions();

		UINT32 getKeepAliveSendInterval()
		{
			return m_u32KeepAliveSendInterval;
		}

		UINT32 getKeepAliveRecvInterval()
		{
			return m_u32KeepAliveRecvInterval;
		}
		bool isInfoServiceEnabled()
		{
			return (m_addrInfoServicesSize>0);
		}
#endif //ONLY_LOCAL_PROXY
		bool getCompressLogs()
		{
			return m_bCompressedLogs;
		}
		SINT32 getLogDir(UINT8* name,UINT32 len);
		SINT32 setLogDir(const UINT8* name,UINT32 len);
		SINT64 getMaxLogFileSize()
		{
			return m_maxLogFileSize;
		}

		UINT32 getMaxLogFiles()
		{
			return m_maxLogFiles;
		}

		SINT32 getUser(UINT8* user,UINT32 len);
		SINT32 getPidFile(UINT8* pidfile,UINT32 len);

#ifdef SERVER_MONITORING
		char *getMonitoringListenerHost();
		UINT16 getMonitoringListenerPort();
#endif /* SERVER_MONITORING */

		bool isLocalProxy();
		bool isFirstMix();
		bool isMiddleMix();
		bool isLastMix();


		bool isSock5sSupported()
		{
			return m_bSocksSupport;
		}


		bool getAutoReconnect()
		{
			return m_bAutoReconnect;
		}

		bool getAutoRestart()
		{
			return m_bAutoRestart;
		}
#ifdef LOG_CRIME
		regex_t* getCrimeRegExpsURL(UINT32* len)
		{
			*len=m_nCrimeRegExpsURL;
			return m_arCrimeRegExpsURL;
		}

		regex_t* getCrimeRegExpsPayload(UINT32* len)
		{
			*len=m_nCrimeRegExpsPayload;
			return m_arCrimeRegExpsPayload;
		}

		CASocketAddrINet* getCrimeSurveillanceIPs()
		{
			return m_surveillanceIPs;
		}

		UINT32 getNrOfCrimeSurveillanceIPs()
		{
			return m_nrOfSurveillanceIPs;
		}

		bool isPayloadLogged()
		{
			return m_logPayload;
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
		// accounting database
		SINT32 getDatabaseHost(UINT8 * host, UINT32 len);
		UINT16 getDatabasePort();
		SINT32 getDatabaseName(UINT8 * name, UINT32 len);
		SINT32 getDatabaseUsername(UINT8 * user, UINT32 len);
		SINT32 getDatabasePassword(UINT8 * pass, UINT32 len);
		SINT32 getAiID(UINT8 * id, UINT32 len);
		CAXMLBI* getBI();
		UINT32 getPaymentHardLimit();
		UINT32 getPaymentSoftLimit();
		UINT32 getPrepaidInterval();
		UINT32 getPaymentSettleInterval();
#endif

#ifdef DATA_RETENTION_LOG
		SINT32 getDataRetentionLogDir(UINT8* strLogDir,UINT32 len);
		SINT32 getDataRetentionPublicEncryptionKey(CAASymCipher** pKey)
		{
			*pKey=m_pDataRetentionPublicEncryptionKey;
			return E_SUCCESS;
		}
#endif

#ifndef ONLY_LOCAL_PROXY
		// added by ronin <ronin2@web.de>
		// needed for autoconfiguration
		SINT32 setNextMix(XERCES_CPP_NAMESPACE::DOMDocument* pDoc);
		SINT32 setPrevMix(XERCES_CPP_NAMESPACE::DOMDocument* pDoc);
		bool acceptReconfiguration() { return m_bAcceptReconfiguration; }

		friend THREAD_RETURN threadReConfigure(void *param);

		/** Writes a default configuration file into the file named by filename*/
		static SINT32 createMixOnCDConfiguration(const UINT8* strFileName);
		static SINT32 saveToFile(XERCES_CPP_NAMESPACE::DOMDocument* a_doc, const UINT8* a_strFileName);
		UINT32 getMaxNrOfUsers()
		{
			return m_maxNrOfUsers;
		}

#ifdef DYNAMIC_MIX
		/* LERNGRUPPE (refactoring + new) */
		//SINT32 createMixOnCDConfiguration(const UINT8* strFileName);
		SINT32 createDefaultConfiguration();
		SINT32 addListenerInterface(DOM_Element a_elem);
		SINT32 resetNetworkConfiguration();
		SINT32 getRandomInfoService(CASocketAddrINet *&r_address);
		bool isDynamic() { return m_bDynamic; }
		SINT32 changeMixType(CAMix::tMixType a_newMixType);
		SINT32 resetNextMix();
		SINT32 resetPrevMix();
		SINT32 setCascadeProposal(UINT8* a_strCascadeProposal, UINT32 a_len)
		{
			if(m_strLastCascadeProposal != NULL)
			{
				delete m_strLastCascadeProposal;
				m_strLastCascadeProposal = NULL;
			}
			if(a_strCascadeProposal == NULL)
				return E_SUCCESS;
			m_strLastCascadeProposal = new UINT8[ a_len + 1 ];
			memcpy(m_strLastCascadeProposal, a_strCascadeProposal, a_len+1);
			return E_SUCCESS;
		}
		SINT32 getLastCascadeProposal(UINT8* r_strCascadeProposal, UINT32 r_len)
		{
			if(m_strLastCascadeProposal == NULL)
			{
				return E_UNKNOWN;
			}
			if(r_len >= strlen((char*)m_strLastCascadeProposal))
			{
				r_len = strlen((char*)m_strLastCascadeProposal);
				memcpy(r_strCascadeProposal, m_strLastCascadeProposal, r_len + 1);
				return E_SUCCESS;
			}
			return E_UNKNOWN;
		}
#endif // DYNAMIC_MIX
		XERCES_CPP_NAMESPACE::DOMDocument **m_termsAndConditionsTemplates;
		UINT32 m_nrOfTermsAndConditionsTemplates;
	private:
#ifdef DYNAMIC_MIX
		UINT8* m_strLastCascadeProposal;
		UINT32 getRandom(UINT32 a_max);
		SINT32 checkInfoServices(UINT32 *r_runningInfoServices);
		SINT32 checkMixId();
		SINT32 checkListenerInterfaces();
		SINT32 checkCertificates();
#endif //DYNAMIC_MIX
		bool m_bDynamic;
		SINT32 parseInfoServices(DOMElement* a_infoServiceNode);
		/* END LERNGRUPPE */
		static SINT32 buildDefaultConfig(XERCES_CPP_NAMESPACE::DOMDocument* a_doc,bool bForLastMix);
#endif //only_LOCAL_PROXY
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
		UINT32 m_addrInfoServicesSize;

		//CASignature*		m_pSignKey;
		//CACertificate*		m_pOwnCertificate;
		CAMultiSignature* 	m_pMultiSignature;
		//CACertificate** 	m_ownCerts;
		//UINT32 				m_ownCertsLength;
#ifdef PAYMENT
		CAXMLPriceCert*		m_pPriceCertificate;
#endif

		CACertificate* 		m_OpCert;
		//CACertificate** 	m_opCerts;
		//UINT32 				m_opCertsLength;
		DOMNodeList*		m_opCertList;

		CACertificate*	m_pPrevMixCertificate;
		CACertificate*	m_pNextMixCertificate;
		CACertificate*	m_pLogEncryptionCertificate;

		UINT32 m_maxNrOfUsers;

		// added by ronin <ronin2@web.de>
		DOMElement* m_pCascadeXML;
		bool m_bAcceptReconfiguration;
		XERCES_CPP_NAMESPACE::DOMDocument* m_docMixInfo;
		XERCES_CPP_NAMESPACE::DOMDocument* m_docMixXml;
		XERCES_CPP_NAMESPACE::DOMDocument* m_docOpTnCs;

		UINT32 m_u32KeepAliveSendInterval;
		UINT32 m_u32KeepAliveRecvInterval;

		bool m_perfTestEnabled;
#endif //ONLY_LOCAL_PROXY

		bool		m_bLocalProxy,m_bFirstMix,m_bMiddleMix,m_bLastMix;
		bool		m_bAutoReconnect; //auto reconnect if connection to first mix lost ??
		bool		m_bAutoRestart; //auto restart if Mix dies unexpectly?
		UINT8*	m_strCascadeName;
		char*		m_strLogDir;
		SINT64	m_maxLogFileSize;
		UINT32	m_maxLogFiles; //how many log files can be created before starting again with the first one
		char*		m_strEncryptedLogDir;
		bool		m_bCompressedLogs;
		bool 		m_bSocksSupport;
		bool		m_bSyslog;
		char*		m_strUser;
		char*		m_strPidFile;
		SINT32	m_nrOfOpenFiles; //How many open files (sockets) should we use

		//char*		m_strMixXml;
		char*		m_strMixID;
		char*		m_strMixName;

		bool m_bIsEncryptedLogEnabled;

		TargetInterface*			m_arTargetInterfaces;
		UINT32								m_cnTargets;
		CAListenerInterface**	m_arListenerInterfaces;
		UINT32								m_cnListenerInterfaces;
		UINT8**								m_arStrVisibleAddresses;
		UINT32								m_cnVisibleAddresses;


#ifdef LOG_CRIME
		bool m_logPayload;
		regex_t* m_arCrimeRegExpsURL;
		UINT32 m_nCrimeRegExpsURL;
		regex_t* m_arCrimeRegExpsPayload;
		UINT32 m_nCrimeRegExpsPayload;
		UINT32 m_nrOfSurveillanceIPs;
		CASocketAddrINet* m_surveillanceIPs;
#endif

#ifdef DATA_RETENTION_LOG
		UINT8*				m_strDataRetentionLogDir;
		CAASymCipher* m_pDataRetentionPublicEncryptionKey;
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
		UINT32 m_iPrepaidInterval;
		UINT32 m_iPaymentSettleInterval;


#endif
		optionSetter_pt *mainOptionSetters;
		optionSetter_pt *generalOptionSetters;
		optionSetter_pt *certificateOptionSetters;
		optionSetter_pt *accountingOptionSetters;
		optionSetter_pt *networkOptionSetters;
		optionSetter_pt *termsAndConditionsOptionSetters;
		optionSetter_pt *crimeDetectionOptionSetters;

#ifdef SERVER_MONITORING
	private:
		char *m_strMonitoringListenerHost;
		UINT16 m_iMonitoringListenerPort;
#endif

	private:
		SINT32 setNewValues(CACmdLnOptions& newOptions);
#ifndef ONLY_LOCAL_PROXY
		SINT32 readXmlConfiguration(XERCES_CPP_NAMESPACE::DOMDocument* & docConfig,const UINT8* const configFileName);
		SINT32 readXmlConfiguration(XERCES_CPP_NAMESPACE::DOMDocument* & docConfig,const UINT8* const buf, UINT32 len);
		SINT32 processXmlConfiguration(XERCES_CPP_NAMESPACE::DOMDocument* docConfig);
		SINT32 clearVisibleAddresses();
		SINT32 addVisibleAddresses(DOMNode* nodeProxy);
#ifdef COUNTRY_STATS
		char* m_dbCountryStatsHost;
		char* m_dbCountryStatsUser;
		char* m_dbCountryStatsPasswd;
#endif //COUNTRY_STATS
#endif //ONLY_LOCAL_PROXY
		SINT32 clearTargetInterfaces();
		SINT32 clearListenerInterfaces();



		/* NR of all Option types, i.e. General, Certificates, Networking, etc. (excluding *mainOptionSetters)
		 * these options are all direct children of <MixConfiguration>*/
#define MAIN_OPTION_SETTERS_NR 8
		SINT32 setGeneralOptions(DOMElement* elemRoot);
		SINT32 setMixDescription(DOMElement* elemRoot); /* mix decription for the mix info */
		SINT32 setCertificateOptions(DOMElement* elemRoot);
		SINT32 setAccountingOptions(DOMElement *elemRoot);
		SINT32 setNetworkOptions(DOMElement *elemRoot);
		SINT32 setRessourceOptions(DOMElement *elemRoot);
		SINT32 setTermsAndConditions(DOMElement *elemRoot);
		SINT32 setCrimeDetectionOptions(DOMElement *elemRoot);

		/* General Options */
#define GENERAL_OPTIONS_NR 11
		SINT32 setMixType(DOMElement* elemGeneral);
		SINT32 setMixName(DOMElement* elemGeneral);
		SINT32 setMixID(DOMElement* elemGeneral);
		SINT32 setDynamicMix(DOMElement* elemGeneral);
		SINT32 setMinCascadeLength(DOMElement* elemGeneral);
		SINT32 setCascadeNameFromOptions(DOMElement* elemGeneral);
		SINT32 setUserID(DOMElement* elemGeneral);
		SINT32 setNrOfFileDescriptors(DOMElement* elemGeneral);
		SINT32 setDaemonMode(DOMElement* elemGeneral);
		SINT32 setMaxUsers(DOMElement* elemGeneral);
		SINT32 setLoggingOptions(DOMElement* elemGeneral);

		/* Certificate Options */
#define CERTIFICATE_OPTIONS_NR 4
		SINT32 setOwnCertificate(DOMElement *elemCertificates);
		SINT32 setOwnOperatorCertificate(DOMElement *elemCertificates);
		SINT32 setNextMixCertificate(DOMElement *elemCertificates);
		SINT32 setPrevMixCertificate(DOMElement *elemCertificates);

		/* Payment Options */
#define ACCOUNTING_OPTIONS_NR 7
		SINT32 setPriceCertificate(DOMElement *elemAccounting);
		SINT32 setPaymentInstance(DOMElement *elemAccounting);
		SINT32 setAccountingSoftLimit(DOMElement *elemAccounting);
		SINT32 setAccountingHardLimit(DOMElement *elemAccounting);
		SINT32 setPrepaidInterval(DOMElement *elemAccounting);
		SINT32 setSettleInterval(DOMElement *elemAccounting);
		SINT32 setAccountingDatabase(DOMElement *elemAccounting);

		/* Network Options */
#define NETWORK_OPTIONS_NR 5
		SINT32 setInfoServices(DOMElement *elemNetwork);
		SINT32 setListenerInterfaces(DOMElement *elemNetwork);
		SINT32 setTargetInterfaces(DOMElement *elemNetwork);
		SINT32 setServerMonitoring(DOMElement *elemNetwork);
		SINT32 setKeepAliveTraffic(DOMElement *elemNetwork);

		/* Terms & Conditions options */
#define TERMS_AND_CONDITIONS_OPTIONS_NR 2
		SINT32 setTermsAndConditionsTemplates(DOMElement *elemTnCs);
		SINT32 setTermsAndConditionsList(DOMElement *elemTnCs);

		/* Crime Logging Options */
#define CRIME_DETECTION_OPTIONS_NR 3
		SINT32 setCrimeURLRegExp(DOMElement *elemCrimeDetection);
		SINT32 setCrimePayloadRegExp(DOMElement *elemCrimeDetection);
		SINT32 setCrimeSurveillanceIP(DOMElement *elemCrimeDetection);

		SINT32 appendMixInfo_internal(DOMNode* a_node, bool with_subtree);
		inline SINT32 addMixIdToMixInfo();

		SINT32 invokeOptionSetters
		(optionSetter_pt *optionsSetters, DOMElement* target, SINT32 optionsSettersLength);

		void initMainOptionSetters();
		void initGeneralOptionSetters();
		void initMixDescriptionSetters();
		void initCertificateOptionSetters();
		void initAccountingOptionSetters();
		void initNetworkOptionSetters();
		void initTermsAndConditionsOptionSetters();
		void initCrimeDetectionOptionSetters();
};

SINT32 setRegExpressions(DOMElement *rootElement, const char* const childElementName,
		regex_t **regExContainer, UINT32* regExNr);

#endif

