#ifndef __CADYNAMICCASCADECONFIGURATOR__
#define __CADYNAMICCASCADECONFIGURATOR__

#include "StdAfx.h"
#ifdef DYNAMIC_MIX

#include "xml/DOM_Output.hpp"
#include "CASocketAddrINet.hpp"
#include "CAInfoServiceAware.hpp"
#include "CAInfoService.hpp"

struct _proposer_t
{
	struct _proposer_t* next;
	UINT8* ski;
};
typedef struct _proposer_t PROPOSERENTRY;
typedef struct _proposer_t PROPOSERLIST;

struct _proposal_t
{
	struct _proposal_t* next;
	PROPOSERLIST* proposers;
	UINT8* proposal;
	UINT32 count;
	DOM_Node elem;
};
typedef struct _proposal_t PROPOSALENTRY;
typedef struct _proposal_t PROPOSALLIST;


#define MIN_MAJORITY_QUOTE(x) (UINT32)((2*x)/3)+1
#define MAX_CONTENT_LENGTH 0x00FFFF

class CADynamicCascadeConfigurator : CAInfoServiceAware
{

public:
	CADynamicCascadeConfigurator(CASignature *a_pSignature, CAMix *a_pMix);

	~CADynamicCascadeConfigurator();
	SINT32 configure();
	

private:
	PROPOSALLIST* m_proposals;
	SINT32 getMajorityVote(UINT32 a_nrInfoServices, DOM_Node *&r_majorityElem, UINT8* r_strProposal);
	SINT32 addProposal(DOM_Element a_elem);
	PROPOSALENTRY *createProposal(UINT8* a_strProposal, UINT32 a_lenProposal, UINT8 *a_strProposer, UINT32 a_lenProposer, DOM_Element *a_elem);
	SINT32 reconfigureMix(DOM_Node a_elemNewCascade, UINT8* a_strProposal);
	CASignature *m_pSignature;
	CAMix *m_pMix;
};
#endif //DYNAMIC_MIX
#endif
