#define SIGKEY_XML 1
class CASignature
	{
		public:
			CASignature();
			~CASignature();
			int setSignKey(char* buff,int len,int type);
			int sign(unsigned char* in,int inlen,unsigned char* sig,unsigned int* siglen);
			int getSignatureSize();
		private:
			DSA* dsa;
			int parseSignKeyXML(char* buff,int len);
	};