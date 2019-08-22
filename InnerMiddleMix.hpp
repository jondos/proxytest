#include "CAMiddleMix.hpp"

#ifdef USE_SGX_INSIDE
class InnerMiddleMix{
	public:
		InnerMiddleMix(){
			m_pMiddleMixChannelList=NULL;
		}
		
		~InnerMiddleMix(){
			clean();
		}
		
		SINT32 start();
		
	friend THREAD_RETURN loopUpstream(void*);
	friend THREAD_RETURN loopDownstream(void*);	
		
	private:
		SINT32 loop();
		SINT32 init();
		
		SINT32 clean();
		CAASymCipher* m_pRSA;
		CAMiddleMixChannelList* m_pMiddleMixChannelList;

		

		/*shared memory*/
		const char *upstreamMemoryPreName="upstreamshmempre";
		const char *upstreamMemoryPostName="upstreamshmempost";
		const char *downstreamMemoryPreName="downstreamshmempre";
		const char *downstreamMemoryPostName="downstreamshmempost";

		void* upstreamPreBuffer;
		void* upstreamPostBuffer;
		void* downstreamPreBuffer;
		void* downstreamPostBuffer;
		
		/*shared memory semaphors*/
		int upstreamSemPreId, upstreamSemPostId;
		int downstreamSemPreId, downstreamSemPostId;
		
		int accessSharedMemory(int semId, void* destination, void* source, int mode);


};
#endif //USE_SGX_INSIDE
