/* rijndael-api-fst.h   v2.0   August '99
 * Optimised ANSI C code
 */


//#include <stdio.h>
//#include "rijndael-alg-fst.h"

#define MAXKC				(256/32)
#define MAXROUNDS			10 //14 fixed sized for 128 bit keys....

typedef unsigned char		word8;	
typedef unsigned short		word16;	
typedef unsigned int		word32;
/*  Defines:
	Add any additional defines you need
*/

//#define     DIR_ENCRYPT     0    /*  Are we encrpyting?  */
//#define     DIR_DECRYPT     1    /*  Are we decrpyting?  */
//#define     MODE_ECB        1    /*  Are we ciphering in ECB mode?   */
//#define     MODE_CBC        2    /*  Are we ciphering in CBC mode?   */
//#define     MODE_CFB1       3    /*  Are we ciphering in 1-bit CFB mode? */
//#define     TRUE            1
//#define     FALSE           0
//#define	BITSPERBLOCK		128		/* Default number of bits in a cipher block */

/*  Error Codes - CHANGE POSSIBLE: inclusion of additional error codes  */
//#define     BAD_KEY_DIR        -1  /*  Key direction is invalid, e.g.,
//					unknown value */
//#define     BAD_KEY_MAT        -2  /*  Key material not of correct 
//					length */
#define     BAD_KEY_INSTANCE   -3  /*  Key passed is not valid  */
//#define     BAD_CIPHER_MODE    -4  /*  Params struct passed to 
//					cipherInit invalid */
//#define     BAD_CIPHER_STATE   -5  /*  Cipher in wrong state (e.g., not 
//					initialized) */
//#define     BAD_BLOCK_LENGTH   -6 
//#define     BAD_CIPHER_INSTANCE   -7 


/*  CHANGE POSSIBLE:  inclusion of algorithm specific defines  */
//#define     MAX_KEY_SIZE	64  /* # of ASCII char's needed to
//					represent a key */
//#define     MAX_IV_SIZE		32  /* # bytes needed to
//					represent an IV  */

/*  Typedefs:

	Typedef'ed data storage elements.  Add any algorithm specific 
parameters at the bottom of the structs as appropriate.
*/

//typedef    unsigned char    BYTE;

/*  The structure for key information */
// For Konstant KEy of 128 Bit!!!!!
//typedef struct 
//	{
//		BYTE  direction;	/*  Key used for encrypting or decrypting? */
//    int   keyLen;	/*  Length of the key  */
      /*  The following parameters are algorithm dependent, replace or
      		add as necessary  */
//    int   blockLen;   /* block length */
 //   word8 keySched[MAXROUNDS+1][4][4];	/* key schedule		*/
 // } keyInstance;

typedef  word8 keyInstance[MAXROUNDS+1][4][4];	
int rijndaelEncrypt (word8 a[16], word8 b[16], word8 rk[MAXROUNDS+1][4][4]);
int makeKey(keyInstance key, /*int keyLen,*/ char *keyMaterial);

