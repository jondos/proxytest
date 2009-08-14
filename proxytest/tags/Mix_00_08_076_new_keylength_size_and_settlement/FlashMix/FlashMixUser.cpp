#include "../StdAfx.h"
#include "../CACmdLnOptions.hpp"
#include "../CAMsg.hpp"
#include "elgamal.hpp"
#include "CABNSend.hpp"
#include "FlashMixGlobal.hpp"
#include <iostream>

#ifdef __BORLANDC__
    #pragma argsused
#endif

CACmdLnOptions* pglobalOptions;

void log(int line, SINT32 result, char* des, int msg)
{
    std::cout << "Line: " << line << " Result: " << result << " " << des << " " << msg << std::endl;
}

void coutbn(BIGNUM* bn)
{
    char* c = BN_bn2hex(bn);
    CAMsg::printMsg(LOG_DEBUG, "BN: %s\n", c);
    OPENSSL_free(c);
}

unsigned int port = 65000;
unsigned int msg = 1024;
unsigned int bbPort = 16000;
UINT8* bbIP = NULL;
UINT8* localIP = NULL;
UINT8 localIPArr[4];

void readParam(int argc, char* argv[])
{
    for (int i = 0; i < argc; i++) {
        if (strstr(argv[i], "--port=") == argv[i])
            port = atoi(argv[i] + 7);
        else if (strstr(argv[i], "--bbPort=") == argv[i])
            bbPort = atoi(argv[i] + 9);
        else if (strstr(argv[i], "--msg=") == argv[i])
            msg = atoi(argv[i] + 6);
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
        else if (strstr(argv[i], "/msg=") == argv[i])
            msg = atoi(argv[i] + 5);
        else if (strstr(argv[i], "/bbIP=") == argv[i])
        {
            free((char*)bbIP);
            bbIP = (UINT8*)strdup(argv[i] + 6);
        }
        else if (strstr(argv[i], "/localIP=") == argv[i])
        {
            free((char*)localIP);
            localIP = (UINT8*)strdup(argv[i] + 9);
        }
    }
}

void fillIPArr(UINT8 r_ipArr[4], UINT8* a_cIP)
{
    UINT32 cIPPtrL = 0;
    UINT32 cIPPtrH = 0;

    for (UINT32 i = 0; i < 4; i++)
    {
        // find '.'
        if (i < 3)
        {
            for ( ; a_cIP[cIPPtrH] != '.'; cIPPtrH++) ;

            a_cIP[cIPPtrH] = 0;
            r_ipArr[i] = atoi((char*)(a_cIP + cIPPtrL));
            a_cIP[cIPPtrH] = '.';
            cIPPtrH++;
            cIPPtrL = cIPPtrH;
        }
        else
            r_ipArr[i] = atoi((char*)(a_cIP + cIPPtrL));
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

    bbIP = (UINT8*)strdup("127.0.0.1");
    localIP = (UINT8*)strdup("127.0.0.1");

    readParam(argc, argv);

    fillIPArr(localIPArr, localIP);

    SINT32 result = E_SUCCESS;
    CASocketAddrINet* pAddr = NULL;
    CASocket* pSocket = NULL;
    pSocket->create();
    ELGAMAL* elBBSignKey = NULL;
    ELGAMAL* elGroupKey = NULL;
    UINT32 cnt = 0;

    UINT64 tStart = 0, tEndFirst = 0, tEndLast = 0;

    // connect to BB
    initSocket(result, &pSocket);
    initSocketAddrINet(result, &pAddr);

    if (result == E_SUCCESS)
        result = pAddr->setAddr(bbIP, bbPort);

    if (result == E_SUCCESS)
        result = pSocket->connect(*pAddr, 50, 1);

    sendUINT32(result, pSocket, BB_REQ_GET_SIGN_KEY);
    receivePublicKey(result, &elBBSignKey, pSocket);
    if (pSocket != NULL) pSocket->close();

    if (result == E_SUCCESS)
        result = pSocket->connect(*pAddr, 50, 1);
    sendUINT32(result, pSocket, BB_REQ_GET_GROUP_KEY);
    receiveSignedPublicKey(result, &elGroupKey, elBBSignKey, pSocket);
    if (pSocket != NULL) pSocket->close();

    delete pSocket;
    pSocket = NULL;

    for (UINT32 i = 0; (i < msg) && (result == E_SUCCESS); i++)
    {
        BIGNUM* bnMsg = NULL;
        BIGNUM* bnEncA = NULL;
        BIGNUM* bnEncB = NULL;
        UINT32 uDataRes = USERDATA_UNDEF;
        UINT8* uBN = NULL;
        UINT32 uBNSize = 0;

        initBNRand(result, &bnMsg, 64);

        uBNSize = BN_num_bytes(bnMsg);
        if ((uBN = new UINT8[uBNSize]) == NULL)
            result = E_UNKNOWN;
        else
            if (BN_bn2bin(bnMsg, (unsigned char*)uBN) != uBNSize)
                result = E_UNKNOWN;

        for (UINT32 j = 0; (j < 4) && (result == E_SUCCESS); j++)
            uBN[j] = localIPArr[j];

        ((UINT16*)uBN)[2] = port;

        if (result == E_SUCCESS)
            if (BN_bin2bn((unsigned char*)uBN, uBNSize, bnMsg) == NULL)
                result = E_UNKNOWN;

//        coutbn(bnMsg);

        if (result == E_SUCCESS)
            if (ELGAMAL_encrypt(&bnEncA, &bnEncB, elGroupKey, bnMsg) == 0)
                result = E_UNKNOWN;

        if (initSocket(result, &pSocket) == E_SUCCESS)
            result = pSocket->connect(*pAddr, 50, 1);

        sendUINT32(result, pSocket, BB_REQ_USER_DATA);
        sendBN(result, pSocket, bnEncA);
        sendBN(result, pSocket, bnEncB);

        receiveUINT32(result, uDataRes, pSocket);
        if (i == msg - 1)
            getcurrentTimeMillis(tStart);
        if (uDataRes != USERDATA_SUCCESS)
            result = E_UNKNOWN;

        if (pSocket != NULL) pSocket->close();
        delete pSocket;
        pSocket = NULL;

        delete[] uBN;
        if (bnMsg != NULL) BN_free(bnMsg);
        if (bnEncA != NULL) BN_free(bnEncA);
        if (bnEncB != NULL) BN_free(bnEncB);
    }
    CAMsg::printMsg(LOG_DEBUG, "messages send: %s\n", (result == E_SUCCESS) ? "successfull" : "ERROR");

    if (pSocket != NULL) pSocket->close();
    delete pSocket;
    pSocket = NULL;

    if (initSocket(result, &pSocket) == E_SUCCESS)
        result = pSocket->listen(port);

    for (UINT32 i = 0; (i < msg) && (result == E_SUCCESS); i++)
    {
        BIGNUM* bn = NULL;
        CASocket* pClient = new CASocket();
        UINT8* cBN = NULL;

        if (pClient == NULL)
            result = E_UNKNOWN;

        if (result == E_SUCCESS)
            result = pSocket->accept(*pClient);

        if (receiveBN(result, &bn, pClient) == E_SUCCESS)
        {
            if (i == 0)
                getcurrentTimeMillis(tEndFirst);
            if (i == msg - 1)
                getcurrentTimeMillis(tEndLast);

//            coutbn(bn);
        }

        if (bn != NULL) BN_free(bn);
        if (pClient != NULL) pClient->close();
        delete pClient;
    }

    CAMsg::printMsg(LOG_DEBUG, "first message received after %u milliseconds\n", diff64(tEndFirst, tStart));
    CAMsg::printMsg(LOG_DEBUG, "last  message received after %u milliseconds\n", diff64(tEndLast , tStart));


    ELGAMAL_free(elGroupKey);
    ELGAMAL_free(elBBSignKey);
    if (pSocket != NULL) pSocket->close();
    delete pSocket;
    delete pAddr;

    return result;
}
