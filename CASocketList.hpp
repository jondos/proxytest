#include "CASocket.hpp"
#include "CAMuxSocket.hpp"

typedef struct connlist
	{
		CASocket* pSocket;
		HCHANNEL id;
		connlist* next;
	} CONNECTIONLIST,CONNECTION;
		
//typedef CONNECTIONLIST CONNECTION;

class CASocketList
	{
		public:
			CASocketList();
			~CASocketList();
//			int add(CASocket* pSocket);
			int add(HCHANNEL id,CASocket* pSocket);
			CASocket* get(HCHANNEL id);
			CASocket* remove(HCHANNEL id);
			CONNECTION* getFirst();
			CONNECTION* getNext();
		protected:
			CONNECTIONLIST* connections;
			CONNECTIONLIST* pool;
			CRITICAL_SECTION cs;
			CONNECTIONLIST* aktEnumPos;
	};	