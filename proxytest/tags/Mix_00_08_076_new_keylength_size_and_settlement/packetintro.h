/*$Id$*/
/*********************************************************************************************************/
/*                                                                                                       */
/*  tramo V0.9, server traffic monitor, (c) SDTFA 05/2007                                                */
/*                                                                                                       */
/*  Declarations PacketIntro                                                                             */
/*                                                                                                       */
/* Date      CRQ   Revision  Change description                                            Autor         */
/*-------------------------------------------------------------------------------------------------------*/
/* 16.05.07        1.0       file created                                                  Freddy        */
/*                                                                                                       */
/*********************************************************************************************************/
#ifdef SDTFA

#ifndef	PACKETINTRO_H
#define	PACKETINTRO_H

#define	SDTFA_MIXPACKET_SIZE		998 				/* size of a mix packet */
#define	SDTFA_MIX_SHARED_KEY		0xAB0B5EA7			/* key for shm access */
#define	SDTFA_SHM_MODE		0664				/* owner and group rw, other read */

  /** Mix shared data */

class SDTFA_MixShared
  {
  public:
    unsigned int PacketCount;			/**< Count of mix packets */
  };

void SDTFA_IncrementShmPacketCount(void);

#endif
#endif