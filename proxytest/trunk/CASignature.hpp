class CASignature
	{
		public:
			CASignature();
			~CASignature();
			int sign(unsigned char* in,int inlen,unsigned char* sig,unsigned int* siglen);
			int getSignatureSize();
		private:
			DSA* dsa;
	};