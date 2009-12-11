/*$Id$*/
/*********************************************************************************************************/
/*                                                                                                       */
/*  tramo V0.9, server traffic monitor, (c) SDTFA 05/2007                                                */
/*                                                                                                       */
/*  PacketIntro, Send packet count to tramo process                                                      */
/*                                                                                                       */
/* Date      CRQ   Revision  Change description                                            Autor         */
/*-------------------------------------------------------------------------------------------------------*/
/* 16.05.07        1.0       file created                                                  Freddy        */
/*                                                                                                       */
/*********************************************************************************************************/

#ifdef SDTFA

#include "packetintro.h"
#include "StdAfx.h"
#include "CAMsg.hpp"

#include <stdlib.h>
#include <sys/shm.h>
#include <errno.h>
#include <assert.h>

static void SDTFA_NeedMixSharedMemory(void);
static SDTFA_MixShared *SDTFA_TryMixSharedMemory(void);

static SDTFA_MixShared *SDTFA_MixShm = NULL;		/**< Shared memory with mix */

/**
 *  Increment the packet count in the shared memory
 */

void SDTFA_IncrementShmPacketCount(void)
{
// create/attach shared memory (if not done)
if (!MixShm)
  SDTFA_NeedMixSharedMemory();
// atomic access (32 bit)
assert(sizeof(MixShm->PacketCount)==4);
MixShm->PacketCount++;
}

/**
 *  Create/attach shared memory of mix process
 */

static void SDTFA_NeedMixSharedMemory(void)
{
// try to create/attach shared memory of mix process
if (!SDTFA_TryMixSharedMemory())
  {
  // if not creatable, use dummy
  MixShm=(SDTFA_MixShared*)malloc(sizeof(SDTFA_MixShared));
  MixShm->PacketCount=0;
  }
}

/**
 *  Try to create/attach shared memory of mix process
 */

static SDTFA_MixShared *SDTFA_TryMixSharedMemory(void)
{
int mid; void *base;
CAMsg::printMsg(LOG_DEBUG,"Create/attach shared memory of mix process.\n");
assert(sizeof(key_t)==4);
if ((mid=shmget(SDTFA_MIX_SHARED_KEY,sizeof(SDTFA_MixShared),SHM_MODE))<0)
  {
  // no such file or directory ?
  if (errno==ENOENT)
    {
    // cant attach, try to create
    if ((mid=shmget(SDTFA_MIX_SHARED_KEY,sizeof(SDTFA_MixShared),IPC_CREAT|SHM_MODE))<0)
      {
      CAMsg::printMsg(LOG_ERR,"Cant create shared memory of mix process.\n");
      return(NULL);
      }
    }
  else
    {
    CAMsg::printMsg(LOG_ERR,"Cant attach to shared memory of mix process.\n");
    return(NULL);
    }
  }
// locate the shared memory
if ((base=shmat(mid,NULL,0))==(void*)(-1))
  {
  CAMsg::printMsg(LOG_ERR,"Cant locate shared memory.\n");
  return(NULL);
  }
// success
MixShm=(SDTFA_MixShared*)base;
return(MixShm);
}

#endif