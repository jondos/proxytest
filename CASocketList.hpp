#ifndef __CASOCKETLIST__
#define __CASOCKETLIST__
#include "CASocket.hpp"
#include "CAMuxSocket.hpp"

typedef struct connlist
	{
		HCHANNEL id;
		union
			{
				CASocket* pSocket;
				HCHANNEL outChannel;
			};
		connlist* next;
	} CONNECTIONLIST,CONNECTION;
		
class CASocketList
	{
		public:
			CASocketList();
			~CASocketList();
			int add(HCHANNEL id,CASocket* pSocket);
			int add(HCHANNEL in,HCHANNEL out);
			CASocket* get(HCHANNEL id);
			bool	get(HCHANNEL in,HCHANNEL* out);
			bool	get(HCHANNEL* in,HCHANNEL out);
			CASocket* remove(HCHANNEL id);
			CONNECTION* getFirst();
			CONNECTION* getNext();
		protected:
			CONNECTIONLIST* connections;
			CONNECTIONLIST* pool;
			CRITICAL_SECTION cs;
			CONNECTIONLIST* aktEnumPos;
	};	
#endif