class CABase64
	{
		public:
			static SINT32 decode(UINT8* in,UINT32 len,UINT8* out,UINT32* outlen);
			static SINT32 encode(UINT8* in,UINT32 len,UINT8* out,UINT32* outlen);
	};
