#ifndef __CASOCKETLIST__
#define __CASOCKETLIST__
#include "CAMuxSocket.hpp"
#include "CASymCipher.hpp"

typedef struct connlist
	{
		CASymCipher* pCipher;
		connlist* next;
		union
			{
				CASocket* pSocket;
				HCHANNEL outChannel;
			};
		HCHANNEL id;
	} CONNECTIONLIST,CONNECTION;
		
struct t_MEMBLOCK;

class CASocketList
	{
		public:
			CASocketList();
			~CASocketList();
			SINT32 add(HCHANNEL id,CASocket* pSocket,CASymCipher* pCipher);
			SINT32 add(HCHANNEL in,HCHANNEL out,CASymCipher* pCipher);
			bool	get(HCHANNEL in,CONNECTION* out);
			bool	get(CONNECTION* in,HCHANNEL out);
			CASocket* remove(HCHANNEL id);
			CONNECTION* getFirst();
			CONNECTION* getNext();
		protected:
			int increasePool();
			CONNECTIONLIST* connections;
			CONNECTIONLIST* pool;
			#ifdef _REENTRANT
//				CRITICAL_SECTION cs;
			#endif
			CONNECTIONLIST* aktEnumPos;
			t_MEMBLOCK* memlist;
	};	
#endif
