#ifndef __CABNSEND__
#define __CABNSEND__

#include "../CASocket.hpp"
#include "elgamal.hpp"

SINT32 sendUINT32(CASocket* a_pClient, UINT32 a_uValue);
SINT32 receiveUINT32(UINT32& r_uValue, CASocket* a_pClient);
SINT32 sendUINT32(SINT32& ar_sResult, CASocket* a_pClient, UINT32 a_uValue);
SINT32 receiveUINT32(SINT32& ar_sResult, UINT32& r_uValue, CASocket* a_pClient);

SINT32 sendPChar(CASocket* a_pClient, UINT8* a_uChar);
SINT32 receivePChar(UINT8** r_uChar, CASocket* a_pClient);
SINT32 sendPChar(SINT32& ar_sResult, CASocket* a_pClient, UINT8* a_uChar);
SINT32 receivePChar(SINT32& ar_sResult, UINT8** r_uChar, CASocket* a_pClient);

SINT32 sendUINT8Arr(CASocket* a_pClient, UINT32 a_uSize, UINT8* a_uArr);
SINT32 receiveUINT8Arr(UINT32& r_uSize, UINT8** r_uArr, CASocket* a_pClient);
SINT32 sendUINT8Arr(SINT32& ar_sResult, CASocket* a_pClient, UINT32 a_uSize, UINT8* a_uArr);
SINT32 receiveUINT8Arr(SINT32& ar_sResult, UINT32& r_uSize, UINT8** r_uArr, CASocket* a_pClient);

SINT32 sendBN(CASocket* a_pClient, BIGNUM* a_bn);
SINT32 receiveBN(BIGNUM** r_bn, CASocket* a_pClient);
SINT32 sendBN(SINT32& ar_sResult, CASocket* a_pClient, BIGNUM* a_bn);
SINT32 receiveBN(SINT32& ar_sResult, BIGNUM** r_bn, CASocket* a_pClient);

SINT32 sendSignedBN(ELGAMAL* a_elSignKey, CASocket* a_pClient, BIGNUM* a_bn);
SINT32 receiveSignedBN(BIGNUM** r_bn, ELGAMAL* a_elTestKey, CASocket* a_pClient);
SINT32 sendSignedBN(SINT32& ar_sResult, ELGAMAL* a_elSignKey, CASocket* a_pClient, BIGNUM* a_bn);
SINT32 receiveSignedBN(SINT32& ar_sResult, BIGNUM** r_bn, ELGAMAL* a_elTestKey, CASocket* a_pClient);

SINT32 sendEncSignedBN(ELGAMAL* a_elSignKey, ELGAMAL* a_elEncKey, CASocket* a_pClient, BIGNUM* a_bn);
SINT32 receiveEncSignedBN(BIGNUM** r_bn, ELGAMAL* a_elTestKey, ELGAMAL* a_elDecryptKey, CASocket* a_pClient);
SINT32 sendEncSignedBN(SINT32& ar_sResult, ELGAMAL* a_elSignKey, ELGAMAL* a_elEncKey, CASocket* a_pClient, BIGNUM* a_bn);
SINT32 receiveEncSignedBN(SINT32& ar_sResult, BIGNUM** r_bn, ELGAMAL* a_elTestKey, ELGAMAL* a_elDecryptKey, CASocket* a_pClient);

SINT32 sendPublicKey(CASocket* a_pClient, ELGAMAL* a_elKey);
SINT32 receivePublicKey(ELGAMAL** r_elKey, CASocket* a_pClient);
SINT32 sendPublicKey(SINT32& ar_sResult, CASocket* a_pClient, ELGAMAL* a_elKey);
SINT32 receivePublicKey(SINT32& ar_sResult, ELGAMAL** r_elKey, CASocket* a_pClient);

SINT32 sendSignedPublicKey(ELGAMAL* a_elSignKey, CASocket* a_pClient, ELGAMAL* a_elKey);
SINT32 receiveSignedPublicKey(ELGAMAL** r_elKey, ELGAMAL* a_elTestKey, CASocket* a_pClient);
SINT32 sendSignedPublicKey(SINT32& ar_sResult, ELGAMAL* a_elSignKey, CASocket* a_pClient, ELGAMAL* a_elKey);
SINT32 receiveSignedPublicKey(SINT32& ar_sResult, ELGAMAL** r_elKey, ELGAMAL* a_elTestKey, CASocket* a_pClient);

SINT32 sendSignedBNArray(ELGAMAL* a_elSignKey, CASocket* a_pClient, UINT32 a_uBNArrLength, BIGNUM** a_bnArr);
SINT32 receiveSignedBNArray(UINT32& r_uBNArrLength, BIGNUM*** r_bnArr, ELGAMAL* a_elTestKey, CASocket* a_pClient);
SINT32 sendSignedBNArray(SINT32& ar_sResult, ELGAMAL* a_elSignKey, CASocket* a_pClient, UINT32 a_uBNArrLength, BIGNUM** a_bnArr);
SINT32 receiveSignedBNArray(SINT32& ar_sResult, UINT32& r_uBNArrLength, BIGNUM*** r_bnArr, ELGAMAL* a_elTestKey, CASocket* a_pClient);

SINT32 BN_bnArr2pCharArr(UINT8*** r_uPCharArray, UINT32 a_bnArrSize, BIGNUM** a_bnArr);
SINT32 BN_bnArr2pCharArr(SINT32& ar_sResult, UINT8*** r_uPCharArray, UINT32 a_bnArrSize, BIGNUM** a_bnArr);
SINT32 BN_pCharArr2bnArr(BIGNUM*** r_bnArr, UINT32 a_bnArrSize, UINT8** a_uPChar);
SINT32 BN_pCharArr2bnArr(SINT32& ar_sResult, BIGNUM*** r_bnArr, UINT32 a_bnArrSize, UINT8** a_uPChar);
SINT32 pCharArr2pChar(UINT8** r_uPChar, UINT32 a_uArrSize, UINT8** a_uPCharArr);
SINT32 pCharArr2pChar(SINT32& ar_sResult, UINT8** r_uPChar, UINT32 a_uArrSize, UINT8** a_uPCharArr);

SINT32 BN_bnArr2UINT8Arr(UINT32& r_uSize, UINT8** r_uArr, UINT32 a_uBNArrSize, BIGNUM** a_bnArr);
SINT32 BN_bnArr2UINT8Arr(SINT32& ar_sResult, UINT32& r_uSize, UINT8** r_uArr, UINT32 a_uBNArrSize, BIGNUM** a_bnArr);

#endif // __CABNSEND__
