#define RSA_SIZE 128
class CAASymCipher
	{
		public:
			CAASymCipher();
			~CAASymCipher();
			int decrypt(unsigned char* from,unsigned char* to);
			int encrypt(unsigned char* from,unsigned char* to);
			int generateKeyPair(int size);
			int getPublicKey(unsigned char* buff,int *len);
			int getPublicKeySize();
			int setPublicKey(unsigned char* buff,int *len);
		private:
			RSA* rsa;
	};

