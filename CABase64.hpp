class CABase64
	{
		public:
			static int decode(char* in,unsigned int len,char* out,unsigned int* outlen);
			static int encode(char* in,unsigned int len,char* out,unsigned int* outlen);
	};
