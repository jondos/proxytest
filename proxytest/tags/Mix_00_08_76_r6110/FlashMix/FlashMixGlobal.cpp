#include "../StdAfx.h"
#include "FlashMixGlobal.hpp"

MIXDATA* MIXDATA_new()
{
    MIXDATA* ret;
    ret = (MIXDATA*)OPENSSL_malloc(sizeof(MIXDATA));
    if (ret == NULL)
        return NULL;

    ret->cIP = NULL;
    ret->elEncKey = NULL;
    ret->elSignKey = NULL;

    return ret;
}

void MIXDATA_free(MIXDATA* a_pMIXData)
{
    if (a_pMIXData == NULL)
        return;

    ELGAMAL_free(a_pMIXData->elEncKey);
    ELGAMAL_free(a_pMIXData->elSignKey);
    delete[] a_pMIXData->cIP;

    OPENSSL_free(a_pMIXData);
    a_pMIXData = NULL;
}

SINT32 initBN(BIGNUM** r_bn, UINT32 a_uSetWord)
{
    if (r_bn == NULL)
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;

    if (*r_bn != NULL)
        BN_clear_free(*r_bn);

    if ((*r_bn = BN_new()) == NULL)
        result = E_UNKNOWN;
    else
    {
        BN_init(*r_bn);
        if (BN_set_word(*r_bn, a_uSetWord) == 0)
            result = E_UNKNOWN;
    }

    if (result != E_SUCCESS)
    {
        if (*r_bn != NULL) BN_free(*r_bn);
        *r_bn = NULL;
    }

    return result;
}

SINT32 initBN(SINT32& ar_sResult, BIGNUM** r_bn, UINT32 a_uSetWord)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    ar_sResult = initBN(r_bn, a_uSetWord);

    return ar_sResult;
}

SINT32 initBNRand(BIGNUM** r_bn, UINT32 a_uBitLength)
{
    if (r_bn == NULL)
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;

    initBN(result, r_bn);

    if (result == E_SUCCESS)
        if (BN_rand(*r_bn, a_uBitLength, 1, 1) == 0)
            result = E_UNKNOWN;

    if (result != E_SUCCESS)
    {
        if (*r_bn != NULL) BN_free(*r_bn);
        *r_bn = NULL;
    }

    return result;
}

SINT32 initBNRand(SINT32& ar_sResult, BIGNUM** r_bn, UINT32 a_uBitLength)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    ar_sResult = initBNRand(r_bn, a_uBitLength);

    return ar_sResult;
}

void clearBN(BIGNUM** a_bn)
{
    if (*a_bn != NULL) BN_clear_free(*a_bn);
    *a_bn = NULL;
}

SINT32 initCTX(BN_CTX** r_ctx)
{
    if (r_ctx == NULL)
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;

    if (*r_ctx != NULL)
        BN_CTX_free(*r_ctx);

    if ((*r_ctx = BN_CTX_new()) == NULL)
        result = E_UNKNOWN;
    else
        BN_CTX_init(*r_ctx);

    if (result != E_SUCCESS)
    {
        if (*r_ctx != NULL) BN_CTX_free(*r_ctx);
        *r_ctx = NULL;
    }

    return result;
}

SINT32 initCTX(SINT32& ar_sResult, BN_CTX** r_ctx)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    ar_sResult = initCTX(r_ctx);

    return ar_sResult;
}

void clearCTX(BN_CTX** a_ctx)
{
    if (*a_ctx != NULL) BN_CTX_free(*a_ctx);
    *a_ctx = NULL;
}

SINT32 initBNList(BIGNUM*** r_bnList, UINT32 a_bnListSize)
{
    if (r_bnList == NULL)
        return E_UNKNOWN;
    if (*r_bnList != NULL)
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;

    if ((*r_bnList = new BIGNUM*[a_bnListSize]) == NULL)
        result = E_UNKNOWN;
    else
        for (UINT32 i = 0; i < a_bnListSize; i++)
            (*r_bnList)[i] = NULL;

    if (result != E_SUCCESS)
    {
        delete[] *r_bnList;
        *r_bnList = NULL;
    }

    return result;
}

SINT32 initBNList(SINT32& ar_sResult, BIGNUM*** r_bnList, UINT32 a_bnListSize)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    ar_sResult = initBNList(r_bnList, a_bnListSize);

    return ar_sResult;
}

SINT32 clearBNList(UINT32 a_uListSize, BIGNUM** a_bnList, UINT8 a_uFreeArray)
{
    if (a_bnList == NULL)
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;

    try
    {
        for (UINT32 i = 0; i < a_uListSize; i++)
            if (a_bnList[i] != NULL)
            {
                BN_clear_free(a_bnList[i]);
                a_bnList[i] = NULL;
            }
    }
    catch ( ... )
    {
        result = E_UNKNOWN;
    }

    if (a_uFreeArray != 0)
        delete[] a_bnList;

    return result;
}

SINT32 clearBNList(SINT32& ar_sResult, UINT32 a_uListSize, BIGNUM** a_bnList, UINT8 a_uFreeArray)
{
    if (ar_sResult != E_SUCCESS)
        clearBNList(a_uListSize, a_bnList, a_uFreeArray);
    else
        ar_sResult = clearBNList(a_uListSize, a_bnList, a_uFreeArray);

    return ar_sResult;
}

SINT32 copyBNList(BIGNUM*** a_bnCopy, UINT32 a_uListSize, BIGNUM** a_bnSource)
{
    if ((a_bnCopy == NULL) || (a_bnSource == NULL))
        return E_UNKNOWN;
    if (*a_bnCopy != NULL)
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;

    initBNList(result, a_bnCopy, a_uListSize);

    for (UINT32 i = 0; (i < a_uListSize) && (result == E_SUCCESS); i++)
    {
        if (a_bnSource[i] == NULL)
            continue;

        if (((*a_bnCopy)[i] = BN_dup(a_bnSource[i])) == NULL)
            result = E_UNKNOWN;
    }

    return result;
}

SINT32 copyBNList(SINT32& ar_sResult, BIGNUM*** a_bnCopy, UINT32 a_uListSize, BIGNUM** a_bnSource)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    ar_sResult = copyBNList(a_bnCopy, a_uListSize, a_bnSource);

    return ar_sResult;
}

SINT32 getFreeIdx(UINT32 a_uListSize, void** a_vList)
{
    if (a_vList == NULL)
        return -1;

    for (UINT32 i = 0; i < a_uListSize; i++)
        if (a_vList[i] == NULL)
            return i;

    return -1;
}

SINT32 insertToList(UINT32 a_uListSize, void** a_vList, void* a_vListElement)
{
    if ((a_vList == NULL) || (a_vListElement == NULL))
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;
    SINT32 sIdx = -1;

    if ((sIdx = getFreeIdx(a_uListSize, a_vList)) < 0)
        result = E_UNKNOWN;

    if (result == E_SUCCESS)
        a_vList[sIdx] = a_vListElement;

    return result;
}

SINT32 insertToList(CAMutex* a_pMutex, UINT32 a_uListSize, void** a_vList, void* a_vListElement)
{
    if (a_pMutex == NULL)
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;

    while (a_pMutex->lock() != E_SUCCESS) ;
    result = insertToList(a_uListSize, a_vList, a_vListElement);
    while (a_pMutex->unlock() != E_SUCCESS) ;

    return result;
}

SINT32 insertToMIXList(CAMutex* a_pMIXMutex, UINT32 a_uMIXCnt, MIXDATA** a_vMIXData, MIXDATA* a_pMIXData)
{
    return insertToList(a_pMIXMutex, a_uMIXCnt, (void**)a_vMIXData, (void*)a_pMIXData);
}

SINT32 deleteFromMIXList(CAMutex* a_pMIXMutex, UINT32 a_uMIXCnt, MIXDATA** a_vMIXData, UINT32 a_uIdx)
{
    if ((a_pMIXMutex == NULL) || (a_vMIXData == NULL) || (a_uIdx >= a_uMIXCnt))
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;
    MIXDATA* pMIXData = NULL;

    if (a_vMIXData[a_uIdx] == NULL)
        result = E_UNKNOWN;
    else
    {
        while (a_pMIXMutex->lock() != E_SUCCESS) ;

        pMIXData = a_vMIXData[a_uIdx];
        a_vMIXData[a_uIdx] = NULL;

        MIXDATA_free(pMIXData);

/*        ELGAMAL_free(pMIXData->elSignKey);
        ELGAMAL_free(pMIXData->elEncKey);
        delete[] pMIXData->cIP;
        delete pMIXData;*/

        while (a_pMIXMutex->unlock() != E_SUCCESS) ;
    }

    return result;
}

UINT32 getMIXIdx(CAMutex* a_pMIXMutex, UINT32 a_uMIXCnt, MIXDATA** a_vMIXData, UINT8* a_uIP, UINT32 a_uMainPort)
{
    if ((a_vMIXData == NULL) || (a_uIP == NULL))
        return 0xFFFFFFFF;

    UINT32 result = 0xFFFFFFFF;

    while (a_pMIXMutex->lock() != E_SUCCESS) ;
        for (UINT32 i = 0; i < a_uMIXCnt; i++)
            if (a_vMIXData[i] != NULL)
                if (a_vMIXData[i]->uMainPort == a_uMainPort)
                    if (strcmp((char*)a_vMIXData[i]->cIP, (char*)a_uIP) == 0)
                    {
                        result = i;
                        break;
                    }
    while (a_pMIXMutex->unlock() != E_SUCCESS) ;

    return result;
}

SINT32 clearMIXList(CAMutex* a_pMIXMutex, UINT32 a_uMIXCnt, MIXDATA** a_vMIXData)
{
    if (a_pMIXMutex == NULL)
        return E_UNKNOWN;
    if (a_vMIXData == NULL)
        return E_SUCCESS;

    SINT32 result = E_SUCCESS;

    for (UINT32 i = 0; i < a_uMIXCnt; i++)
        if (deleteFromMIXList(a_pMIXMutex, a_uMIXCnt, a_vMIXData, i) != E_SUCCESS)
            result = E_UNKNOWN;

    delete[] a_vMIXData;

    return result;
}

SINT32 initMIXList(MIXDATA*** a_vMIXData, UINT32 a_uMIXCnt)
{
    if (a_vMIXData == NULL)
        return E_UNKNOWN;
    if (*a_vMIXData != NULL)
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;

    if ((*a_vMIXData = new MIXDATA*[a_uMIXCnt]) == NULL)
        result = E_UNKNOWN;
    else
        for (UINT32 i = 0; i < a_uMIXCnt; i++)
            (*a_vMIXData)[i] = NULL;

    return result;
}

SINT32 initMIXList(SINT32& ar_sResult, MIXDATA*** a_vMIXData, UINT32 a_uMIXCnt)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    ar_sResult = initMIXList(a_vMIXData, a_uMIXCnt);

    return ar_sResult;
}

SINT32 getNextPermElement(SINT32& ar_sResult, UINT32& r_uNextElement, UINT32 a_uListSize, UINT32* a_uPermList, UINT32 a_uPrevElement)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;
    if (a_uPermList == NULL)
    {
        ar_sResult = E_UNKNOWN;
        return ar_sResult;
    }

    SINT32 uIdx = getIdx(a_uListSize, a_uPermList, a_uPrevElement);
    if (uIdx < 0)
        ar_sResult = E_UNKNOWN;
    if (uIdx == a_uListSize - 1)
        uIdx = 0;
    else
        uIdx++;

    if (ar_sResult == E_SUCCESS)
        r_uNextElement = a_uPermList[uIdx];

    return ar_sResult;
}

SINT32 getIdx(UINT32 a_uListSize, UINT32* a_uList, UINT32 a_uValue)
{
    if (a_uList == NULL)
        return -1;

    for (UINT32 i = 0; i < a_uListSize; i++)
        if (a_uList[i] == a_uValue)
            return i;

    return -1;
}

SINT32 createRndList(UINT32** r_uList, UINT32 a_uListSize, UINT32 a_uModul)
{
    if (r_uList == NULL) return E_UNKNOWN;
    if (*r_uList != NULL) return E_UNKNOWN;

    SINT32 result = E_SUCCESS;
    UINT32 tmp = 0;
    UINT8 unique = 0;

    if ((*r_uList = new UINT32[a_uListSize]) == NULL)
        result = E_UNKNOWN;
    else
        for (UINT32 i = 0; i < a_uListSize; i++)
            (*r_uList)[i] = 0xFFFFFFFF;

    if (a_uModul >= a_uListSize)
        unique = 1;

    for (UINT32 i = 0; (i < a_uListSize) && (result == E_SUCCESS); i++)
    {
        tmp = rand() % a_uModul;

        while ((unique == 1) && (getIdx(a_uListSize, *r_uList, tmp) >= 0))
            tmp = rand() % a_uModul;

        (*r_uList)[i] = tmp;
    }

    return result;
}

SINT32 createRndList(SINT32& ar_sResult, UINT32** r_uList, UINT32 a_uListSize, UINT32 a_uModul)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    ar_sResult = createRndList(r_uList, a_uListSize, a_uModul);

    return ar_sResult;
}

SINT32 initSocket(CASocket** r_pSocket)
{
    if (r_pSocket == NULL)
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;

    // allowed on NULL
    delete *r_pSocket;

    if ((*r_pSocket = new CASocket()) == NULL)
        result = E_UNKNOWN;
    else
        result = (*r_pSocket)->create();

    return result;
}

SINT32 initSocket(SINT32& ar_sResult, CASocket** r_pSocket)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    ar_sResult = initSocket(r_pSocket);

    return ar_sResult;
}

SINT32 initSocketAddrINet(CASocketAddrINet** r_pAddr)
{
    if (r_pAddr == NULL)
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;

    // allowed on NULL
    delete *r_pAddr;

    if ((*r_pAddr = new CASocketAddrINet()) == NULL)
        result = E_UNKNOWN;

    return result;
}

SINT32 initSocketAddrINet(SINT32& ar_sResult, CASocketAddrINet** r_pAddr)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    ar_sResult = initSocketAddrINet(r_pAddr);

    return ar_sResult;
}

SINT32 initAddrSocket(CASocket** r_pSocket, CASocketAddrINet** r_pAddr, UINT8* a_uIP, UINT16 a_uPort, UINT8 a_uConnect)
{
    SINT32 result = E_SUCCESS;

    initSocket(result, r_pSocket);
    initSocketAddrINet(result, r_pAddr);

    if (result == E_SUCCESS)
        result = (*r_pAddr)->setAddr(a_uIP, a_uPort);

    if ((result == E_SUCCESS) && (a_uConnect != 0))
        result = (*r_pSocket)->connect(**r_pAddr);

    return result;
}

SINT32 initAddrSocket(SINT32& ar_sResult, CASocket** r_pSocket, CASocketAddrINet** r_pAddr, UINT8* a_uIP, UINT16 a_uPort, UINT8 a_uConnect)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    ar_sResult = initAddrSocket(r_pSocket, r_pAddr, a_uIP, a_uPort, a_uConnect);

    return ar_sResult;
}

SINT32 initSocketList(CASocket*** r_pSockets, UINT32 a_uListSize)
{
    if (r_pSockets == NULL) return E_UNKNOWN;
    if (*r_pSockets != NULL) return E_UNKNOWN;

    SINT32 result = E_SUCCESS;

    if ((*r_pSockets = new CASocket*[a_uListSize]) == NULL)
        result = E_UNKNOWN;
    for (UINT32 i = 0; (i < a_uListSize) && (result == E_SUCCESS); i++)
    {
        if (((*r_pSockets)[i] = new CASocket()) == NULL)
            result = E_UNKNOWN;
        else
            result = (*r_pSockets)[i]->create();
    }

    return result;
}

SINT32 initSocketList(SINT32& ar_sResult, CASocket*** r_pSockets, UINT32 a_uListSize)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    ar_sResult = initSocketList(r_pSockets, a_uListSize);

    return ar_sResult;
}

SINT32 clearSocketList(UINT32 a_uListSize, CASocket** a_pSockets)
{
    if (a_pSockets == NULL)
        return E_UNKNOWN;

    for (UINT32 i = 0; i < a_uListSize; i++)
    {
        if (a_pSockets[i] != NULL)
            a_pSockets[i]->close();
        delete a_pSockets[i];
    }

    delete[] a_pSockets;

    return E_SUCCESS;
}

SINT32 initSocketAddrINetList(CASocketAddrINet*** r_pAddrs, UINT32 a_uListSize)
{
    if (r_pAddrs == NULL) return E_UNKNOWN;
    if (*r_pAddrs != NULL) return E_UNKNOWN;

    SINT32 result = E_SUCCESS;

    if ((*r_pAddrs = new CASocketAddrINet*[a_uListSize]) == NULL)
        result = E_UNKNOWN;
    for (UINT32 i = 0; (i < a_uListSize) && (result == E_SUCCESS); i++)
        if (((*r_pAddrs)[i] = new CASocketAddrINet()) == NULL)
            result = E_UNKNOWN;

    return result;
}

SINT32 initSocketAddrINetList(SINT32& ar_sResult, CASocketAddrINet*** r_pAddrs, UINT32 a_uListSize)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    ar_sResult = initSocketAddrINetList(r_pAddrs, a_uListSize);

    return ar_sResult;
}

SINT32 clearSocketAddrINetList(UINT32 a_uListSize, CASocketAddrINet** a_pAddrs)
{
    if (a_pAddrs == NULL)
        return E_SUCCESS;

    for (UINT32 i = 0; i < a_uListSize; i++)
        delete a_pAddrs[i];

    delete[] a_pAddrs;

    return E_SUCCESS;
}
