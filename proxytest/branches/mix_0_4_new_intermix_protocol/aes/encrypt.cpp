#ifndef __AES_ENCRYPT__
#define __AES_ENCRYPT__

#include "rijndael-api-fst.h"
#include <stdlib.h>
#define SC	((BC - 4) >> 1)

#include "boxes-fst.dat.h"
#define ROUNDS 10 //128/*keyLen*//32 + 6 //	ROUNDS = 128/*keyLen*//32 + 6;


int rijndaelEncrypt (word8 a[16], word8 b[16], word8 rk[MAXROUNDS+1][4][4])
{
	/* Encryption of one block. 
	 */
	int r;
   word8 temp[4][4];

    *((word32*)temp[0]) = *((word32*)a) ^ *((word32*)rk[0][0]);
    *((word32*)temp[1]) = *((word32*)(a+4)) ^ *((word32*)rk[0][1]);
    *((word32*)temp[2]) = *((word32*)(a+8)) ^ *((word32*)rk[0][2]);
    *((word32*)temp[3]) = *((word32*)(a+12)) ^ *((word32*)rk[0][3]);
    *((word32*)b) = *((word32*)T1[temp[0][0]])
           ^ *((word32*)T2[temp[1][1]])
           ^ *((word32*)T3[temp[2][2]]) 
           ^ *((word32*)T4[temp[3][3]]);
    *((word32*)(b+4)) = *((word32*)T1[temp[1][0]])
           ^ *((word32*)T2[temp[2][1]])
           ^ *((word32*)T3[temp[3][2]]) 
           ^ *((word32*)T4[temp[0][3]]);
    *((word32*)(b+8)) = *((word32*)T1[temp[2][0]])
           ^ *((word32*)T2[temp[3][1]])
           ^ *((word32*)T3[temp[0][2]]) 
           ^ *((word32*)T4[temp[1][3]]);
    *((word32*)(b+12)) = *((word32*)T1[temp[3][0]])
           ^ *((word32*)T2[temp[0][1]])
           ^ *((word32*)T3[temp[1][2]]) 
           ^ *((word32*)T4[temp[2][3]]);
   for(r = 1; r < ROUNDS-1; r++) {
		*((word32*)temp[0]) = *((word32*)b) ^ *((word32*)rk[r][0]);
		*((word32*)temp[1]) = *((word32*)(b+4)) ^ *((word32*)rk[r][1]);
		*((word32*)temp[2]) = *((word32*)(b+8)) ^ *((word32*)rk[r][2]);
		*((word32*)temp[3]) = *((word32*)(b+12)) ^ *((word32*)rk[r][3]);
   *((word32*)b) = *((word32*)T1[temp[0][0]])
           ^ *((word32*)T2[temp[1][1]])
           ^ *((word32*)T3[temp[2][2]]) 
           ^ *((word32*)T4[temp[3][3]]);
   *((word32*)(b+4)) = *((word32*)T1[temp[1][0]])
           ^ *((word32*)T2[temp[2][1]])
           ^ *((word32*)T3[temp[3][2]]) 
           ^ *((word32*)T4[temp[0][3]]);
   *((word32*)(b+8)) = *((word32*)T1[temp[2][0]])
           ^ *((word32*)T2[temp[3][1]])
           ^ *((word32*)T3[temp[0][2]]) 
           ^ *((word32*)T4[temp[1][3]]);
   *((word32*)(b+12)) = *((word32*)T1[temp[3][0]])
           ^ *((word32*)T2[temp[0][1]])
           ^ *((word32*)T3[temp[1][2]]) 
           ^ *((word32*)T4[temp[2][3]]);
   }
   /* last round is special */   
	*((word32*)temp[0]) = *((word32*)b) ^ *((word32*)rk[ROUNDS-1][0]);
	*((word32*)temp[1]) = *((word32*)(b+4)) ^ *((word32*)rk[ROUNDS-1][1]);
	*((word32*)temp[2]) = *((word32*)(b+8)) ^ *((word32*)rk[ROUNDS-1][2]);
	*((word32*)temp[3]) = *((word32*)(b+12)) ^ *((word32*)rk[ROUNDS-1][3]);
   b[0] = T1[temp[0][0]][1];
   b[1] = T1[temp[1][1]][1];
   b[2] = T1[temp[2][2]][1]; 
   b[3] = T1[temp[3][3]][1];
   b[4] = T1[temp[1][0]][1];
   b[5] = T1[temp[2][1]][1];
   b[6] = T1[temp[3][2]][1]; 
   b[7] = T1[temp[0][3]][1];
   b[8] = T1[temp[2][0]][1];
   b[9] = T1[temp[3][1]][1];
   b[10] = T1[temp[0][2]][1]; 
   b[11] = T1[temp[1][3]][1];
   b[12] = T1[temp[3][0]][1];
   b[13] = T1[temp[0][1]][1];
   b[14] = T1[temp[1][2]][1]; 
   b[15] = T1[temp[2][3]][1];
	*((word32*)b) ^= *((word32*)rk[ROUNDS][0]);
	*((word32*)(b+4)) ^= *((word32*)rk[ROUNDS][1]);
	*((word32*)(b+8)) ^= *((word32*)rk[ROUNDS][2]);
	*((word32*)(b+12)) ^= *((word32*)rk[ROUNDS][3]);

	return 0;
}

inline int rijndaelKeySched (word8 k[MAXKC][4], word8 W[MAXROUNDS+1][4][4])
{
	/* Calculate the necessary round keys
	 * The number of calculations depends on keyBits and blockBits
	 */ 
	int j, r, t, rconpointer = 0;
	word8 tk[MAXKC][4];
	int KC = ROUNDS - 6;
	
	for(j = KC-1; j >= 0; j--)
		*((word32*)tk[j]) = *((word32*)k[j]);
	r = 0;
	t = 0;
	/* copy values into round key array */
	for(j = 0; (j < KC) && (r < (ROUNDS+1)); ) {
		for (; (j < KC) && (t < 4); j++, t++)
			*((word32*)W[r][t]) = *((word32*)tk[j]);
		if (t == 4) {
			r++;
			t = 0;
		}
	}
		
	while (r < (ROUNDS+1)) { /* while not enough round key material calculated */
		/* calculate new values */
		tk[0][0] ^= S[tk[KC-1][1]];
		tk[0][1] ^= S[tk[KC-1][2]];
		tk[0][2] ^= S[tk[KC-1][3]];
		tk[0][3] ^= S[tk[KC-1][0]];
		tk[0][0] ^= rcon[rconpointer++];

		if (KC != 8)
			for(j = 1; j < KC; j++)
				*((word32*)tk[j]) ^= *((word32*)tk[j-1]);
		else {
			for(j = 1; j < KC/2; j++)
				*((word32*)tk[j]) ^= *((word32*)tk[j-1]);
			tk[KC/2][0] ^= S[tk[KC/2 - 1][0]];
			tk[KC/2][1] ^= S[tk[KC/2 - 1][1]];
			tk[KC/2][2] ^= S[tk[KC/2 - 1][2]];
			tk[KC/2][3] ^= S[tk[KC/2 - 1][3]];
			for(j = KC/2 + 1; j < KC; j++)
				*((word32*)tk[j]) ^= *((word32*)tk[j-1]);
		}
		/* copy values into round key array */
		for(j = 0; (j < KC) && (r < (ROUNDS+1)); ) {
			for (; (j < KC) && (t < 4); j++, t++)
				*((word32*)W[r][t]) = *((word32*)tk[j]);
			if (t == 4) {
				r++;
				t = 0;
			}
		}
	}		

	return 0;
}


//KEylen=konst=128 bit!!!
int makeKey(keyInstance key, /*int keyLen,*/ char *keyMaterial)
{
	word8 k[MAXKC][4];
	int i;
	
	if (key == NULL) {
		return BAD_KEY_INSTANCE;
	}

//	if ((keyLen == 128) || (keyLen == 192) || (keyLen == 256)) { 
//		key->keyLen = keyLen;
//	} else {
//		return BAD_KEY_MAT;
//	}

//	ROUNDS = 128/*keyLen*//32 + 6;

	/* initialize key schedule: */
 	for(i = 0; i < 128/*key->keyLen*//8; i++) {
		k[i / 4][i % 4] = keyMaterial[i]; 
	}
	rijndaelKeySched (k, key/*->keySched*/);

	return 1;
}


#endif 
