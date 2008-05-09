#include "../StdAfx.h"
#include "../CACmdLnOptions.hpp"
#include "CAFlashMix.hpp"

#ifdef __BORLANDC__
    #pragma argsused
#endif

CACmdLnOptions* pglobalOptions;

UINT8* localIP = NULL;
UINT8* bbIP = NULL;
UINT32 bbPort = 16000;
UINT32 port = 6000;

void readParam(int argc, char* argv[])
{
    for (int i = 0; i < argc; i++) {
        if (strstr(argv[i], "--port=") == argv[i])
            port = atoi(argv[i] + 7);
        else if (strstr(argv[i], "--bbPort=") == argv[i])
            bbPort = atoi(argv[i] + 9);
        else if (strstr(argv[i], "--bbIP=") == argv[i])
        {
            free((char*)bbIP);
            bbIP = (UINT8*)strdup(argv[i] + 7);
        }
        else if (strstr(argv[i], "--localIP=") == argv[i])
        {
            free((char*)localIP);
            localIP = (UINT8*)strdup(argv[i] + 10);
        }
        else if (strstr(argv[i], "/port=") == argv[i])
            port = atoi(argv[i] + 6);
        else if (strstr(argv[i], "/bbPort=") == argv[i])
            bbPort = atoi(argv[i] + 8);
        else if (strstr(argv[i], "/bbIP=") == argv[i])
        {
            free((char*)bbIP);
            bbIP = (UINT8*)strdup(argv[i] + 8);
        }
        else if (strstr(argv[i], "/localIP=") == argv[i])
        {
            free((char*)localIP);
            localIP = (UINT8*)strdup(argv[i] + 9);
        }
    }
}


int main( int argc, char * argv[] )
{
    #ifdef _WIN32
        int err = 0;
        WSADATA wsadata;
        err = WSAStartup(0x0202, &wsadata);
    #endif
    CASocketAddrINet::init();
    CAMsg::init();

    localIP = (UINT8*)strdup("127.0.0.1");
    bbIP = (UINT8*)strdup("127.0.0.1");

    readParam(argc, argv);
    CAFlashMIX* fm = new CAFlashMIX(localIP, port, port + 1, port + 2, port + 3, port + 4, bbIP, bbPort);

    free((char*)localIP);
    free((char*)bbIP);

    fm->init();
    fm->start();

    delete fm;

    return 0;
}
