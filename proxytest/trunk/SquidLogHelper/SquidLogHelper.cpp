#include "../StdAfx.h"

#ifdef LOG_CRIME
#include "SquidLogHelper.hpp"
#include "../CASocket.hpp"
#include "../CASocketAddrINet.hpp"
#include "../CALastMix.hpp"


CASquidLogHelper::CASquidLogHelper(CALastMix* pLastMix,UINT16 port)
{
	m_pThreadProcessingLoop = NULL;
	m_pLastMix = pLastMix;
}

/*** Receives log lines from syslog over TCP/IP.
Format should be:
  -- the fixed start word: "ANONLOG:" (without the ")
	-- timestamp (seconds since epoch)
	-- SrcIP (from last Mix)
	-- SrcPort (from last Mix)
	-- DstIP (to proxy)
	-- DstPort (to proxy)
	-- SrcIP (from Squid to Destination)
	-- SrcPort (from Squid to Destination)
	-- DstIP (from Squid to Destination)
	-- DstPort (from Squid to Destination)
	-- optional: extra infos

	separated by ','

Squid configuration:
logformat anonlogformat ANONLOG:%ts,%>a,%>p,%>la,%>lp,%<a,%<p,%<la,%<lp

	Example:
Oct  1 10:45:20 anonvpn squid[24915]: ANONLOG:2343435,127.0.0.1,38786,127.0.0.1,3128,23.63.133.254,443,141.76.46.165,59708


*/
SINT32 CASquidLogHelper::processLogLine(UINT8* strLine)
{
	UINT8* pEntry = strLine;
	UINT8 lastMixToProxyConnectionSrcIP[4];
	UINT16 lastMixToProxyConnectionSrcPort;
	UINT8 lastMixToProxyConnectionDstIP[4];
	UINT16 lastMixToProxyConnectionDstPort;

	//find start
	pEntry =(UINT8*)strstr((const char*)pEntry,"ANONLOG:");
	if (pEntry == NULL)
	{//start not found...
		return E_UNKNOWN;
	}

	pEntry += 8; //move start to the beginn of first IP address

	//timestamp - ignored for now
	char* pKomma=(char*)strchr((const char*)pEntry, ',');
	if (pKomma == NULL)
	{
		return E_UNKNOWN;
	}
	pEntry = (UINT8*)(pKomma + 1);
	pKomma = (char*)strchr((const char*)pEntry, ',');
	if (pKomma == NULL)
	{
		return E_UNKNOWN;
	}

	*pKomma = 0;
	SINT32 ret = CASocketAddrINet::getIPForString(pEntry, lastMixToProxyConnectionSrcIP);
	*pKomma = ',';
	if (ret != E_SUCCESS)
		return E_UNKNOWN;
	pEntry =(UINT8*)( pKomma + 1);
	pKomma = (char*)strchr((const char*)pEntry, ',');
	if (pKomma == NULL)
	{
		return E_UNKNOWN;
	}
	*pKomma = 0;
	ret=parseUINT16(pEntry, lastMixToProxyConnectionSrcPort);
	*pKomma = ',';
	if (ret != E_SUCCESS)
		return E_UNKNOWN;

	pEntry = (UINT8*)(pKomma + 1);
	pKomma = (char*)strchr((const char*)pEntry, ',');
	if (pKomma == NULL)
	{
		return E_UNKNOWN;
	}
	*pKomma = 0;
	ret=CASocketAddrINet::getIPForString(pEntry, lastMixToProxyConnectionDstIP);
	*pKomma = ',';
	if (ret != E_SUCCESS)
		return E_UNKNOWN;

	pEntry = (UINT8*)(pKomma + 1);
	pKomma = (char*)strchr((const char*)pEntry, ',');
	if (pKomma != NULL)
	{
		*pKomma = 0;
		ret = parseUINT16(pEntry, lastMixToProxyConnectionDstPort);
		*pKomma = ',';
		pEntry = (UINT8*)(pKomma + 1);
	}
	else
	{
		ret = parseUINT16(pEntry, lastMixToProxyConnectionDstPort);
		pEntry = NULL;
	}
	if (ret != E_SUCCESS)
		return E_UNKNOWN;

	//now we have everything....
	m_pLastMix->externalCrimeNotifier(lastMixToProxyConnectionSrcIP,lastMixToProxyConnectionSrcPort,lastMixToProxyConnectionDstIP,lastMixToProxyConnectionDstPort,strLine);

	return E_SUCCESS;
}

THREAD_RETURN squidloghelper_ProcessingLoop(void* param)
{
	CASquidLogHelper* m_pSquidLogHelper = (CASquidLogHelper*)param;
	CASocket* psocketListener = new CASocket();
	CASocketAddrINet* pAddr = new CASocketAddrINet();
	pAddr->setAddr((const UINT8*)"127.0.0.1", 6789);
	psocketListener->listen(*pAddr);
	delete pAddr;
	int file = open("/tmp/test.log", O_APPEND | O_CREAT | O_WRONLY, S_IRUSR| S_IWUSR);
	const UINT32 BUFF_SIZE = 0xFFFF;
	UINT8* in = new UINT8[BUFF_SIZE];
	for (;;)
	{
		CASocket* psocketClient = new CASocket();
		psocketListener->accept(*psocketClient);
		UINT32 pos = 0;
		UINT32 len = 0;
		for (;;)
		{
next_read:
			SINT32 ret = psocketClient->receive(in+pos, BUFF_SIZE-pos);
			if (ret > 0)
			{//we read the next chunck of input
				len += ret;
				for (UINT32 i = pos; i < len; i++)
				{//search for line end
					if (in[i] == '\n')
					{//line end found --> write line
						if(file>=0)
							myfilewrite(file, in, i);
						in[i] = 0;
						m_pSquidLogHelper->processLogLine(in);

						len = 0;
						pos = 0;
						goto next_read;
					}
				}
				pos += len;
			}
			else //Socket closed
			{
				//write out current read in buffer?
				break;
			}
		}
		delete psocketClient;
	}
	close(file);
	delete psocketListener;
	delete[] in;
    return 0;
}

SINT32 CASquidLogHelper::start()
{
	m_pThreadProcessingLoop = new CAThread((UINT8*)"SquidHelperProcessingLoop");
	m_pThreadProcessingLoop->setMainLoop(squidloghelper_ProcessingLoop);
	m_pThreadProcessingLoop->start(this, true);
	return E_SUCCESS;
}

SINT32 CASquidLogHelper::stop()
{
	return E_SUCCESS;
}

#endif //LOG_CRIME
