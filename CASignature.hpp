#ifndef __CASIGNATURE__
#define __CASIGNATURE__
#define SIGKEY_XML 1
class CASignature
	{
		public:
			CASignature();
			~CASignature();
			SINT32 setSignKey(UINT8* buff,UINT32 len,int type);
			SINT32 sign(UINT8* in,UINT32 inlen,UINT8* sig,UINT32* siglen);
			SINT32 signXML(UINT8* in,UINT32 inlen,UINT8* out,UINT32* outlen);
			SINT32 getSignatureSize();
			SINT32 getXMLSignatureSize();
		private:
			DSA* dsa;
			SINT32 parseSignKeyXML(UINT8* buff,UINT32 len);
			SINT32 makeXMLCanonical(UINT8* in,UINT32 len,UINT8* out,UINT32 *outlen);
	};
#endif
