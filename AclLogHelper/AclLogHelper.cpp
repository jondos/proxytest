#include "../StdAfx.h"
#include "../CASocket.hpp"
#include "../CASocketAddrINet.hpp"
#include "../CALibProxytest.hpp"
CASocket* pSocketSquidLogHelper=NULL;
CASocketAddrINet* pAddrSquidLogHelper = NULL;

SINT32 sendToSquidLogHelper(UINT8* strLine)
{
	if (pSocketSquidLogHelper == NULL)
	{
		pSocketSquidLogHelper = new CASocket();
	}
	if (pSocketSquidLogHelper->isClosed())
	{
		pSocketSquidLogHelper->connect(*pAddrSquidLogHelper);
	}
	pSocketSquidLogHelper->sendFully(strLine, strlen((char*)strLine));
	return E_SUCCESS;
}

SINT32 processAclLine(UINT8* strLine)
{
	sendToSquidLogHelper(strLine);
	myfilewrite(1, "OK\n", 3);
	return E_SUCCESS;
}

int main()
{
	CALibProxytest::init();
	const UINT32 BUFF_SIZE = 0xFFFF;
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
							myfilewrite(file, in, i);
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

