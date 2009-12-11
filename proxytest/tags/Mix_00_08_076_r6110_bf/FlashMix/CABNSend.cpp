#include "../StdAfx.h"
#include "CABNSend.hpp"

// send and receive UINT32
SINT32 sendUINT32(CASocket* a_pClient, UINT32 a_uValue)
{
    if (a_pClient == NULL)
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;

    result = a_pClient->sendFully((UINT8*)&a_uValue, sizeof(UINT32));

    return result;
}

SINT32 receiveUINT32(UINT32& r_uValue, CASocket* a_pClient)
{
    if (a_pClient == NULL)
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;
    UINT32 uValue = 0;

    result = a_pClient->receiveFully((UINT8*)&uValue, sizeof(UINT32));
    r_uValue = uValue;

    return result;
}

SINT32 sendUINT32(SINT32& ar_sResult, CASocket* a_pClient, UINT32 a_uValue)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    ar_sResult = sendUINT32(a_pClient, a_uValue);

    return ar_sResult;
}

SINT32 receiveUINT32(SINT32& ar_sResult, UINT32& r_uValue, CASocket* a_pClient)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    ar_sResult = receiveUINT32(r_uValue, a_pClient);

    return ar_sResult;
}




// send and receive PChar
SINT32 sendPChar(CASocket* a_pClient, UINT8* a_uChar)
{
    if ((a_pClient == NULL) || (a_uChar == NULL))
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;
    UINT32 uCLength = 0;

    uCLength = strlen((char*)a_uChar);

    sendUINT32(result, a_pClient, uCLength);

    if (result == E_SUCCESS)
        result = a_pClient->sendFully(a_uChar, uCLength);

    return result;
}

SINT32 receivePChar(UINT8** r_uChar, CASocket* a_pClient)
{
    if ((a_pClient == NULL) || (r_uChar == NULL))
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;
    UINT32 uCLength = 0;

    if (*r_uChar != NULL)
        delete[] *r_uChar;

    receiveUINT32(result, uCLength, a_pClient);

    if (result == E_SUCCESS)
        if ((*r_uChar = new UINT8[uCLength + 1]) == NULL)
            result = E_UNKNOWN;

    if (result == E_SUCCESS)
        result = a_pClient->receiveFully(*r_uChar, uCLength);

    if (result == E_SUCCESS)
        (*r_uChar)[uCLength] = 0;

    return result;
}

SINT32 sendPChar(SINT32& ar_sResult, CASocket* a_pClient, UINT8* a_uChar)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    ar_sResult = sendPChar(a_pClient, a_uChar);

    return ar_sResult;
}

SINT32 receivePChar(SINT32& ar_sResult, UINT8** r_uChar, CASocket* a_pClient)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    ar_sResult = receivePChar(r_uChar, a_pClient);

    return ar_sResult;
}



// send and receive UINT8*
SINT32 sendUINT8Arr(CASocket* a_pClient, UINT32 a_uSize, UINT8* a_uArr)
{
    if ((a_pClient == NULL) || (a_uArr == NULL))
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;

    sendUINT32(result, a_pClient, a_uSize);

    if (result == E_SUCCESS)
        result = a_pClient->sendFully(a_uArr, a_uSize);

    return result;
}

SINT32 receiveUINT8Arr(UINT32& r_uSize, UINT8** r_uArr, CASocket* a_pClient)
{
    if ((a_pClient == NULL) || (r_uArr == NULL))
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;

    if (*r_uArr != NULL)
       delete[] *r_uArr;

    receiveUINT32(result, r_uSize, a_pClient);

    if (result == E_SUCCESS)
       if ((*r_uArr = new UINT8[r_uSize]) == NULL)
           result = E_UNKNOWN;

    if (result == E_SUCCESS)
       result = a_pClient->receiveFully(*r_uArr, r_uSize);

    return result;
}

SINT32 sendUINT8Arr(SINT32& ar_sResult, CASocket* a_pClient,
    UINT32 a_uSize, UINT8* a_uArr)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    ar_sResult = sendUINT8Arr(a_pClient, a_uSize, a_uArr);

    return ar_sResult;
}

SINT32 receiveUINT8Arr(SINT32& ar_sResult, UINT32& r_uSize,
    UINT8** r_uArr, CASocket* a_pClient)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    ar_sResult = receiveUINT8Arr(r_uSize, r_uArr, a_pClient);

    return ar_sResult;
}



// send and receive BN
SINT32 sendBN(CASocket* a_pClient, BIGNUM* a_bn)
{
    if ((a_pClient == NULL) || (a_bn == NULL))
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;
    UINT32 uSize = 0;
    UINT8* uBN = NULL;

    uSize = BN_num_bytes(a_bn);
    if ((uBN = new UINT8[uSize]) == NULL)
        result = E_UNKNOWN;

    if (result == E_SUCCESS)
        if (BN_bn2bin(a_bn, (unsigned char*)uBN) != uSize)
            result = E_UNKNOWN;

    sendUINT8Arr(result, a_pClient, uSize, uBN);

    delete[] uBN;

    return result;

    /*SINT32 result = E_SUCCESS;
    UINT8* uC = NULL;

    if ((uC = (UINT8*)BN_bn2hex(a_bn)) == NULL)
        result = E_UNKNOWN;

    sendPChar(result, a_pClient, uC);

    return result;*/
}

SINT32 receiveBN(BIGNUM** r_bn, CASocket* a_pClient)
{
    if ((r_bn == NULL) || (a_pClient == NULL))
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;
    UINT32 uSize = 0;
    UINT8* uBN = NULL;

    if (*r_bn == NULL)
    {
        if ((*r_bn = BN_new()) == NULL)
            result = E_UNKNOWN;
        else
            BN_init(*r_bn);
    }

    receiveUINT8Arr(result, uSize, &uBN, a_pClient);

    if (result == E_SUCCESS)
        if (BN_bin2bn((unsigned char*)uBN, uSize, *r_bn) == NULL)
            result = E_UNKNOWN;

    delete[] uBN;

    return result;


    /*SINT32 result = E_SUCCESS;
    UINT8* uC = NULL;

    receivePChar(result, &uC, a_pClient);

    if (result == E_SUCCESS)
        if (BN_hex2bn(r_bn, (char*)uC) == 0)
            result = E_UNKNOWN;

    if (uC != NULL)
        delete[] uC;

    return result;*/
}

SINT32 sendBN(SINT32& ar_sResult, CASocket* a_pClient, BIGNUM* a_bn)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    ar_sResult = sendBN(a_pClient, a_bn);

    return ar_sResult;
}

SINT32 receiveBN(SINT32& ar_sResult, BIGNUM** r_bn, CASocket* a_pClient)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    ar_sResult = receiveBN(r_bn, a_pClient);

    return ar_sResult;
}







// send and receive signed BN
SINT32 sendSignedBN(ELGAMAL* a_elSignKey, CASocket* a_pClient, BIGNUM* a_bn)
{
    if ((ELGAMAL_isValidPrivateKey(a_elSignKey) == 0) || (a_pClient == NULL) ||
        (a_bn == NULL))
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;
    BIGNUM* bnSignS = NULL;
    BIGNUM* bnSignK = NULL;

    if (ELGAMAL_sign(&bnSignS, &bnSignK, a_elSignKey, a_bn) == 0)
        result = E_UNKNOWN;

    sendBN(result, a_pClient, a_bn);
    sendBN(result, a_pClient, bnSignS);
    sendBN(result, a_pClient, bnSignK);

    if (bnSignS != NULL) BN_free(bnSignS);
    if (bnSignK != NULL) BN_free(bnSignK);

    return result;
}

SINT32 receiveSignedBN(BIGNUM** r_bn, ELGAMAL* a_elTestKey, CASocket* a_pClient)
{
    if ((r_bn == NULL) || (ELGAMAL_isValidPublicKey(a_elTestKey) == 0) ||
        (a_pClient == NULL))
        return E_UNKNOWN;

    if (*r_bn != NULL)
    {
        BN_free(*r_bn);
        *r_bn = NULL;
    }

    SINT32 result = E_SUCCESS;
    BIGNUM* bnSignS = NULL;
    BIGNUM* bnSignK = NULL;

    receiveBN(result, r_bn, a_pClient);
    receiveBN(result, &bnSignS, a_pClient);
    receiveBN(result, &bnSignK, a_pClient);

    if (result == E_SUCCESS)
        if (ELGAMAL_test_signature(a_elTestKey, *r_bn, bnSignS, bnSignK) == 0)
            result = E_UNKNOWN;

    if (bnSignS != NULL) BN_free(bnSignS);
    if (bnSignK != NULL) BN_free(bnSignK);

    return result;
}

SINT32 sendSignedBN(SINT32& ar_sResult, ELGAMAL* a_elSignKey, CASocket* a_pClient, BIGNUM* a_bn)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    ar_sResult = sendSignedBN(a_elSignKey, a_pClient, a_bn);

    return ar_sResult;
}

SINT32 receiveSignedBN(SINT32& ar_sResult, BIGNUM** r_bn, ELGAMAL* a_elTestKey, CASocket* a_pClient)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    ar_sResult = receiveSignedBN(r_bn, a_elTestKey, a_pClient);

    return ar_sResult;
}






// send and receive encrypted public key
SINT32 sendEncSignedBN(ELGAMAL* a_elSignKey, ELGAMAL* a_elEncKey, CASocket* a_pClient, BIGNUM* a_bn)
{
    if ((ELGAMAL_isValidPrivateKey(a_elSignKey) == 0) || (a_pClient == NULL) ||
        (ELGAMAL_isValidPublicKey(a_elSignKey) == 0) || (a_bn == NULL))
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;
    BIGNUM** vBNEnc = NULL;

    if ((vBNEnc = new BIGNUM*[2]) == NULL)
        result = E_UNKNOWN;
    else
    {
        vBNEnc[0] = NULL;   // enc A
        vBNEnc[1] = NULL;   // enc B
    }

    if (result == E_SUCCESS)
        if (ELGAMAL_encrypt(&(vBNEnc[0]), &(vBNEnc[1]), a_elEncKey, a_bn) == 0)
            result = E_UNKNOWN;

    sendSignedBNArray(result, a_elSignKey, a_pClient, 2, vBNEnc);

    if (vBNEnc != NULL)
    {
        if (vBNEnc[0] != NULL) BN_free(vBNEnc[0]);
        if (vBNEnc[1] != NULL) BN_free(vBNEnc[1]);
    }
    delete[] vBNEnc;

    return result;
}

SINT32 receiveEncSignedBN(BIGNUM** r_bn, ELGAMAL* a_elTestKey, ELGAMAL* a_elDecryptKey, CASocket* a_pClient)
{
    if ((ELGAMAL_isValidPublicKey(a_elTestKey) == 0) || (r_bn == NULL) ||
        (ELGAMAL_isValidPrivateKey(a_elDecryptKey) == 0) || (a_pClient == NULL))
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;
    BIGNUM** vBNEnc = NULL;
    UINT32 uLength = 0;

    receiveSignedBNArray(result, uLength, &vBNEnc, a_elTestKey, a_pClient);

    if (uLength != 2)
        result = E_UNKNOWN;

    if (result == E_SUCCESS)
        if (ELGAMAL_decrypt(r_bn, a_elDecryptKey, vBNEnc[0], vBNEnc[1]) == 0)
            result = E_UNKNOWN;

    for (UINT32 i = 0; (i < uLength) && (vBNEnc != NULL); i++)
        if (vBNEnc[i] != NULL)
            BN_free(vBNEnc[i]);
    delete[] vBNEnc;

    return result;
}

SINT32 sendEncSignedBN(SINT32& ar_sResult, ELGAMAL* a_elSignKey, ELGAMAL* a_elEncKey, CASocket* a_pClient, BIGNUM* a_bn)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    ar_sResult = sendEncSignedBN(a_elSignKey, a_elEncKey, a_pClient, a_bn);

    return ar_sResult;
}

SINT32 receiveEncSignedBN(SINT32& ar_sResult, BIGNUM** r_bn, ELGAMAL* a_elTestKey, ELGAMAL* a_elDecryptKey, CASocket* a_pClient)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    ar_sResult = receiveEncSignedBN(r_bn, a_elTestKey, a_elDecryptKey, a_pClient);

    return ar_sResult;
}








// send and receive public key
SINT32 sendPublicKey(CASocket* a_pClient, ELGAMAL* a_elKey)
{
    if ((a_pClient == NULL) || (ELGAMAL_isValidPublicKey(a_elKey) == 0))
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;

    sendBN(result, a_pClient, a_elKey->p);
    sendBN(result, a_pClient, a_elKey->pm1);
    sendBN(result, a_pClient, a_elKey->q);
    sendBN(result, a_pClient, a_elKey->g);
    sendBN(result, a_pClient, a_elKey->y);

    return result;
}

SINT32 receivePublicKey(ELGAMAL** r_elKey, CASocket* a_pClient)
{
    if ((a_pClient == NULL) || (r_elKey == NULL))
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;

    if (*r_elKey != NULL)
        ELGAMAL_free(*r_elKey);

    *r_elKey = (ELGAMAL*)OPENSSL_malloc(sizeof(ELGAMAL));
    memset(*r_elKey, 0, sizeof(ELGAMAL));

    receiveBN(result, &(*r_elKey)->p, a_pClient);
    receiveBN(result, &(*r_elKey)->pm1, a_pClient);
    receiveBN(result, &(*r_elKey)->q, a_pClient);
    receiveBN(result, &(*r_elKey)->g, a_pClient);
    receiveBN(result, &(*r_elKey)->y, a_pClient);

    if (result == E_SUCCESS)
    {
        if (((*r_elKey)->x = BN_new()) == NULL)
            result = E_UNKNOWN;
        else
        {
            BN_init((*r_elKey)->x);
            BN_zero((*r_elKey)->x);
        }
    }

    return result;
}

SINT32 sendPublicKey(SINT32& ar_sResult, CASocket* a_pClient, ELGAMAL* a_elKey)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    ar_sResult = sendPublicKey(a_pClient, a_elKey);

    return ar_sResult;
}

SINT32 receivePublicKey(SINT32& ar_sResult, ELGAMAL** r_elKey, CASocket* a_pClient)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    ar_sResult = receivePublicKey(r_elKey, a_pClient);

    return ar_sResult;
}



// send and receive signed Public keys
SINT32 sendSignedPublicKey(ELGAMAL* a_elSignKey, CASocket* a_pClient, ELGAMAL* a_elKey)
{
    if ((ELGAMAL_isValidPrivateKey(a_elSignKey) == 0) || (a_pClient == NULL) ||
        (ELGAMAL_isValidPublicKey(a_elKey) == 0))
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;
    BIGNUM** bnArr = NULL;

    if ((bnArr = new BIGNUM*[5]) == NULL)
        result = E_UNKNOWN;

    if (result == E_SUCCESS)
    {
        bnArr[0] = a_elKey->p;
        bnArr[1] = a_elKey->pm1;
        bnArr[2] = a_elKey->q;
        bnArr[3] = a_elKey->g;
        bnArr[4] = a_elKey->y;
    }

    sendSignedBNArray(result, a_elSignKey, a_pClient, 5, bnArr);

    delete[] bnArr;

    return result;
}

SINT32 receiveSignedPublicKey(ELGAMAL** r_elKey, ELGAMAL* a_elTestKey, CASocket* a_pClient)
{
    if ((ELGAMAL_isValidPublicKey(a_elTestKey) == 0) || (a_pClient == NULL) ||
        (*r_elKey != NULL))
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;
    BIGNUM** bnArr = NULL;
    UINT32 uArrLength = 0;

    if ((*r_elKey = (ELGAMAL*)OPENSSL_malloc(sizeof(ELGAMAL))) == NULL)
        result = E_SUCCESS;

    receiveSignedBNArray(result, uArrLength, &bnArr, a_elTestKey, a_pClient);

    if (result == E_SUCCESS)
    {
        (*r_elKey)->p = bnArr[0];
        (*r_elKey)->pm1 = bnArr[1];
        (*r_elKey)->q = bnArr[2];
        (*r_elKey)->g = bnArr[3];
        (*r_elKey)->y = bnArr[4];
        if (((*r_elKey)->x = BN_new()) == NULL)
            result = E_UNKNOWN;
        else
        {
            BN_init((*r_elKey)->x);
            BN_zero((*r_elKey)->x);
        }
    }

    delete[] bnArr;

    return result;
}

SINT32 sendSignedPublicKey(SINT32& ar_sResult, ELGAMAL* a_elSignKey, CASocket* a_pClient, ELGAMAL* a_elKey)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    ar_sResult = sendSignedPublicKey(a_elSignKey, a_pClient, a_elKey);

    return ar_sResult;
}

SINT32 receiveSignedPublicKey(SINT32& ar_sResult, ELGAMAL** r_elKey, ELGAMAL* a_elTestKey, CASocket* a_pClient)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    ar_sResult = receiveSignedPublicKey(r_elKey, a_elTestKey, a_pClient);

    return ar_sResult;
}





// send and receive BN arrays
SINT32 sendSignedBNArray(ELGAMAL* a_elSignKey, CASocket* a_pClient, UINT32 a_uBNArrLength, BIGNUM** a_bnArr)
{
    if ((ELGAMAL_isValidPrivateKey(a_elSignKey) == 0) || (a_pClient == NULL) ||
        (a_bnArr == NULL))
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;
    UINT8* uBNArr = NULL;
    UINT32 uSize = 0;
    UINT8* uHash = NULL;
    BIGNUM* bnHash = NULL;
    BIGNUM* bnSignS = NULL;
    BIGNUM* bnSignK = NULL;

    if ((uHash = new UINT8[20]) == NULL)
        result = E_UNKNOWN;
    if ((bnHash = BN_new()) == NULL)
        result = E_UNKNOWN;
    else
        BN_init(bnHash);

    BN_bnArr2UINT8Arr(result, uSize, &uBNArr, a_uBNArrLength, a_bnArr);

    if (result == E_SUCCESS)
        SHA1((unsigned char*)uBNArr, uSize, (unsigned char*)uHash);

    // transform Hash-Value into BIGNUM for sign
    if (result == E_SUCCESS)
        if (BN_bin2bn((unsigned char*)uHash, 20, bnHash) == NULL)
            result = E_UNKNOWN;

    bnSignS = bnSignK = NULL;
    if (result == E_SUCCESS)
        if (ELGAMAL_sign(&bnSignS, &bnSignK, a_elSignKey, bnHash) == 0)
            result = E_UNKNOWN;

    // send array
    sendUINT32(result, a_pClient, a_uBNArrLength);
    for (UINT32 i = 0; (i < a_uBNArrLength) && (result == E_SUCCESS); i++)
        result = sendBN(a_pClient, a_bnArr[i]);

//    sendBN(result, a_pClient, bnHash);
    sendBN(result, a_pClient, bnSignS);
    sendBN(result, a_pClient, bnSignK);

    if (bnSignS != NULL) BN_free(bnSignS);
    if (bnSignK != NULL) BN_free(bnSignK);
    if (bnHash != NULL) BN_free(bnHash);
    delete[] uHash;
    delete[] uBNArr;

    return result;



/*    SINT32 result = E_SUCCESS;
    UINT8* uC = NULL;
    UINT8** uBNChar = NULL;
    BIGNUM* bnHash = NULL;
    BIGNUM* bnSignS = NULL;
    BIGNUM* bnSignK = NULL;
    UINT8* upHash = NULL;

    BN_bnArr2pCharArr(result, &uBNChar, a_uBNArrLength, a_bnArr);
    pCharArr2pChar(result, &uC, a_uBNArrLength, uBNChar);

    if ((upHash = new UINT8[20]) == NULL)
        result = E_UNKNOWN;

    if (result == E_SUCCESS)
    {
        SHA1((unsigned char*)uC, strlen((char*)uC), (unsigned char*)upHash);
        if ((bnHash = BN_new()) == NULL)
            result = E_UNKNOWN;
        else
        {
            BN_init(bnHash);
            BN_rand(bnHash, 160, 1, 0);
            memset(bnHash->d, 0, bnHash->dmax * sizeof(BN_ULONG));
            memcpy(bnHash->d, upHash, 20);
        }
    }

    bnSignS = bnSignK = NULL;
    if (result == E_SUCCESS)
        if (ELGAMAL_sign(&bnSignS, &bnSignK, a_elSignKey, bnHash) == 0)
            result = E_UNKNOWN;

    // send array size
    sendUINT32(result, a_pClient, a_uBNArrLength);
    // send array
    for (UINT32 i = 0; (i < a_uBNArrLength) && (result == E_SUCCESS); i++)
        sendPChar(result, a_pClient, uBNChar[i]);

    // send hash
    sendBN(result, a_pClient, bnHash);
    sendBN(result, a_pClient, bnSignS);
    sendBN(result, a_pClient, bnSignK);

    for (UINT32 i = 0; (i < a_uBNArrLength) && (uBNChar != NULL); i++)
        delete[] uBNChar[i];
    delete[] uBNChar;
    if (bnHash != NULL) BN_free(bnHash);
    if (bnSignS != NULL) BN_free(bnSignS);
    if (bnSignK != NULL) BN_free(bnSignK);
    delete[] upHash;
    delete[] uC;

    return result;*/
}

SINT32 receiveSignedBNArray(UINT32& r_uBNArrLength, BIGNUM*** r_bnArr,
    ELGAMAL* a_elTestKey, CASocket* a_pClient)
{
    if ((*r_bnArr != NULL) || (ELGAMAL_isValidPublicKey(a_elTestKey) == 0) ||
        (a_pClient == NULL))
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;
    UINT8* uBNArr = NULL;
    UINT32 uSize = 0;
    UINT8* uHash = NULL;
    BIGNUM* bnHash = NULL;
    BIGNUM* bnSignS = NULL;
    BIGNUM* bnSignK = NULL;

    if ((uHash = new UINT8[20]) == NULL)
        result = E_UNKNOWN;
    if ((bnHash = BN_new()) == NULL)
        result = E_UNKNOWN;
    else
        BN_init(bnHash);

    receiveUINT32(result, r_uBNArrLength, a_pClient);
    if (result == E_SUCCESS)
        if ((*r_bnArr = new BIGNUM*[r_uBNArrLength]) == NULL)
            result = E_UNKNOWN;
    // receive array
    for (UINT32 i = 0; (i < r_uBNArrLength) && (result == E_SUCCESS); i++)
    {
        (*r_bnArr)[i] = NULL;
        result = receiveBN(&((*r_bnArr)[i]), a_pClient);
    }

    // receive signature
    receiveBN(result, &bnSignS, a_pClient);
    receiveBN(result, &bnSignK, a_pClient);

    BN_bnArr2UINT8Arr(result, uSize, &uBNArr, r_uBNArrLength, *r_bnArr);

    if (result == E_SUCCESS)
        SHA1((unsigned char*)uBNArr, uSize, (unsigned char*)uHash);

    // transform Hash-Value into BIGNUM for sign
    if (result == E_SUCCESS)
        if (BN_bin2bn((unsigned char*)uHash, 20, bnHash) == NULL)
            result = E_UNKNOWN;

    if (result == E_SUCCESS)
        if (ELGAMAL_test_signature(a_elTestKey, bnHash, bnSignS, bnSignK) == 0)
            result = E_UNKNOWN;

    delete[] uBNArr;
    delete[] uHash;
    if (bnHash != NULL) BN_free(bnHash);
    if (bnSignS != NULL) BN_free(bnSignS);
    if (bnSignK != NULL) BN_free(bnSignK);

    return result;

    /*SINT32 result = E_SUCCESS;
    UINT8** uBNChar = NULL;
    UINT8* uC = NULL;
    BIGNUM* bnHash = NULL;
    BIGNUM* bnSignS = NULL;
    BIGNUM* bnSignK = NULL;
    UINT8* upHash = NULL;

    receiveUINT32(result, r_uBNArrLength, a_pClient);
    if ((uBNChar = new (UINT8*)[r_uBNArrLength]) == NULL)
        result = E_UNKNOWN;
    else
        for (UINT32 i = 0; i < r_uBNArrLength; i++)
            uBNChar[i] = NULL;

    for (UINT32 i = 0; (i < r_uBNArrLength) && (result == E_SUCCESS); i++)
        receivePChar(result, &(uBNChar[i]), a_pClient);

    receiveBN(result, &bnHash, a_pClient);
    receiveBN(result, &bnSignS, a_pClient);
    receiveBN(result, &bnSignK, a_pClient);

    pCharArr2pChar(result, &uC, r_uBNArrLength, uBNChar);

    if ((upHash = new UINT8[20]) == NULL)
        result = E_UNKNOWN;

    if (result == E_SUCCESS)
    {
        SHA1((unsigned char*)uC, strlen((char*)uC), (unsigned char*)upHash);
        for (UINT32 i = 0; (i < 20) && (result == E_SUCCESS); i++)
            if (upHash[i] != ((UINT8*)bnHash->d)[i])
                result = E_UNKNOWN;
    }

    if (result == E_SUCCESS)
        if (ELGAMAL_test_signature(a_elTestKey, bnHash, bnSignS, bnSignK) == 0)
            result = E_UNKNOWN;

    BN_pCharArr2bnArr(result, r_bnArr, r_uBNArrLength, uBNChar);

    for (UINT32 i = 0; (i < r_uBNArrLength) && (uBNChar != NULL); i++)
        delete[] uBNChar[i];
    delete[] uBNChar;
    delete[] uC;
    if (bnHash != NULL) BN_free(bnHash);
    if (bnSignS != NULL) BN_free(bnSignS);
    if (bnSignK != NULL) BN_free(bnSignK);
    delete[] upHash;
    return result;*/
}

SINT32 sendSignedBNArray(SINT32& ar_sResult, ELGAMAL* a_elSignKey, CASocket* a_pClient, UINT32 a_uBNArrLength, BIGNUM** a_bnArr)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    ar_sResult = sendSignedBNArray(a_elSignKey, a_pClient, a_uBNArrLength, a_bnArr);

    return ar_sResult;
}

SINT32 receiveSignedBNArray(SINT32& ar_sResult, UINT32& r_uBNArrLength, BIGNUM*** r_bnArr, ELGAMAL* a_elTestKey, CASocket* a_pClient)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    ar_sResult = receiveSignedBNArray(r_uBNArrLength, r_bnArr, a_elTestKey, a_pClient);

    return ar_sResult;
}





// normal used functions
SINT32 BN_bnArr2pCharArr(UINT8*** r_uPCharArray, UINT32 a_bnArrSize, BIGNUM** a_bnArr)
{
    if ((r_uPCharArray == NULL) || (a_bnArr == NULL) || (*r_uPCharArray != NULL))
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;

    if ((*r_uPCharArray = new UINT8*[a_bnArrSize]) == NULL)
        result = E_UNKNOWN;

    for (UINT32 i = 0; (i < a_bnArrSize) && (result == E_SUCCESS); i++)
    {
        if (a_bnArr[i] == NULL)
        {
            result = E_UNKNOWN;
            continue;
        }
        if (((*r_uPCharArray)[i] = (UINT8*)BN_bn2hex(a_bnArr[i])) == NULL)
            result = E_UNKNOWN;
    }

    return result;
}

SINT32 BN_bnArr2pCharArr(SINT32& ar_sResult, UINT8*** r_uPCharArray, UINT32 a_bnArrSize, BIGNUM** a_bnArr)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    ar_sResult = BN_bnArr2pCharArr(r_uPCharArray, a_bnArrSize, a_bnArr);

    return ar_sResult;
}

SINT32 BN_pCharArr2bnArr(BIGNUM*** r_bnArr, UINT32 a_bnArrSize, UINT8** a_uPChar)
{
    if ((*r_bnArr != NULL) || (a_uPChar == NULL))
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;

    if ((*r_bnArr = new BIGNUM*[a_bnArrSize]) == NULL)
        result = E_UNKNOWN;

    for (UINT32 i = 0; (i < a_bnArrSize) && (result == E_SUCCESS); i++)
    {
        (*r_bnArr)[i] = NULL;
        if (BN_hex2bn(&((*r_bnArr)[i]), (char*)a_uPChar[i]) == 0)
        {
            (*r_bnArr)[i] = NULL;
            result = E_UNKNOWN;
        }
    }

    if ((result != E_SUCCESS) && (*r_bnArr != NULL))
    {
        for (UINT32 i = 0; i < a_bnArrSize; i++)
            if ((*r_bnArr)[i] != NULL)
                BN_clear_free((*r_bnArr)[i]);
        delete[] *r_bnArr;
        *r_bnArr = NULL;
    }

    return result;
}

SINT32 BN_pCharArr2bnArr(SINT32& ar_sResult, BIGNUM*** r_bnArr, UINT32 a_bnArrSize, UINT8** a_uPChar)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    ar_sResult = BN_pCharArr2bnArr(r_bnArr, a_bnArrSize, a_uPChar);

    return ar_sResult;
}


SINT32 pCharArr2pChar(UINT8** r_uPChar, UINT32 a_uArrSize, UINT8** a_uPCharArr)
{
    if ((r_uPChar == NULL) || (a_uPCharArr == NULL))
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;
    UINT32 uCLength = 0;
    UINT32 uCOffset = 0;
    UINT32 uLen = 0;

    if (*r_uPChar != NULL)
        delete[] *r_uPChar;

    for (UINT32 i = 0; i < a_uArrSize; i++)
        uCLength += strlen((char*)a_uPCharArr[i]);

    if ((*r_uPChar = new UINT8[uCLength + 1]) == NULL)
        result = E_UNKNOWN;

    uCOffset = 0;
    for (UINT32 i = 0; (i < a_uArrSize) && (result == E_SUCCESS); i++)
    {
        uLen = strlen((char*)a_uPCharArr[i]);
        memcpy(*r_uPChar + uCOffset, a_uPCharArr[i], uLen);
        uCOffset += uLen;
    }

    if (result == E_SUCCESS)
        (*r_uPChar)[uCLength] = 0;
    else
        delete[] *r_uPChar;

    return result;
}

SINT32 pCharArr2pChar(SINT32& ar_sResult, UINT8** r_uPChar, UINT32 a_uArrSize, UINT8** a_uPCharArr)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    ar_sResult = pCharArr2pChar(r_uPChar, a_uArrSize, a_uPCharArr);

    return ar_sResult;
}

SINT32 BN_bnArr2UINT8Arr(UINT32& r_uSize, UINT8** r_uArr,
    UINT32 a_uBNArrSize, BIGNUM** a_bnArr)
{
    if ((r_uArr == NULL) || (a_bnArr == NULL))
        return E_UNKNOWN;

    SINT32 result = E_SUCCESS;
    UINT32 uOffset = 0;

    if (*r_uArr != NULL)
        delete[] *r_uArr;

    r_uSize = 0;
    for (UINT32 i = 0; (i < a_uBNArrSize) && (result == E_SUCCESS); i++)
    {
        if (a_bnArr[i] == NULL)
            result = E_UNKNOWN;
        else
            r_uSize += BN_num_bytes(a_bnArr[i]);
    }

    if ((*r_uArr = new UINT8[r_uSize]) == NULL)
        result = E_UNKNOWN;

    uOffset = 0;
    for (UINT32 i = 0; (i < a_uBNArrSize) && (result == E_SUCCESS); i++)
    {
        BN_bn2bin(a_bnArr[i], ((unsigned char*)*r_uArr) + uOffset);
        uOffset += BN_num_bytes(a_bnArr[i]);
    }

    if (result != E_SUCCESS)
    {
        delete[] *r_uArr;
        *r_uArr = NULL;
    }

    return result;
}

SINT32 BN_bnArr2UINT8Arr(SINT32& ar_sResult, UINT32& r_uSize,
    UINT8** r_uArr, UINT32 a_uBNArrSize, BIGNUM** a_bnArr)
{
    if (ar_sResult != E_SUCCESS)
        return ar_sResult;

    ar_sResult = BN_bnArr2UINT8Arr(r_uSize, r_uArr, a_uBNArrSize, a_bnArr);

    return ar_sResult;
}
