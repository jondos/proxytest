#include "../StdAfx.h"
#include "../CASocket.hpp"
#include "../CASocketAddrINet.hpp"

/*** Receives log lines from syslog over TCP/IP.
Format should be: 
	-- SrcIP (from last Mix)
	-- SrcPort (from last Mix)
	-- DstIP (to proxy)
	-- DstPort (to proxy)
	-- optional: extra infos

	separated by ','
	Example:
	10.10.0.1,34732,127.0.0.1,3128, extra info
*/
int processLogLine(UINT8* strLine)
{
	UINT8* pEntry = strLine;
	UINT8 lastMixToProxyConnectionSrcIP[4];
	UINT16 lastMixToProxyConnectionSrcPort;
	UINT8 lastMixToProxyConnectionDstIP[4];
	UINT16 lastMixToProxyConnectionDstPort;

	char* pKomma=(char*)strchr((const char*)pEntry, ',');
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
	return E_SUCCESS;
}

int squidloghelp_main()
{
	CASocket* psocketListener = new CASocket();
	psocketListener->listen(6789);
	int file = open("test.log", O_APPEND | O_CREAT | O_WRONLY, S_IRUSR| S_IWUSR);
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
						myfilewrite(file, in, i);
						in[i] = 0;
						processLogLine(in);

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

