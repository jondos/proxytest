#include "../StdAfx.h"
#include "CAFlashMIX.hpp"
//#include <iostream>
//#include <string>
void logBN(char* a_cFormatStr, BIGNUM* a_bn)
{
    char* c = BN_bn2dec(a_bn);
    CAMsg::printMsg(LOG_DEBUG, a_cFormatStr, c);
    OPENSSL_free(c);
}

void logBNArr(char* a_cFormatStr, int a_i, BIGNUM* a_bn)
{
    char* c = BN_bn2dec(a_bn);
    CAMsg::printMsg(LOG_DEBUG, a_cFormatStr, a_i, c);
    OPENSSL_free(c);
}/**/

CAFlashMIX::CAFlashMIX(
                    UINT8* a_uLocalIP,
                    UINT32 a_uPort,
                    UINT32 a_uPortKeyShare,
                    UINT32 a_uPortDummy,
                    UINT32 a_uPortBlind,
                    UINT32 a_uPortBlindOK,
                    UINT8* a_uBBIp,
                    UINT32 a_uBBPort):CAMainThread(a_uPort, handleRequest)
{
    m_uLocalIP = (UINT8*)strdup((char*)a_uLocalIP);
    m_uPortKeyShare = a_uPortKeyShare;
    m_uPortDummy = a_uPortDummy;
    m_uPortBlind = a_uPortBlind;
    m_uPortDecrypt = a_uPortBlindOK;

    m_elBBSignKey = NULL;
    m_elBBEncKey = NULL;
    m_elSignKey = NULL;
    m_elEncKey = NULL;
    m_elGroupKey = NULL;

    if ((m_pAddrBB = new CASocketAddrINet()) != NULL)
        m_pAddrBB->setAddr(a_uBBIp, a_uBBPort);

    m_vMIXData = NULL;
    m_pMutexMIXData = new CAMutex();

    m_elGroupKey = NULL;

    CAMsg::init();
}

CAFlashMIX::~CAFlashMIX()
{
    stop();
    join();

    logout();
    ELGAMAL_free(m_elSignKey);
    ELGAMAL_free(m_elEncKey);
    ELGAMAL_free(m_elGroupKey);
    ELGAMAL_free(m_elBBSignKey);
    ELGAMAL_free(m_elBBEncKey);

    clearMIXList(m_pMutexMIXData, m_uMIXCnt, m_vMIXData);
    delete m_pMutexMIXData;

    delete m_pAddrBB;
    free(m_uLocalIP);
}

SINT32 CAFlashMIX::init()
{
    SINT32 result = E_SUCCESS;
    CASocket* pSocket = NULL;
    UINT32 procRes = PROCESS_UNDEF;

    if (initSocket(result, &pSocket) == E_SUCCESS)
        result = pSocket->connect(*m_pAddrBB, 5000, 1);

    CAMsg::printMsg(LOG_DEBUG, "connection to BulletinBoard: %s\n", (result == E_SUCCESS) ? "successfully finished" : "ERROR");

    sendUINT32(result, pSocket, BB_REQ_INIT_MIX);
    receiveUINT32(result, m_uKeyLength, pSocket);
    receiveUINT32(result, m_uMIXCnt, pSocket);
    receiveUINT32(result, m_uMIXPermCnt, pSocket);
    receiveUINT32(result, m_uMIXDecryptCnt, pSocket);

    receivePublicKey(result, &m_elBBSignKey, pSocket);
    receiveSignedPublicKey(result, &m_elBBEncKey, m_elBBSignKey, pSocket);

    CAMsg::printMsg(LOG_DEBUG, "BulletinBoard data receive: %s\n", (result == E_SUCCESS) ? "successfully finished" : "ERROR");

    if (result == E_SUCCESS)
        if ((m_elSignKey = ELGAMAL_generate_key(m_uKeyLength)) == NULL)
            result = E_UNKNOWN;
    if (result == E_SUCCESS)
        if ((m_elEncKey = ELGAMAL_generate_key(m_uKeyLength)) == NULL)
            result = E_UNKNOWN;

    CAMsg::printMsg(LOG_DEBUG, "key generation: %s\n", (result == E_SUCCESS) ? "successfully finished" : "ERROR");

    sendPChar(result, pSocket, m_uLocalIP);
    sendUINT32(result, pSocket, m_uPort);
    sendUINT32(result, pSocket, m_uPortKeyShare);
    sendUINT32(result, pSocket, m_uPortDummy);
    sendUINT32(result, pSocket, m_uPortBlind);
    sendUINT32(result, pSocket, m_uPortDecrypt);
    sendPublicKey(result, pSocket, m_elSignKey);
    sendSignedPublicKey(result, m_elSignKey, pSocket, m_elEncKey);

    receiveUINT32(result, procRes, pSocket);

    if ((result == E_SUCCESS) && (procRes == PROCESS_SUCCESS))
        CAMsg::printMsg(LOG_DEBUG, "add to BulletinBoard: successfully finished\n");
    else
        CAMsg::printMsg(LOG_DEBUG, "add to BulletinBoard: ERROR\n");

    if (pSocket != NULL)
        pSocket->close();
    delete pSocket;

    return result;
}

SINT32 CAFlashMIX::logout()
{
    SINT32 result = E_SUCCESS;
    CASocket* pSocket = NULL;
    UINT32 procRes = PROCESS_UNDEF;

    if (initSocket(result, &pSocket) == E_SUCCESS)
        result = pSocket->connect(*m_pAddrBB, 5000, 1);

    sendUINT32(result, pSocket, BB_REQ_REMOVE_MIX);
    sendPChar(result, pSocket, m_uLocalIP);
    sendUINT32(result, pSocket, m_uPort);

    receiveUINT32(result, procRes, pSocket);

    if ((result == E_SUCCESS) && (procRes == PROCESS_SUCCESS))
        CAMsg::printMsg(LOG_DEBUG, "remove from BulletinBoard: successfully finished\n");
    else
        CAMsg::printMsg(LOG_DEBUG, "remove from BulletinBoard: ERROR\n");

    if (pSocket != NULL)
        pSocket->close();
    delete pSocket;
    return result;
}

SINT32 CAFlashMIX::receiveMIXData(SINT32& ar_sResult, CASocket* a_pClient)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    if (a_pClient == NULL)
    {
        ar_sResult = E_UNKNOWN;
        return ar_sResult;
    }

    clearMIXList(m_pMutexMIXData, m_uMIXCnt, m_vMIXData);
    m_vMIXData = NULL;
    initMIXList(ar_sResult, &m_vMIXData, m_uMIXCnt);

    for (UINT32 i = 0; (i < m_uMIXCnt) && (ar_sResult == E_SUCCESS); i++)
    {
        MIXDATA* pMIXData = NULL;

        if ((pMIXData = MIXDATA_new()) == NULL)
            ar_sResult = E_UNKNOWN;

        receivePChar(ar_sResult, &(pMIXData->cIP), a_pClient);
        receiveUINT32(ar_sResult, pMIXData->uMainPort, a_pClient);
        receiveUINT32(ar_sResult, pMIXData->uPortKeyShare, a_pClient);
        receiveUINT32(ar_sResult, pMIXData->uPortDummy, a_pClient);
        receiveUINT32(ar_sResult, pMIXData->uPortBlind, a_pClient);
        receiveUINT32(ar_sResult, pMIXData->uPortDecrypt, a_pClient);
        receiveSignedPublicKey(ar_sResult, &(pMIXData->elSignKey), m_elBBSignKey, a_pClient);
        receiveSignedPublicKey(ar_sResult, &(pMIXData->elEncKey), m_elBBSignKey, a_pClient);

        if (ar_sResult == E_SUCCESS)
            ar_sResult = insertToMIXList(m_pMutexMIXData, m_uMIXCnt, m_vMIXData, pMIXData);

        if ((ar_sResult != E_SUCCESS) && (pMIXData != NULL))
            MIXDATA_free(pMIXData);
    }

    return ar_sResult;
}

SINT32 CAFlashMIX::receiveMIXValues(SINT32& ar_sResult, BIGNUM*** r_bnAllF, BIGNUM*** r_bnAllSj, BIGNUM** a_bnF, BIGNUM** a_bnSi)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    if ((r_bnAllF == NULL) || (r_bnAllSj == NULL) || (a_bnF == NULL) || (a_bnSi == NULL))
    {
        ar_sResult = E_UNKNOWN;
        return ar_sResult;
    }

    if ((*r_bnAllF != NULL) || (*r_bnAllSj != NULL))
    {
        ar_sResult = E_UNKNOWN;
        return ar_sResult;
    }

    CASocket* pSocket = NULL;
    UINT32 uSelfIdx = 0xFFFFFFFF;
    UINT32 procRes = PROCESS_UNDEF;

    initBNList(ar_sResult, r_bnAllF, m_uMIXCnt * m_uMIXDecryptCnt);
    initBNList(ar_sResult, r_bnAllSj, m_uMIXCnt);

    if ((uSelfIdx = getMIXIdx(m_pMutexMIXData, m_uMIXCnt, m_vMIXData, m_uLocalIP, m_uPort)) >= m_uMIXCnt)
        ar_sResult = E_UNKNOWN;

    if (initSocket(ar_sResult, &pSocket) == E_SUCCESS)
        ar_sResult = pSocket->listen(m_uPortKeyShare);

    for (UINT32 i = 0; (i < uSelfIdx) && (ar_sResult == E_SUCCESS); i++)
    {
        CASocket* pClient = NULL;

        if ((pClient = new CASocket()) == NULL)
            ar_sResult = E_UNKNOWN;

        if (ar_sResult == E_SUCCESS)
            ar_sResult = pSocket->accept(*pClient);

        // receive all F values
        for (UINT32 j = 0; (j < m_uMIXDecryptCnt) && (ar_sResult == E_SUCCESS); j++)
            receiveSignedBN(ar_sResult, &((*r_bnAllF)[i * m_uMIXDecryptCnt + j]), m_vMIXData[i]->elSignKey, pClient);

        // receive sj
        receiveEncSignedBN(ar_sResult, &((*r_bnAllSj)[i]), m_vMIXData[i]->elSignKey, m_elEncKey, pClient);

        // send all F values
        for (UINT32 j = 0; (j < m_uMIXDecryptCnt) && (ar_sResult == E_SUCCESS); j++)
            sendSignedBN(ar_sResult, m_elSignKey, pClient, a_bnF[j]);

        // send si
        sendEncSignedBN(ar_sResult, m_elSignKey, m_vMIXData[i]->elEncKey, pClient, a_bnSi[i]);

        receiveUINT32(ar_sResult, procRes, pClient);

        if (procRes != PROCESS_SUCCESS)
            ar_sResult = E_UNKNOWN;

        if (pClient != NULL)
            pClient->close();
        delete pClient;
    }

    if (pSocket != NULL)
        pSocket->close();
    delete pSocket;

    return ar_sResult;
}

SINT32 CAFlashMIX::sendMIXValues(SINT32&ar_sResult, BIGNUM*** r_bnAllF, BIGNUM*** r_bnAllSj, BIGNUM** a_bnF, BIGNUM** a_bnSi)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    if ((r_bnAllF == NULL) || (r_bnAllSj == NULL) || (a_bnF == NULL) || (a_bnSi == NULL))
    {
        ar_sResult = E_UNKNOWN;
        return ar_sResult;
    }

    if ((*r_bnAllF == NULL) || (*r_bnAllSj == NULL))
    {
        ar_sResult = E_UNKNOWN;
        return ar_sResult;
    }

    CASocket** pSockets = NULL;
    CASocketAddrINet** pAddrs = NULL;
    UINT32 uSelfIdx = 0xFFFFFFFF;

    // init all variables
    initSocketList(ar_sResult, &pSockets, m_uMIXCnt);
    initSocketAddrINetList(ar_sResult, &pAddrs, m_uMIXCnt);

    if ((uSelfIdx = getMIXIdx(m_pMutexMIXData, m_uMIXCnt, m_vMIXData, m_uLocalIP, m_uPort)) >= m_uMIXCnt)
        ar_sResult = E_UNKNOWN;


    // send and receive Data
    for (UINT32 i = uSelfIdx + 1; (i < m_uMIXCnt) && (ar_sResult == E_SUCCESS); i++)
    {
        // connect to mixes
        ar_sResult = pAddrs[i]->setAddr(m_vMIXData[i]->cIP, m_vMIXData[i]->uPortKeyShare);
        if (ar_sResult == E_SUCCESS)
            ar_sResult = pSockets[i]->connect(*(pAddrs[i]), 5000, 1);

        // send all F values
        for (UINT32 j = 0; (j < m_uMIXDecryptCnt) && (ar_sResult == E_SUCCESS); j++)
            sendSignedBN(ar_sResult, m_elSignKey, pSockets[i], a_bnF[j]);

        // send si value
        sendEncSignedBN(ar_sResult, m_elSignKey, m_vMIXData[i]->elEncKey, pSockets[i], a_bnSi[i]);

        // receive all F values
        for (UINT32 j = 0; (j < m_uMIXDecryptCnt) && (ar_sResult == E_SUCCESS); j++)
            receiveSignedBN(ar_sResult, &((*r_bnAllF)[i * m_uMIXDecryptCnt + j]), m_vMIXData[i]->elSignKey, pSockets[i]);

        // receive sj value
        receiveEncSignedBN(ar_sResult, &((*r_bnAllSj)[i]), m_vMIXData[i]->elSignKey, m_elEncKey, pSockets[i]);
    }

    // send ready
    for (UINT32 i = m_uMIXCnt - 1; (i > uSelfIdx); i--)
    {
        if (ar_sResult == E_SUCCESS)
            ar_sResult = sendUINT32(pSockets[i], PROCESS_SUCCESS);
        else
            sendUINT32(pSockets[i], PROCESS_ERROR);
    }

    // free all Variables
    clearSocketList(m_uMIXCnt, pSockets);
    clearSocketAddrINetList(m_uMIXCnt, pAddrs);

    return ar_sResult;
}

THREAD_RETURN CAFlashMIX::handleRequest(void* a_req)
{
    CAFlashMIX* ths = (CAFlashMIX*)((UINT32*)a_req)[0];
    CASocket* pClient = (CASocket*)((UINT32*)a_req)[1];

    UINT32 reqCode = MIX_REQ_UNDEF;

    delete[] (UINT32*)a_req;

    if (receiveUINT32(reqCode, pClient) == E_SUCCESS)
    {
        switch (reqCode)
        {
            case MIX_REQ_KEY_SHARE:     startKeyShare(ths, pClient); break;
//            case MIX_REQ_DUMMY:         createDummy(ths, pClient); break;
//            case MIX_REQ_BLIND:         blind(ths, pClient); break;
            case MIX_REQ_MIX_PROCESS:   mixProcess(ths, pClient); break;
            case MIX_REQ_DECRYPT:       decrypt(ths, pClient); break;
        }
    }

    if (pClient != NULL)
        pClient->close();
    delete pClient;

    THREAD_RETURN_SUCCESS;
}

SINT32 CAFlashMIX::startKeyShare(CAFlashMIX* a_ths, CASocket* a_pClient)
{
    if ((a_ths == NULL) || (a_pClient == NULL))
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;
    BIGNUM* bnTmp = NULL;
    BIGNUM* bnX = NULL;
    BIGNUM* bnY = NULL;
    BN_CTX* ctx = NULL;
    BIGNUM** f = NULL;      // the polynom
    BIGNUM** vBNF = NULL;   // the F values Fij = g^fij
    BIGNUM** vBNsj = NULL;  // the Values fi(j)
    UINT32 uSelfIdx = 0xFFFFFFFF;
    BIGNUM** vBNAllF = NULL;   // the F values of all mixes
    BIGNUM** vBNAllsj = NULL;  // the values fi(j) of the mixes (MIXCnt)
    UINT32 procRes = PROCESS_UNDEF;

    CAMsg::printMsg(LOG_DEBUG, "key shareing started\n");

    // init all variables
    initBN(result, &bnX);
    initBN(result, &bnTmp);
    initCTX(result, &ctx);

    a_ths->receiveMIXData(result, a_pClient);

    CAMsg::printMsg(LOG_DEBUG, "mix data receive: %s\n", (result == E_SUCCESS) ? "successfully finished" : "ERROR");

    // receive public group key (y is not valid)
    ELGAMAL_free(a_ths->m_elGroupKey);
    a_ths->m_elGroupKey = NULL;
    receiveSignedPublicKey(result, &(a_ths->m_elGroupKey), a_ths->m_elBBSignKey, a_pClient);

    // calc secret
    if (result == E_SUCCESS)
        if (BN_rand(bnX, a_ths->m_uKeyLength - 1, -1, 1) == 0)
            result = E_UNKNOWN;
   while ((BN_cmp(bnX, a_ths->m_elGroupKey->q) >= 0) && (result == E_SUCCESS))
       if (BN_rand(bnX, a_ths->m_uKeyLength - 1, -1, 1) == 0)
            result = E_UNKNOWN;

    if (result == E_SUCCESS)
        if (BN_mod_exp(bnTmp, a_ths->m_elGroupKey->g, bnX, a_ths->m_elGroupKey->p, ctx) == 0)
            result = E_UNKNOWN;

    CAMsg::printMsg(LOG_DEBUG, "g^x calc: %s\n", (result == E_SUCCESS) ? "successfully finished" : "ERROR");

    // send g exp x
    sendSignedBN(result, a_ths->m_elSignKey, a_pClient, bnTmp);

    // receive group y
    if (receiveSignedBN(result, &bnY, a_ths->m_elBBSignKey, a_pClient) == E_SUCCESS)
    {
        if (a_ths->m_elGroupKey->y != NULL)
            BN_free(a_ths->m_elGroupKey->y);
        a_ths->m_elGroupKey->y = bnY;
    }
    else
        if (bnY != NULL) BN_free(bnY);

    CAMsg::printMsg(LOG_DEBUG, "y receive: %s\n", (result == E_SUCCESS) ? "successfully finished" : "ERROR");

    // choose polynom
    if (result == E_SUCCESS)
        if (ELGAMAL_choose_polynom(&f, a_ths->m_uMIXDecryptCnt - 1, a_ths->m_elGroupKey, bnX) == 0)
            result = E_UNKNOWN;

    vBNF = NULL;
    if (result == E_SUCCESS)
        if (ELGAMAL_calcF(&vBNF, a_ths->m_elGroupKey, a_ths->m_uMIXDecryptCnt, f) == 0)
            result = E_UNKNOWN;

    if (result == E_SUCCESS)
        if (ELGAMAL_calc_allfij(&vBNsj, a_ths->m_elGroupKey->q, a_ths->m_uMIXCnt, a_ths->m_uMIXDecryptCnt, f) == 0)
            result = E_UNKNOWN;

    CAMsg::printMsg(LOG_DEBUG, "F and s_{ij} calc: %s\n", (result == E_SUCCESS) ? "successfully finished" : "ERROR");

    // connect to other mixes
    if ((uSelfIdx = getMIXIdx(a_ths->m_pMutexMIXData, a_ths->m_uMIXCnt, a_ths->m_vMIXData, a_ths->m_uLocalIP, a_ths->m_uPort)) >= a_ths->m_uMIXCnt)
        result = E_UNKNOWN;

    // receive all mix values
    a_ths->receiveMIXValues(result, &vBNAllF, &vBNAllsj, vBNF, vBNsj);

    // connect all next mixes
    a_ths->sendMIXValues(result, &vBNAllF, &vBNAllsj, vBNF, vBNsj);

/*    for (UINT32 i = 0; i < a_ths->m_uMIXCnt; i++)
    {
        if (i == uSelfIdx)
            continue;
        logBNArr("all sj = s%d = %s\n", i, vBNAllsj[i]);
    }/**/

    CAMsg::printMsg(LOG_DEBUG, "F and secret s_j send to mixes: %s\n", (result == E_SUCCESS) ? "successfully finished" : "ERROR");

    // test the received values
    for (UINT32 i = 0; (i < a_ths->m_uMIXCnt) && (result == E_SUCCESS); i++)
    {
        if (i == uSelfIdx)
            continue;           // the own values are not in this array

        if (ELGAMAL_testSecret(a_ths->m_elGroupKey, uSelfIdx, vBNAllsj[i], a_ths->m_uMIXDecryptCnt, vBNAllF + i * a_ths->m_uMIXDecryptCnt) == 0)
            result = E_UNKNOWN;
    }

    // test own values
    if (result == E_SUCCESS)
        if (ELGAMAL_testSecret(a_ths->m_elGroupKey, uSelfIdx, vBNsj[uSelfIdx], a_ths->m_uMIXDecryptCnt, vBNF) == 0)
            result = E_UNKNOWN;

    CAMsg::printMsg(LOG_DEBUG, "test secret: %s\n", (result == E_SUCCESS) ? "successfully finished" : "ERROR");

    if (bnX != NULL)
    {
        BN_clear_free(bnX);
        bnX = NULL;
    }

    if (result == E_SUCCESS)
    {
        vBNAllsj[uSelfIdx] = vBNsj[uSelfIdx];
        if (ELGAMAL_calcSi(&bnX, a_ths->m_elGroupKey->q, a_ths->m_uMIXCnt, vBNAllsj) == 0)
            result == E_UNKNOWN;
        // the value is freed on another place
        vBNAllsj[uSelfIdx] = NULL;
    }

    // check secret for 0
    if (result == E_SUCCESS)
        if (BN_is_zero(bnX) == 1)
            result = E_UNKNOWN;

    // save the private key
    if (result == E_SUCCESS)
    {
        if (a_ths->m_elGroupKey->x != NULL) BN_free(a_ths->m_elGroupKey->x);
        if ((a_ths->m_elGroupKey->x = BN_dup(bnX)) == NULL)
            result = E_UNKNOWN;
    }

    CAMsg::printMsg(LOG_DEBUG, "local secret calc: %s\n", (result == E_SUCCESS) ? "successfully finished" : "ERROR");

    // send process state
    if (result == E_SUCCESS)
        result = sendUINT32(a_pClient, PROCESS_SUCCESS);
    else
        sendUINT32(a_pClient, PROCESS_ERROR);

    if (result == E_SUCCESS)
        CAMsg::printMsg(LOG_DEBUG, "key share: successfully finished\n");
    else
        CAMsg::printMsg(LOG_DEBUG, "key share: ERROR\n");

    // free all variables
    clearBNList(a_ths->m_uMIXCnt, vBNAllsj, 1);
    clearBNList(a_ths->m_uMIXCnt * a_ths->m_uMIXDecryptCnt, vBNAllF, 1);
    clearBNList(a_ths->m_uMIXCnt, vBNsj, 1);
    clearBNList(a_ths->m_uMIXDecryptCnt, vBNF, 1);
    clearBNList(a_ths->m_uMIXDecryptCnt, f, 1);
    if (bnTmp != NULL) BN_free(bnTmp);
    if (bnX != NULL) BN_clear_free(bnX);
    if (ctx != NULL) BN_CTX_free(ctx);
    return result;
}

SINT32 CAFlashMIX::createDummies(CAFlashMIX* a_ths, CASocket* a_pClient, CASocket* a_pPrevMIX,
    CASocket* a_pNextMIX, UINT32 a_uSelfIdx, UINT32* a_uPermList)
{
    if ((a_ths == NULL) || (a_pClient == NULL) || (a_uPermList == NULL))
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;
    BIGNUM** bnDummies = NULL;
    BN_CTX* ctx = NULL;
    UINT32 uTmp = 0;

    // init
    initCTX(result, &ctx);

    // receive dummies
    if (a_uSelfIdx == 0)
        // receive from BulletinBoard
        receiveSignedBNArray(result, uTmp, &bnDummies, a_ths->m_elBBSignKey, a_pClient);
    else
        receiveSignedBNArray(result, uTmp, &bnDummies, a_ths->m_vMIXData[a_uPermList[a_uSelfIdx - 1]]->elSignKey, a_pPrevMIX);

    // work on dummies
    for (UINT32 i = 0; (i < 4) && (result == E_SUCCESS); i++)
    {
        BIGNUM* bnTmp = NULL;

        initBNRand(result, &bnTmp, a_ths->m_uKeyLength);
        if (result == E_SUCCESS)
            if (BN_mod_mul(bnDummies[i], bnDummies[i], bnTmp, a_ths->m_elGroupKey->p, ctx) == 0)
                result = E_UNKNOWN;

        if (bnTmp != NULL) BN_clear_free(bnTmp);
    }

    // send dummies
    if (a_uSelfIdx == a_ths->m_uMIXPermCnt - 1)
        // send to BulletinBoard
        sendSignedBNArray(result, a_ths->m_elSignKey, a_pClient, 4, bnDummies);
    else
        sendSignedBNArray(result, a_ths->m_elSignKey, a_pNextMIX, 4, bnDummies);

    if (ctx != NULL) BN_CTX_free(ctx);
    clearBNList(4, bnDummies, 1);

    return result;
}

SINT32 CAFlashMIX::blindReceiveInpList(SINT32& ar_sResult, UINT32& r_uListSize,
    BIGNUM*** r_bnInpList, CAFlashMIX* a_ths, CASocket* a_pBBClient,
    CASocket* a_pPrevMIX, UINT32 a_uSelfIdx, UINT32* a_uPermList)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;
    if ((r_bnInpList == NULL) || (a_ths == NULL) || (a_pBBClient == NULL) ||
        (a_uPermList == NULL))
    {
        ar_sResult = E_UNKNOWN;
        return ar_sResult;
    }
    if (*r_bnInpList != NULL)
    {
        ar_sResult = E_UNKNOWN;
        return ar_sResult;
    }
    if ((a_uSelfIdx > 0) && (a_pPrevMIX == NULL))
    {
        ar_sResult = E_UNKNOWN;
        return ar_sResult;
    }

    if (a_uSelfIdx == 0)
        receiveSignedBNArray(ar_sResult, r_uListSize, r_bnInpList, a_ths->m_elBBSignKey, a_pBBClient);
    else
        receiveSignedBNArray(ar_sResult, r_uListSize, r_bnInpList, a_ths->m_vMIXData[a_uPermList[a_uSelfIdx - 1]]->elSignKey, a_pPrevMIX);

    return ar_sResult;
}

SINT32 CAFlashMIX::blindSendInpList(SINT32& ar_sResult, CAFlashMIX* a_ths,
    CASocket* a_pBBClient, CASocket* a_pNextMIX, UINT32 a_uSelfIdx, UINT32* a_uPermList,
    UINT32 a_uListSize, BIGNUM** a_bnList)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    if ((a_ths == NULL) || (a_pBBClient == NULL) || (a_uPermList == NULL) ||
        (a_bnList == NULL))
    {
        ar_sResult = E_UNKNOWN;
        return ar_sResult;
    }
    if ((a_uSelfIdx < a_ths->m_uMIXPermCnt - 1) && (a_pNextMIX == NULL))
    {
        ar_sResult = E_UNKNOWN;
        return ar_sResult;
    }

    if (a_uSelfIdx == a_ths->m_uMIXPermCnt - 1)
        sendSignedBNArray(ar_sResult, a_ths->m_elSignKey, a_pBBClient, a_uListSize, a_bnList);
    else
        sendSignedBNArray(ar_sResult, a_ths->m_elSignKey, a_pNextMIX, a_uListSize, a_bnList);

    return ar_sResult;
}

SINT32 CAFlashMIX::blindReenc(SINT32& ar_sResult, BIGNUM*** r_bnExp, BIGNUM** ar_bnList,
    ELGAMAL* a_elGroupKey, UINT32 a_uListSize)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    if ((r_bnExp == NULL) || (ELGAMAL_isValidPublicKey(a_elGroupKey) == 0) ||
        (ar_bnList == NULL))
    {
        ar_sResult = E_UNKNOWN;
        return ar_sResult;
    }
    if (*r_bnExp != NULL)
        return ar_sResult;

    // work on the input list
    if (ar_sResult == E_SUCCESS)
        if (ELGAMAL_gen_exp(r_bnExp, a_elGroupKey, a_uListSize >> 1) == 0)
            ar_sResult = E_UNKNOWN;

    if (ar_sResult == E_SUCCESS)
        if (ELGAMAL_reencrypt(ar_bnList, a_elGroupKey, a_uListSize >> 1, *r_bnExp) == 0)
            ar_sResult = E_UNKNOWN;

    return ar_sResult;
}

SINT32 CAFlashMIX::blind(UINT32* r_uInpListsSize, BIGNUM*** r_bnInpList,
    UINT32** r_uInpPermLists, BIGNUM*** r_bnExp,
    CAFlashMIX* a_ths, CASocket* a_pClient, CASocket* a_pPrevMIX,
    CASocket* a_pNextMIX, UINT32 a_uSelfIdx, UINT32* a_uPermList)
{
    if ((a_ths == NULL) || (a_pClient == NULL) || (a_uPermList == NULL) || (r_bnExp == NULL) ||
        (r_uInpPermLists == NULL))
        return E_UNKNOWN;

    UINT32 LISTCNT = 4;

    SINT32 result = E_SUCCESS;

    for (UINT32 i = 0; (i < LISTCNT) && (result == E_SUCCESS); i++)
    {
        r_uInpPermLists[i] = NULL;
        r_bnInpList[i] = NULL;
        r_bnExp[i] = NULL;
        r_uInpListsSize[i] = 0;
    }

    for (UINT32 i = 0; (i < LISTCNT) && (result == E_SUCCESS); i++)
    {
        blindReceiveInpList(result, r_uInpListsSize[i], &(r_bnInpList[i]), a_ths, a_pClient, a_pPrevMIX, a_uSelfIdx, a_uPermList);
        blindReenc(result, &(r_bnExp[i]), r_bnInpList[i], a_ths->m_elGroupKey, r_uInpListsSize[i]);
        createRndList(result, &(r_uInpPermLists[i]), r_uInpListsSize[i] >> 1, r_uInpListsSize[i] >> 1);

/*        for (UINT32 j = 0; j < r_uInpListsSize[i] >> 1; j++)
            cout << " " << r_uInpPermLists[i][j];
        cout << endl;/**/
/*        for (UINT32 j = 0; j < r_uInpListsSize[i] >> 1; j++)
        {
            char* c = BN_bn2dec(r_bnExp[i][j]);
            CAMsg::printMsg(LOG_DEBUG, "used exp: [%d, %d] = %s\n", i, j, c);
            OPENSSL_free(c);
        }/**/

        permutate(result, r_bnInpList[i], r_uInpListsSize[i], r_uInpPermLists[i]);
        blindSendInpList(result, a_ths, a_pClient, a_pNextMIX, a_uSelfIdx, a_uPermList, r_uInpListsSize[i], r_bnInpList[i]);
    }

    return result;
}

SINT32 CAFlashMIX::mixProcess(CAFlashMIX* a_ths, CASocket* a_pClient)
{
    if ((a_ths == NULL) || (a_pClient == NULL))
        return E_UNKNOWN;

    UINT32 MIXCNT = a_ths->m_uMIXPermCnt;
    UINT32 LISTCNT = 2;
    UINT32 DUMMYCNT = 2;
    UINT32 REENCCNT = 2;

    SINT32 result = E_SUCCESS;
    UINT32 uSelfIdx = 0xFFFFFFFF;
    UINT32* uPermList = NULL;
    UINT32 uBlindRes = BLIND_UNDEF;
    UINT32** uInpPerm = NULL;
    UINT32* uInpListsSize = NULL;

    BIGNUM*** bnExp = NULL;
    BIGNUM*** bnInpList = NULL;

    CASocket* pSocketNextMIX = NULL;
    CASocket* pSocketPrevMIX = NULL;
    CASocketAddrINet* pAddr = NULL;

    CAMsg::printMsg(LOG_DEBUG, "mix process started\n");

    // init variables
    if ((uPermList = new UINT32[MIXCNT]) == NULL)
        result = E_UNKNOWN;
    if ((uInpPerm = new UINT32*[LISTCNT * REENCCNT]) == NULL)
        result = E_UNKNOWN;
    else
        for (UINT32 i = 0; i < LISTCNT * REENCCNT; i++)
            uInpPerm[i] = NULL;
    if ((uInpListsSize = new UINT32[LISTCNT * REENCCNT]) == NULL)
        result = E_UNKNOWN;
    if ((bnExp = new BIGNUM**[LISTCNT * REENCCNT]) == NULL)
        result = E_UNKNOWN;
    else
        for (UINT32 i = 0; i < LISTCNT * REENCCNT; i++)
            bnExp[i] = NULL;
    if ((bnInpList = new BIGNUM**[LISTCNT * REENCCNT]) == NULL)
        result = E_UNKNOWN;
    else
        for (UINT32 i = 0; i < LISTCNT * REENCCNT; i++)
            bnInpList[i] = NULL;

    // wait for selfindex and permutation list
    receiveUINT32(result, uSelfIdx, a_pClient);
    for (UINT32 i = 0; (i < a_ths->m_uMIXPermCnt) && (result == E_SUCCESS); i++)
        receiveUINT32(result, uPermList[i], a_pClient);

    CAMsg::printMsg(LOG_DEBUG, "permutation list: %s\n", (result == E_SUCCESS) ? "successfully received" : "ERROR on receive");

    // connect prev and next mix
    if (uSelfIdx > 0)
    {
        if ((pSocketNextMIX = new CASocket()) == NULL)
            result = E_UNKNOWN;
        else
            result = pSocketNextMIX->listen(a_ths->m_uPortBlind);
        if ((pSocketPrevMIX = new CASocket()) == NULL)
            result = E_UNKNOWN;
        if (result == E_SUCCESS)
            result = pSocketNextMIX->accept(*pSocketPrevMIX);

        if (pSocketNextMIX != NULL) pSocketNextMIX->close();
        delete pSocketNextMIX;
        pSocketNextMIX = NULL;
    }
    // connect to next mixes
    if (uSelfIdx < a_ths->m_uMIXPermCnt - 1)
    {
        if (pSocketNextMIX != NULL)
        {
            pSocketNextMIX->close();
            delete pSocketNextMIX;
            pSocketNextMIX = NULL;
        }
        initSocket(result, &pSocketNextMIX);
        MIXDATA* pMIXData = a_ths->m_vMIXData[uPermList[uSelfIdx + 1]];
        if (initSocketAddrINet(result, &pAddr) == E_SUCCESS)
            result = pAddr->setAddr(pMIXData->cIP, pMIXData->uPortBlind);
        if (result == E_SUCCESS)
            result = pSocketNextMIX->connect(*pAddr, 5000, 1);
    }

    CAMsg::printMsg(LOG_DEBUG, "MIX connection: %s\n", (result == E_SUCCESS) ? "successfully finished" : "ERROR");

    // create Dummmy
    if (result == E_SUCCESS)
        result = createDummies(a_ths, a_pClient, pSocketPrevMIX, pSocketNextMIX, uSelfIdx, uPermList);

    CAMsg::printMsg(LOG_DEBUG, "dummy creation: %s\n", (result == E_SUCCESS) ? "successfully finished" : "ERROR");

    // blinding
    if (result == E_SUCCESS)
        result = blind(uInpListsSize, bnInpList, uInpPerm, bnExp, a_ths, a_pClient, pSocketPrevMIX, pSocketNextMIX, uSelfIdx, uPermList);

    // wait for blind success
    receiveUINT32(result, uBlindRes, a_pClient);
    if ((uBlindRes != BLIND_SUCCESS) && (result == E_SUCCESS))
        result = E_UNKNOWN;

    CAMsg::printMsg(LOG_DEBUG, "blind: %s\n", (uBlindRes == BLIND_SUCCESS) ? "successfully finished" : "ERROR");

    if (result == E_SUCCESS)
        result = unblind(a_ths, a_pClient, pSocketPrevMIX, pSocketNextMIX, uSelfIdx, uPermList, uInpListsSize, bnInpList, uInpPerm, bnExp);

    CAMsg::printMsg(LOG_DEBUG, "unblind: %s\n", (result == E_SUCCESS) ? "successfully finished" : "ERROR");

    // send unblind result
    if (result == E_SUCCESS)
        result = sendUINT32(a_pClient, UNBLIND_SUCCESS);
    else
        sendUINT32(a_pClient, UNBLIND_ERROR);

    // free all variablen
    for (UINT32 i = 0; (i < LISTCNT * REENCCNT) && (bnInpList != NULL); i++)
        clearBNList(uInpListsSize[i], bnInpList[i], 1);
    delete[] bnInpList;
    for (UINT32 i = 0; (i < LISTCNT * REENCCNT) && (bnExp != NULL); i++)
        clearBNList(uInpListsSize[i] >> 1, bnExp[i], 1);
    delete[] bnExp;
    for (UINT32 i = 0; (i < LISTCNT * REENCCNT) && (uInpPerm != NULL); i++)
        delete[] uInpPerm[i];
    delete[] uInpPerm;
    delete[] uInpListsSize;
    if (pSocketNextMIX != NULL) pSocketNextMIX->close();
    if (pSocketPrevMIX != NULL) pSocketPrevMIX->close();
    delete pSocketNextMIX;
    delete pSocketPrevMIX;
    delete pAddr;
    delete[] uPermList;

    return result;
}

SINT32 CAFlashMIX::permutate(SINT32& ar_sResult, BIGNUM** ar_bnList,
    UINT32 a_uListSize, UINT32* a_uPerm)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;
    if ((ar_bnList == NULL) || (a_uPerm == NULL))
    {
        ar_sResult E_UNKNOWN;
        return ar_sResult;
    }

    for (UINT32 i = 1; i < (a_uListSize >> 1); i++)
    {
        switchEncListElements(a_uListSize, ar_bnList, a_uPerm[0], a_uPerm[i]);
    }

    return ar_sResult;
}

SINT32 CAFlashMIX::unblind(CAFlashMIX* a_ths, CASocket* a_pBB, CASocket* a_pPrevMIX,
    CASocket* a_pNextMIX, UINT32 a_uSelfIdx, UINT32* a_uPermList,
    UINT32* a_uInpListsSize, BIGNUM*** a_bnInpList, UINT32** a_uInpPerm, BIGNUM*** a_bnExp)
{
    if ((a_ths == NULL) || (a_uPermList == NULL) || (a_uInpListsSize == NULL) || (a_pBB == NULL))
        return E_UNKNOWN;
    if ((a_uSelfIdx > 0) && (a_pPrevMIX == NULL))
        return E_UNKNOWN;
    if ((a_uSelfIdx < a_ths->m_uMIXPermCnt - 1) && (a_pNextMIX == NULL))
        return E_UNKNOWN;

    UINT32 MIXCNT = a_ths->m_uMIXPermCnt;
    UINT32 LISTCNT = 2;
    UINT32 DUMMYCNT = 2;
    UINT32 REENCCNT = 2;
    UINT32 DUMMYLISTSIZE = MIXCNT * LISTCNT * DUMMYCNT * REENCCNT;

    UINT32 MIXFAC = LISTCNT * DUMMYCNT * REENCCNT;
    UINT32 LISTFAC = DUMMYCNT * REENCCNT;
    UINT32 DUMMYFAC = REENCCNT;
    UINT32 REENCFAC = 1;

    SINT32 result = E_SUCCESS;
    UINT32* uDummyPerm = NULL;      // linearisized 3 dim array [M][L][D] = [M*LC*DC + L*DC + D]
    BIGNUM** bnInpList = NULL;
    BIGNUM** bnFirstListFirstReenc = NULL;
    BIGNUM** bnSecondListFirstReenc = NULL;
    BIGNUM** bnFirstListSecondReenc = NULL;
    BIGNUM** bnSecondListSecondReenc = NULL;
    UINT32 uInpListSize = 0;
    BIGNUM** bnDummyExp2ReEnc = NULL;

    // init
    if ((uDummyPerm = new UINT32[DUMMYLISTSIZE]) == NULL)
        result = E_UNKNOWN;

    // receive input, first reenc and second reenc list
    receiveSignedBNArray(result, uInpListSize, &bnInpList, a_ths->m_elBBSignKey, a_pBB);
    receiveSignedBNArray(result, uInpListSize, &bnFirstListFirstReenc, a_ths->m_elBBSignKey, a_pBB);
    receiveSignedBNArray(result, uInpListSize, &bnSecondListFirstReenc, a_ths->m_elBBSignKey, a_pBB);
    receiveSignedBNArray(result, uInpListSize, &bnFirstListSecondReenc, a_ths->m_elBBSignKey, a_pBB);
    receiveSignedBNArray(result, uInpListSize, &bnSecondListSecondReenc, a_ths->m_elBBSignKey, a_pBB);

    CAMsg::printMsg(LOG_DEBUG, "unblind receive lists: %s\n", (result == E_SUCCESS) ? "successfully finished" : "ERROR");

    unblindSendReceiveDummyData(result, uDummyPerm, a_pPrevMIX, a_pNextMIX,
        a_uSelfIdx, MIXCNT, LISTCNT, DUMMYCNT, REENCCNT, a_uInpListsSize, a_uInpPerm, (UINT32)0);
    unblindSendReceiveDummyData(result, uDummyPerm, a_pPrevMIX, a_pNextMIX,
        a_uSelfIdx, MIXCNT, LISTCNT, DUMMYCNT, REENCCNT, a_uInpListsSize, a_uInpPerm, (UINT32)1);

    CAMsg::printMsg(LOG_DEBUG, "unblind receive dummy permutations: %s\n", (result == E_SUCCESS) ? "successfully finished" : "ERROR");

    unblindSendReceiveDummyExp2ReEnc(result, &bnDummyExp2ReEnc, a_pPrevMIX,
        a_pNextMIX, a_uSelfIdx, a_bnExp, uDummyPerm, MIXCNT, LISTCNT, DUMMYCNT, REENCCNT, uInpListSize >> 1);

    CAMsg::printMsg(LOG_DEBUG, "unblind receive dummy exponents: %s\n", (result == E_SUCCESS) ? "successfully finished" : "ERROR");

    unblindCheck2Dummy(result, a_ths, bnFirstListFirstReenc, bnSecondListFirstReenc,
        bnFirstListSecondReenc, bnSecondListSecondReenc, uDummyPerm, bnDummyExp2ReEnc,
        MIXCNT, MIXFAC, LISTFAC, DUMMYFAC, REENCFAC);

    CAMsg::printMsg(LOG_DEBUG, "unblind check second dummy: %s\n", (result == E_SUCCESS) ? "successfully finished" : "ERROR");

    unblindCheckProd(result, a_ths, a_pPrevMIX, a_pNextMIX, a_uSelfIdx,
        a_uInpListsSize, bnFirstListFirstReenc, bnSecondListFirstReenc,
        a_bnInpList, a_bnExp, MIXCNT, LISTCNT, DUMMYCNT, REENCCNT);

    CAMsg::printMsg(LOG_DEBUG, "unblind check products: %s\n", (result == E_SUCCESS) ? "successfully finished" : "ERROR");

    unblindCheckFirstReEnc(result, a_ths, a_pPrevMIX, a_pNextMIX, a_uSelfIdx,
        uInpListSize, bnInpList, bnFirstListFirstReenc, bnSecondListFirstReenc, a_uInpPerm, a_bnExp, MIXCNT);

    CAMsg::printMsg(LOG_DEBUG, "unblind check first reenc: %s\n", (result == E_SUCCESS) ? "successfully finished" : "ERROR");

    // send the dummy-positions to bulletinboard
    // only the last mix sends the positions
    if (a_uSelfIdx == a_ths->m_uMIXPermCnt - 1)
    {
        sendUINT32(result, a_pBB, uDummyPerm[a_uSelfIdx*MIXFAC + 0*LISTFAC + 0*DUMMYFAC + 1*REENCFAC]);
        sendUINT32(result, a_pBB, uDummyPerm[a_uSelfIdx*MIXFAC + 0*LISTFAC + 1*DUMMYFAC + 1*REENCFAC]);
    }

    // free all
    delete[] uDummyPerm;
    clearBNList(uInpListSize, bnInpList, 1);
    clearBNList(uInpListSize, bnFirstListFirstReenc, 1);
    clearBNList(uInpListSize, bnSecondListFirstReenc, 1);
    clearBNList(uInpListSize, bnFirstListSecondReenc, 1);
    clearBNList(uInpListSize, bnSecondListSecondReenc, 1);
    clearBNList(2*MIXCNT, bnDummyExp2ReEnc, 1);

    return result;
}

SINT32 CAFlashMIX::unblindSendReceiveDummyData(SINT32& ar_sResult,
    UINT32* ar_uDummies, CASocket* a_pPrevMIX, CASocket* a_pNextMIX,
    UINT32 a_uSelfIdx, UINT32 MIXCNT, UINT32 LISTCNT, UINT32 DUMMYCNT, UINT32 REENCCNT,
    UINT32* a_uInpPermSize, UINT32** a_uInpPerm, UINT32 a_rReEnc)
{
    if (ar_sResult != E_SUCCESS) return ar_sResult;
    if (ar_uDummies == NULL) { ar_sResult = E_UNKNOWN; return ar_sResult; }
    if ((a_uSelfIdx > 0) && (a_pPrevMIX == NULL)) { ar_sResult = E_UNKNOWN; return ar_sResult; }
    if ((a_uSelfIdx < MIXCNT - 1) && (a_pNextMIX == NULL)) { ar_sResult = E_UNKNOWN; return ar_sResult; }

    UINT32 MIXFAC = LISTCNT * DUMMYCNT * REENCCNT;
    UINT32 LISTFAC = DUMMYCNT * REENCCNT;
    UINT32 DUMMYFAC = REENCCNT;
    UINT32 REENCFAC = 1;
    UINT32 DUMMYLISTSIZE = MIXCNT * LISTCNT * DUMMYCNT * REENCCNT;

    UINT32 uReEnc = 0;
    if (a_rReEnc != 0)
        uReEnc = 1;

    // first MIX init the dummy-list
    if ((uReEnc == 0) && (a_uSelfIdx == 0))
        for (UINT32 i = 0; i < DUMMYLISTSIZE; i++)
            ar_uDummies[i] = 0xFFFFFFFF;

    // receive dummy data from prev mix
    if (a_uSelfIdx > 0)
        for (UINT32 i = 0; (i < DUMMYLISTSIZE) && (ar_sResult == E_SUCCESS); i++)
            ar_sResult = receiveUINT32(ar_uDummies[i], a_pPrevMIX);

    // add own perm data
    if (a_uSelfIdx == 0)
    {
        // list loop
        for (UINT32 l = 0; (l < LISTCNT) && (ar_sResult == E_SUCCESS); l++)
            // dummy loop
            for (UINT32 d = 0; (d < DUMMYCNT) && (ar_sResult == E_SUCCESS); d++)
                getNextPermElement(ar_sResult,
                    ar_uDummies[a_uSelfIdx*MIXFAC + l*LISTFAC + d*DUMMYFAC + uReEnc*REENCFAC],
                    a_uInpPermSize[l] >> 1,
                    a_uInpPerm[l + uReEnc * 2],
                    (1 - uReEnc) * ((a_uInpPermSize[l] >> 1) - 2 + d) +
                         uReEnc  * (ar_uDummies[(MIXCNT - 1)*MIXFAC + l*LISTFAC + d*DUMMYFAC + (1 - uReEnc)*REENCFAC]));
    }
    else
    {
        // list loop
        for (UINT32 l = 0; (l < LISTCNT) && (ar_sResult == E_SUCCESS); l++)
            // dummy loop
            for (UINT32 d = 0; (d < DUMMYCNT) && (ar_sResult == E_SUCCESS); d++)
                getNextPermElement(ar_sResult,
                            ar_uDummies[a_uSelfIdx*MIXFAC + l*LISTFAC + d*DUMMYFAC + uReEnc*REENCFAC],
                            a_uInpPermSize[l] >> 1,
                            a_uInpPerm[l + uReEnc * 2],
                            ar_uDummies[(a_uSelfIdx - 1)*MIXFAC + l*LISTFAC + d*DUMMYFAC + uReEnc*REENCFAC]);
    }

    if (a_uSelfIdx < MIXCNT - 1)
    {
        // send dummy-list to next mix
        for (UINT32 i = 0; (i < DUMMYLISTSIZE) && (ar_sResult == E_SUCCESS); i++)
            ar_sResult = sendUINT32(a_pNextMIX, ar_uDummies[i]);

        // receive dummy-list from next mix
        for (UINT32 i = 0; (i < DUMMYLISTSIZE) && (ar_sResult == E_SUCCESS); i++)
            ar_sResult = receiveUINT32(ar_uDummies[i], a_pNextMIX);
    }

    // send dummy-list to prev mix
    if (a_uSelfIdx > 0)
        for (UINT32 i = 0; (i < DUMMYLISTSIZE) && (ar_sResult == E_SUCCESS); i++)
            ar_sResult = sendUINT32(a_pPrevMIX, ar_uDummies[i]);

    return ar_sResult;
}

SINT32 CAFlashMIX::unblindSendReceiveDummyExp2ReEnc(SINT32& ar_sResult,
    BIGNUM*** r_bnExp2ReEnc, CASocket* a_pPrevMIX, CASocket* a_pNextMIX, UINT32 a_uSelfIdx,
    BIGNUM*** a_bnExp, UINT32* a_uDummyPerm, UINT32 MIXCNT, UINT32 LISTCNT,
    UINT32 DUMMYCNT, UINT32 REENCCNT, UINT32 explistsize)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    if (*r_bnExp2ReEnc != NULL)
    {
        ar_sResult = E_UNKNOWN;
        return ar_sResult;
    }

    UINT32 MIXFAC = LISTCNT * DUMMYCNT * REENCCNT;
    UINT32 LISTFAC = DUMMYCNT * REENCCNT;
    UINT32 DUMMYFAC = REENCCNT;
    UINT32 REENCFAC = 1;

    if ((*r_bnExp2ReEnc = new BIGNUM*[2*MIXCNT]) == NULL)
        ar_sResult = E_UNKNOWN;
    else
        for (UINT32 i = 0; i < 2*MIXCNT; i++)
            (*r_bnExp2ReEnc)[i] = NULL;

    // receive exp from prev mixes
    if (a_uSelfIdx > 0)
        for (UINT32 i = 0; (i < a_uSelfIdx) && (ar_sResult == E_SUCCESS); i++)
        {
            receiveBN(ar_sResult, &((*r_bnExp2ReEnc)[2*i]), a_pPrevMIX);
            receiveBN(ar_sResult, &((*r_bnExp2ReEnc)[2*i + 1]), a_pPrevMIX);
        }

    // add own exp
    if (a_uSelfIdx == 0)
    {
        (*r_bnExp2ReEnc)[2*a_uSelfIdx] = BN_dup(a_bnExp[2][a_uDummyPerm[(MIXCNT - 1)*MIXFAC + 0*LISTFAC + 1*DUMMYFAC + 0*REENCFAC]]);
        (*r_bnExp2ReEnc)[2*a_uSelfIdx + 1] = BN_dup(a_bnExp[3][a_uDummyPerm[(MIXCNT - 1)*MIXFAC + 1*LISTFAC + 1*DUMMYFAC + 0*REENCFAC]]);
    }
    else
    {
        (*r_bnExp2ReEnc)[2*a_uSelfIdx] = BN_dup(a_bnExp[2][a_uDummyPerm[(a_uSelfIdx - 1)*MIXFAC + 0*LISTFAC + 1*DUMMYFAC + 1*REENCFAC]]);
        (*r_bnExp2ReEnc)[2*a_uSelfIdx + 1] = BN_dup(a_bnExp[3][a_uDummyPerm[(a_uSelfIdx - 1)*MIXFAC + 1*LISTFAC + 1*DUMMYFAC + 1*REENCFAC]]);
    }

    // send data to next mix
    if (a_uSelfIdx < MIXCNT - 1)
    {
        for (UINT32 i = 0; (i <= a_uSelfIdx) && (ar_sResult == E_SUCCESS); i++)
        {
            sendBN(ar_sResult, a_pNextMIX, (*r_bnExp2ReEnc)[2*i]);
            sendBN(ar_sResult, a_pNextMIX, (*r_bnExp2ReEnc)[2*i + 1]);
        }

        for (UINT32 i = a_uSelfIdx + 1; (i < MIXCNT) && (ar_sResult == E_SUCCESS); i++)
        {
            receiveBN(ar_sResult, &((*r_bnExp2ReEnc)[2*i]), a_pNextMIX);
            receiveBN(ar_sResult, &((*r_bnExp2ReEnc)[2*i + 1]), a_pNextMIX);
        }
    }

    // send data to prev mix
    if (a_uSelfIdx > 0)
        for (UINT32 i = a_uSelfIdx; (i < MIXCNT) && (ar_sResult == E_SUCCESS); i++)
        {
            sendBN(ar_sResult, a_pPrevMIX, (*r_bnExp2ReEnc)[2*i]);
            sendBN(ar_sResult, a_pPrevMIX, (*r_bnExp2ReEnc)[2*i + 1]);
        }

    return ar_sResult;
}

SINT32 CAFlashMIX::switchEncListElements(UINT32 a_uListSize, BIGNUM** ar_bnList,
    UINT32 a_uMsgIdx1, UINT32 a_uMsgIdx2)
{
    if ((a_uMsgIdx1 >= a_uListSize >> 1) || (a_uMsgIdx2 >= a_uListSize))
        return E_UNKNOWN;
    if (ar_bnList == NULL)
        return E_UNKNOWN;

    BIGNUM* bnTmp = ar_bnList[2 * a_uMsgIdx1];
    ar_bnList[2 * a_uMsgIdx1] = ar_bnList[2 * a_uMsgIdx2];
    ar_bnList[2 * a_uMsgIdx2] = bnTmp;

    bnTmp = ar_bnList[2 * a_uMsgIdx1 + 1];
    ar_bnList[2 * a_uMsgIdx1 + 1] = ar_bnList[2 * a_uMsgIdx2 + 1];
    ar_bnList[2 * a_uMsgIdx2 + 1] = bnTmp;

    return E_SUCCESS;
}

SINT32 CAFlashMIX::unblindCheck2Dummy(SINT32& ar_sResult, CAFlashMIX* a_ths,
    BIGNUM** a_bnL0R0, BIGNUM** a_bnL1R0, BIGNUM** a_bnL0R1, BIGNUM** a_bnL1R1,
    UINT32* a_uDummyPerm, BIGNUM** a_bnDummyExp,
    UINT32 MIXCNT, UINT32 MIXFAC, UINT32 LISTFAC, UINT32 DUMMYFAC, UINT32 REENCFAC)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;
    if ((a_bnL0R0 == NULL) || (a_bnL1R0 == NULL) || (a_bnL0R1 == NULL) || (a_bnL1R1 == NULL) ||
        (a_uDummyPerm == NULL) || (a_bnDummyExp == NULL))
    {
        ar_sResult = E_UNKNOWN;
        return ar_sResult;
    }

    // list 0
    BIGNUM* bnA = BN_dup(a_bnL0R0[2 * a_uDummyPerm[(MIXCNT - 1)*MIXFAC + 0*LISTFAC + 1*DUMMYFAC + 0*REENCFAC]]);
    BIGNUM* bnB = BN_dup(a_bnL0R0[2 * a_uDummyPerm[(MIXCNT - 1)*MIXFAC + 0*LISTFAC + 1*DUMMYFAC + 0*REENCFAC] + 1]);

    if ((bnA == NULL) || (bnB == NULL))
        ar_sResult = E_UNKNOWN;

    for (UINT32 i = 0; (i < MIXCNT) && (ar_sResult == E_SUCCESS); i++)
        if (ELGAMAL_reencrypt(bnA, bnB, a_ths->m_elGroupKey, a_bnDummyExp[2*i]) == 0)
            ar_sResult = E_UNKNOWN;

    if (ar_sResult == E_SUCCESS)
        if (BN_cmp(bnA, a_bnL0R1[2 * a_uDummyPerm[(MIXCNT-1)*MIXFAC + 0*LISTFAC + 1*DUMMYFAC + 1*REENCFAC]]) != 0)
            ar_sResult = E_UNKNOWN;
    if (ar_sResult == E_SUCCESS)
        if (BN_cmp(bnB, a_bnL0R1[2 * a_uDummyPerm[(MIXCNT-1)*MIXFAC + 0*LISTFAC + 1*DUMMYFAC + 1*REENCFAC] + 1]) != 0)
            ar_sResult = E_UNKNOWN;

    BN_free(bnB); bnB = NULL;
    BN_free(bnA); bnA = NULL;

    // list 1
    bnA = BN_dup(a_bnL1R0[2 * a_uDummyPerm[(MIXCNT - 1)*MIXFAC + 1*LISTFAC + 1*DUMMYFAC + 0*REENCFAC]]);
    bnB = BN_dup(a_bnL1R0[2 * a_uDummyPerm[(MIXCNT - 1)*MIXFAC + 1*LISTFAC + 1*DUMMYFAC + 0*REENCFAC] + 1]);

    if ((bnA == NULL) || (bnB == NULL))
        ar_sResult = E_UNKNOWN;

    for (UINT32 i = 0; (i < MIXCNT) && (ar_sResult == E_SUCCESS); i++)
        if (ELGAMAL_reencrypt(bnA, bnB, a_ths->m_elGroupKey, a_bnDummyExp[2*i + 1]) == 0)
            ar_sResult = E_UNKNOWN;

    if (ar_sResult == E_SUCCESS)
        if (BN_cmp(bnA, a_bnL1R1[2 * a_uDummyPerm[(MIXCNT-1)*MIXFAC + 1*LISTFAC + 1*DUMMYFAC + 1*REENCFAC]]) != 0)
            ar_sResult = E_UNKNOWN;
    if (ar_sResult == E_SUCCESS)
        if (BN_cmp(bnB, a_bnL1R1[2 * a_uDummyPerm[(MIXCNT-1)*MIXFAC + 1*LISTFAC + 1*DUMMYFAC + 1*REENCFAC] + 1]) != 0)
            ar_sResult = E_UNKNOWN;

    BN_free(bnB); bnB = NULL;
    BN_free(bnA); bnA = NULL;

    return ar_sResult;
}

SINT32 CAFlashMIX::unblindCheckProd(SINT32& ar_sResult, CAFlashMIX* a_ths,
    CASocket* a_pPrevMIX, CASocket* a_pNextMIX, UINT32 a_uSelfIdx,
    UINT32* a_uInpListsSize, BIGNUM** a_bnInpFirstReEncList, BIGNUM** a_bnInpSecondReEncList,
    BIGNUM*** a_bnOutLists, BIGNUM*** a_bnExp, UINT32 MIXCNT, UINT32 LISTCNT,
    UINT32 DUMMYCNT, UINT32 REENCCNT)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;
    if ((a_uInpListsSize == NULL) || (a_bnOutLists == NULL) || (a_bnExp == NULL) ||
        (a_bnInpFirstReEncList == NULL) || (a_bnInpSecondReEncList == NULL))
    {
        ar_sResult = E_UNKNOWN;
        return ar_sResult;
    }

    BIGNUM** bnProdList = NULL;
    BIGNUM** bnExpList = NULL;
    BIGNUM* bnProdInpA0 = NULL;
    BIGNUM* bnProdInpB0 = NULL;
    BIGNUM* bnProdInpA1 = NULL;
    BIGNUM* bnProdInpB1 = NULL;
    BN_CTX* ctx = NULL;

    initBNList(ar_sResult, &bnProdList, 4*MIXCNT);
    initBNList(ar_sResult, &bnExpList, 2*MIXCNT);

    if ((ctx = BN_CTX_new()) == NULL)
        ar_sResult = E_UNKNOWN;
    else
        BN_CTX_init(ctx);

    // receive data of prev mixes
    for (UINT32 i = 0; (i < a_uSelfIdx) && (ar_sResult == E_SUCCESS); i++)
    {
        for (UINT32 j = 0; (j < 4) && (ar_sResult == E_SUCCESS); j++)
            receiveBN(ar_sResult, &(bnProdList[4 * i + j]), a_pPrevMIX);
        for (UINT32 j = 0; (j < 2) && (ar_sResult == E_SUCCESS); j++)
            receiveBN(ar_sResult, &(bnExpList[2 * i + j]), a_pPrevMIX);
    }

    // add own data
    if (ar_sResult == E_SUCCESS)
    {
        // data of outputlist
        BIGNUM* bnProdA0 = BN_new();
        BIGNUM* bnProdB0 = BN_new();
        BIGNUM* bnProdA1 = BN_new();
        BIGNUM* bnProdB1 = BN_new();
        BIGNUM* bnSumExp0 = BN_new();
        BIGNUM* bnSumExp1 = BN_new();

        if ((bnProdA0 == NULL) || (bnProdB0 == NULL) || (bnSumExp0 == NULL) ||
            (bnProdA1 == NULL) || (bnProdB1 == NULL) || (bnSumExp1 == NULL))
            ar_sResult = E_UNKNOWN;
        else
        {
            BN_init(bnProdA0); BN_one(bnProdA0);
            BN_init(bnProdB0); BN_one(bnProdB0);
            BN_init(bnSumExp0); BN_zero(bnSumExp0);
            BN_init(bnProdA1); BN_one(bnProdA1);
            BN_init(bnProdB1); BN_one(bnProdB1);
            BN_init(bnSumExp1); BN_zero(bnSumExp1);
        }

        for (UINT32 i = 0; (i < a_uInpListsSize[0]) && (ar_sResult == E_SUCCESS); i += 2)
        {
            if (BN_mod_mul(bnProdA0, bnProdA0, a_bnOutLists[2][i + 0], a_ths->m_elGroupKey->p, ctx) == 0) ar_sResult = E_UNKNOWN;
            if (BN_mod_mul(bnProdB0, bnProdB0, a_bnOutLists[2][i + 1], a_ths->m_elGroupKey->p, ctx) == 0) ar_sResult = E_UNKNOWN;
            if (BN_mod_mul(bnProdA1, bnProdA1, a_bnOutLists[3][i + 0], a_ths->m_elGroupKey->p, ctx) == 0) ar_sResult = E_UNKNOWN;
            if (BN_mod_mul(bnProdB1, bnProdB1, a_bnOutLists[3][i + 1], a_ths->m_elGroupKey->p, ctx) == 0) ar_sResult = E_UNKNOWN;
        }

        for (UINT32 i = 0; (i < a_uInpListsSize[0] >> 1) && (ar_sResult == E_SUCCESS); i++)
        {
            if (BN_mod_add(bnSumExp0, bnSumExp0, a_bnExp[2][i], a_ths->m_elGroupKey->q, ctx) == 0) ar_sResult == E_UNKNOWN;
            if (BN_mod_add(bnSumExp1, bnSumExp1, a_bnExp[3][i], a_ths->m_elGroupKey->q, ctx) == 0) ar_sResult == E_UNKNOWN;
        }

        if (ar_sResult == E_SUCCESS)
        {
            bnProdList[4*a_uSelfIdx + 0] = bnProdA0;
            bnProdList[4*a_uSelfIdx + 1] = bnProdB0;
            bnProdList[4*a_uSelfIdx + 2] = bnProdA1;
            bnProdList[4*a_uSelfIdx + 3] = bnProdB1;
            bnExpList[2*a_uSelfIdx + 0]  = bnSumExp0;
            bnExpList[2*a_uSelfIdx + 1]  = bnSumExp1;
        }
        else
        {
            if (bnProdA0 != NULL) BN_free(bnProdA0);
            if (bnProdB0 != NULL) BN_free(bnProdB0);
            if (bnProdA1 != NULL) BN_free(bnProdA1);
            if (bnProdB1 != NULL) BN_free(bnProdB1);
            if (bnSumExp0 != NULL) BN_free(bnSumExp0);
            if (bnSumExp1 != NULL) BN_free(bnSumExp1);
        }
    }

    if (a_uSelfIdx < MIXCNT - 1)
    {
        // send data to next mix
        for (UINT32 i = 0; (i <= a_uSelfIdx) && (ar_sResult == E_SUCCESS); i++)
        {
            for (UINT32 j = 0; (j < 4) && (ar_sResult == E_SUCCESS); j++)
                sendBN(ar_sResult, a_pNextMIX, bnProdList[4 * i + j]);
            for (UINT32 j = 0; (j < 2) && (ar_sResult == E_SUCCESS); j++)
                sendBN(ar_sResult, a_pNextMIX, bnExpList[2 * i + j]);
        }

        // receive data from next mix
        for (UINT32 i = a_uSelfIdx + 1; (i < MIXCNT) && (ar_sResult == E_SUCCESS); i++)
        {
            for (UINT32 j = 0; (j < 4) && (ar_sResult == E_SUCCESS); j++)
                receiveBN(ar_sResult, &(bnProdList[4 * i + j]), a_pNextMIX);
            for (UINT32 j = 0; (j < 2) && (ar_sResult == E_SUCCESS); j++)
                receiveBN(ar_sResult, &(bnExpList[2 * i + j]), a_pNextMIX);
        }
    }

    // send data to prev mix
    if (a_uSelfIdx > 0)
        for (UINT32 i = a_uSelfIdx; (i < MIXCNT) && (ar_sResult == E_SUCCESS); i++)
        {
            for (UINT32 j = 0; (j < 4) && (ar_sResult == E_SUCCESS); j++)
                sendBN(ar_sResult, a_pPrevMIX, bnProdList[4 * i + j]);
            for (UINT32 j = 0; (j < 2) && (ar_sResult == E_SUCCESS); j++)
                sendBN(ar_sResult, a_pPrevMIX, bnExpList[2 * i + j]);
        }



    // calc the product of inputlist
    bnProdInpA0 = BN_new();
    bnProdInpB0 = BN_new();
    bnProdInpA1 = BN_new();
    bnProdInpB1 = BN_new();

    if ((bnProdInpA0 == NULL) || (bnProdInpB0 == NULL) || (bnProdInpA1 == NULL) || (bnProdInpB1 == NULL))
        ar_sResult = E_UNKNOWN;
    else
    {
        BN_init(bnProdInpA0); BN_one(bnProdInpA0);
        BN_init(bnProdInpB0); BN_one(bnProdInpB0);
        BN_init(bnProdInpA1); BN_one(bnProdInpA1);
        BN_init(bnProdInpB1); BN_one(bnProdInpB1);
    }

    for (UINT32 i = 0; (i < a_uInpListsSize[0]) && (ar_sResult == E_SUCCESS); i += 2)
    {
        if (BN_mod_mul(bnProdInpA0, bnProdInpA0, a_bnInpFirstReEncList[i + 0], a_ths->m_elGroupKey->p, ctx) == 0) ar_sResult = E_UNKNOWN;
        if (BN_mod_mul(bnProdInpB0, bnProdInpB0, a_bnInpFirstReEncList[i + 1], a_ths->m_elGroupKey->p, ctx) == 0) ar_sResult = E_UNKNOWN;
        if (BN_mod_mul(bnProdInpA1, bnProdInpA1, a_bnInpSecondReEncList[i + 0], a_ths->m_elGroupKey->p, ctx) == 0) ar_sResult = E_UNKNOWN;
        if (BN_mod_mul(bnProdInpB1, bnProdInpB1, a_bnInpSecondReEncList[i + 1], a_ths->m_elGroupKey->p, ctx) == 0) ar_sResult = E_UNKNOWN;
    }

    // check the first mix data
    if (ar_sResult == E_SUCCESS)
        if (ELGAMAL_reencrypt(bnProdInpA0, bnProdInpB0, a_ths->m_elGroupKey, bnExpList[0]) == 0)
            ar_sResult = E_UNKNOWN;
    if (ar_sResult == E_SUCCESS)
        if (BN_cmp(bnProdInpA0, bnProdList[0]) != 0)
            ar_sResult = E_UNKNOWN;
    if (ar_sResult == E_SUCCESS)
        if (BN_cmp(bnProdInpB0, bnProdList[1]) != 0)
            ar_sResult = E_UNKNOWN;
    if (ar_sResult == E_SUCCESS)
        if (ELGAMAL_reencrypt(bnProdInpA1, bnProdInpB1, a_ths->m_elGroupKey, bnExpList[1]) == 0)
            ar_sResult = E_UNKNOWN;
    if (ar_sResult == E_SUCCESS)
        if (BN_cmp(bnProdInpA1, bnProdList[2]) != 0)
            ar_sResult = E_UNKNOWN;
    if (ar_sResult == E_SUCCESS)
        if (BN_cmp(bnProdInpB1, bnProdList[3]) != 0)
            ar_sResult = E_UNKNOWN;

    // check the products
    for (UINT32 i = 1; (i < MIXCNT) && (ar_sResult == E_SUCCESS); i++)
    {
        if (ELGAMAL_reencrypt(bnProdList[4*(i - 1)], bnProdList[4*(i - 1) + 1], a_ths->m_elGroupKey, bnExpList[2*i]) == 0)
            ar_sResult = E_UNKNOWN;
        if (ar_sResult == E_SUCCESS)
            if (BN_cmp(bnProdList[4*(i - 1)], bnProdList[4*i]) != 0)
                ar_sResult = E_UNKNOWN;
        if (ar_sResult == E_SUCCESS)
            if (BN_cmp(bnProdList[4*(i - 1) + 1], bnProdList[4*i + 1]) != 0)
                ar_sResult = E_UNKNOWN;
        if (ELGAMAL_reencrypt(bnProdList[4*(i - 1) + 2], bnProdList[4*(i - 1) + 3], a_ths->m_elGroupKey, bnExpList[2*i + 1]) == 0)
            ar_sResult = E_UNKNOWN;
        if (ar_sResult == E_SUCCESS)
            if (BN_cmp(bnProdList[4*(i - 1) + 2], bnProdList[4*i + 2]) != 0)
                ar_sResult = E_UNKNOWN;
        if (ar_sResult == E_SUCCESS)
            if (BN_cmp(bnProdList[4*(i - 1) + 3], bnProdList[4*i + 3]) != 0)
                ar_sResult = E_UNKNOWN;
    }


    if (bnProdInpA0 != NULL) BN_free(bnProdInpA0);
    if (bnProdInpB0 != NULL) BN_free(bnProdInpB0);
    if (bnProdInpA1 != NULL) BN_free(bnProdInpA1);
    if (bnProdInpB1 != NULL) BN_free(bnProdInpB1);
    clearBNList(2*MIXCNT, bnExpList, 1);
    clearBNList(4*MIXCNT, bnProdList, 1);
    if (ctx != NULL) BN_CTX_free(ctx);

    return ar_sResult;
}

SINT32 CAFlashMIX::unblindCheckFirstReEnc(SINT32& ar_sResult, CAFlashMIX* a_ths,
    CASocket* a_pPrevMIX, CASocket* a_pNextMIX, UINT32 a_uSelfIdx,
    UINT32 a_uInpListSize, BIGNUM** a_bnInpList, BIGNUM** a_bnFistList, BIGNUM** a_bnSecondList,
    UINT32** a_uInpPerm, BIGNUM*** a_bnExp,
    UINT32 MIXCNT)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    UINT32 LISTSIZE = a_uInpListSize >> 1;

    UINT32* uPermList = NULL;
    BIGNUM** bnExpList = NULL;
    UINT32 uListSize = 0;
    BN_CTX* ctx = NULL;
    BIGNUM** bnPermInp = NULL;

    UINT32* uPerm0 = NULL;
    UINT32* uPerm1 = NULL;

    initCTX(ar_sResult, &ctx);

    unblindFirstReEncPermData(ar_sResult, &uPermList, a_pPrevMIX, a_pNextMIX,
        a_uSelfIdx, LISTSIZE, a_uInpPerm, MIXCNT);
    unblindFirstReEncExpList(ar_sResult, &bnExpList, a_pPrevMIX, a_pNextMIX,
        a_uSelfIdx, LISTSIZE, a_bnExp, MIXCNT);

    CAMsg::printMsg(LOG_DEBUG, "check 1 Reenc receive Perm and Exp %s\n", (ar_sResult == E_SUCCESS) ? "successfully finished" : "ERROR");
    unblindCalcAkk(ar_sResult, &uPerm0, &uPerm1, LISTSIZE, uPermList, MIXCNT);

    CAMsg::printMsg(LOG_DEBUG, "check 1 Reenc calc akk perm %s\n", (ar_sResult == E_SUCCESS) ? "successfully finished" : "ERROR");

    // calc sum exp of list 0
    for (UINT32 i = 1; (i < MIXCNT) && (ar_sResult == E_SUCCESS); i++)
    {
        unblindPermutate(ar_sResult, bnExpList, LISTSIZE, uPermList + 2*(i-1)*LISTSIZE);
        for (UINT32 j = 0; (j < LISTSIZE) && (ar_sResult == E_SUCCESS); j++)
            if (BN_mod_add(bnExpList[j], bnExpList[j], bnExpList[(2*i + 0) * LISTSIZE + j], a_ths->m_elGroupKey->p, ctx) == 0)
                ar_sResult = E_UNKNOWN;
    }
    unblindPermutate(ar_sResult, bnExpList, LISTSIZE, uPermList + 2*(MIXCNT-1)*LISTSIZE);

    for (UINT32 i = 1; (i < MIXCNT) && (ar_sResult == E_SUCCESS); i++)
    {
        unblindPermutate(ar_sResult, bnExpList + LISTSIZE, LISTSIZE, uPermList + (2*(i-1)+1)*LISTSIZE);
        for (UINT32 j = 0; (j < LISTSIZE) && (ar_sResult == E_SUCCESS); j++)
            if (BN_mod_add(bnExpList[LISTSIZE + j], bnExpList[LISTSIZE + j], bnExpList[(2*i + 1)*LISTSIZE + j], a_ths->m_elGroupKey->p, ctx) == 0)
                ar_sResult = E_UNKNOWN;
    }
    unblindPermutate(ar_sResult, bnExpList + LISTSIZE, LISTSIZE, uPermList + (2*(MIXCNT-1)+1)*LISTSIZE);

    copyBNList(ar_sResult, &bnPermInp, a_uInpListSize, a_bnInpList);
    unblindPermutateMsg(ar_sResult, bnPermInp, a_uInpListSize, uPerm0);

    if (ar_sResult == E_SUCCESS)
        if (ELGAMAL_reencrypt(bnPermInp, a_ths->m_elGroupKey, LISTSIZE, bnExpList) == 0)
            ar_sResult = E_UNKNOWN;

    // check the values
    for (UINT32 i = 0; (i < a_uInpListSize) && (ar_sResult == E_SUCCESS); i++)
        if (BN_cmp(bnPermInp[i], a_bnFistList[i]) != 0)
            ar_sResult = E_UNKNOWN;

    clearBNList(a_uInpListSize, bnPermInp);
    bnPermInp = NULL;

    copyBNList(ar_sResult, &bnPermInp, a_uInpListSize, a_bnInpList);
    unblindPermutateMsg(ar_sResult, bnPermInp, a_uInpListSize, uPerm1);

    if (ar_sResult == E_SUCCESS)
        if (ELGAMAL_reencrypt(bnPermInp, a_ths->m_elGroupKey, LISTSIZE, bnExpList + LISTSIZE) == 0)
            ar_sResult = E_UNKNOWN;

    // check the values
    for (UINT32 i = 0; (i < a_uInpListSize) && (ar_sResult == E_SUCCESS); i++)
        if (BN_cmp(bnPermInp[i], a_bnSecondList[i]) != 0)
            ar_sResult = E_UNKNOWN;

    clearBNList(2 * MIXCNT * LISTSIZE, bnExpList, 1);
    clearBNList(a_uInpListSize, bnPermInp);
    delete[] uPermList;
    delete[] uPerm0;
    delete[] uPerm1;
    clearCTX(&ctx);

    return ar_sResult;
}

SINT32 CAFlashMIX::unblindFirstReEncPermData(SINT32& ar_sResult,
    UINT32** r_uPermList, CASocket* a_pPrevMIX, CASocket* a_pNextMIX,
    UINT32 a_uSelfIdx, UINT32 LISTSIZE, UINT32** a_uInpPerm,
    UINT32 MIXCNT)
{
    if (ar_sResult != E_SUCCESS) return ar_sResult;

    if ((*r_uPermList = new UINT32[2 * MIXCNT * LISTSIZE]) == NULL)
        ar_sResult = E_UNKNOWN;

    // receive input permutation of prev mix
    for (UINT32 i = 0; (i < a_uSelfIdx) && (ar_sResult == E_SUCCESS); i++)
    {
        for (UINT32 j = 0; (j < LISTSIZE) && (ar_sResult == E_SUCCESS); j++)
            ar_sResult = receiveUINT32((*r_uPermList)[(2*i + 0)*LISTSIZE + j], a_pPrevMIX);
        for (UINT32 j = 0; (j < LISTSIZE) && (ar_sResult == E_SUCCESS); j++)
            ar_sResult = receiveUINT32((*r_uPermList)[(2*i + 1)*LISTSIZE + j], a_pPrevMIX);
    }

    // add own permutation
    for (UINT32 i = 0; (i < LISTSIZE) && (ar_sResult == E_SUCCESS); i++)
    {
        (*r_uPermList)[(2*a_uSelfIdx + 0)*LISTSIZE + i] = a_uInpPerm[0][i];
        (*r_uPermList)[(2*a_uSelfIdx + 1)*LISTSIZE + i] = a_uInpPerm[1][i];
    }

    if (a_uSelfIdx < MIXCNT - 1)
    {
        // send data to next mix
        for (UINT32 i = 0; (i <= a_uSelfIdx) && (ar_sResult == E_SUCCESS); i++)
        {
            for (UINT32 j = 0; (j < LISTSIZE) && (ar_sResult == E_SUCCESS); j++)
                ar_sResult = sendUINT32(a_pNextMIX, (*r_uPermList)[(2*i + 0)*LISTSIZE + j]);
            for (UINT32 j = 0; (j < LISTSIZE) && (ar_sResult == E_SUCCESS); j++)
                ar_sResult = sendUINT32(a_pNextMIX, (*r_uPermList)[(2*i + 1)*LISTSIZE + j]);
        }

        // receive data from next mix
        for (UINT32 i = a_uSelfIdx + 1; (i < MIXCNT) && (ar_sResult == E_SUCCESS); i++)
        {
            for (UINT32 j = 0; (j < LISTSIZE) && (ar_sResult == E_SUCCESS); j++)
                ar_sResult = receiveUINT32((*r_uPermList)[(2*i + 0)*LISTSIZE + j], a_pNextMIX);
            for (UINT32 j = 0; (j < LISTSIZE) && (ar_sResult == E_SUCCESS); j++)
                ar_sResult = receiveUINT32((*r_uPermList)[(2*i + 1)*LISTSIZE + j], a_pNextMIX);
        }
    }

    // send to prev mix
    if (a_uSelfIdx > 0)
        for (UINT32 i = a_uSelfIdx; (i < MIXCNT) && (ar_sResult == E_SUCCESS); i++)
        {
            for (UINT32 j = 0; (j < LISTSIZE) && (ar_sResult == E_SUCCESS); j++)
                ar_sResult = sendUINT32(a_pPrevMIX, (*r_uPermList)[(2*i + 0)*LISTSIZE + j]);
            for (UINT32 j = 0; (j < LISTSIZE) && (ar_sResult == E_SUCCESS); j++)
                ar_sResult = sendUINT32(a_pPrevMIX, (*r_uPermList)[(2*i + 1)*LISTSIZE + j]);
        }

    return ar_sResult;
}

SINT32 CAFlashMIX::unblindFirstReEncExpList(SINT32& ar_sResult,
    BIGNUM*** r_bnExpList, CASocket* a_pPrevMIX, CASocket* a_pNextMIX,
    UINT32 a_uSelfIdx, UINT32 LISTSIZE, BIGNUM*** a_bnExp, UINT32 MIXCNT)
{
    if (ar_sResult != E_SUCCESS) return ar_sResult;

    initBNList(ar_sResult, r_bnExpList, 2 * MIXCNT * LISTSIZE);

    // receive exp from prev mix
    for (UINT32 i = 0; (i < a_uSelfIdx) && (ar_sResult == E_SUCCESS); i++)
    {
        for (UINT32 j = 0; (j < LISTSIZE) && (ar_sResult == E_SUCCESS); j++)
            ar_sResult = receiveBN(&((*r_bnExpList)[(2*i + 0)*LISTSIZE + j]), a_pPrevMIX);
        for (UINT32 j = 0; (j < LISTSIZE) && (ar_sResult == E_SUCCESS); j++)
            ar_sResult = receiveBN(&((*r_bnExpList)[(2*i + 1)*LISTSIZE + j]), a_pPrevMIX);
    }

    // add own data to list
    for (UINT32 i = 0; (i < LISTSIZE) && (ar_sResult == E_SUCCESS); i++)
    {
        (*r_bnExpList)[(2*a_uSelfIdx + 0)*LISTSIZE + i] = BN_dup(a_bnExp[0][i]);
        (*r_bnExpList)[(2*a_uSelfIdx + 1)*LISTSIZE + i] = BN_dup(a_bnExp[1][i]);
    }

    if (a_uSelfIdx < MIXCNT - 1)
    {
        // send data to next mix
        for (UINT32 i = 0; (i <= a_uSelfIdx) && (ar_sResult == E_SUCCESS); i++)
        {
            for (UINT32 j = 0; (j < LISTSIZE) && (ar_sResult == E_SUCCESS); j++)
                ar_sResult = sendBN(a_pNextMIX, (*r_bnExpList)[(2*i + 0)*LISTSIZE + j]);
            for (UINT32 j = 0; (j < LISTSIZE) && (ar_sResult == E_SUCCESS); j++)
                ar_sResult = sendBN(a_pNextMIX, (*r_bnExpList)[(2*i + 1)*LISTSIZE + j]);
        }

        // receive data from next mix
        for (UINT32 i = a_uSelfIdx + 1; (i < MIXCNT) && (ar_sResult == E_SUCCESS); i++)
        {
            for (UINT32 j = 0; (j < LISTSIZE) && (ar_sResult == E_SUCCESS); j++)
                ar_sResult = receiveBN(&((*r_bnExpList)[(2*i + 0)*LISTSIZE + j]), a_pNextMIX);
            for (UINT32 j = 0; (j < LISTSIZE) && (ar_sResult == E_SUCCESS); j++)
                ar_sResult = receiveBN(&((*r_bnExpList)[(2*i + 1)*LISTSIZE + j]), a_pNextMIX);

        }
    }

    // send data to prev mix
    if (a_uSelfIdx > 0)
        for (UINT32 i = a_uSelfIdx; (i < MIXCNT) && (ar_sResult == E_SUCCESS); i++)
        {
            for (UINT32 j = 0; (j < LISTSIZE) && (ar_sResult == E_SUCCESS); j++)
                ar_sResult = sendBN(a_pPrevMIX, (*r_bnExpList)[(2*i + 0)*LISTSIZE + j]);
            for (UINT32 j = 0; (j < LISTSIZE) && (ar_sResult == E_SUCCESS); j++)
                ar_sResult = sendBN(a_pPrevMIX, (*r_bnExpList)[(2*i + 1)*LISTSIZE + j]);
        }

    return ar_sResult;
}

SINT32 CAFlashMIX::unblindCalcAkkPerm(SINT32& ar_sResult,
    UINT32** r_uPerm0, UINT32** r_uPerm1,
    UINT32* a_uPermList, UINT32 MIXCNT, UINT32 LISTSIZE)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    UINT32 uTmp = 0;

    if ((*r_uPerm0 = new UINT32[LISTSIZE]) == NULL)
        ar_sResult = E_UNKNOWN;
    if ((*r_uPerm1 = new UINT32[LISTSIZE]) == NULL)
        ar_sResult = E_UNKNOWN;

    uTmp = a_uPermList[0];
    for (UINT32 i = 0; (i < LISTSIZE) && (ar_sResult == E_SUCCESS); i++)
    {
        for (UINT32 j = 0; (j < MIXCNT) && (ar_sResult == E_SUCCESS); j++)
            getNextPermElement(ar_sResult, uTmp, LISTSIZE, a_uPermList + 2*j*LISTSIZE, uTmp);

        (*r_uPerm0)[i] = uTmp;
    }

    uTmp = a_uPermList[LISTSIZE];
    for (UINT32 i = 0; (i < LISTSIZE) && (ar_sResult == E_SUCCESS); i++)
    {
        for (UINT32 j = 0; (j < MIXCNT) && (ar_sResult == E_SUCCESS); j++)
            getNextPermElement(ar_sResult, uTmp, LISTSIZE, a_uPermList + (2*j+1)*LISTSIZE, uTmp);

        (*r_uPerm1)[i] = uTmp;
    }

    return ar_sResult;
}

SINT32 CAFlashMIX::unblindCalcAkk(SINT32& ar_sResult,
    UINT32** r_uPerm0, UINT32** r_uPerm1,
    UINT32 LISTSIZE, UINT32* a_uPermList, UINT32 MIXCNT)
{
    if (ar_sResult != E_SUCCESS) return ar_sResult;

    UINT32 uTmp = 0;
    SINT32 i = 0;
    bool bNew = false;

    if ((*r_uPerm0 = new UINT32[2*LISTSIZE]) == NULL)
        ar_sResult = E_UNKNOWN;
    if ((*r_uPerm1 = new UINT32[2*LISTSIZE]) == NULL)
        ar_sResult = E_UNKNOWN;

    for (i = 0; (i < 2*LISTSIZE) && (ar_sResult == E_SUCCESS); i++)
    {
        (*r_uPerm0)[i] = 0xFFFFFFFF;
        (*r_uPerm1)[i] = 0xFFFFFFFF;
    }

    i = 0;
    while (unblindGetFirstL1NotInL2(ar_sResult, uTmp, LISTSIZE, a_uPermList, 2*LISTSIZE, *r_uPerm0) == E_SUCCESS)
    {
        bNew = false;
        while (!bNew)
        {
            for (UINT32 j = 0; (j < MIXCNT) && (ar_sResult == E_SUCCESS); j++)
                getNextPermElement(ar_sResult, uTmp, LISTSIZE, a_uPermList + 2*j*LISTSIZE, uTmp);
            if (getIdx(2*LISTSIZE, *r_uPerm0, uTmp) < 0)
                (*r_uPerm0)[i] = uTmp;
            else
                bNew = true;
            i++;
        }
    }/**/

    i = 0;
    while (unblindGetFirstL1NotInL2(ar_sResult, uTmp, LISTSIZE, a_uPermList + LISTSIZE, 2*LISTSIZE, *r_uPerm1) == E_SUCCESS)
    {
        bNew = false;
        while (!bNew)
        {
            for (UINT32 j = 0; (j < MIXCNT) && (ar_sResult == E_SUCCESS); j++)
                getNextPermElement(ar_sResult, uTmp, LISTSIZE, a_uPermList + (2*j+1)*LISTSIZE, uTmp);
            if (getIdx(2*LISTSIZE, *r_uPerm1, uTmp) < 0)
                (*r_uPerm1)[i] = uTmp;
            else
                bNew = true;
            i++;
        }
    }/**/

    return ar_sResult;
}

SINT32 CAFlashMIX::unblindGetFirstL1NotInL2(SINT32& ar_sResult, UINT32& r_uElement,
    UINT32 a_uList1Size, UINT32* a_uList1, UINT32 a_uList2Size, UINT32* a_uList2)
{
    if (ar_sResult != E_SUCCESS) return ar_sResult;
    if ((a_uList1 == NULL) || (a_uList2 == NULL)) { ar_sResult = E_UNKNOWN; return ar_sResult; }

    for (UINT32 i = 0; i < a_uList1Size; i++)
    {
        bool bIsInList = false;
        for (UINT32 j = 0; (j < a_uList2Size) && (bIsInList == false); j++)
            if (a_uList2[j] == a_uList1[i])
                bIsInList = true;

        if (bIsInList == false)
        {
            r_uElement = a_uList1[i];
            return E_SUCCESS;
        }
    }

    return E_UNKNOWN;
}

SINT32 CAFlashMIX::unblindPermutate(SINT32& ar_sResult,
    BIGNUM** ar_bnList, UINT32 a_uListSize, UINT32* a_uPerm)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;
    if ((ar_bnList == NULL) || (a_uPerm == NULL))
    {
        ar_sResult E_UNKNOWN;
        return ar_sResult;
    }

    for (UINT32 i = 1; i < a_uListSize; i++)
    {
        BIGNUM* bnTmp = ar_bnList[a_uPerm[0]];
        ar_bnList[a_uPerm[0]] = ar_bnList[a_uPerm[i]];
        ar_bnList[a_uPerm[i]] = bnTmp;
    }

    return ar_sResult;
}/**/

SINT32 CAFlashMIX::unblindPermutateMsg(SINT32& ar_sResult, BIGNUM** ar_bnList,
    UINT32 a_uListSize, UINT32* a_uPerm)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;
    if ((ar_bnList == NULL) || (a_uPerm == NULL))
    {
        ar_sResult=E_UNKNOWN;
        return ar_sResult;
    }

    UINT32 uPL = 0;
    UINT32 uPH = 0;

    while (unblindPermSetPtr(uPL, uPH, a_uListSize, a_uPerm) == E_SUCCESS)
    {
        switchEncListElements(a_uListSize, ar_bnList, a_uPerm[uPL], a_uPerm[uPH]);
    }

    return ar_sResult;
}

SINT32 CAFlashMIX::unblindPermSetPtr(UINT32& ar_uPtrL,
    UINT32& ar_uPtrH, UINT32 a_uListSize, UINT32* a_uPerm)
{
    ar_uPtrH++;
    if (ar_uPtrH >= a_uListSize)
        return E_UNKNOWN;

    if (a_uPerm[ar_uPtrH] == 0xFFFFFFFF)
    {
        ar_uPtrH++;
        if (a_uPerm[ar_uPtrH] == 0xFFFFFFFF)
            return E_UNKNOWN;

        ar_uPtrL = ar_uPtrH;
        ar_uPtrH++;
        if (ar_uPtrH == 0xFFFFFFFF)
            ar_uPtrH--;
    }

    return E_SUCCESS;
}

SINT32 CAFlashMIX::decrypt(CAFlashMIX* a_ths, CASocket* a_pBB)
{
    if ((a_ths == NULL) || (a_pBB == NULL)) return E_UNKNOWN;

    CAMsg::printMsg(LOG_DEBUG, "decryption startet\n");

    UINT32 MIXCNT = a_ths->m_uMIXDecryptCnt;

    SINT32 result = E_SUCCESS;

    UINT32* uDecryptList = NULL;
    UINT32 uSelfIdx = 0xFFFFFFFF;
    UINT32 uThsMIXIdx = 0xFFFFFFFF;
    BIGNUM** bnInpList = NULL;
    UINT32 uInpListSize = 0;
    BIGNUM* bnSharedExp = NULL;
    BIGNUM* bnTmp = NULL;
    BN_CTX* ctx = NULL;

    CASocket* pNextMIX = NULL;
    CASocket* pPrevMIX = NULL;

    initBN(result, &bnTmp);
    initCTX(result, &ctx);
    if ((uDecryptList = new UINT32[MIXCNT]) == NULL)
        result = E_UNKNOWN;

    receiveUINT32(result, uSelfIdx, a_pBB);
    for (UINT32 i = 0; (i < MIXCNT) && (result == E_SUCCESS); i++)
        result = receiveUINT32(uDecryptList[i], a_pBB);

    CAMsg::printMsg(LOG_DEBUG, "receive decryption list: %s\n", (result == E_SUCCESS) ? "successfully finished" : "ERROR");

    // connect
    if (result == E_SUCCESS)
        result = decryptConnectMIX(&pPrevMIX, &pNextMIX, a_ths, uSelfIdx, uDecryptList);

    CAMsg::printMsg(LOG_DEBUG, "connect to prev and next mix: %s\n", (result == E_SUCCESS) ? "successfully finished" : "ERROR");

    // receive inputlist
    if (uSelfIdx == 0)
        receiveSignedBNArray(result, uInpListSize, &bnInpList, a_ths->m_elBBSignKey, a_pBB);
    else
        receiveSignedBNArray(result, uInpListSize, &bnInpList, a_ths->m_vMIXData[uDecryptList[uSelfIdx - 1]]->elSignKey, pPrevMIX);

    CAMsg::printMsg(LOG_DEBUG, "receive inputlist: %s\n", (result == E_SUCCESS) ? "successfully finished" : "ERROR");

    // decrypt
    uThsMIXIdx = getMIXIdx(a_ths->m_pMutexMIXData, a_ths->m_uMIXCnt, a_ths->m_vMIXData, a_ths->m_uLocalIP, a_ths->m_uPort);
    if (result == E_SUCCESS)
        if (ELGAMAL_calcSharedExp(&bnSharedExp, a_ths->m_uMIXCnt, MIXCNT, uDecryptList, uThsMIXIdx, a_ths->m_elGroupKey->x, a_ths->m_elGroupKey->q) != ELGAMAL_SUCCESS)
            result = E_UNKNOWN;

    for (UINT32 i = 0; (i < uInpListSize) && (result == E_SUCCESS); i += 2)
    {
        if (BN_mod_exp(bnTmp, bnInpList[i + 1], bnSharedExp, a_ths->m_elGroupKey->p, ctx) == 0)
            result = E_UNKNOWN;
        else
            if (BN_mod_mul(bnInpList[i], bnInpList[i], bnTmp, a_ths->m_elGroupKey->p, ctx) == 0)
                result = E_UNKNOWN;
    }

    // send the list
    if (uSelfIdx < MIXCNT - 1)
        sendSignedBNArray(result, a_ths->m_elSignKey, pNextMIX, uInpListSize, bnInpList);
    else
    {
        // send to receivers
        for (UINT32 i = 0; (i < uInpListSize) && (result == E_SUCCESS); i += 2)
        {
            decryptSendMessage(result, bnInpList[i]);
        }
    }

    CAMsg::printMsg(LOG_DEBUG, "list send to next mix: %s\n", (result == E_SUCCESS) ? "successfully finished" : "ERROR");

    if (result == E_SUCCESS)
        result = sendUINT32(a_pBB, DECRYPT_SUCCESS);
    else
        sendUINT32(a_pBB, DECRYPT_ERROR);

    CAMsg::printMsg(LOG_DEBUG, "decryption finished\n");

    if (pPrevMIX != NULL) pPrevMIX->close();
    if (pNextMIX != NULL) pNextMIX->close();
    delete pPrevMIX;
    delete pNextMIX;
    delete[] uDecryptList;
    clearBN(&bnSharedExp);
    clearBN(&bnTmp);
    clearCTX(&ctx);

    return result;
}

SINT32 CAFlashMIX::decryptConnectMIX(CASocket** r_pPrevMIX, CASocket** r_pNextMIX,
    CAFlashMIX* a_ths, UINT32 a_uSelfIdx, UINT32* a_uDecryptList)
{
    if ((r_pPrevMIX == NULL) || (r_pNextMIX == NULL) || (a_ths == NULL))
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;

    delete *r_pPrevMIX; *r_pPrevMIX = NULL;
    delete *r_pNextMIX; *r_pNextMIX = NULL;

    // connect prev and next mix
    if (a_uSelfIdx > 0)
    {
        if ((*r_pNextMIX = new CASocket()) == NULL)
            result = E_UNKNOWN;
        else
            result = (*r_pNextMIX)->listen(a_ths->m_uPortDecrypt);
        if ((*r_pPrevMIX = new CASocket()) == NULL)
            result = E_UNKNOWN;
        if (result == E_SUCCESS)
            result = (*r_pNextMIX)->accept(**r_pPrevMIX);

        if (*r_pNextMIX != NULL) (*r_pNextMIX)->close();
        delete *r_pNextMIX;
        *r_pNextMIX = NULL;
    }
    // connect to next mixes
    if (a_uSelfIdx < a_ths->m_uMIXDecryptCnt - 1)
    {
        CASocketAddrINet* pAddr = NULL;

        initSocket(result, r_pNextMIX);
        initSocketAddrINet(result, &pAddr);
        MIXDATA* pMIXData = a_ths->m_vMIXData[a_uDecryptList[a_uSelfIdx + 1]];
        if (initSocketAddrINet(result, &pAddr) == E_SUCCESS)
            result = pAddr->setAddr(pMIXData->cIP, pMIXData->uPortDecrypt);
        if (result == E_SUCCESS)
            result = (*r_pNextMIX)->connect(*pAddr, 5000, 1);

        delete pAddr;
    }

    if (result != E_SUCCESS)
    {
        delete *r_pPrevMIX; *r_pPrevMIX = NULL;
        delete *r_pNextMIX; *r_pNextMIX = NULL;
    }

    return result;
}

SINT32 CAFlashMIX::decryptSendMessage(SINT32& ar_sResult, BIGNUM* a_bnMsg)
{
    if (ar_sResult != E_SUCCESS) return ar_sResult;
    if (a_bnMsg == NULL) { ar_sResult = E_UNKNOWN; return ar_sResult; }

    CASocket* pSocket = NULL;
    CASocketAddrINet* pAddr = NULL;
    UINT8* uBN = NULL;
    UINT32 uBNSize = 0;

    initSocket(ar_sResult, &pSocket);
    initSocketAddrINet(ar_sResult, &pAddr);

    uBNSize = BN_num_bytes(a_bnMsg);
    if ((uBN = new UINT8[uBNSize]) == NULL)
        ar_sResult = E_UNKNOWN;

    if (ar_sResult == E_SUCCESS)
        if (BN_bn2bin(a_bnMsg, uBN) != uBNSize)
            ar_sResult = E_UNKNOWN;

    if (ar_sResult == E_SUCCESS)
        ar_sResult = pAddr->setIP(uBN);
    if (ar_sResult == E_SUCCESS)
        ar_sResult = pAddr->setPort(((UINT16*)uBN)[2]);

    if (ar_sResult == E_SUCCESS)
        ar_sResult = pSocket->connect(*pAddr, 5, 1);

    sendUINT8Arr(pSocket, uBNSize - 6, uBN + 6);

    delete[] uBN;
    if (pSocket != NULL) pSocket->close();
    delete pSocket;
    delete pAddr;

    return ar_sResult;
}
