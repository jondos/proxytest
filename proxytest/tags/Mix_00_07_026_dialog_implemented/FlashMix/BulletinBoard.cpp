#include "../StdAfx.h"
#include "../CACmdLnOptions.hpp"
#include "../CAMsg.hpp"
#include "elgamal.hpp"
#include "CABNSend.hpp"
#include "FlashMixGlobal.hpp"
#include "CABulletinBoard.hpp"
#ifdef __BORLANDC__
    #pragma argsused
#endif

#include <iostream>

CACmdLnOptions* pglobalOptions;

unsigned int port = 16000;
unsigned int keylength = 1024;
unsigned int mix_n = 3;
unsigned int mix_p = 2;
unsigned int mix_d = 2;
unsigned int msg = 1024;

void readParam(int argc, char* argv[])
{
    for (int i = 0; i < argc; i++) {
        if (strstr(argv[i], "--port=") == argv[i])
            port = atoi(argv[i] + 7);
        else if (strstr(argv[i], "--keylength=") == argv[i])
            keylength = atoi(argv[i] + 12);
        else if (strstr(argv[i], "--mix-d=") == argv[i])
            mix_d = atoi(argv[i] + 8);
        else if (strstr(argv[i], "--mix-p=") == argv[i])
            mix_p = atoi(argv[i] + 8);
        else if (strstr(argv[i], "--mix-n=") == argv[i])
            mix_n = atoi(argv[i] + 8);
        else if (strstr(argv[i], "--msg=") == argv[i])
            msg = atoi(argv[i] + 6);
        else if (strstr(argv[i], "/port=") == argv[i])
            port = atoi(argv[i] + 6);
        else if (strstr(argv[i], "/keylength=") == argv[i])
            keylength = atoi(argv[i] + 11);
        else if (strstr(argv[i], "/mix-d=") == argv[i])
            mix_d = atoi(argv[i] + 7);
        else if (strstr(argv[i], "/mix-p=") == argv[i])
            mix_p = atoi(argv[i] + 7);
        else if (strstr(argv[i], "/mix-n=") == argv[i])
            mix_n = atoi(argv[i] + 7);
        else if (strstr(argv[i], "/msg=") == argv[i])
            msg = atoi(argv[i] + 5);
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

    readParam(argc, argv);
    CABulletinBoard* bb = new CABulletinBoard(port, keylength, mix_n, mix_p, mix_d, msg);

    bb->init();
    bb->start();

    char c;
    std::cin >> c;

    delete bb;

    return 0;
}
