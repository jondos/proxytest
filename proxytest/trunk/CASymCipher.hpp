#ifndef __CASYMCIPHER__
#define __CASYMCIPHER__

#define KEY_SIZE 16
class CASymCipher
	{
		public:
			CASymCipher(){bEncKeySet=false;}
			SINT32 generateEncryptionKey();
			SINT32 getEncryptionKey(UINT8* key);
			SINT32 setEncryptionKey(UINT8* key);
			bool isEncyptionKeyValid();
			SINT32 setDecryptionKey(UINT8* key);
			SINT32 encrypt(UINT8* in,UINT32 len);
			SINT32 decrypt(UINT8* in,UINT8* out,UINT32 len);
		protected:
			BF_KEY keyEnc,keyDec;
			UINT8 rawKeyEnc[16];
			bool bEncKeySet;
	};

#endif
