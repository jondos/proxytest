#ifndef __CAPOOL__
#define __CAPOOL__
#include "CAMuxSocket.hpp"

struct t_pool_list
	{
		struct t_pool_list * next;
		MIXPACKET mixpacket;
	};

typedef t_pool_list tPoolListEntry;

class CAPool
	{
		public:
			CAPool(UINT32 poolsize);
			~CAPool();
			
			SINT32 pool(MIXPACKET* pMixPacket); 
		
		private:
			tPoolListEntry* m_pPoolList;
		
	};
#endif
