#include "../StdAfx.h"
#include "../CASocket.hpp"

int main()
{
	CASocket* psocketListener = new CASocket();
	psocketListener->listen(6789);
	CASocket* psocketClient = new CASocket();
	psocketListener->accept(*psocketClient);

	UINT8* in = new UINT8[0xFFFF];
	int file;
	file=open("test.log", O_APPEND | O_CREAT | O_WRONLY);
	for (;;)
	{
		SINT32 len=psocketClient->receive(in, 0xFFFF);
		if (len > 0)
			myfilewrite(file, in, len);
		else
			break;
	}
	close(file);
    return 0;
}

