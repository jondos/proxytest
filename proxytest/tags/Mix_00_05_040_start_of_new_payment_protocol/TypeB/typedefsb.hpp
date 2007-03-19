/*
 * Copyright (c) 2006, The JAP-Team 
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   - Redistributions of source code must retain the above copyright notice, 
 *     this list of conditions and the following disclaimer.
 *
 *   - Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *   - Neither the name of the University of Technology Dresden, Germany nor
 *     the names of its contributors may be used to endorse or promote
 *     products derived from this software without specific prior written
 *     permission. 
 *
 *  
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE
 */
#ifndef __TYPEDEFSB__
#define __TYPEDEFSB__

#define CHANNEL_DOWNSTREAM_PACKETS 10
#define CHANNEL_TIMEOUT 15
#define CHAIN_TIMEOUT 30
#define DEADLINE_TIMEOUT 5

#define CHAIN_ID_LENGTH 8

#ifdef DELAY_CHANNELS
  /* The maximum delay-bucket size of the data-chains. It's defined here
   * because currently only type-B mixes support it.
   */
  #define MAX_DELAY_BUCKET_SIZE 30000
#endif


#ifdef LOG_CHANNEL
  /* channel logging doesn't make much sense for type-B mixes -> we will log
   * chains instead of channels
   */
  #define LOG_CHAIN_STATISTICS
#endif


// chainflags for both directions
#define CHAINFLAG_STREAM_CLOSED 0x4000

// chainflags for upstream
#define CHAINFLAG_NEW_CHAIN 0x2000
#define CHAINFLAG_FAST_RESPONSE 0x8000

// chainflags for downstream
#define CHAINFLAG_CONNECTION_ERROR 0x8000
#define CHAINFLAG_UNKNOWN_CHAIN 0x2000

#define CHAINFLAG_LENGTH_MASK 0x03FF

#define MAX_FIRST_UPSTREAM_CHAINCELL_PAYLOAD DATA_SIZE - 2 - 1
#define MAX_SEQUEL_UPSTREAM_CHAINCELL_PAYLOAD DATA_SIZE - 2 - CHAIN_ID_LENGTH

#if ((defined(WIN32) || defined(__sgi)) && !defined(__GNUC__))
  #pragma pack(push, t_upstream_chain_cell)
  #pragma pack(1)

  struct t_first_upstream_chain_cell {
    UINT8 type;
    UINT8 data[MAX_FIRST_UPSTREAM_CHAINCELL_PAYLOAD];
  };

  struct t_sequel_upstream_chain_cell {
    UINT8 chainId[CHAIN_ID_LENGTH];
    UINT8 data[MAX_SEQUEL_UPSTREAM_CHAINCELL_PAYLOAD];
  };

  struct t_upstream_chain_cell {
    UINT16 lengthAndFlags;
    union {
      struct t_first_upstream_chain_cell firstCell;
      struct t_sequel_upstream_chain_cell sequelCell;
    };
  };

  #pragma pack(pop, t_upstream_chain_cell)
#else
  struct t_first_upstream_chain_cell {
    UINT8 type;
    UINT8 data[MAX_FIRST_UPSTREAM_CHAINCELL_PAYLOAD];
  } __attribute__ ((__packed__));

  struct t_sequel_upstream_chain_cell {
    UINT8 chainId[CHAIN_ID_LENGTH];
    UINT8 data[MAX_SEQUEL_UPSTREAM_CHAINCELL_PAYLOAD];
  } __attribute__ ((__packed__));

  struct t_upstream_chain_cell {
    UINT16 lengthAndFlags;
    union {
      t_first_upstream_chain_cell firstCell;
      t_sequel_upstream_chain_cell sequelCell;
    };
  } __attribute__ ((__packed__));
#endif //WIN32 

typedef t_upstream_chain_cell t_upstreamChainCell;

#define MAX_FIRST_DOWNSTREAM_CHAINCELL_PAYLOAD DATA_SIZE - 2 - CHAIN_ID_LENGTH
#define MAX_SEQUEL_DOWNSTREAM_CHAINCELL_PAYLOAD DATA_SIZE - 2

#if ((defined(WIN32) || defined(__sgi)) && !defined(__GNUC__))
  #pragma pack(push, t_downstream_chain_cell)
  #pragma pack(1)

  struct t_first_downstream_chain_cell {
    UINT8 chainId[CHAIN_ID_LENGTH];
    UINT8 data[MAX_FIRST_DOWNSTREAM_CHAINCELL_PAYLOAD];
  };

  struct t_sequel_downstream_chain_cell {
    UINT8 data[MAX_SEQUEL_DOWNSTREAM_CHAINCELL_PAYLOAD];
  };

  struct t_downstream_chain_cell {
    UINT16 lengthAndFlags;
    union {
      t_first_downstream_chain_cell firstCell;
      t_sequel_downstream_chain_cell sequelCell;
    };
  };

  #pragma pack(pop, t_downstream_chain_cell)
#else
  struct t_first_downstream_chain_cell {
    UINT8 chainId[CHAIN_ID_LENGTH];
    UINT8 data[MAX_FIRST_DOWNSTREAM_CHAINCELL_PAYLOAD];
  } __attribute__ ((__packed__));

  struct t_sequel_downstream_chain_cell {
    UINT8 data[MAX_SEQUEL_DOWNSTREAM_CHAINCELL_PAYLOAD];
  } __attribute__ ((__packed__));

  struct t_downstream_chain_cell {
    UINT16 lengthAndFlags;
    union {
      t_first_downstream_chain_cell firstCell;
      t_sequel_downstream_chain_cell sequelCell;
    };
  } __attribute__ ((__packed__));
#endif //WIN32 

typedef t_downstream_chain_cell t_downstreamChainCell;


#endif //__TYPEDEFSB__
