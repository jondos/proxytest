#ifndef HEADER_ELGAMAL_H
#define HEADER_ELGAMAL_H

static const int ELGAMAL_SUCCESS        = 1;
static const int ELGAMAL_ERROR          = 0;

typedef struct elgamal_st ELGAMAL;

struct elgamal_st
{
    BIGNUM* p;      // public save prime p = 2q + 1
    BIGNUM* pm1;    // p - 1
    BIGNUM* q;      // public prime q
    BIGNUM* g;      // public generator of Gp (it holds: g^q mod p = 1)
    BIGNUM* x;      // private key x
    BIGNUM* y;      // public key y = g^x mod p
};

ELGAMAL*    ELGAMAL_new(void);
void        ELGAMAL_free(ELGAMAL* r);

ELGAMAL*    ELGAMAL_generate_key(int a_iBits);

int         ELGAMAL_isValidPublicKey(ELGAMAL* a_elKey);
int         ELGAMAL_isValidPrivateKey(ELGAMAL* a_elKey);

// (a, b) = (m*y^alpha mod p, g^alpha mod p)
// if r_bnEncA or r_bnEncB is NULL a new BIGNUM is created
// 1 is returnd for success, 0 on error
int         ELGAMAL_encrypt(BIGNUM** r_bnEncA, BIGNUM** r_bnEncB, ELGAMAL* a_elKey, BIGNUM* a_bnAlpha, BIGNUM* a_bnMsg);
int         ELGAMAL_encrypt(BIGNUM** r_bnEncA, BIGNUM** r_bnEncB, ELGAMAL* a_elKey, BIGNUM* a_bnMsg);

// encrypts a encrytped message with alpha
// r_bnEncA and r_bnEncB is the encrypted Message and the return value
int         ELGAMAL_reencrypt(BIGNUM* r_bnEncA, BIGNUM* r_bnEncB, ELGAMAL* a_elKey, BIGNUM* a_bnAlpha);
int         ELGAMAL_reencrypt(BIGNUM** ar_bnList, ELGAMAL* a_elKey, unsigned int a_uListSize, BIGNUM** a_bnAlphaList);

// generates an list of alphas which can be used for reencryption
// 1 is returnd for succes, 0 on error
int         ELGAMAL_gen_exp(BIGNUM*** r_bnExp, ELGAMAL* a_elPublicKey, unsigned int a_uCount);

// m = a*(b^x)^-1 mod p
// if r_bnMsg is NULL a new BIGNUM is created
// 1 is returnd for success, 0 on error
int         ELGAMAL_decrypt(BIGNUM** r_bnMsg, ELGAMAL* a_elKey, BIGNUM* a_bnEncA, BIGNUM* a_bnEncB);

// (s, k) = ((m - x*k)*alpha^-1 mod p-1, g^alpha mod p)
// !!! (p-1) mod alpha != 0 !!!
// 1 is returned for success, 0 on error
// if r_bnSignS or r_bnSignK is NULL a new BIGNUM is created
int         ELGAMAL_sign(BIGNUM** r_bnSignS, BIGNUM** r_bnSignK, ELGAMAL* a_elKey, BIGNUM* a_bnAlpha, BIGNUM* a_bnMsg);
int         ELGAMAL_sign(BIGNUM** r_bnSignS, BIGNUM** r_bnSignK, ELGAMAL* a_elKey, BIGNUM* a_bnMsg);

// test: g^m mod p = y^k * k^s mod p
// 1 if the condition holds, 0 if not and -1 on error
int         ELGAMAL_test_signature(ELGAMAL* a_elKey, BIGNUM* a_bnMsg, BIGNUM* a_bnSignS, BIGNUM* a_bnSignK);

// generates the polynom of degree k - 1
// 1 is returned on success, 0 on error
int         ELGAMAL_choose_polynom(BIGNUM*** r_bnPolynom, unsigned int a_uDegree, ELGAMAL* a_elPublicSharedKey, BIGNUM* a_bnLocalSecret);

// calculates Fij = g^fij
// 1 is returned on success, 0 on error
int         ELGAMAL_calcF(BIGNUM*** r_bnF, ELGAMAL* a_elSharedGroupKey, unsigned int a_ufSize, BIGNUM** a_bnf);

// calculates fi(j)
// 1 is returned on success, 0 on error
int         ELGAMAL_calc_fij(BIGNUM** r_bn, BIGNUM* a_bnModulo, BIGNUM* a_bnJ, unsigned int a_ufSize, BIGNUM** a_bnf);
int         ELGAMAL_calc_allfij(BIGNUM*** r_bnfij, BIGNUM* a_bnModulo, unsigned int a_uMIXCnt, unsigned int a_ufSize, BIGNUM** a_bnf);

// calculates si = SUM sji mod p
// 1 is returned on success, 0 on error
int         ELGAMAL_calcSi(BIGNUM** r_bn, BIGNUM* a_bnModulo, unsigned int a_uSjiSize, BIGNUM** a_bnSi);

// tests the received secret value from a MIX
// 1 is returned if the condition g^sji = \prod F_{jl}^{i^l} holds, 0 if not, -1 on error
int         ELGAMAL_testSecret(ELGAMAL* a_elGroupKey, unsigned int a_uSelfMIXIdx, BIGNUM* a_bnSecret, unsigned int a_uMIXDecryptCnt, BIGNUM** a_bnF);

// calculates the exponent for the shared decryption
// 1 is returned on success, 0 on error
int         ELGAMAL_calcSharedExp(BIGNUM** r_bnExp, unsigned int MIXCNT, unsigned int a_uListSize, unsigned int* a_uDecryptList, unsigned int a_uSelfIdx, BIGNUM* a_bnSecret, BIGNUM* a_bnModulo);

#endif // HEADER_ELGAMAL_H
