#include "CASocket.hpp"

typedef struct connlist
	{
		CASocket* pSocket;
		int id;
		connlist* next;
	} CONNECTIONLIST,CONNECTION;
		
//typedef CONNECTIONLIST CONNECTION;

class CASocketList
	{
		public:
			CASocketList();
			~CASocketList();
			int add(CASocket* pSocket);
			int add(int id,CASocket* pSocket);
			CASocket* get(int id);
			CASocket* remove(int id);
			CONNECTION* getFirst();
			CONNECTION* getNext();
		protected:
			CONNECTIONLIST* connections;
			CONNECTIONLIST* pool;
			CRITICAL_SECTION cs;
			CONNECTIONLIST* aktEnumPos;
	};	