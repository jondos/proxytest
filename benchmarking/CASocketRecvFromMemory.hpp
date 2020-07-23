#ifdef DO_MIDDLE_MIX_BENCHMARK
#include "../CASocket.hpp"
class CASocketRecvFromMemory:public CASocket
{
		public:
			SINT32 receive(UINT8 *buff, UINT32 len);

		private:
			SINT32 setSocket(SOCKET s);
			SINT32 create(SINT32 type, bool a_bShowTypicalError);
};
#endif //DO_MIDDLE_MIX_BENCHMARK

