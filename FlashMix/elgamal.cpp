#include "../StdAfx.h"
#include "elgamal.hpp"

int getListIdx(unsigned int a_uListSize, unsigned int* a_uList, unsigned int a_uValue)
{
    if (a_uList == NULL)
        return -1;

    for (unsigned int i = 0; i < a_uListSize; i++)
        if (a_uList[i] == a_uValue)
            return i;

    return -1;
}

int BNInit(int& ar_sResult, BIGNUM** r_bn, unsigned int a_uSetWord = 0)
{
    if (ar_sResult != ELGAMAL_SUCCESS) return ar_sResult;
    if (r_bn == NULL) { ar_sResult = ELGAMAL_ERROR; return ar_sResult; }
    if (*r_bn != NULL) { ar_sResult = ELGAMAL_ERROR; return ar_sResult; }

    if ((*r_bn = BN_new()) == NULL)
        ar_sResult = ELGAMAL_ERROR;
    else
        BN_init(*r_bn);

    if (ar_sResult == ELGAMAL_SUCCESS)
        if (BN_set_word(*r_bn, a_uSetWord) != 1)
            ar_sResult = ELGAMAL_ERROR;

    if (ar_sResult != ELGAMAL_SUCCESS)
    {
        if (*r_bn != NULL) BN_free(*r_bn);
        *r_bn = NULL;
    }

    return ar_sResult;
}

int CTXInit(int& ar_sResult, BN_CTX** r_ctx)
{
    if (ar_sResult != ELGAMAL_SUCCESS) return ar_sResult;
    if (r_ctx == NULL) { ar_sResult = ELGAMAL_ERROR; return ar_sResult; }
    if (*r_ctx != NULL) { ar_sResult = ELGAMAL_ERROR; return ar_sResult; }

    if ((*r_ctx = BN_CTX_new()) == NULL)
        ar_sResult = ELGAMAL_ERROR;
    else
        BN_CTX_init(*r_ctx);

    return ar_sResult;
}

int BNListInit(int& ar_sResult, BIGNUM*** r_bnList, unsigned int a_uSize)
{
    if (ar_sResult != ELGAMAL_SUCCESS) return ar_sResult;
    if (r_bnList == NULL) { ar_sResult = ELGAMAL_ERROR; return ar_sResult; }
    if (*r_bnList != NULL) { ar_sResult = ELGAMAL_ERROR; return ar_sResult; }

    if ((*r_bnList = new BIGNUM*[a_uSize]) == NULL)
        ar_sResult = ELGAMAL_ERROR;
    else
        for (unsigned int i = 0; i < a_uSize; i++)
            (*r_bnList)[i] = NULL;

    return ar_sResult;
}

void BNFree(BIGNUM** a_bn)
{
    if (*a_bn != NULL) BN_free(*a_bn);
    *a_bn = NULL;
}

void CTXFree(BN_CTX** a_ctx)
{
    if (*a_ctx != NULL) BN_CTX_free(*a_ctx);
    *a_ctx = NULL;
}

void BNClearFree(BIGNUM** a_bn)
{
    if (*a_bn != NULL) BN_clear_free(*a_bn);
    *a_bn = NULL;
}

void BNListFree(unsigned int a_uSize, BIGNUM*** a_bnList, unsigned int a_uFreeArray = 1)
{
    if (a_bnList == NULL) return;

    for (unsigned int i = 0; i < a_uSize; i++)
        BNClearFree(&((*a_bnList)[i]));

    if (a_uFreeArray == 1)
    {
        delete[] *a_bnList;
        *a_bnList = NULL;
    }
}

ELGAMAL* ELGAMAL_new(void)
{
    ELGAMAL* ret;
    int result = ELGAMAL_SUCCESS;

    if ((ret = (ELGAMAL*)OPENSSL_malloc(sizeof(ELGAMAL))) == NULL)
        result = ELGAMAL_ERROR;
    else
    {
        ret->p = NULL;
        ret->pm1 = NULL;
        ret->q = NULL;
        ret->g = NULL;
        ret->x = NULL;
        ret->y = NULL;
    }

    BNInit(result, &(ret->p));
    BNInit(result, &(ret->pm1));
    BNInit(result, &(ret->q));
    BNInit(result, &(ret->g));
    BNInit(result, &(ret->x));
    BNInit(result, &(ret->y));

    if (result != ELGAMAL_SUCCESS)
    {
        BNFree(&(ret->p));
        BNFree(&(ret->pm1));
        BNFree(&(ret->q));
        BNFree(&(ret->g));
        BNFree(&(ret->x));
        BNFree(&(ret->y));
        OPENSSL_free(ret);
        ret = NULL;
    }

    return ret;
}

void ELGAMAL_free(ELGAMAL* r)
{
    if (r == NULL)
        return;

    BNClearFree(&(r->p));
    BNClearFree(&(r->pm1));
    BNClearFree(&(r->q));
    BNClearFree(&(r->g));
    BNClearFree(&(r->x));
    BNClearFree(&(r->y));
    OPENSSL_free(r);
}

ELGAMAL* ELGAMAL_generate_key(int a_iBits)
{
    int      result = ELGAMAL_SUCCESS;
    ELGAMAL* ret = NULL;
    BIGNUM*  bnTmp = NULL;
    BN_CTX*  ctx = NULL;

    if ((ret = ELGAMAL_new()) == NULL)
        result = ELGAMAL_ERROR;
    CTXInit(result, &ctx);
    BNInit(result, &bnTmp);

    // find p
    if (result == ELGAMAL_SUCCESS)
        if (BN_generate_prime(ret->p, a_iBits, 1, NULL, NULL, NULL, NULL) == NULL)
            result = ELGAMAL_ERROR;

    // calc p - 1
    if (result == ELGAMAL_SUCCESS)
        if (BN_copy(ret->pm1, ret->p) == NULL)
            result = ELGAMAL_ERROR;
    if (result == ELGAMAL_SUCCESS)
        if (BN_clear_bit(ret->pm1, 0) == 0)
            result = ELGAMAL_ERROR;

    // calc q = (p - 1) / 2
    if (result == ELGAMAL_SUCCESS)
        if (BN_rshift(ret->q, ret->pm1, 1) == 0)
            result = ELGAMAL_ERROR;

    // find generator g of Gp
    if (result == ELGAMAL_SUCCESS)
        if (BN_zero(bnTmp) == 0)
            result = ELGAMAL_ERROR;
    while ((!BN_is_one(bnTmp)) && (result == ELGAMAL_SUCCESS))
    {
        if (BN_rand(ret->g, a_iBits, -1, 0) == 0)
            result = ELGAMAL_ERROR;
        if (result == ELGAMAL_SUCCESS)
            if (BN_is_one(ret->g))
                continue;
        if (result == ELGAMAL_SUCCESS)
            if (BN_cmp(ret->g, ret->q) >= 0)
                continue;
        if (result == ELGAMAL_SUCCESS)
            if (BN_mod_exp(bnTmp, ret->g, ret->q, ret->p, ctx) == 0)
                result = ELGAMAL_ERROR;
    }

    if (result == ELGAMAL_SUCCESS)
    {
        do
        {
            if (BN_rand(ret->x, a_iBits - 1, 0, 0) == 0)
                result = ELGAMAL_ERROR;
        }
        while ((BN_is_one(ret->x)) && (result == ELGAMAL_SUCCESS));
    }

    if (result == ELGAMAL_SUCCESS)
        result = BN_mod_exp(ret->y, ret->g, ret->x, ret->p, ctx);

    BNClearFree(&bnTmp);
    CTXFree(&ctx);

    if (result != ELGAMAL_SUCCESS)
    {
        ELGAMAL_free(ret);
        ret = NULL;
    }

    return ret;
}

int ELGAMAL_isValidPublicKey(ELGAMAL* a_elKey)
{
    if (a_elKey == NULL)
        return ELGAMAL_ERROR;
    if ((a_elKey->p == NULL) || (a_elKey->q == NULL) || (a_elKey->g == NULL) ||
        (a_elKey->y == NULL) || (a_elKey->pm1 == NULL))
        return ELGAMAL_ERROR;

    return ELGAMAL_SUCCESS;
}

int ELGAMAL_isValidPrivateKey(ELGAMAL* a_elKey)
{
    if (a_elKey == NULL)
        return ELGAMAL_ERROR;
    if ((a_elKey->p == NULL) || (a_elKey->q == NULL) || (a_elKey->g == NULL) ||
        (a_elKey->x == NULL) || (a_elKey->pm1 == NULL))
        return ELGAMAL_ERROR;

    return ELGAMAL_SUCCESS;
}

int ELGAMAL_encrypt(BIGNUM** r_bnEncA, BIGNUM** r_bnEncB, ELGAMAL* a_elKey,
    BIGNUM* a_bnAlpha, BIGNUM* a_bnMsg)
{
    if ((ELGAMAL_isValidPublicKey(a_elKey) == 0) || (a_bnAlpha == NULL) ||
        (a_bnMsg == NULL))
        return ELGAMAL_ERROR;

    BN_CTX* ctx = NULL;
    BIGNUM* tmp = NULL;
    int result = ELGAMAL_SUCCESS;

    // init
    BNFree(r_bnEncA);
    BNFree(r_bnEncB);
    BNInit(result, r_bnEncA);
    BNInit(result, r_bnEncB);
    CTXInit(result, &ctx);
    BNInit(result, &tmp);

    // calc a = m*y^alpha mod p
    if (result == ELGAMAL_SUCCESS)
        if (BN_mod_exp(tmp, a_elKey->y, a_bnAlpha, a_elKey->p, ctx) == 0)
            result = ELGAMAL_ERROR;
    if (result == ELGAMAL_SUCCESS)
        if (BN_mod_mul(*r_bnEncA, a_bnMsg, tmp, a_elKey->p, ctx) == 0)
            result = ELGAMAL_ERROR;

    // calc b = g^alpha mod p
    if (result == ELGAMAL_SUCCESS)
        if (BN_mod_exp(*r_bnEncB, a_elKey->g, a_bnAlpha, a_elKey->p, ctx) == 0)
            result = ELGAMAL_ERROR;

    if (result != ELGAMAL_SUCCESS)
    {
        BNFree(r_bnEncA);
        BNFree(r_bnEncB);
    }

    BNFree(&tmp);
    CTXFree(&ctx);

    return result;
}

int ELGAMAL_encrypt(BIGNUM** r_bnEncA, BIGNUM** r_bnEncB, ELGAMAL* a_elKey,
    BIGNUM* a_bnMsg)
{
    if ((ELGAMAL_isValidPublicKey(a_elKey) == 0) || (a_bnMsg == NULL))
        return ELGAMAL_ERROR;

    int     result = ELGAMAL_SUCCESS;
    BIGNUM* bnAlpha = NULL;

    if (BNInit(result, &bnAlpha) == ELGAMAL_SUCCESS)
        if (BN_rand(bnAlpha, BN_num_bits(a_elKey->q) - 1, 1, 1) == 0)
            result = ELGAMAL_ERROR;

    if (result == ELGAMAL_SUCCESS)
        result = ELGAMAL_encrypt(r_bnEncA, r_bnEncB, a_elKey, bnAlpha, a_bnMsg);

    BNClearFree(&bnAlpha);

    return result;
}

int ELGAMAL_reencrypt(BIGNUM* r_bnEncA, BIGNUM* r_bnEncB, ELGAMAL* a_elKey,
    BIGNUM* a_bnAlpha)
{
    if ((r_bnEncA == NULL) || (r_bnEncB == NULL) || (a_bnAlpha == NULL) ||
        (ELGAMAL_isValidPublicKey(a_elKey) == 0))
        return ELGAMAL_ERROR;

    int     result = ELGAMAL_SUCCESS;
    BIGNUM* bnYAl = NULL;
    BIGNUM* bnGAl = NULL;
    BN_CTX* ctx = NULL;

    BNInit(result, &bnYAl);
    BNInit(result, &bnGAl);
    CTXInit(result, &ctx);

    if (result == ELGAMAL_SUCCESS)
        if (BN_mod_exp(bnYAl, a_elKey->y, a_bnAlpha, a_elKey->p, ctx) == 0)
            result = ELGAMAL_ERROR;
    if (result == ELGAMAL_SUCCESS)
        if (BN_mod_exp(bnGAl, a_elKey->g, a_bnAlpha, a_elKey->p, ctx) == 0)
            result = ELGAMAL_ERROR;
    if (result == ELGAMAL_SUCCESS)
        if (BN_mod_mul(r_bnEncA, r_bnEncA, bnYAl, a_elKey->p, ctx) == 0)
            result = ELGAMAL_ERROR;
    if (result == ELGAMAL_SUCCESS)
        if (BN_mod_mul(r_bnEncB, r_bnEncB, bnGAl, a_elKey->p, ctx) == 0)
            result = ELGAMAL_ERROR;

    CTXFree(&ctx);
    BNFree(&bnYAl);
    BNFree(&bnGAl);

    return result;
}

int ELGAMAL_reencrypt(BIGNUM** ar_bnList, ELGAMAL* a_elKey,
    unsigned int a_uListSize, BIGNUM** a_bnAlphaList)
{
    if ((ar_bnList == NULL) || (ELGAMAL_isValidPublicKey(a_elKey) == 0) ||
        (a_bnAlphaList == NULL))
        return ELGAMAL_SUCCESS;

    int result = ELGAMAL_SUCCESS;

    for (unsigned int i = 0; (i < a_uListSize) && (result == 1); i++)
        result = ELGAMAL_reencrypt(ar_bnList[2 * i], ar_bnList[2 * i + 1], a_elKey, a_bnAlphaList[i]);

    return result;
}

int ELGAMAL_gen_exp(BIGNUM*** r_bnExp, ELGAMAL* a_elPublicKey,
    unsigned int a_uCount)
{
    if ((r_bnExp == NULL) || (ELGAMAL_isValidPublicKey(a_elPublicKey) == 0)) return ELGAMAL_ERROR;
    if (*r_bnExp != NULL) return ELGAMAL_ERROR;

    int result = ELGAMAL_SUCCESS;
    int sSize = BN_num_bits(a_elPublicKey->q) - 1;

    BNListInit(result, r_bnExp, a_uCount);

    for (unsigned int i = 0; (i < a_uCount) && (result == ELGAMAL_SUCCESS); i++)
    {
        BNInit(result, &((*r_bnExp)[i]));
        if (result == ELGAMAL_SUCCESS)
            if (BN_rand((*r_bnExp)[i], sSize, 1, 1) == 0)
                result = ELGAMAL_ERROR;
    }

    if (result == 0)
        BNListFree(a_uCount, r_bnExp);

    return result;
}

int ELGAMAL_decrypt(BIGNUM** r_bnMsg, ELGAMAL* a_elKey, BIGNUM* a_bnEncA, BIGNUM* a_bnEncB)
{
    if ((ELGAMAL_isValidPrivateKey(a_elKey) == 0) ||
        (a_bnEncA == NULL) || (a_bnEncB == NULL))
        return ELGAMAL_ERROR;

    int     result = ELGAMAL_SUCCESS;
    BN_CTX* ctx = NULL;
    BIGNUM* tmp = NULL;


    BNClearFree(r_bnMsg);
    BNInit(result, r_bnMsg);
    BNInit(result, &tmp);
    CTXInit(result, &ctx);

    if (result == ELGAMAL_SUCCESS)
        if (BN_mod_exp(tmp, a_bnEncB, a_elKey->x, a_elKey->p, ctx) == 0)
            result = ELGAMAL_ERROR;
    if (result == ELGAMAL_SUCCESS)
        if (BN_mod_inverse(tmp, tmp, a_elKey->p, ctx) == NULL)
            result = ELGAMAL_ERROR;
    if (result == ELGAMAL_SUCCESS)
        if (BN_mod_mul(*r_bnMsg, a_bnEncA, tmp, a_elKey->p, ctx) == 0)
            result = ELGAMAL_ERROR;

    if (result != ELGAMAL_SUCCESS)
        BNClearFree(r_bnMsg);

    BNClearFree(&tmp);
    CTXFree(&ctx);

    return result;
}

int ELGAMAL_sign(BIGNUM** r_bnSignS, BIGNUM** r_bnSignK, ELGAMAL* a_elKey,
    BIGNUM* a_bnAlpha, BIGNUM* a_bnMsg)
{
    if ((ELGAMAL_isValidPrivateKey(a_elKey) == 0) ||
        (a_bnAlpha == NULL) || (a_bnMsg == NULL))
        return ELGAMAL_ERROR;

    int     result = ELGAMAL_SUCCESS;
    BIGNUM* tmp1 = NULL;
    BIGNUM* tmp2 = NULL;
    BN_CTX* ctx = NULL;

    BNFree(r_bnSignS);
    BNFree(r_bnSignK);
    BNInit(result, r_bnSignS);
    BNInit(result, r_bnSignK);
    BNInit(result, &tmp1);
    BNInit(result, &tmp2);
    CTXInit(result, &ctx);

    // k = g^alpha mod p
    if (result == ELGAMAL_SUCCESS)
        if (BN_mod_exp(*r_bnSignK, a_elKey->g, a_bnAlpha, a_elKey->p, ctx) == 0)
            result = ELGAMAL_ERROR;

    // s = (m - x*k)*alpha^-1 mod p-1
    if (result == ELGAMAL_SUCCESS)
        if (BN_mod_inverse(tmp2, a_bnAlpha, a_elKey->pm1, ctx) == NULL)
            result = ELGAMAL_ERROR;
    if (result == ELGAMAL_SUCCESS)
        if (BN_mul(tmp1, a_elKey->x, *r_bnSignK, ctx) == 0)
            result = ELGAMAL_ERROR;
    if (result == ELGAMAL_SUCCESS)
        if (BN_sub(tmp1, a_bnMsg, tmp1) == 0)
            result = ELGAMAL_ERROR;
    if (result == ELGAMAL_SUCCESS)
        if (BN_mod_mul(*r_bnSignS, tmp1, tmp2, a_elKey->pm1, ctx) == 0)
            result = ELGAMAL_ERROR;

    if (result != ELGAMAL_SUCCESS)
    {
        BNFree(r_bnSignS);
        BNFree(r_bnSignK);
    }

    BNFree(&tmp2);
    BNFree(&tmp1);
    CTXFree(&ctx);

    return result;
}

int ELGAMAL_sign(BIGNUM** r_bnSignS, BIGNUM** r_bnSignK, ELGAMAL* a_elKey,
    BIGNUM* a_bnMsg)
{
    if ((r_bnSignS == NULL) || (r_bnSignK == NULL) ||
        (ELGAMAL_isValidPrivateKey(a_elKey) == 0) || (a_bnMsg == NULL))
        return ELGAMAL_ERROR;

    int     result = 1;
    BIGNUM* alpha = NULL;
    BIGNUM* tmp = NULL;
    BIGNUM* tmp2 = NULL;
    BN_CTX* ctx = NULL;

    int bitCnt = 0;
    bool isInvertable = false;

    BNInit(result, &alpha);
    BNInit(result, &tmp, 0);
    BNInit(result, &tmp2, 0);
    CTXInit(result, &ctx);

    bitCnt = BN_num_bits(a_elKey->p);
    isInvertable = false;
    while ((result == ELGAMAL_SUCCESS) && ((BN_is_zero(tmp) == 1) || (isInvertable == false)))
    {
        if (BN_rand(alpha, bitCnt, -1, 0) == 0)
            result = ELGAMAL_ERROR;
        if (result == ELGAMAL_SUCCESS)
            if (BN_nnmod(tmp, a_elKey->pm1, alpha, ctx) == 0)
                result = ELGAMAL_ERROR;
        if (result == ELGAMAL_SUCCESS)
        {
            if (BN_mod_inverse(tmp2, alpha, a_elKey->pm1, ctx) == NULL)
                isInvertable = false;
            else
                isInvertable = true;
        }
    }

    if (result == ELGAMAL_SUCCESS)
        result = ELGAMAL_sign(r_bnSignS, r_bnSignK, a_elKey, alpha, a_bnMsg);

    if (result != ELGAMAL_SUCCESS)
    {
        BNFree(r_bnSignS);
        BNFree(r_bnSignK);
    }

    BNClearFree(&alpha);
    BNClearFree(&tmp2);
    BNClearFree(&tmp);
    CTXFree(&ctx);

    return result;
}

int ELGAMAL_test_signature(ELGAMAL* a_elKey, BIGNUM* a_bnMsg, BIGNUM* a_bnSignS,
    BIGNUM* a_bnSignK)
{
    if ((ELGAMAL_isValidPublicKey(a_elKey) == ELGAMAL_ERROR) ||
        (a_bnMsg == NULL) || (a_bnSignS == NULL) || (a_bnSignK == NULL))
        return ELGAMAL_ERROR;

    int     result = ELGAMAL_SUCCESS;
    BIGNUM* gm = NULL;
    BIGNUM* yk = NULL;
    BIGNUM* ks = NULL;
    BN_CTX* ctx = NULL;

    BNInit(result, &gm);
    BNInit(result, &yk);
    BNInit(result, &ks);
    CTXInit(result, &ctx);

    if (result == ELGAMAL_SUCCESS)
        if (BN_mod_exp(gm, a_elKey->g, a_bnMsg, a_elKey->p, ctx) == 0)
            result = ELGAMAL_ERROR;
    if (result == ELGAMAL_SUCCESS)
        if (BN_mod_exp(yk, a_elKey->y, a_bnSignK, a_elKey->p, ctx) == 0)
            result = ELGAMAL_ERROR;
    if (result == ELGAMAL_SUCCESS)
        if (BN_mod_exp(ks, a_bnSignK, a_bnSignS, a_elKey->p, ctx) == 0)
            result = ELGAMAL_ERROR;
    if (result == ELGAMAL_SUCCESS)
        if (BN_mod_mul(yk, yk, ks, a_elKey->p, ctx) == 0)
            result = ELGAMAL_ERROR;

    if (result == ELGAMAL_SUCCESS)
        if (BN_cmp(gm, yk) != 0)
            result = ELGAMAL_ERROR;

    BNFree(&gm);
    BNFree(&yk);
    BNFree(&ks);
    CTXFree(&ctx);

    return result;
}

int ELGAMAL_choose_polynom(BIGNUM*** r_bnPolynom, unsigned int a_uDegree,
    ELGAMAL* a_elPublicSharedKey, BIGNUM* a_bnLocalSecret)
{
    if ((*r_bnPolynom != NULL) || (ELGAMAL_isValidPublicKey(a_elPublicSharedKey) == 0) ||
        (a_bnLocalSecret == NULL))
        return ELGAMAL_ERROR;

    int result = ELGAMAL_SUCCESS;
    BN_CTX* ctx = NULL;

    BNListInit(result, r_bnPolynom, a_uDegree + 1);
    CTXInit(result, &ctx);

    if (result == ELGAMAL_SUCCESS)
        if (((*r_bnPolynom)[0] = BN_dup(a_bnLocalSecret)) == NULL)
            result = ELGAMAL_ERROR;
    for (unsigned int i = 1; (i <= a_uDegree) && (result == ELGAMAL_SUCCESS); i++)
    {
        if (BNInit(result, &((*r_bnPolynom)[i])) == ELGAMAL_SUCCESS)
            if (BN_rand((*r_bnPolynom)[i], BN_num_bits(a_elPublicSharedKey->q), 0, 0) == 0)
                result = ELGAMAL_ERROR;
        if (result == ELGAMAL_SUCCESS)
            if (BN_nnmod((*r_bnPolynom)[i], (*r_bnPolynom)[i], a_elPublicSharedKey->q, ctx) == 0)
                result = ELGAMAL_ERROR;
    }

    if (result != ELGAMAL_SUCCESS)
        BNListFree(a_uDegree + 1, r_bnPolynom, 1);

    CTXFree(&ctx);

    return result;
}

int ELGAMAL_calcF(BIGNUM*** r_bnF, ELGAMAL* a_elSharedGroupKey,
    unsigned int a_ufSize, BIGNUM** a_bnf)
{
    if ((*r_bnF != NULL) || (a_bnf == NULL))
        return ELGAMAL_ERROR;

    int result = ELGAMAL_SUCCESS;
    BN_CTX* ctx = NULL;

    BNListInit(result, r_bnF, a_ufSize);
    CTXInit(result, &ctx);

    for (unsigned int i = 0; (i < a_ufSize) && (result == ELGAMAL_SUCCESS); i++)
    {
        if (BNInit(result, &((*r_bnF)[i])) == ELGAMAL_SUCCESS)
            if (BN_mod_exp((*r_bnF)[i], a_elSharedGroupKey->g, a_bnf[i], a_elSharedGroupKey->p, ctx) == 0)
                result = ELGAMAL_ERROR;
    }

    if (result != ELGAMAL_SUCCESS)
        BNListFree(a_ufSize, r_bnF);

    CTXFree(&ctx);

    return result;
}

int ELGAMAL_calc_fij(BIGNUM** r_bn, BIGNUM* a_bnModulo, BIGNUM* a_bnJ, unsigned int a_ufSize, BIGNUM** a_bnf)
{
    if ((r_bn == NULL) || (a_bnModulo == NULL) || (a_bnJ == NULL) ||
        (a_bnf == NULL))
        return ELGAMAL_ERROR;

    int result = ELGAMAL_SUCCESS;
    BN_CTX* ctx = NULL;
    BIGNUM* exp = NULL;

    BNClearFree(r_bn);
    BNInit(result, r_bn, 0);
    BNInit(result, &exp, 0);
    CTXInit(result, &ctx);

    for (unsigned int i = 0; (i < a_ufSize) && (result == ELGAMAL_SUCCESS); i++)
    {
        if (BN_set_word(exp, i) == 0)
            result = ELGAMAL_ERROR;
        if (result == ELGAMAL_SUCCESS)
            if (BN_mod_exp(exp, a_bnJ, exp, a_bnModulo, ctx) == 0)
                result = ELGAMAL_ERROR;
        if (result == ELGAMAL_SUCCESS)
            if (BN_mod_mul(exp, exp, a_bnf[i], a_bnModulo, ctx) == 0)
                result = ELGAMAL_ERROR;
        if (result == ELGAMAL_SUCCESS)
            if (BN_mod_add(*r_bn, *r_bn, exp, a_bnModulo, ctx) == 0)
                result = ELGAMAL_ERROR;
    }

    if (result != ELGAMAL_SUCCESS)
        BNFree(r_bn);

    BNFree(&exp);
    CTXFree(&ctx);

    return result;
}

int ELGAMAL_calc_allfij(BIGNUM*** r_bnfij, BIGNUM* a_bnModulo,
    unsigned int a_uMIXCnt, unsigned int a_ufSize, BIGNUM** a_bnf)
{
    if ((*r_bnfij != NULL) || (a_bnModulo == NULL) || (a_bnf == NULL))
        return ELGAMAL_ERROR;

    int result = ELGAMAL_SUCCESS;
    BIGNUM* bnJ = NULL;

    BNInit(result, &bnJ);
    BNListInit(result, r_bnfij, a_uMIXCnt);

    for (unsigned int i = 0; (i < a_uMIXCnt) && (result == ELGAMAL_SUCCESS); i++)
    {
        if (BN_set_word(bnJ, i + 1) == 0)
            result = ELGAMAL_ERROR;
        if (result == ELGAMAL_SUCCESS)
            result = ELGAMAL_calc_fij(&(*r_bnfij)[i], a_bnModulo, bnJ, a_ufSize, a_bnf);
    }

    if (result != ELGAMAL_SUCCESS)
        BNListFree(a_uMIXCnt, r_bnfij, 1);

    BNFree(&bnJ);

    return result;
}

int ELGAMAL_calcSi(BIGNUM** r_bn, BIGNUM* a_bnModulo, unsigned int a_uSjiSize,
    BIGNUM** a_bnSi)
{
    if ((a_bnSi == NULL) || (r_bn == NULL)) return ELGAMAL_ERROR;

    int result = ELGAMAL_SUCCESS;
    BN_CTX* ctx = NULL;

    BNClearFree(r_bn);
    BNInit(result, r_bn, 0);
    CTXInit(result, &ctx);

    for (unsigned int i = 0; (i < a_uSjiSize) && (result == ELGAMAL_SUCCESS); i++)
    {
        if (a_bnSi[i] == NULL)
        {
            result = ELGAMAL_ERROR;
            break;
        }
        if (BN_mod_add(*r_bn, *r_bn, a_bnSi[i], a_bnModulo, ctx) == 0)
            result = ELGAMAL_ERROR;
    }

    if (result != ELGAMAL_SUCCESS)
        BNFree(r_bn);

    CTXFree(&ctx);

    return result;
}

int ELGAMAL_testSecret(ELGAMAL* a_elGroupKey, unsigned int a_uSelfMIXIdx,
    BIGNUM* a_bnSecret, unsigned int a_uMIXDecryptCnt, BIGNUM** a_bnF)
{
    if ((a_elGroupKey == NULL) || (a_bnSecret == NULL) || (a_bnF == NULL))
        return -1;
    if ((a_elGroupKey->g == NULL) || (a_elGroupKey->p == NULL))
        return -1;

    int result = ELGAMAL_SUCCESS;

    BIGNUM* bnGSji = NULL;
    BIGNUM* tmp = NULL;
    BIGNUM* exp = NULL;
    BIGNUM* expPl = NULL;
    BN_CTX* ctx = NULL;

    BNInit(result, &bnGSji);
    BNInit(result, &tmp, 1);
    BNInit(result, &exp, a_uSelfMIXIdx + 1);
    BNInit(result, &expPl);
    CTXInit(result, &ctx);

    if (result == ELGAMAL_SUCCESS)
        if (BN_mod_exp(bnGSji, a_elGroupKey->g, a_bnSecret, a_elGroupKey->p, ctx) == 0)
            result = ELGAMAL_ERROR;

    for (unsigned int l = 0; (l < a_uMIXDecryptCnt) && (result == ELGAMAL_SUCCESS); l++)
    {
        if (BN_set_word(expPl, l) == 0)
            result = ELGAMAL_ERROR;
        if (result == ELGAMAL_SUCCESS)
            if (BN_exp(expPl, exp, expPl, ctx) == 0)
                result = ELGAMAL_ERROR;

        if (result == ELGAMAL_SUCCESS)
            if (BN_mod_exp(expPl, a_bnF[l], expPl, a_elGroupKey->p, ctx) == 0)
                result = ELGAMAL_ERROR;
        if (result == ELGAMAL_SUCCESS)
            if (BN_mod_mul(tmp, tmp, expPl, a_elGroupKey->p, ctx) == 0)
                result = ELGAMAL_ERROR;
    }

    if (result == ELGAMAL_SUCCESS)
        if (BN_cmp(tmp, bnGSji) != 0)
            result = ELGAMAL_ERROR;

    CTXFree(&ctx);
    BNFree(&expPl);
    BNFree(&exp);
    BNFree(&tmp);
    BNFree(&bnGSji);

    return result;
}

unsigned int fac(unsigned int a_dValue)
{
    if (a_dValue < 2)
        return 1;

    unsigned int result = 1;
    for (unsigned int i = 2; i <= a_dValue; i++)
        result *= i;

    return result;
}

int BN_neg(BIGNUM* ar_bn)
{
    if (ar_bn == NULL) return ELGAMAL_ERROR;

    int result = ELGAMAL_SUCCESS;
    BIGNUM* bnM1 = NULL;
    BN_CTX* ctx = NULL;

    CTXInit(result, &ctx);

    if (BN_dec2bn(&bnM1, "-1") == 0)
        result = ELGAMAL_ERROR;
    else
        if (BN_mul(ar_bn, ar_bn, bnM1, ctx) == 0)
            result = ELGAMAL_ERROR;

    BNFree(&bnM1);
    CTXFree(&ctx);

    return result;
}

int ELGAMAL_calcSharedExp(BIGNUM** r_bnExp, unsigned int MIXCNT,
    unsigned int a_uListSize, unsigned int* a_uDecryptList,
    unsigned int a_uSelfIdx, BIGNUM* a_bnSecret, BIGNUM* a_bnModulo)
{
    if ((r_bnExp == NULL) || (a_bnSecret == NULL) || (a_bnModulo == NULL))
        return ELGAMAL_ERROR;

    int result = ELGAMAL_SUCCESS;
    int num = -1;
    int denom = 1;
    unsigned int uIdx = a_uSelfIdx + 1;
    BIGNUM* bnNum = NULL;
    BIGNUM* bnDenom = NULL;
    BN_CTX* ctx = NULL;

    BNFree(r_bnExp);
    BNInit(result, r_bnExp);
    BNInit(result, &bnNum);
    BNInit(result, &bnDenom);
    CTXInit(result, &ctx);

    for (unsigned int i = 1; (i <= MIXCNT) && (result == ELGAMAL_SUCCESS); i++)
    {
        if (i == uIdx)
            continue;
        if (getListIdx(a_uListSize, a_uDecryptList, i-1) < 0)
            continue;

        num = num * (-i);
        denom = denom * (uIdx - i);
    }

    if (num < 0)
    {
        if (BN_set_word(bnNum, num*-1) == 0)
            result = ELGAMAL_ERROR;
        else
            result = BN_neg(bnNum);
    }
    else
        if (BN_set_word(bnNum, num) == 0)
            result = ELGAMAL_ERROR;

    if (denom < 0)
    {
        if (BN_set_word(bnDenom, denom * -1) == 0)
            result = ELGAMAL_ERROR;
        else
            result = BN_neg(bnDenom);
    }
    else
        if (BN_set_word(bnDenom, denom) == 0)
            result = ELGAMAL_ERROR;

/*    if (BN_set_word(bnNum, fac(MIXCNT)/uIdx) == 0)
        result = ELGAMAL_ERROR;
    if (BN_set_word(bnDenom, fac(uIdx - 1) * fac(MIXCNT - uIdx)) == 0)
        result = ELGAMAL_ERROR;

    if (result == ELGAMAL_SUCCESS)
        if ((MIXCNT & 0x01) > 0)
            result = BN_neg(bnNum);
    if (result == ELGAMAL_SUCCESS)
        if (((MIXCNT - uIdx) & 0x01) > 0)
            result = BN_neg(bnDenom);*/

    if (result == ELGAMAL_SUCCESS)
        if (BN_nnmod(bnDenom, bnDenom, a_bnModulo, ctx) == 0)
            result = ELGAMAL_ERROR;

    if (result == ELGAMAL_SUCCESS)
        if (BN_mod_inverse(bnDenom, bnDenom, a_bnModulo, ctx) == NULL)
            result = ELGAMAL_ERROR;

    if (result == ELGAMAL_SUCCESS)
        if (BN_mod_mul(*r_bnExp, a_bnSecret, bnNum, a_bnModulo, ctx) == 0)
            result = ELGAMAL_ERROR;
    if (result == ELGAMAL_SUCCESS)
        if (BN_mod_mul(*r_bnExp, *r_bnExp, bnDenom, a_bnModulo, ctx) == 0)
            result = ELGAMAL_ERROR;

    BNClearFree(&bnNum);
    BNClearFree(&bnDenom);
    CTXFree(&ctx);

    return result;
}
