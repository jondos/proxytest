#ifndef __CASYMCIPHER__
#define __CASYMCIPHER__

class CASymCipher
	{
		public:
			setEncryptionKey(unsigned char* key);
			setDecryptionKey(unsigned char* key);
			encrypt(unsigned char* in,int len);
			decrypt(unsigned char* in,int len);
		protected:
			BF_KEY keyEnc,keyDec;
	};

#endif