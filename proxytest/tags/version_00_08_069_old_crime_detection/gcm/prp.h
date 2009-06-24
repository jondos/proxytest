/**
 * prp.h
 *
 * Optimised ANSI C AES code modified from reference code.  Decrypt
 * routine is removed, and the code is optimized for GCM's awesome
 * brand of counter mode, where only the lower 32 bits change.  This
 * means we can skip nearly a whole round per block cipher invocation.
 */
#ifndef __PRP_H
#define __PRP_H

#define MAXKC	(256/32)
#define MAXKB	(256/8)
#define MAXNR	14

/*  This implementation operates on fixed-size types.  We assume that
 *  one has a 64-bit int type.  While few machines still provide such
 *  a type natively, most compilers will automatically emulate a
 *  64-bit type when there isn't a native one available.
 */



typedef UINT32 AES_CTR_CTX[64];

int aes_enc_setup(UINT32 rk[], const UINT8 cipherKey[], int keyBits);
void aes_ctr_first(UINT32 rk[64], UINT8 nonce[12], UINT32 *ctr, 
		   UINT8 ct[16], int keyBits);
void aes_enc(const UINT32 rk[], const UINT8 pt[16], UINT8 ct[16], int keyBits);
void aes_ctr(UINT32 rk[64], UINT32 ctr[1], UINT8 ct[16], int keyBits);

#define KEY_SCHED  AES_CTR_CTX
#define ENCRYPT_INIT(sched, key, keylen) aes_enc_setup((UINT32 *)(sched), key, keylen)
#define CTR_INIT(sched, nonce, ctr, out, keylen) aes_ctr_first((UINT32 *)(sched), (UINT8 *)(nonce), (UINT32 *)(ctr), (UINT8 *)(out), keylen)
#define CTR_ENCRYPT(sched, ctr, out, keylen) aes_ctr((UINT32 *)(sched), (UINT32 *)(ctr), (UINT8 *)(out), keylen)
#define DO_ENCRYPT(sched, in, out, keylen) aes_enc((UINT32 *)(sched), (UINT8 *)(in), (UINT8 *)(out), keylen)

#endif /* __PRP_H */
