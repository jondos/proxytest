#include "CADatagramSocket.hpp"
class CARoundTripTime
	{
		public:
			CARoundTripTime(){m_bRun=false;}
			SINT32 start();
			SINT32 stop();
			bool getRun(){return m_bRun;}
		private:
			bool m_bRun;
	};