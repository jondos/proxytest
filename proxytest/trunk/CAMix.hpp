#ifndef __CAMIX__
#define __CAMIX__
class CAMix
	{
		public:
			SINT32 start();
		protected:
			virtual SINT32 init()=0;
			virtual SINT32 loop()=0;
	};
#endif