#ifndef __CASOCKETLIST__
#define __CASOCKETLIST__
#include "CASocket.hpp"
#include "CAMuxSocket.hpp"
#include "CASymCipher.hpp"

typedef struct connlist
	{
		HCHANNEL id;
		union
			{
				CASocket* pSocket;
				HCHANNEL outChannel;
			};
		CASymCipher* pCipher;
		connlist* next;
	} CONNECTIONLIST,CONNECTION;
		
struct t_MEMBLOCK;

class CASocketList
	{
		public:
			CASocketList();
			~CASocketList();
			int add(HCHANNEL id,CASocket* pSocket,CASymCipher* pCipher=NULL);
			int add(HCHANNEL in,HCHANNEL out,CASymCipher* pCipher=NULL);
	//		CONNECTION get(HCHANNEL id);
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
				CRITICAL_SECTION cs;
			#endif
			CONNECTIONLIST* aktEnumPos;
			t_MEMBLOCK* memlist;
	};	
#endif