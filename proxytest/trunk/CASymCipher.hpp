#ifndef __CASYMCIPHER__
#define __CASYMCIPHER__

#define KEY_SIZE 16
class CASymCipher
	{
		public:
			CASymCipher(){bEncKeySet=false;}
			int setEncryptionKey(unsigned char* key);
			bool isEncyptionKeyValid();
			int setDecryptionKey(unsigned char* key);
			int encrypt(unsigned char* in,int len);
			int decrypt(unsigned char* in,unsigned char* out,int len);
		protected:
			BF_KEY keyEnc,keyDec;
			bool bEncKeySet;
	};

#endif