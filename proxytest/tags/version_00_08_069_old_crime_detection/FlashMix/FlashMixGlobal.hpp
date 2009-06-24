#ifndef __FLASH_MIX_GLOBAL__
#define __FLASH_MIX_GLOBAL__

#include "../StdAfx.h"
#include "../CAMutex.hpp"
#include "../CASocket.hpp"
#include "../CASocketAddrINet.hpp"
#include "elgamal.hpp"

static const UINT32 BB_REQ_USER_DATA        = 0x00000000;
static const UINT32 BB_REQ_INIT_MIX         = 0x00000001;
static const UINT32 BB_REQ_REMOVE_MIX       = 0x00000002;
static const UINT32 BB_REQ_GET_SIGN_KEY     = 0x00000003;
static const UINT32 BB_REQ_GET_ENC_KEY      = 0x00000004;
static const UINT32 BB_REQ_GET_GROUP_KEY    = 0x00000005;
static const UINT32 BB_REQ_UNDEF            = 0xFFFFFFFF;

static const UINT32 MIX_REQ_KEY_SHARE       = 0x00000010;
static const UINT32 MIX_REQ_DUMMY           = 0x00000011;
static const UINT32 MIX_REQ_BLIND           = 0x00000012;
static const UINT32 MIX_REQ_MIX_PROCESS     = 0x00000013;
static const UINT32 MIX_REQ_DECRYPT         = 0x00000014;
static const UINT32 MIX_REQ_UNDEF           = 0xFFFFFFFF;

static const UINT32 PROCESS_SUCCESS         = 0x00000020;
static const UINT32 PROCESS_FINISH          = 0x00000021;
static const UINT32 PROCESS_ERROR           = 0x00000022;
static const UINT32 PROCESS_UNDEF           = 0xFFFFFFFF;

static const UINT32 BLIND_SUCCESS           = 0x00000030;
static const UINT32 BLIND_ERROR             = 0x00000031;
static const UINT32 BLIND_UNDEF             = 0xFFFFFFFF;

static const UINT32 UNBLIND_SUCCESS         = 0x00000040;
static const UINT32 UNBLIND_ERROR           = 0x00000041;
static const UINT32 UNBLIND_UNDEF           = 0xFFFFFFFF;

static const UINT32 USERDATA_SUCCESS        = 0x00000050;
static const UINT32 USERDATA_ERROR          = 0x00000051;
static const UINT32 USERDATA_UNDEF          = 0xFFFFFFFF;

static const UINT32 DECRYPT_SUCCESS         = 0x00000060;
static const UINT32 DECRYPT_ERROR           = 0x00000061;
static const UINT32 DECRYPT_UNDEF           = 0xFFFFFFFF;

typedef struct stMIXData MIXDATA;

struct stMIXData
{
    UINT8* cIP;
    UINT32 uMainPort;
    UINT32 uPortKeyShare;
    UINT32 uPortDummy;
    UINT32 uPortBlind;
    UINT32 uPortDecrypt;
    ELGAMAL* elEncKey;
    ELGAMAL* elSignKey;
};

MIXDATA* MIXDATA_new();
void MIXDATA_free(MIXDATA* a_pMIXData);

// initialize a BIGNUM
SINT32 initBN(BIGNUM** r_bn, UINT32 a_uSetWord = 0);
SINT32 initBN(SINT32& ar_sResult, BIGNUM** r_bn, UINT32 a_uSetWord = 0);
void clearBN(BIGNUM** a_bn);
SINT32 initBNRand(BIGNUM** r_bn, UINT32 a_uBitLength);
SINT32 initBNRand(SINT32& ar_sResult, BIGNUM** r_bn, UINT32 a_uBitLength);
SINT32 initCTX(BN_CTX** r_ctx);
SINT32 initCTX(SINT32& ar_sResult, BN_CTX** r_ctx);
void clearCTX(BN_CTX** a_ctx);

// allocates memory for the list and init all values with NULL
SINT32 initBNList(BIGNUM*** r_bnList, UINT32 a_bnListSize);
SINT32 initBNList(SINT32& ar_sResult, BIGNUM*** r_bnList, UINT32 a_bnListSize);

// frees the complete list of bignum
SINT32 clearBNList(UINT32 a_uListSize, BIGNUM** a_bnList, UINT8 a_uFreeArray = 0);
SINT32 clearBNList(SINT32& ar_sResult, UINT32 a_uListSize, BIGNUM** a_bnList, UINT8 a_uFreeArray = 0);

// create a copy of a bignum list
SINT32 copyBNList(BIGNUM*** a_bnCopy, UINT32 a_uListSize, BIGNUM** a_bnSource);
SINT32 copyBNList(SINT32& ar_sResult, BIGNUM*** a_bnCopy, UINT32 a_uListSize, BIGNUM** a_bnSource);

// returns the first free index to this list
// returns -1 if the list is full or error
SINT32 getFreeIdx(UINT32 a_uListSize, void** a_vList);

SINT32 insertToList(UINT32 a_uListSize, void** a_vList, void* a_vListElement);
SINT32 insertToList(CAMutex* a_pMutex, UINT32 a_uListSize, void** a_vList, void* a_vListElement);

SINT32 insertToMIXList(CAMutex* a_pMIXMutex, UINT32 a_uMIXCnt, MIXDATA** a_vMIXData, MIXDATA* a_pMIXData);
SINT32 deleteFromMIXList(CAMutex* a_pMIXMutex, UINT32 a_uMIXCnt, MIXDATA** a_vMIXData, UINT32 a_uIdx);
UINT32 getMIXIdx(CAMutex* a_pMIXMutex, UINT32 a_uMIXCnt, MIXDATA** a_vMIXData, UINT8* a_uIP, UINT32 a_uMainPort);
SINT32 clearMIXList(CAMutex* a_pMIXMutex, UINT32 a_uMIXCnt, MIXDATA** a_vMIXData);
SINT32 initMIXList(MIXDATA*** a_vMIXData, UINT32 a_uMIXCnt);
SINT32 initMIXList(SINT32& ar_sResult, MIXDATA*** a_vMIXData, UINT32 a_uMIXCnt);

SINT32 getNextPermElement(SINT32& ar_sResult, UINT32& r_uIdx, UINT32 a_uListSize, UINT32* a_uPermList, UINT32 a_uPrevElement);

// returns the index of a_uValue in the list and -1 of it isnt in the list
SINT32 getIdx(UINT32 a_uListSize, UINT32* a_uList, UINT32 a_uValue);
SINT32 createRndList(UINT32** r_uList, UINT32 a_uListSize, UINT32 a_uModul);
SINT32 createRndList(SINT32& ar_sResult, UINT32** r_uList, UINT32 a_uListSize, UINT32 a_uModul);

SINT32 initSocket(CASocket** r_pSocket);
SINT32 initSocket(SINT32& ar_sResult, CASocket** r_pSocket);
SINT32 initSocketAddrINet(CASocketAddrINet** r_pAddr);
SINT32 initSocketAddrINet(SINT32& ar_sResult, CASocketAddrINet** r_pAddr);

SINT32 initAddrSocket(CASocket** r_pSocket, CASocketAddrINet** r_pAddr, UINT8* a_uIP, UINT16 a_uPort, UINT8 a_uConnect = 0);
SINT32 initAddrSocket(SINT32& ar_sResult, CASocket** r_pSocket, CASocketAddrINet** r_pAddr, UINT8* a_uIP, UINT16 a_uPort, UINT8 a_uConnect = 0);

SINT32 initSocketList(CASocket*** r_pSockets, UINT32 a_uListSize);
SINT32 initSocketList(SINT32& ar_sResult, CASocket*** r_pSockets, UINT32 a_uListSize);
SINT32 clearSocketList(UINT32 a_uListSize, CASocket** a_pSockets);
SINT32 initSocketAddrINetList(CASocketAddrINet*** r_pAddrs, UINT32 a_uListSize);
SINT32 initSocketAddrINetList(SINT32& ar_sResult, CASocketAddrINet*** r_pAddrs, UINT32 a_uListSize);
SINT32 clearSocketAddrINetList(UINT32 a_uListSize, CASocketAddrINet** a_pAddrs);

#endif
