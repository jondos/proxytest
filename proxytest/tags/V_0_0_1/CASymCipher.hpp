#ifndef __CASYMCIPHER__
#define __CASYMCIPHER__

class CASymCipher
	{
		public:
			int setEncryptionKey(unsigned char* key);
			int setDecryptionKey(unsigned char* key);
			int encrypt(unsigned char* in,int len);
			int decrypt(unsigned char* in,int len);
		protected:
			BF_KEY keyEnc,keyDec;
	};

#endif