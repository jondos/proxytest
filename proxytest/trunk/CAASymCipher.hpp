#ifndef __CAASYMCIPHER__
#define __CAASYMCIPHER__
#define RSA_SIZE 128
class CAASymCipher
	{
		public:
			CAASymCipher();
			~CAASymCipher();
			int decrypt(UINT8* from,UINT8* to);
			int encrypt(UINT8* from,UINT8* to);
			SINT32 generateKeyPair(UINT32 size);
			SINT32 getPublicKey(UINT8* buff,UINT32 *len);
			SINT32 getPublicKeySize();
			SINT32 setPublicKey(UINT8* buff,UINT32 *len);
		private:
			RSA* rsa;
	};

#endif