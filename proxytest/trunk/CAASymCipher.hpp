#define RSA_SIZE 128
class CAASymCipher
	{
		public:
			CAASymCipher();
			~CAASymCipher();
			int decrypt(unsigned char* from,unsigned char* to);
			int encrypt(unsigned char* from,unsigned char* to);
		private:
			RSA* rsa;
	};