#include "CAMuxSocket.hpp"
#include "CASocketList.hpp"

typedef struct ReverseMuxList_t
	{
		CAMuxSocket* pMuxSocket;
		HCHANNEL inChannel;
		HCHANNEL outChannel;
		CASymCipher* pCipher;
		ReverseMuxList_t* next;
	} REVERSEMUXLIST,REVERSEMUXLISTENTRY;

typedef struct MuxList_t
	{
		CAMuxSocket* pMuxSocket;
		CASocketList* pSocketList;
		MuxList_t* next;
	} MUXLIST,MUXLISTENTRY;



class CAMuxChannelList
	{
		public:
			CAMuxChannelList();
			int add(CAMuxSocket* pMuxSocket);
			MUXLISTENTRY* get(CAMuxSocket* pMuxSocket);
			bool remove(CAMuxSocket* pMuxSocket,MUXLISTENTRY* pEntry);
			int add(MUXLISTENTRY* pEntry,HCHANNEL in,HCHANNEL out,CASymCipher* pCipher);
			bool get(MUXLISTENTRY* pEntry,HCHANNEL in,CONNECTION* out);
			REVERSEMUXLISTENTRY* get(HCHANNEL out);
			bool remove(HCHANNEL out,REVERSEMUXLISTENTRY* reverseEntry);
			MUXLISTENTRY* getFirst();
			MUXLISTENTRY* getNext();
		protected:
			MUXLIST* list;
			MUXLISTENTRY* aktEnumPos;
			REVERSEMUXLIST* reverselist;
	};

