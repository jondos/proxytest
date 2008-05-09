#ifndef __MXML_INPUT_SOURCE__
#define __MXML_INPUT_SOURCE__

class InputSource
	{
		protected:
			virtual char* getBuff() const=0;
			friend class XercesDOMParser;
	};

#endif //__MXML_INPUT_SOURCE__
