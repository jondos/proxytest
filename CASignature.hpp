#ifndef __CASIGNATURE__
#define __CASIGNATURE__
#define SIGKEY_XML 1
class CASignature
	{
		public:
			CASignature();
			~CASignature();
			int setSignKey(char* buff,int len,int type);
			int sign(unsigned char* in,int inlen,unsigned char* sig,unsigned int* siglen);
			int signXML(char* in,unsigned int inlen,char* out,unsigned int* outlen);
			int getSignatureSize();
			int getXMLSignatureSize();
		private:
			DSA* dsa;
			int parseSignKeyXML(char* buff,int len);
			int makeXMLCanonical(char* in,unsigned int len,char* out,unsigned int *outlen);
	};
#endif
