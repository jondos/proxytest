#include "../StdAfx.h"
#include "../CASocket.hpp"
#include "../CASocketAddrINet.hpp"
#include "../CALibProxytest.hpp"
CASocket* pSocketSquidLogHelper=NULL;
CASocketAddrINet* pAddrSquidLogHelper = NULL;

UINT8* pSquidLogHelperBuffer;
const char* STR_LOG_LINE_TEMPLATE = "ANONLOG:";
const UINT32 STR_LOG_LINE_TEMPLATE_LEN = strlen(STR_LOG_LINE_TEMPLATE);

const char* STR_LOG_LINE_TEMPLATE_SUFFIX = ",0.0.0.0,0,0.0.0.0,0\n";
const UINT32 STR_LOG_LINE_TEMPLATE_SUFFIX_LEN = strlen(STR_LOG_LINE_TEMPLATE_SUFFIX)+1;

const UINT32 BUFF_SIZE = 0xFFFF;

SINT32 sendToSquidLogHelper(UINT8* strLine,UINT32 len)
{
	if (pSocketSquidLogHelper == NULL)
	{
		pSocketSquidLogHelper = new CASocket();
	}
	if (pSocketSquidLogHelper->isClosed())
	{
		pSocketSquidLogHelper->connect(*pAddrSquidLogHelper);
	}
	pSocketSquidLogHelper->sendFully(strLine, len);
	return E_SUCCESS;
}

/*** Receives log lines from squid.
Format should be:
	-- timestamp (seconds since epoch)
	-- SrcIP (from last Mix)
	-- SrcPort (from last Mix)
	-- DstIP (to proxy)
	-- DstPort (to proxy)
	
	separated by ' '

Squid configuration:
logformat anonlogformat ANONLOG:%ts,%>a,%>p,%>la,%>lp,%<a,%<p,%<la,%<lp

	Example:
2343435 127.0.0.1 38786 127.0.0.1 3128

This information is then forwarded to the SquidLogHelper according to the expected SquidLogHelper format.
*/

SINT32 processAclLine(UINT8* strLine)
{
	UINT8* pEntry = strLine;
	// copy input to tmp buffer and replace ' ' by ','
	UINT32 k = STR_LOG_LINE_TEMPLATE_LEN;
	for (UINT32 i = 0; i < BUFF_SIZE; i++)
	{
		if (strLine[i] == 0)
			break;
		else if (strLine[i] == '-')
		{
			k --;
			break;
		}
		else if (strLine[i] == ' ')
			pSquidLogHelperBuffer[k++] = ',';
		else
			pSquidLogHelperBuffer[k++]=strLine[i];
	}
	//append default values for missing ones
	memcpy(pSquidLogHelperBuffer+k, STR_LOG_LINE_TEMPLATE_SUFFIX, STR_LOG_LINE_TEMPLATE_SUFFIX_LEN);
	k += STR_LOG_LINE_TEMPLATE_SUFFIX_LEN;
	//send to SquidLogHelper
	sendToSquidLogHelper(pSquidLogHelperBuffer,k);

  //send to squid that this ACL is ok.
	myfilewrite(1, "OK\n", 3);
	return E_SUCCESS;
}

int main()
{
	CALibProxytest::init();
	pSquidLogHelperBuffer = new UINT8[BUFF_SIZE];
	strcpy((char*)pSquidLogHelperBuffer, "ANONLOG:");
	UINT8* in = new UINT8[BUFF_SIZE];
	int file = open("/tmp/acl.log", O_APPEND | O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
	pAddrSquidLogHelper = new CASocketAddrINet();
	pAddrSquidLogHelper->setAddr((UINT8*)"127.0.0.1", 6789);

	for (;;)
	{
		UINT32 pos = 0;
		UINT32 len = 0;
		for (;;)
		{
		next_read:
			SINT32 ret = read(0,in + pos, BUFF_SIZE - pos);
			if (ret > 0)
			{//we read the next chunck of input
				len += ret;
				for (UINT32 i = pos; i < len; i++)
				{//search for line end
					if (in[i] == '\n')
					{//line end found --> write line
						if (file >= 0)
							myfilewrite(file, in, i+1);
						in[i] = 0;
						processAclLine(in);

						len = 0;
						pos = 0;
						goto next_read;
					}
				}
				pos += len;
			}
			else 
			{
				//write out current read in buffer?
				break;
			}
		}
	}
	close(file);
	delete[] in;


	return E_SUCCESS;
}

