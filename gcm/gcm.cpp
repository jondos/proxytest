#include "../StdAfx.h"
#include "gcm.h"

static void mul_alpha(UINT32 *z) {
	int carry = z[3] & 1;

	z[3] >>= 1;
	z[3] |= ((z[2] & 1) << 31);
	z[2] >>= 1;
	z[2] |= ((z[1] & 1) << 31);
	z[1] >>= 1;
	z[1] |= ((z[0] & 1) << 31);
	z[0] >>= 1;

	if (carry) 
		z[0] ^= GHASH_ALPHA;
}

static void build_hash_table_64k(gcm_ctx_64k *c, UINT32 hkey[4]) {
	UINT32 a[4];
	int    i, j, k, t;

	a[0] = htonl(hkey[0]);
	a[1] = htonl(hkey[1]);
	a[2] = htonl(hkey[2]);
	a[3] = htonl(hkey[3]);

	for (t=0;t<16;t++) {
		c->table[t][0][0] = c->table[t][0][1] = c->table[t][0][2] = 
			c->table[t][0][3] = 0;
		i = 128, j = 256;
		while (i) {
			c->table[t][i][0] = htonl(a[0]);
			c->table[t][i][1] = htonl(a[1]);
			c->table[t][i][2] = htonl(a[2]);
			c->table[t][i][3] = htonl(a[3]);
			mul_alpha(a);
			i >>= 1;
			j >>= 1;
		}
	}
	
	for (i=1;i<256;i<<=1) {
		for (j=1;j<i;j++) {
			k = i + j;
			for (t=0;t<16;t++) {
	c->table[t][k][0] = c->table[t][i][0] ^ c->table[t][j][0];
	c->table[t][k][1] = c->table[t][i][1] ^ c->table[t][j][1];
	c->table[t][k][2] = c->table[t][i][2] ^ c->table[t][j][2];
	c->table[t][k][3] = c->table[t][i][3] ^ c->table[t][j][3];
			}
		}
	}
}

#ifdef  BYTE_ORDER_BIG_ENDIAN
#define GMULWI64K(e,t,i,s) \
	e = (UINT32 *)t[i][s>>24]; t0 = e[0]; t1 = e[1]; t2 = e[2]; t3 = e[3];\
	e = (UINT32 *)t[i+1][(s>>16)&0xff]; t0 ^= e[0]; t1 ^= e[1]; t2 ^= e[2]; t3 ^= e[3];\
	e = (UINT32 *)t[i+2][(s>>8)&0xff]; t0 ^= e[0]; t1 ^= e[1]; t2 ^= e[2]; t3 ^= e[3];\
	e = (UINT32 *)t[i+3][s&0xff]; t0 ^= e[0]; t1 ^= e[1]; t2 ^= e[2]; t3 ^= e[3]

#define GMULW64K(e,t,i,s) \
	e = (UINT32 *)t[i][s>>24]; t0 ^= e[0]; t1 ^= e[1]; t2 ^= e[2]; t3 ^= e[3];\
	e = (UINT32 *)t[i+1][(s>>16)&0xff]; t0 ^= e[0]; t1 ^= e[1]; t2 ^= e[2]; t3 ^= e[3];\
	e = (UINT32 *)t[i+2][(s>>8)&0xff]; t0 ^= e[0]; t1 ^= e[1]; t2 ^= e[2]; t3 ^= e[3];\
	e = (UINT32 *)t[i+3][s&0xff]; t0 ^= e[0]; t1 ^= e[1]; t2 ^= e[2]; t3 ^= e[3]
#else
#define GMULWI64K(e,t,i,s) \
	e = (UINT32 *)t[i][s&0xff]; t0 = e[0]; t1 = e[1]; t2 = e[2]; t3 = e[3];\
	e = (UINT32 *)t[i+1][(s>>8)&0xff]; t0 ^= e[0]; t1 ^= e[1]; t2 ^= e[2]; t3 ^= e[3];\
	e = (UINT32 *)t[i+2][(s>>16)&0xff]; t0 ^= e[0]; t1 ^= e[1]; t2 ^= e[2]; t3 ^= e[3];\
	e = (UINT32 *)t[i+3][s>>24]; t0 ^= e[0]; t1 ^= e[1]; t2 ^= e[2]; t3 ^= e[3]

#define GMULW64K(e,t,i,s) \
	e = (UINT32 *)t[i][s&0xff]; t0 ^= e[0]; t1 ^= e[1]; t2 ^= e[2]; t3 ^= e[3];\
	e = (UINT32 *)t[i+1][(s>>8)&0xff]; t0 ^= e[0]; t1 ^= e[1]; t2 ^= e[2]; t3 ^= e[3];\
	e = (UINT32 *)t[i+2][(s>>16)&0xff]; t0 ^= e[0]; t1 ^= e[1]; t2 ^= e[2]; t3 ^= e[3];\
	e = (UINT32 *)t[i+3][s>>24]; t0 ^= e[0]; t1 ^= e[1]; t2 ^= e[2]; t3 ^= e[3]
#endif

#define GHB64K(t, b) \
	s0 ^= ((UINT32 *)b)[0];		\
	s1 ^= ((UINT32 *)b)[1];		\
	s2 ^= ((UINT32 *)b)[2];		\
	s3 ^= ((UINT32 *)b)[3];		\
	GMULWI64K(entry, t, 0, s0);\
	GMULW64K(entry, t, 4, s1);\
	GMULW64K(entry, t, 8, s2);\
	GMULW64K(entry, t, 12, s3);\
	s0 = t0; \
	s1 = t1; \
	s2 = t2; \
	s3 = t3;

MODIFIERS void gcm_init_64k(gcm_ctx_64k *c, UINT8 key[], size_t keylen) {
	UINT32 hkgen[4] = {0,};
	UINT32 hkey[4];

	if (keylen != 128 && keylen != 192 && keylen != 256) {
		return;
	}
	ENCRYPT_INIT(&(c->ck), key, keylen);
	c->keylen = keylen;
	DO_ENCRYPT(&(c->ck), (UINT8 *)hkgen, (UINT8 *)hkey, keylen);
	build_hash_table_64k(c, hkey);
}
MODIFIERS void /*inline*/ gcm_encrypt_64k(gcm_ctx_64k *c, const UINT32 *nonce, 
							const UINT8 *data, size_t dlen, UINT8 *out, UINT32 *tag) {
	UINT32 tmp[8] = {0, 0, 0, 0, 0, 0, 0, htonl(dlen << 3)};
	UINT32 ctr[4];
	size_t b, l, i;
	register UINT32 t0, t1, t2, t3;
	register UINT32 *entry;
	register UINT32 s0 = 0, s1 = 0, s2 = 0, s3 = 0;

	ctr[0] =nonce[0];
	ctr[1] =nonce[1];
	ctr[2] =nonce[2];
	ctr[3] = 1;
	CTR_INIT(&(c->ck), ctr, ctr + 3, tag, c->keylen);

	/* Hash ciphertext. */
	b = dlen >> 4;
	l = dlen & 15;

	for (i=0;i<b;i++) {
		CTR_ENCRYPT(&(c->ck), (&ctr[3]), out, c->keylen);
		((UINT32 *)out)[0] ^= ((UINT32 *)data)[0];
		((UINT32 *)out)[1] ^= ((UINT32 *)data)[1];
		((UINT32 *)out)[2] ^= ((UINT32 *)data)[2];
		((UINT32 *)out)[3] ^= ((UINT32 *)data)[3];
		GHB64K(c->table, out);
		data += GHASH_BLK_SZ;
		out += GHASH_BLK_SZ;
	}

	if (l) {
		CTR_ENCRYPT(&(c->ck), &ctr[3], ctr, c->keylen);
		for (i=0;i<l;i++)
			out[i] = ((UINT8 *)ctr)[i] ^ data[i];
		memcpy(tmp, out, l);
		GHB64K(c->table, tmp);
	}

	GHB64K(c->table, (tmp + 4));
	
	tag[0] ^= s0;
	tag[1] ^= s1;
	tag[2] ^= s2;
	tag[3] ^= s3;
}

MODIFIERS int gcm_decrypt_64k(gcm_ctx_64k *c, const UINT32 *nonce, const UINT8 *ct,
						size_t ctlen,const UINT8 *tag, UINT8 *pt) {
	UINT32 tmp[8] = {0, 0, 0, 0, 0, 0, htonl(ctlen >> 29), htonl(ctlen << 3)};
	UINT32 ctr[4];
	UINT8  chksm[16];
	register char *p;
	size_t b, l, i;
	register UINT32 t0, t1, t2, t3;
	register UINT32 *entry;
	register UINT32 s0 = 0, s1 = 0, s2 = 0, s3 = 0;

	
		ctr[0] = nonce[0];
		ctr[1] = nonce[1];
		ctr[2] = nonce[2];
		ctr[3] = 1;
	CTR_INIT(&(c->ck), ctr, ctr + 3, chksm, c->keylen);
	for (i=0;i<16;i++) 
		chksm[i] ^= tag[i];

	b = ctlen >> 4;
	l = ctlen & 15;
	p = (char*)ct;
	
	for (i=0;i<b;i++) {
		GHB64K(c->table, p);
		p += GHASH_BLK_SZ;
	}
	if (l) {
		while (l--)
			((UINT8 *)tmp)[l] = p[l];
		GHB64K(c->table, tmp);
	}
	l = ctlen & 15;
	GHB64K(c->table, (tmp + 4));
	
	((UINT32 *)chksm)[0] ^= s0;
	((UINT32 *)chksm)[1] ^= s1;
	((UINT32 *)chksm)[2] ^= s2;
	((UINT32 *)chksm)[3] ^= s3;

	//for (i=taglen;i<16;i++)
	//		chksm[i] = 0;

	if (((UINT32 *)chksm)[0] || ((UINT32 *)chksm)[1] || ((UINT32 *)chksm)[2] ||
			((UINT32 *)chksm)[3]) return 0;

	for (i=0;i<b;i++) {
		CTR_ENCRYPT(&(c->ck), (&ctr[3]), pt, c->keylen);
		((UINT32 *)pt)[0] ^= ((UINT32 *)ct)[0];
		((UINT32 *)pt)[1] ^= ((UINT32 *)ct)[1];
		((UINT32 *)pt)[2] ^= ((UINT32 *)ct)[2];
		((UINT32 *)pt)[3] ^= ((UINT32 *)ct)[3];
		ct += GHASH_BLK_SZ;
		pt += GHASH_BLK_SZ;
	}

	if (l) {
		CTR_ENCRYPT(&(c->ck), &ctr[3], ctr, c->keylen);
		for (i=0;i<l;i++)
			pt[i] = ((UINT8 *)ctr)[i] ^ ct[i];
	}
	return 1;
}

MODIFIERS int gcm_decrypt_64k(gcm_ctx_64k *c, const UINT32 *nonce, const UINT8 *ct,
						size_t ctlen, UINT8 *pt) {
	//UINT32 tmp[8] = {0, 0, 0, 0, 0, 0, htonl(ctlen >> 29), htonl(ctlen << 3)};
	UINT32 ctr[4];
	UINT8  chksm[16];
	//register char *p;
	size_t b, l, i;
	//register UINT32 t0, t1, t2, t3;
	//register UINT32 *entry;
	//register UINT32 s0 = 0, s1 = 0, s2 = 0, s3 = 0;

	
		ctr[0] = nonce[0];
		ctr[1] = nonce[1];
		ctr[2] = nonce[2];
		ctr[3] = 1;
	CTR_INIT(&(c->ck), ctr, ctr + 3, chksm, c->keylen);
	
	b = ctlen >> 4;
	l = ctlen & 15;
	//p = (char*)ct;
	
	/*for (i=0;i<b;i++) {
		GHB64K(c->table, p);
		p += GHASH_BLK_SZ;
	}
	if (l) {
		while (l--)
			((UINT8 *)tmp)[l] = p[l];
		GHB64K(c->table, tmp);
	}
	l = ctlen & 15;
	GHB64K(c->table, (tmp + 4));
	
	memset(chksm,0,16);
	*/
	//((UINT32 *)chksm)[0] ^= s0;
	//((UINT32 *)chksm)[1] ^= s1;
	//((UINT32 *)chksm)[2] ^= s2;
	//((UINT32 *)chksm)[3] ^= s3;

	//for (i=taglen;i<16;i++)
	//		chksm[i] = 0;

	//if (((UINT32 *)chksm)[0] || ((UINT32 *)chksm)[1] || ((UINT32 *)chksm)[2] ||
	//		((UINT32 *)chksm)[3]) return 0;

	for (i=0;i<b;i++) {
		CTR_ENCRYPT(&(c->ck), (&ctr[3]), pt, c->keylen);
		((UINT32 *)pt)[0] ^= ((UINT32 *)ct)[0];
		((UINT32 *)pt)[1] ^= ((UINT32 *)ct)[1];
		((UINT32 *)pt)[2] ^= ((UINT32 *)ct)[2];
		((UINT32 *)pt)[3] ^= ((UINT32 *)ct)[3];
		ct += GHASH_BLK_SZ;
		pt += GHASH_BLK_SZ;
	}

	if (l) {
		CTR_ENCRYPT(&(c->ck), &ctr[3], ctr, c->keylen);
		for (i=0;i<l;i++)
			pt[i] = ((UINT8 *)ctr)[i] ^ ct[i];
	}
	return 1;
}

MODIFIERS void gcm_destroy_64k(gcm_ctx_64k *c) {
	memset(c, '0', sizeof(gcm_ctx_64k));
}
