#ifndef __CAQUEUE__
#define __CAQUEUE__
typedef struct _t_queue
	{
		UINT8* pBuff;
		UINT32 size;
		_t_queue* next;
	}QUEUE;

class CAQueue
	{
		public:
			CAQueue(){InitializeCriticalSection(&csQueue);m_Queue=NULL;}
			~CAQueue();
			SINT32 add(UINT8* buff,UINT32 size);
			SINT32 getNext(UINT8* pbuff,UINT32* psize);
			bool isEmpty(){return m_Queue==NULL;}
		private:
			QUEUE* m_Queue;
			QUEUE* m_lastElem;
			#ifdef _REENTRANT
				CRITICAL_SECTION csQueue;
			#endif
	
	};
#endif