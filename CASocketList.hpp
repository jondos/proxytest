#include "CASocket.hpp"

typedef struct connlist
	{
		CASocket* pSocket;
		int id;
		connlist* next;
	} CONNECTIONLIST;

class CASocketList
	{
		public:
			CASocketList();
			~CASocketList();
			int add(CASocket* pSocket);
			CASocket* get(int id);
			CASocket* remove(int id);
		protected:
			CONNECTIONLIST* connections;
			CONNECTIONLIST* pool;
			CRITICAL_SECTION cs;
	};	