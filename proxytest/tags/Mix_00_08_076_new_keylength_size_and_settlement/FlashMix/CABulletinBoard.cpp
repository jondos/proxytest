#include "../StdAfx.h"
#include "CABulletinBoard.hpp"
#include <vector>
#include <iostream>

void log(int line, SINT32 result, char* des)
{
    std::cout << "Line: " << line << " Result: " << result << " " << des << std::endl;
}

void coutbn(BIGNUM* bn)
{
    char* c = BN_bn2dec(bn);
    std::cout << c << std::endl;
    OPENSSL_free(c);
}

void coutElGamal(char* a_suffix, ELGAMAL* el)
{
    std::cout << a_suffix << "->p = "; coutbn(el->p);
    std::cout << a_suffix << "->pm1 = "; coutbn(el->pm1);
    std::cout << a_suffix << "->q = "; coutbn(el->q);
    std::cout << a_suffix << "->g = "; coutbn(el->g);
    std::cout << a_suffix << "->x = "; coutbn(el->x);
    std::cout << a_suffix << "->y = "; coutbn(el->y);
}

CABulletinBoard::CABulletinBoard(
                            UINT32 a_uPort,
                            UINT32 a_uKeyLength,
                            UINT32 a_uMIXCnt,
                            UINT32 a_uMIXPermCnt,
                            UINT32 a_uMIXDecryptCnt,
                            UINT32 a_uAnonCnt) : CAMainThread(a_uPort, handleRequest)
{
    m_uKeyLength = a_uKeyLength;
    m_uMIXCnt = a_uMIXCnt;
    m_uMIXPermCnt = a_uMIXPermCnt;
    m_uMIXDecryptCnt = a_uMIXDecryptCnt;
    m_uAnonCnt = a_uAnonCnt;
    m_uAnonListSize = 2 * m_uAnonCnt + 4;

    m_elSignKey = NULL;
    m_elEncKey = NULL;
    m_elGroupKey = NULL;

    // MIX data initialize
    m_vMIXData = NULL;
    initMIXList(&m_vMIXData, m_uMIXCnt);
    m_uUserMsgListPtr = 0;
    m_pMutexMIXData = new CAMutex();

    // init user msg
    m_vBNUserMsg = NULL;
    initBNList(&m_vBNUserMsg, m_uAnonListSize);
    m_pMutexUserMsg = new CAMutex();

    CAMsg::init();
}

CABulletinBoard::~CABulletinBoard()
{
    stop();
    join();

    ELGAMAL_free(m_elSignKey);
    ELGAMAL_free(m_elEncKey);
    ELGAMAL_free(m_elGroupKey);

    clearMIXList(m_pMutexMIXData, m_uMIXCnt, m_vMIXData);
    delete m_pMutexMIXData;

    clearBNList(m_uAnonListSize, m_vBNUserMsg, 1);

    delete m_pMutexUserMsg;
}

SINT32 CABulletinBoard::init()
{
    SINT32 result = E_SUCCESS;

    CAMsg::printMsg(LOG_DEBUG, "key generation: encryption key startet\n");
    if ((m_elSignKey = ELGAMAL_generate_key(m_uKeyLength)) == NULL)
        result = E_UNKNOWN;
    CAMsg::printMsg(LOG_DEBUG, "key generation: encryption key %s\n", (result == E_SUCCESS) ? "successfully finished" : "ERROR");

    CAMsg::printMsg(LOG_DEBUG, "key generation: signature key startet\n");
    if ((m_elEncKey = ELGAMAL_generate_key(m_uKeyLength)) == NULL)
        result = E_UNKNOWN;
    CAMsg::printMsg(LOG_DEBUG, "key generation: signature key %s\n", (result == E_SUCCESS) ? "successfully finished" : "ERROR");

    return result;
}

SINT32 CABulletinBoard::insertToUserList(BIGNUM* a_bnA, BIGNUM* a_bnB)
{
    if ((a_bnA == NULL) || (a_bnB == NULL))
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;

    if (m_uUserMsgListPtr >= m_uAnonListSize - 4)
        result = E_UNKNOWN;
    else
    {
        if ((m_vBNUserMsg[m_uUserMsgListPtr] != NULL) || (m_vBNUserMsg[m_uUserMsgListPtr + 1] != NULL))
            result = E_UNKNOWN;
        else
        {
            m_vBNUserMsg[m_uUserMsgListPtr] = a_bnA;
            m_vBNUserMsg[m_uUserMsgListPtr + 1] = a_bnB;

            m_uUserMsgListPtr += 2;
        }
    }

    return result;
}

SINT32 CABulletinBoard::insertToUserList(SINT32& ar_sResult, BIGNUM* a_bnA, BIGNUM* a_bnB)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    ar_sResult = insertToUserList(a_bnA, a_bnB);

    return ar_sResult;
}

SINT32 CABulletinBoard::insertDummy(BIGNUM** ar_bnList, BIGNUM* a_bnDummyA, BIGNUM* a_bnDummyB)
{
    if ((ar_bnList == NULL) || (a_bnDummyA == NULL) || (a_bnDummyB == NULL))
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;
    SINT32 sIdx = -1;

    if ((sIdx = getFreeIdx(m_uAnonListSize, (void**)ar_bnList)) < 0)
        result = E_UNKNOWN;
    else
    {
        ar_bnList[sIdx] = a_bnDummyA;
        ar_bnList[sIdx + 1] = a_bnDummyB;
    }


    return result;
}

SINT32 CABulletinBoard::insertDummy(SINT32& ar_sResult, BIGNUM** ar_bnList, BIGNUM* a_bnDummyA, BIGNUM* a_bnDummyB)
{
    if (ar_sResult != E_SUCCESS)
       return ar_sResult;

   ar_sResult = insertDummy(ar_bnList, a_bnDummyA, a_bnDummyB);

   return ar_sResult;

}

SINT32 CABulletinBoard::sendSignKey(CABulletinBoard* a_ths, CASocket* a_pClient)
{
    return sendPublicKey(a_pClient, a_ths->m_elSignKey);
}

SINT32 CABulletinBoard::sendEncKey(CABulletinBoard* a_ths, CASocket* a_pClient)
{
    return sendSignedPublicKey(a_ths->m_elSignKey, a_pClient, a_ths->m_elEncKey);
}

SINT32 CABulletinBoard::sendGroupKey(CABulletinBoard* a_ths, CASocket* a_pClient)
{
    return sendSignedPublicKey(a_ths->m_elSignKey, a_pClient, a_ths->m_elGroupKey);
}

THREAD_RETURN CABulletinBoard::handleRequest(void* a_req)
{
    CABulletinBoard* ths = (CABulletinBoard*)((UINT32*)a_req)[0];
    CASocket* pClient = (CASocket*)((UINT32*)a_req)[1];

    UINT32 reqCode = BB_REQ_UNDEF;

    delete[] (UINT32*)a_req;

    if (receiveUINT32(reqCode, pClient) == E_SUCCESS)
    {
        switch (reqCode)
        {
            case BB_REQ_USER_DATA:      receiveUserData(ths, pClient); break;
            case BB_REQ_INIT_MIX:       initMix(ths, pClient); break;
            case BB_REQ_REMOVE_MIX:     removeMix(ths, pClient); break;
            case BB_REQ_GET_SIGN_KEY:   sendSignKey(ths, pClient); break;
            case BB_REQ_GET_ENC_KEY:    sendEncKey(ths, pClient); break;
            case BB_REQ_GET_GROUP_KEY:  sendGroupKey(ths, pClient); break;
        }
    }

    if (pClient != NULL)
        pClient->close();
    delete pClient;

    THREAD_RETURN_SUCCESS;
}

THREAD_RETURN CABulletinBoard::keyShareing(void* a_thisPtr)
{
    SINT32 result = E_SUCCESS;
    CABulletinBoard* ths = (CABulletinBoard*)a_thisPtr;
    CASocket** pSockets = NULL;
    CASocketAddrINet** pAddrs = NULL;
    BIGNUM* bnY = NULL;
    BN_CTX* ctx = NULL;
    UINT32 procRes = PROCESS_UNDEF;

    CAMsg::printMsg(LOG_DEBUG, "key shareing startet\n");

    // init all variables
    ELGAMAL_free(ths->m_elGroupKey);

    if ((ths->m_elGroupKey = ELGAMAL_generate_key(ths->m_uKeyLength)) == NULL)
        result = E_UNKNOWN;

    initSocketList(result, &pSockets, ths->m_uMIXCnt);
    initSocketAddrINetList(result, &pAddrs, ths->m_uMIXCnt);
    initBN(result, &bnY, 1);
    initCTX(result, &ctx);




    // connect to all mixes and send mix data
    for (UINT32 i = 0; (i < ths->m_uMIXCnt) && (result == E_SUCCESS) && (ths->m_vMIXData != NULL); i++)
    {
        if (ths->m_vMIXData[i] == NULL)
        {
            result = E_UNKNOWN;
            continue;
        }

        if (result == E_SUCCESS)
            result = pAddrs[i]->setAddr(ths->m_vMIXData[i]->cIP, ths->m_vMIXData[i]->uMainPort);

        if (result == E_SUCCESS)
        {
            pSockets[i]->create();
            result = pSockets[i]->connect(*(pAddrs[i]), 5000, 1);
        }

        sendUINT32(result, pSockets[i], MIX_REQ_KEY_SHARE);

        // send MIX Data
        for (UINT32 j = 0; (j < ths->m_uMIXCnt) && (result == E_SUCCESS); j++)
        {
            sendPChar(result, pSockets[i], ths->m_vMIXData[j]->cIP);
            sendUINT32(result, pSockets[i], ths->m_vMIXData[j]->uMainPort);
            sendUINT32(result, pSockets[i], ths->m_vMIXData[j]->uPortKeyShare);
            sendUINT32(result, pSockets[i], ths->m_vMIXData[j]->uPortDummy);
            sendUINT32(result, pSockets[i], ths->m_vMIXData[j]->uPortBlind);
            sendUINT32(result, pSockets[i], ths->m_vMIXData[j]->uPortDecrypt);
            sendSignedPublicKey(result, ths->m_elSignKey, pSockets[i], ths->m_vMIXData[j]->elSignKey);
            sendSignedPublicKey(result, ths->m_elSignKey, pSockets[i], ths->m_vMIXData[j]->elEncKey);
        }

        sendSignedPublicKey(result, ths->m_elSignKey, pSockets[i], ths->m_elGroupKey);
    }

    CAMsg::printMsg(LOG_DEBUG, "connect to mixes and data send: %s\n", (result == E_SUCCESS) ? "successfully finished" : "ERROR");


    // wait for g^xi and calc y
    for (UINT32 i = 0; (i < ths->m_uMIXCnt) && (result == E_SUCCESS); i++)
    {
        BIGNUM* bnTmp = NULL;
        receiveSignedBN(result, &bnTmp, ths->m_vMIXData[i]->elSignKey, pSockets[i]);
        if (result == E_SUCCESS)
            if (BN_mod_mul(bnY, bnY, bnTmp, ths->m_elGroupKey->p, ctx) == 0)
                result = E_UNKNOWN;
        if (bnTmp != NULL)
            BN_free(bnTmp);
    }

    CAMsg::printMsg(LOG_DEBUG, "y calc: %s\n", (result == E_SUCCESS) ? "successfully finished" : "ERROR");

    // calc public y
    if (result == E_SUCCESS)
    {
        if (ths->m_elGroupKey->y != NULL)
            BN_free(ths->m_elGroupKey->y);
        ths->m_elGroupKey->y = bnY;
    }
    else
        if (bnY != NULL) BN_free(bnY);


    // send the group y
    for (UINT32 i = 0; (i < ths->m_uMIXCnt) && (result == E_SUCCESS); i++)
        sendSignedBN(result, ths->m_elSignKey, pSockets[i], bnY);


    // wait for MIX okay
    procRes = PROCESS_SUCCESS;
    for (UINT32 i = 0; (i < ths->m_uMIXCnt) && (result == E_SUCCESS) && (procRes == PROCESS_SUCCESS); i++)
        receiveUINT32(result, procRes, pSockets[i]);

    if (procRes != PROCESS_SUCCESS)
        result = E_UNKNOWN;

    CAMsg::printMsg(LOG_DEBUG, "key shareing: %s\n", (result == E_SUCCESS) ? "successfully finished" : "ERROR");


    // bnY is freed in m_elGroupKey
    if (ctx != NULL) BN_CTX_free(ctx);
    clearSocketList(ths->m_uMIXCnt, pSockets);
    clearSocketAddrINetList(ths->m_uMIXCnt, pAddrs);

    THREAD_RETURN_SUCCESS;
}

THREAD_RETURN CABulletinBoard::mixProcess(void* a_thisPtr)
{
    CABulletinBoard* ths = (CABulletinBoard*)a_thisPtr;
    SINT32 result = E_SUCCESS;
    UINT32 procRes = UNBLIND_UNDEF;
    UINT32* uPermList = NULL;
    UINT32* uDecList = NULL;
    CASocket** pSockets = NULL;
    CASocketAddrINet** pAddrs = NULL;
    BIGNUM** vBNUserMsg = NULL;
    BIGNUM** bnDummies = NULL;
    BIGNUM** bnFirstListFirstReenc = NULL;
    BIGNUM** bnSecondListFirstReenc = NULL;
    BIGNUM** bnFirstListSecondReenc = NULL;
    BIGNUM** bnSecondListSecondReenc = NULL;
    BIGNUM** bnCopyFirstList = NULL;

    UINT32 uDummyPos1 = 0xFFFFFFFF;
    UINT32 uDummyPos2 = 0xFFFFFFFF;

    // create copy of userlist (pointer copy only)
    // copy the list (pointer only)
    vBNUserMsg = ths->m_vBNUserMsg;
    // clear user list
    ths->m_vBNUserMsg = NULL;
    initBNList(result, &(ths->m_vBNUserMsg), ths->m_uAnonListSize);
    ths->m_uUserMsgListPtr = 0;
    ths->m_pMutexUserMsg->unlock();

    CAMsg::printMsg(LOG_DEBUG, "mix process startet\n");

    // init all variables
    initSocketList(result, &pSockets, ths->m_uMIXPermCnt);
    initSocketAddrINetList(result, &pAddrs, ths->m_uMIXPermCnt);
    createRndList(result, &uPermList, ths->m_uMIXPermCnt, ths->m_uMIXCnt);
    createRndList(result, &uDecList, ths->m_uMIXDecryptCnt, ths->m_uMIXCnt);
//    BubbleSort<unsigned int>(uDecList, ths->m_uMIXDecryptCnt);
//    for (UINT32 i = 0; i < ths->m_uMIXDecryptCnt; i++)
//        uDecList[i] = i;

/*    for (UINT32 i = 0; (i < ths->m_uMIXPermCnt) && (result == E_SUCCESS); i++)
        CAMsg::printMsg(LOG_DEBUG, "Permutation[%d] = %d\n", i, uPermList[i]);/**/
/*    for (UINT32 i = 0; (i < ths->m_uMIXDecryptCnt) && (result == E_SUCCESS); i++)
        CAMsg::printMsg(LOG_DEBUG, "Permutation[%d] = %d\n", i, uDecList[i]);/**/

    // connect to permutation mixes and send permutation list
    for (UINT32 i = 0; (i < ths->m_uMIXPermCnt) && (result == E_SUCCESS); i++)
    {
        MIXDATA* pMIXData = ths->m_vMIXData[uPermList[i]];
        result = pAddrs[i]->setAddr(pMIXData->cIP, pMIXData->uMainPort);
        if (result == E_SUCCESS)
            result = pSockets[i]->connect((*pAddrs[i]), 5000, 1);

        sendUINT32(result, pSockets[i], MIX_REQ_MIX_PROCESS);   // send request
        sendUINT32(result, pSockets[i], i);                     // send list index

        // send list
        for (UINT32 j = 0; (j < ths->m_uMIXPermCnt) && (result == E_SUCCESS); j++)
            sendUINT32(result, pSockets[i], uPermList[j]);
    }

    CAMsg::printMsg(LOG_DEBUG, "permutation data send: %s\n", (result == E_SUCCESS) ? "successfully finished" : "ERROR");

    // create Dummy
    if (result == E_SUCCESS)
        result = createDummies(&bnDummies, ths, pSockets, uPermList);

    CAMsg::printMsg(LOG_DEBUG, "dummy creation: %s\n", (result == E_SUCCESS) ? "received successfull" : "ERROR on receive");

    // append dummies
    if (result == E_SUCCESS)
    {
        if (ths->insertDummy(result, vBNUserMsg, bnDummies[0], bnDummies[1]) != E_SUCCESS)
        {
            if (bnDummies[0] != NULL) BN_free(bnDummies[0]);
            if (bnDummies[1] != NULL) BN_free(bnDummies[1]);
        }
        if (ths->insertDummy(result, vBNUserMsg, bnDummies[2], bnDummies[3]) != E_SUCCESS)
        {
            if (bnDummies[2] != NULL) BN_free(bnDummies[2]);
            if (bnDummies[3] != NULL) BN_free(bnDummies[3]);
        }
    }

    CAMsg::printMsg(LOG_DEBUG, "dummies: %s to messagelist\n", (result == E_SUCCESS) ? "successfully added" : "ERROR on add");

    // work on input list
    if (result == E_SUCCESS)
        result = blind(&bnFirstListFirstReenc, &bnSecondListFirstReenc,
                       &bnFirstListSecondReenc, &bnSecondListSecondReenc,
                       ths, pSockets, uPermList, vBNUserMsg);

    // send blind ok
    for (UINT32 i = 0; i < ths->m_uMIXPermCnt; i++)
    {
        if (result == E_SUCCESS)
            sendUINT32(result, pSockets[i], BLIND_SUCCESS);
        else
            sendUINT32(pSockets[i], BLIND_ERROR);
    }

    CAMsg::printMsg(LOG_DEBUG, "blinding: %s\n", (result == E_SUCCESS) ? "successfully finished" : "ERROR");

    // send the Lists to the mixes for unblinding
    for (UINT32 i = 0; (i < ths->m_uMIXPermCnt) && (result == E_SUCCESS); i++)
    {
        sendSignedBNArray(result, ths->m_elSignKey, pSockets[i], ths->m_uAnonListSize, vBNUserMsg);
        sendSignedBNArray(result, ths->m_elSignKey, pSockets[i], ths->m_uAnonListSize, bnFirstListFirstReenc);
        sendSignedBNArray(result, ths->m_elSignKey, pSockets[i], ths->m_uAnonListSize, bnSecondListFirstReenc);
        sendSignedBNArray(result, ths->m_elSignKey, pSockets[i], ths->m_uAnonListSize, bnFirstListSecondReenc);
        sendSignedBNArray(result, ths->m_elSignKey, pSockets[i], ths->m_uAnonListSize, bnSecondListSecondReenc);
    }

    CAMsg::printMsg(LOG_DEBUG, "lists send to mixes: %s\n", (result == E_SUCCESS) ? "successfully finished" : "ERROR");

    // wait for the dummy-positions from last mix
    receiveUINT32(result, uDummyPos1, pSockets[ths->m_uMIXPermCnt - 1]);
    receiveUINT32(result, uDummyPos2, pSockets[ths->m_uMIXPermCnt - 1]);

//    CAMsg::printMsg(LOG_DEBUG, "Pos 1: %d\n", uDummyPos1);
//    CAMsg::printMsg(LOG_DEBUG, "Pos 2: %d\n", uDummyPos2);

    if (uDummyPos2 < uDummyPos1)
    {
        UINT32 uTmp = uDummyPos1;
        uDummyPos1 = uDummyPos2;
        uDummyPos2 = uTmp;
    }

    // remove dummies from list
    if (result == E_SUCCESS)
        if ((bnCopyFirstList = new BIGNUM*[ths->m_uAnonListSize - 4]) == NULL)
            result = E_UNKNOWN;
    for (UINT32 i = 0; (i < 2*uDummyPos1) && (result == E_SUCCESS); i += 2)
    {
        bnCopyFirstList[i] = bnFirstListSecondReenc[i];
        bnCopyFirstList[i+1] = bnFirstListSecondReenc[i+1];
    }
    for (UINT32 i = 2*(uDummyPos1 + 1); (i < 2*uDummyPos2) && (result == E_SUCCESS); i += 2)
    {
        bnCopyFirstList[i - 2] = bnFirstListSecondReenc[i];
        bnCopyFirstList[i - 1] = bnFirstListSecondReenc[i + 1];
    }
    for (UINT32 i = 2*(uDummyPos2 + 1); (i < ths->m_uAnonListSize) && (result == E_SUCCESS); i += 2)
    {
        bnCopyFirstList[i - 4] = bnFirstListSecondReenc[i];
        bnCopyFirstList[i - 3] = bnFirstListSecondReenc[i + 1];
    }

    // wait for unblind result
    procRes = UNBLIND_SUCCESS;
    for (UINT32 i = 0; (i < ths->m_uMIXPermCnt) && (result == E_SUCCESS) && (procRes == UNBLIND_SUCCESS); i++)
        receiveUINT32(result, procRes, pSockets[i]);
    if (procRes != UNBLIND_SUCCESS)
        result = E_UNKNOWN;/**/

    CAMsg::printMsg(LOG_DEBUG, "unblinding: %s\n", (result == E_SUCCESS) ? "successfully finished" : "ERROR");

    CAMsg::printMsg(LOG_DEBUG, "mix process finished\n");





    // decrypt
    clearSocketList(ths->m_uMIXPermCnt, pSockets);
    clearSocketAddrINetList(ths->m_uMIXPermCnt, pAddrs);
    pSockets = NULL;
    pAddrs = NULL;

    CAMsg::printMsg(LOG_DEBUG, "decryption started\n");

    initSocketList(result, &pSockets, ths->m_uMIXDecryptCnt);
    initSocketAddrINetList(result, &pAddrs, ths->m_uMIXDecryptCnt);

    // connect to decryption mixes and send decryption list
    for (UINT32 i = 0; (i < ths->m_uMIXDecryptCnt) && (result == E_SUCCESS); i++)
    {
        MIXDATA* pMIXData = ths->m_vMIXData[uDecList[i]];
        result = pAddrs[i]->setAddr(pMIXData->cIP, pMIXData->uMainPort);
        if (result == E_SUCCESS)
            result = pSockets[i]->connect((*pAddrs[i]), 5000, 1);

        sendUINT32(result, pSockets[i], MIX_REQ_DECRYPT);       // send request
        sendUINT32(result, pSockets[i], i);                     // send list index

        // send list
        for (UINT32 j = 0; (j < ths->m_uMIXDecryptCnt) && (result == E_SUCCESS); j++)
            sendUINT32(result, pSockets[i], uDecList[j]);
    }

    CAMsg::printMsg(LOG_DEBUG, "decryption list send to mixes: %s\n", (result == E_SUCCESS) ? "successfully finished" : "ERROR");

    // send the list for decryption to the first mix
    if (result == E_SUCCESS)
        result = sendSignedBNArray(ths->m_elSignKey, pSockets[0], ths->m_uAnonListSize - 4, bnCopyFirstList);

    CAMsg::printMsg(LOG_DEBUG, "inputlist send: %s\n", (result == E_SUCCESS) ? "successfully finished" : "ERROR");

    // wait for result
    procRes = DECRYPT_SUCCESS;
    for (UINT32 i = 0; (i < ths->m_uMIXDecryptCnt) && (result == E_SUCCESS) && (procRes == DECRYPT_SUCCESS); i++)
        receiveUINT32(result, procRes, pSockets[i]);
    if (procRes != DECRYPT_SUCCESS)
        result = E_UNKNOWN;

    CAMsg::printMsg(LOG_DEBUG, "decryption: %s\n", (result == E_SUCCESS) ? "successfully finished" : "ERROR");

    // free all variables
    delete[] bnCopyFirstList;
    delete[] bnDummies;     // the bignum's are freed in vBNUserMsg
    clearBNList(ths->m_uAnonListSize, vBNUserMsg, 1);
    clearBNList(ths->m_uAnonListSize, bnFirstListFirstReenc, 1);
    clearBNList(ths->m_uAnonListSize, bnSecondListFirstReenc, 1);
    clearBNList(ths->m_uAnonListSize, bnFirstListSecondReenc, 1);
    clearBNList(ths->m_uAnonListSize, bnSecondListSecondReenc, 1);
    clearSocketList(ths->m_uMIXDecryptCnt, pSockets);
    clearSocketAddrINetList(ths->m_uMIXDecryptCnt, pAddrs);
    delete[] uDecList;
    delete[] uPermList;

    THREAD_RETURN_SUCCESS;
}

SINT32 CABulletinBoard::initMix(CABulletinBoard* a_ths, CASocket* a_pClient)
{
    if ((a_ths == NULL) || (a_pClient == NULL))
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;
    MIXDATA* pMIXData = NULL;

    // send local data
    sendUINT32(result, a_pClient, a_ths->m_uKeyLength);
    sendUINT32(result, a_pClient, a_ths->m_uMIXCnt);
    sendUINT32(result, a_pClient, a_ths->m_uMIXPermCnt);
    sendUINT32(result, a_pClient, a_ths->m_uMIXDecryptCnt);

    sendPublicKey(result, a_pClient, a_ths->m_elSignKey);
    sendSignedPublicKey(result, a_ths->m_elSignKey, a_pClient, a_ths->m_elEncKey);

    CAMsg::printMsg(LOG_DEBUG, "BulletinBoard data send: %s\n", (result == E_SUCCESS) ? "successfully finished" : "ERROR");

    if ((pMIXData = MIXDATA_new()) == NULL)
        result = E_UNKNOWN;

    receivePChar(result, &(pMIXData->cIP), a_pClient);
    receiveUINT32(result, pMIXData->uMainPort, a_pClient);
    receiveUINT32(result, pMIXData->uPortKeyShare, a_pClient);
    receiveUINT32(result, pMIXData->uPortDummy, a_pClient);
    receiveUINT32(result, pMIXData->uPortBlind, a_pClient);
    receiveUINT32(result, pMIXData->uPortDecrypt, a_pClient);
    receivePublicKey(result, &(pMIXData->elSignKey), a_pClient);
    receiveSignedPublicKey(result, &(pMIXData->elEncKey), pMIXData->elSignKey, a_pClient);

    CAMsg::printMsg(LOG_DEBUG, "mix data receive: %s\n", (result == E_SUCCESS) ? "successfully finished" : "ERROR");

    if (result == E_SUCCESS)
        result = insertToMIXList(a_ths->m_pMutexMIXData, a_ths->m_uMIXCnt, a_ths->m_vMIXData, pMIXData);

    if ((result != E_SUCCESS) && (pMIXData != NULL))
        MIXDATA_free(pMIXData);

    if (result == E_SUCCESS)
        result = sendUINT32(a_pClient, PROCESS_SUCCESS);
    else
        sendUINT32(a_pClient, PROCESS_ERROR);

    if (result == E_SUCCESS)
        CAMsg::printMsg(LOG_DEBUG, "MIX %s:%d successfully added\n", (char*)pMIXData->cIP, pMIXData->uMainPort);
    else
        CAMsg::printMsg(LOG_DEBUG, "MIX %s:%d adding ERROR\n", (char*)pMIXData->cIP, pMIXData->uMainPort);

    // start key shareing
    if (result == E_SUCCESS)
        startKeyShareing(a_ths);

    return result;
}

SINT32 CABulletinBoard::removeMix(CABulletinBoard* a_ths, CASocket* a_pClient)
{
    if ((a_ths == NULL) || (a_pClient == NULL))
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;
    UINT8* uC = NULL;
    UINT32 uMainPort = 0;
    UINT32 uMIXIdx = 0xFFFFFFFF;

    receivePChar(result, &uC, a_pClient);
    receiveUINT32(result, uMainPort, a_pClient);

    if (result == E_SUCCESS)
        if ((uMIXIdx = getMIXIdx(a_ths->m_pMutexMIXData, a_ths->m_uMIXCnt, a_ths->m_vMIXData, uC, uMainPort)) >= a_ths->m_uMIXCnt)
            result = E_UNKNOWN;

    if (result == E_SUCCESS)
        result = deleteFromMIXList(a_ths->m_pMutexMIXData, a_ths->m_uMIXCnt, a_ths->m_vMIXData, uMIXIdx);

    if (result == E_SUCCESS)
        result = sendUINT32(a_pClient, PROCESS_SUCCESS);
    else
        sendUINT32(a_pClient, PROCESS_ERROR);

    if (result == E_SUCCESS)
        CAMsg::printMsg(LOG_DEBUG, "MIX %s:%d successfully removed\n", (char*)uC, uMainPort);
    else
        CAMsg::printMsg(LOG_DEBUG, "MIX %s:%d ERROR on remove\n", (char*)uC, uMainPort);

    delete[] uC;

    return result;
}

SINT32 CABulletinBoard::startKeyShareing(CABulletinBoard* a_ths)
{
    SINT32 result = E_SUCCESS;
    UINT8 start = 1;

    for (UINT32 i = 0; (i < a_ths->m_uMIXCnt) && (start == 1); i++)
        if (a_ths->m_vMIXData[i] == NULL)
            start = 0;

    if (start == 1)
    {
        pthread_t pThread;
        pthread_create(&pThread, NULL, keyShareing, a_ths);
    }

    return result;
}

SINT32 CABulletinBoard::receiveUserData(CABulletinBoard* a_ths, CASocket* a_pClient)
{
    if ((a_ths == NULL) || (a_pClient == NULL))
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;
    BIGNUM* bnEncA = NULL;
    BIGNUM* bnEncB = NULL;
    UINT8 uThread = 0;

    receiveBN(result, &bnEncA, a_pClient);
    receiveBN(result, &bnEncB, a_pClient);

    a_ths->m_pMutexUserMsg->lock();
    if (a_ths->insertToUserList(result, bnEncA, bnEncB) != E_SUCCESS)
    {
        if (bnEncA != NULL) BN_free(bnEncA);
        if (bnEncB != NULL) BN_free(bnEncB);
    }

    if (result == E_SUCCESS)
    {
        if (a_ths->m_uUserMsgListPtr >= a_ths->m_uAnonListSize - 4)
//        if (getFreeIdx(a_ths->m_uAnonListSize - 4, (void**)a_ths->m_vBNUserMsg) == -1)
        {
            // userlist full, start mix process
            pthread_t pThread;
            pthread_create(&pThread, NULL, mixProcess, a_ths);

            uThread = 1;
        }
    }

    if (uThread != 1)
        a_ths->m_pMutexUserMsg->unlock();

    if (result != E_SUCCESS)
        CAMsg::printMsg(LOG_DEBUG, "ERROR on adding user message\n");

    if (result == E_SUCCESS)
        result = sendUINT32(a_pClient, USERDATA_SUCCESS);
    else
        sendUINT32(a_pClient, USERDATA_ERROR);

    return result;
}

SINT32 CABulletinBoard::createDummies(BIGNUM*** r_bnDummyList, CABulletinBoard* a_ths, CASocket** a_pSockets, UINT32* a_uPermList, UINT32 DUMMYCNT)
{
    if ((r_bnDummyList == NULL) || (a_ths == NULL) || (a_pSockets == NULL) || (a_uPermList == NULL))
        return E_UNKNOWN;
    if (*r_bnDummyList != NULL)
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;
    UINT32 uTmp = 0;
    MIXDATA* pMIXData = NULL;

    // init dummy list
    initBNList(result, r_bnDummyList, DUMMYCNT * 2);
    for (UINT32 i = 0; (i < DUMMYCNT * 2) && (result == E_SUCCESS); i++)
        initBNRand(result, &((*r_bnDummyList)[i]), a_ths->m_uKeyLength);
    // send dummies
    sendSignedBNArray(result, a_ths->m_elSignKey, a_pSockets[0], DUMMYCNT * 2, *r_bnDummyList);

    // free dummy list
    clearBNList(DUMMYCNT * 2, *r_bnDummyList, 1);
    *r_bnDummyList = NULL;
    // receive Dummy list
    pMIXData = a_ths->m_vMIXData[a_uPermList[a_ths->m_uMIXPermCnt - 1]];  // last permutation mix
    receiveSignedBNArray(result, uTmp, r_bnDummyList, pMIXData->elSignKey, a_pSockets[a_ths->m_uMIXPermCnt - 1]);

    return result;
}

SINT32 CABulletinBoard::blind(BIGNUM*** r_bnFirstListFirstReenc,
    BIGNUM*** r_bnSecondListFirstReenc, BIGNUM*** r_bnFirstListSecondReenc,
    BIGNUM*** r_bnSecondListSecondReenc, CABulletinBoard* a_ths,
    CASocket** a_pSockets, UINT32* a_uPermList, BIGNUM** a_bnInputList)
{
    if ((r_bnFirstListFirstReenc == NULL) || (r_bnSecondListFirstReenc == NULL) ||
        (r_bnFirstListSecondReenc == NULL) || (r_bnSecondListSecondReenc == NULL) ||
        (a_ths == NULL) || (a_uPermList == NULL) || (a_uPermList == NULL) ||
        (a_bnInputList == NULL))
        return E_UNKNOWN;
    if ((*r_bnFirstListFirstReenc != NULL) || (*r_bnSecondListFirstReenc != NULL) ||
        (*r_bnFirstListSecondReenc != NULL) || (*r_bnSecondListSecondReenc != NULL))
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;
    UINT32 uTmp = 0;
    MIXDATA* pMIXData = NULL;

    // send the first input list
    sendSignedBNArray(result, a_ths->m_elSignKey, a_pSockets[0], a_ths->m_uAnonListSize, a_bnInputList);
    // send the second input list
    sendSignedBNArray(result, a_ths->m_elSignKey, a_pSockets[0], a_ths->m_uAnonListSize, a_bnInputList);

    pMIXData = a_ths->m_vMIXData[a_uPermList[a_ths->m_uMIXPermCnt - 1]];    // last permutation mix
    // receive the first input list
    receiveSignedBNArray(result, uTmp, r_bnFirstListFirstReenc, pMIXData->elSignKey, a_pSockets[a_ths->m_uMIXPermCnt - 1]);

    // receibe the second input list
    receiveSignedBNArray(result, uTmp, r_bnSecondListFirstReenc, pMIXData->elSignKey, a_pSockets[a_ths->m_uMIXPermCnt - 1]);

    // send first list for second reencryption
    sendSignedBNArray(result, a_ths->m_elSignKey, a_pSockets[0], a_ths->m_uAnonListSize, *r_bnFirstListFirstReenc);

    // send second list for second reencryption
    sendSignedBNArray(result, a_ths->m_elSignKey, a_pSockets[0], a_ths->m_uAnonListSize, *r_bnSecondListFirstReenc);

    // receive the first input list
    receiveSignedBNArray(result, uTmp, r_bnFirstListSecondReenc, pMIXData->elSignKey, a_pSockets[a_ths->m_uMIXPermCnt - 1]);

    // receibe the second input list
    receiveSignedBNArray(result, uTmp, r_bnSecondListSecondReenc, pMIXData->elSignKey, a_pSockets[a_ths->m_uMIXPermCnt - 1]);

    return result;
}
