#include "StdAfx.h"
#include "InnerMiddleMix.hpp"
#include "CAThread.hpp"
#include "CAMsg.hpp"
#include "CABase64.hpp"
#include "CAPool.hpp"

#ifdef WITH_SGX

SINT32 InnerMiddleMix::start(){
	if(init()==E_SUCCESS)
		loop();
		
	clean();
	return E_SUCCESS;

}

int InnerMiddleMix::accessSharedMemory(int semId, void* destination, void* source, int mode){
	
	if(locksem(semId, mode)==-1) return -1;
	memcpy(destination, source, sizeof(tPoolEntry));
	if(unlocksem(semId, 1-mode)==-1) return -1;
	
	return 1;
}

SINT32 InnerMiddleMix::init(){

	/*initialize Shared Memory and their Semaphors*/
	int upstreamShMemPre_fd,upstreamShMemPost_fd;
	int downstreamShMemPre_fd,downstreamShMemPost_fd;
   	void *upstreamShMemPreData, *upstreamShMemPostData;
      	void *downstreamShMemPreData, *downstreamShMemPostData;

	/* open the shared memory segment */
	upstreamShMemPre_fd = shm_open(upstreamMemoryPreName, O_RDWR, S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP | S_IWOTH | S_IROTH);
	if (upstreamShMemPre_fd == -1) {
		printf("shared memory failed\n");
		exit(-1);
	}
	upstreamShMemPost_fd = shm_open(upstreamMemoryPostName, O_RDWR, S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP | S_IWOTH | S_IROTH);
	if (upstreamShMemPost_fd == -1) {
		printf("shared memory failed\n");
		exit(-1);
	}
	downstreamShMemPre_fd = shm_open(downstreamMemoryPreName, O_RDWR, S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP | S_IWOTH | S_IROTH);
	if (downstreamShMemPre_fd == -1) {
		printf("shared memory failed\n");
		exit(-1);
	}
	downstreamShMemPost_fd = shm_open(downstreamMemoryPostName, O_RDWR, S_IRUSR | S_IWUSR | S_IWGRP | S_IRGRP | S_IWOTH | S_IROTH);
	if (downstreamShMemPost_fd == -1) {
		printf("shared memory failed\n");
		exit(-1);
	}

	/* now map the shared memory segment in the address space of the process */
	upstreamShMemPreData = mmap(0,SIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED, upstreamShMemPre_fd, 0);
	if (upstreamShMemPreData == MAP_FAILED) {
		printf("Map failed\n");
		exit(-1);
	}
	upstreamShMemPostData = mmap(0,SIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED, upstreamShMemPost_fd, 0);
	if (upstreamShMemPostData == MAP_FAILED) {
		printf("Map failed\n");
		exit(-1);
	}
	downstreamShMemPreData = mmap(0,SIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED, downstreamShMemPre_fd, 0);
	if (downstreamShMemPreData == MAP_FAILED) {
		printf("Map failed\n");
		exit(-1);
	}
	downstreamShMemPostData = mmap(0,SIZE, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_SHARED, downstreamShMemPost_fd, 0);
	if (downstreamShMemPostData == MAP_FAILED) {
		printf("Map failed\n");
		exit(-1);
	}
	upstreamSemPreId = *(int *) upstreamShMemPreData;
   	upstreamPreBuffer = upstreamShMemPreData + sizeof (int);

  	upstreamSemPostId = *(int *) upstreamShMemPostData;
  	upstreamPostBuffer = upstreamShMemPostData + sizeof (int);
  
	downstreamSemPreId = *(int *) downstreamShMemPreData;
   	downstreamPreBuffer = downstreamShMemPreData + sizeof (int);

  	downstreamSemPostId = *(int *) downstreamShMemPostData;
  	downstreamPostBuffer = downstreamShMemPostData + sizeof (int);
	CAMsg::printMsg(LOG_INFO,"configured... now send rsa");
	/*RSA*/
	m_pRSA= new CAASymCipher();
	if(m_pRSA->generateKeyPair(1024)!=E_SUCCESS){
		CAMsg::printMsg(LOG_CRIT,"Could not generate a valid key pair\n");
		return E_UNKNOWN;
	}
	


	


	UINT32 keyFileBuffLen=8096;
	
	UINT8* keyBuff=new UINT8[keyFileBuffLen];
	m_pRSA->getPublicKeyAsXML(keyBuff,&keyFileBuffLen);
	locksem(upstreamSemPostId,SN_EMPTY);
	memcpy(upstreamPostBuffer,keyBuff, keyFileBuffLen);
	unlocksem(upstreamSemPostId, SN_FULL);
	delete[] keyBuff;
	
	
	
	m_pMiddleMixChannelList=new CAMiddleMixChannelList();
	
	//wait for key exchange in middle mix and decode symmetric keys
	
	UINT8 buff[2048];
	UINT32 bufflen=2048;
	//get cipher value
	locksem(upstreamSemPostId,SN_FULL);
	memcpy(&buff,upstreamPostBuffer,bufflen);
	unlocksem(upstreamSemPostId,SN_EMPTY);
	//get new bufflen
	locksem(downstreamSemPostId,SN_FULL);
	memcpy(&bufflen,downstreamPostBuffer,sizeof(UINT32));
	unlocksem(downstreamSemPostId,SN_EMPTY);
	
	CABase64::decode(buff,bufflen,buff,&bufflen);
	m_pRSA->decrypt(buff,buff);
	//send keys
	locksem(upstreamSemPostId,SN_EMPTY);
	memcpy(upstreamPostBuffer,buff,bufflen);
	unlocksem(upstreamSemPostId,SN_FULL);
	
	return E_SUCCESS;
}

#define MIDDLE_MIX_SIZE_OF_SYMMETRIC_KEYS 2*KEY_SIZE
#define MIDDLE_MIX_ASYM_PADDING_SIZE 42

THREAD_RETURN loopUpstream(void* param){
	
	InnerMiddleMix* pMix = (InnerMiddleMix*) param;
	HCHANNEL channelOut = 0, channelIn = 0;
	tPoolEntry* pPoolEntry=new tPoolEntry;
	MIXPACKET* pMixPacket=&pPoolEntry->packet;
	UINT8* tmpRSABuff=new UINT8[RSA_SIZE];
	UINT32 rsaOutLen=RSA_SIZE;			
			
	CASymCipher* pCipher;	 
	

	#ifdef USE_POOL
		CAPool* pPool=new CAPool(MIX_POOL_SIZE);
	#endif	
	CAMsg::printMsg(LOG_INFO,"start upstreamloop");
	
	while(1)
	{	

		if(pMix->accessSharedMemory(pMix->upstreamSemPreId, pPoolEntry, pMix->upstreamPreBuffer, SN_FULL)==-1) break;
#ifdef USE_POOL		
		if(pMixPacket->channel==DUMMY_CHANNEL)
		{
			pPool->pool(pPoolEntry);
			
			if(pMix->accessSharedMemory(pMix->upstreamSemPostId, pMix->upstreamPostBuffer, pPoolEntry, SN_EMPTY)==-1)
				break;
			
			continue;	
		}
#endif		
		channelIn = pMixPacket->channel;
		if(pMix->m_pMiddleMixChannelList->getInToOut(pMixPacket->channel,&channelOut,&pCipher)!=E_SUCCESS)
		{//new connection ?
			if(pMixPacket->flags & CHANNEL_OPEN) //if not channel-open flag set -->drop this packet
			{	
				#ifdef _DEBUG
					CAMsg::printMsg(LOG_DEBUG,"New Connection from previous Mix!\n");
				#endif
				pMix->m_pRSA->decryptOAEP(pMixPacket->data,tmpRSABuff,&rsaOutLen);
				#ifdef REPLAY_DETECTION
					// replace time(NULL) with the real timestamp ()
					// packet-timestamp + m_u64ReferenceTime
					UINT32 stamp=((UINT32)(tmpRSABuff[13]<<16)+(UINT32)(tmpRSABuff[14]<<8)+(UINT32)(tmpRSABuff[15]))*REPLAY_BASE;
					if(pMix->m_pReplayDB->insert(tmpRSABuff,stamp+pMix->m_u64ReferenceTime)!=E_SUCCESS)
	//				if(pMix->m_pReplayDB->insert(tmpRSABuff,time(NULL))!=E_SUCCESS)
					{
						CAMsg::printMsg(LOG_INFO,"Replay: Duplicate packet ignored.\n");
						continue;
					}
				#endif

				pCipher=new CASymCipher();
				pCipher->setKeys(tmpRSABuff,MIDDLE_MIX_SIZE_OF_SYMMETRIC_KEYS);
				pCipher->crypt1(pMixPacket->data+RSA_SIZE,pMixPacket->data+rsaOutLen-MIDDLE_MIX_SIZE_OF_SYMMETRIC_KEYS,DATA_SIZE-RSA_SIZE);
				memcpy(pMixPacket->data,tmpRSABuff+MIDDLE_MIX_SIZE_OF_SYMMETRIC_KEYS,rsaOutLen-MIDDLE_MIX_SIZE_OF_SYMMETRIC_KEYS);
				getRandom(pMixPacket->data+DATA_SIZE-MIDDLE_MIX_SIZE_OF_SYMMETRIC_KEYS-MIDDLE_MIX_ASYM_PADDING_SIZE,MIDDLE_MIX_SIZE_OF_SYMMETRIC_KEYS+MIDDLE_MIX_ASYM_PADDING_SIZE);
				pMix->m_pMiddleMixChannelList->add(pMixPacket->channel,pCipher,&channelOut);
				pMixPacket->channel=channelOut;
				#ifdef USE_POOL
					pPool->pool(pPoolEntry);
				#endif
				#ifdef LOG_CRIME
					crimeSurveillanceUpstream(pMixPacket, channelIn);
				#endif

				if(pMix->accessSharedMemory(pMix->upstreamSemPostId, pMix->upstreamPostBuffer, pPoolEntry, SN_EMPTY)==-1) break;
			}
			else
			{
				pMixPacket->channel=0;
				pMixPacket->flags=CHANNEL_DUMMY;
				if(pMix->accessSharedMemory(pMix->upstreamSemPostId, pMix->upstreamPostBuffer, pPoolEntry, SN_EMPTY)==-1) break;
			}
		}
		else
		{//established connection
			pCipher->crypt1(pMixPacket->data,pMixPacket->data,DATA_SIZE);
			pCipher->unlock();
			#ifdef USE_POOL
				pPool->pool(pPoolEntry);
			#endif
			if((pMixPacket->flags&CHANNEL_CLOSE)!=0)
			{//Channel close received -->remove channel form channellist
				pMix->m_pMiddleMixChannelList->remove(pMixPacket->channel);
			}
			pMixPacket->channel=channelOut;
			#ifdef LOG_CRIME
				crimeSurveillanceUpstream(pMixPacket, channelIn);
			#endif

			/*locksem(pMix->upstreamSemPostId,SN_EMPTY);
			memcpy(pMix->upstreamPostBuffer,pPoolEntry,sizeof(tPoolEntry));
			unlocksem(pMix->upstreamSemPostId,SN_FULL);*/
			if(pMix->accessSharedMemory(pMix->upstreamSemPostId, pMix->upstreamPostBuffer, pPoolEntry, SN_EMPTY)==-1) break;
		}
	


	}
	#ifdef USE_POOL
		delete pPool;
		pPool=NULL;
	#endif
	delete pPoolEntry;
	pPoolEntry=NULL;
	THREAD_RETURN_SUCCESS;
}		

THREAD_RETURN loopDownstream(void* param){
	InnerMiddleMix* pMix = (InnerMiddleMix*) param;
	HCHANNEL channelIn;
	CASymCipher* pCipher;
	tPoolEntry* pPoolEntry=new tPoolEntry;
	MIXPACKET* pMixPacket=&pPoolEntry->packet;

	
#ifdef USE_POOL
		CAPool* pPool=new CAPool(MIX_POOL_SIZE);
#endif

	while(1)
	{
		if(pMix->accessSharedMemory(pMix->downstreamSemPreId, pPoolEntry, pMix->downstreamPreBuffer, SN_FULL)==-1) break;
		
#ifdef USE_POOL		
		if(pMixPacket->channel==DUMMY_CHANNEL)
		{
			pPool->pool(pPoolEntry);
			
			if(pMix->accessSharedMemory(pMix->downstreamSemPostId, pMix->downstreamPostBuffer, pPoolEntry, SN_EMPTY)==-1) 
				break;
			
			continue;	
		}
#endif		
		if(pMix->m_pMiddleMixChannelList->getOutToIn(&channelIn,pMixPacket->channel,&pCipher)==E_SUCCESS)
		{//connection found
#ifdef LOG_CRIME
				HCHANNEL channelOut = pMixPacket->channel;
#endif
			pMixPacket->channel=channelIn;
#ifdef LOG_CRIME
			if((pMixPacket->flags&CHANNEL_SIG_CRIME)==CHANNEL_SIG_CRIME)
			{
				getRandom(pMixPacket->data,DATA_SIZE);
				//Log in and out channel number, to allow
				CAMsg::printMsg(LOG_CRIT,"Detecting crime activity - previous mix channel: %u, "
						"next mix channel: %u\n", channelIn, channelOut);
			}
			else
#endif
			pCipher->crypt2(pMixPacket->data,pMixPacket->data,DATA_SIZE);
			pCipher->unlock();
			#ifdef USE_POOL
				pPool->pool(pPoolEntry);
			#endif
			if((pMixPacket->flags&CHANNEL_CLOSE)!=0)
			{//Channel close received -->remove channel form channellist
				pMix->m_pMiddleMixChannelList->remove(channelIn);
			}
			if(pMix->accessSharedMemory(pMix->downstreamSemPostId, pMix->downstreamPostBuffer, pPoolEntry, SN_EMPTY)==-1) break;

		}
			
	
	
	
	
	}
	#ifdef USE_POOL
		delete pPool;
		pPool=NULL;
	#endif
	delete pPoolEntry;
	pPoolEntry=NULL;

	THREAD_RETURN_SUCCESS;
}

SINT32 InnerMiddleMix::loop(){
	
	CAThread uThread;
	uThread.setMainLoop(loopUpstream);
	uThread.start(this);
	
	CAThread dThread;
	dThread.setMainLoop(loopDownstream);
	dThread.start(this);
	
	uThread.join();
	dThread.join();
	
	return E_UNKNOWN;


}

SINT32 InnerMiddleMix::clean(){
	delete m_pMiddleMixChannelList;
	m_pMiddleMixChannelList=NULL;
	
	delete m_pRSA;
	m_pRSA=NULL;
	
	return E_SUCCESS;
}

#endif //WITH_SGX


