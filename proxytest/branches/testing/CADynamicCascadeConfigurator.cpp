/*
Copyright (c) 2000, The JAP-Team 
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:
 
	- Redistributions of source code must retain the above copyright notice, 
	  this list of conditions and the following disclaimer.
 
	- Redistributions in binary form must reproduce the above copyright notice, 
	  this list of conditions and the following disclaimer in the documentation and/or 
		other materials provided with the distribution.
 
	- Neither the name of the University of Technology Dresden, Germany nor the names of its contributors 
	  may be used to endorse or promote products derived from this software without specific 
		prior written permission. 
 
	
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS 
OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY 
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER 
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE
*/
#include "StdAfx.h"

#ifdef DYNAMIC_MIX
#include "CADynamicCascadeConfigurator.hpp"
#include "CACmdLnOptions.hpp"
#include "CAListenerInterface.hpp"
#include "CASocketAddrINet.hpp"
#include "CASocket.hpp"
#include "CAHttpClient.hpp"
#include "CACertificate.hpp"
#include "CAMsg.hpp"

/**
  * LERNGRUPPE
  * Creates a new CADynamicCascadeConfigurator with the given signature key for the given mix
  * @param a_pSignature The signature key for the given mix
  * @param a_pMix The mix that should be reconfigured
  */
CADynamicCascadeConfigurator::CADynamicCascadeConfigurator(CASignature *a_pSignature, CAMix *a_pMix)
{
	this->m_pSignature = a_pSignature;
	this->m_pMix = a_pMix;
	m_proposals = NULL;
}

/**
  * LERNGRUPPE
  * Destroys this CADynamicCascadeConfigurator and cleans up the mess that might
  * have been left behind (i.e. cleans up memory ;-))
  */
CADynamicCascadeConfigurator::~CADynamicCascadeConfigurator()
{
	if(m_proposals != NULL)
	{
		PROPOSALENTRY *tmpProposal = NULL;
		PROPOSERENTRY *tmpProposer = NULL;
		while( (tmpProposal = m_proposals) != NULL )
		{
			while( (tmpProposer = tmpProposal->proposers) != NULL)
			{
				tmpProposal->proposers = tmpProposer->next;
				delete tmpProposer->ski;
				tmpProposer->ski = NULL;
				delete tmpProposer;
				tmpProposer = NULL;
			}
			delete tmpProposal->proposal;
			tmpProposal->proposal = NULL;
// 			delete tmpProposal->elem;
			m_proposals = tmpProposal->next;
			delete tmpProposal;
			tmpProposal = NULL;
		}
	}

}

/**
  * LERNGRUPPE
  * This is the main method of a CADynamicCascadeConfigurator. When it gets called the following happens:
  * 1) Query all known InfoServices for new cascade information and store them in a PROPOSALLIST
  * 2) Check if there is one proposal that has a 2/3 majority
  * 3) If so determine this mix' position in the new cascade
  * 4) Query a random InfoService for the mix infos about the predecessor and the successor of this mix (if applicable)
  * 5) If needed, reconfigure this mix from FirstMix to MiddleMix or vice versa
  * 6) Terminate existing cascade
  * 7) Establish new cascade
  * @retval E_SUCCESS if everything went smooth
  * @retval E_UNKNOWN otherwise
  *
  * @todo it cannot really be foreseen what happens if this method returns with E_UNKNOWN! Might well be that the world comes to a terrible end...
  */
SINT32 CADynamicCascadeConfigurator::configure()
{
	UINT32 nrAddresses;
	CAListenerInterface** ppSocketAddresses = pglobalOptions->getInfoServices(nrAddresses);

#ifdef DEBUG
	CAMsg::printMsg( LOG_DEBUG, "CADynamicCascadeConfigurator::configure - Querying %i infoservices...\n", nrAddresses);
#endif
	UINT8 bufMixId[255];
	UINT32 mixIdLen = 255;
	UINT8 request[255];
	pglobalOptions->getMixId( bufMixId, mixIdLen );
	sprintf((char*)request, "/reconfigure/%s",bufMixId );

	for (UINT32 i = 0; i < nrAddresses; i++)
	{
		CASocketAddrINet* pAddr=(CASocketAddrINet*)ppSocketAddresses[i]->getAddr();
		DOM_Element elem;
		if( sendInfoserviceGetRequest(pAddr, request, &elem) != E_SUCCESS)
		{
#ifdef DEBUG
		CAMsg::printMsg( LOG_DEBUG, "CADynamicCascadeConfigurator::configure - Query %i WAS NOT SUCCESSFULL!\n", i);
#endif
			delete pAddr;
			pAddr = NULL;
			continue;
		}
#ifdef DEBUG
		CAMsg::printMsg( LOG_DEBUG, "CADynamicCascadeConfigurator::configure - Query %i successful, adding proposal now...\n", i);
#endif
		delete pAddr;
		pAddr = NULL;
/*		UINT8* sendBuff=NULL; 
		UINT32 sendBuffLen = 0;
		sendBuff=DOM_Output::dumpToMem(elem,&sendBuffLen);
		if(sendBuff!=NULL)
			CAMsg::printMsg( LOG_DEBUG, "CADynamicCascadeConfigurator::configure - Proposal %s\n", sendBuff);*/
		addProposal( elem );
}

#ifdef DEBUG
	CAMsg::printMsg( LOG_DEBUG, "CADynamicCascadeConfigurator::configure - Queried all infoservices, will now vote...\n");
#endif

	DOM_Node *newCascade  = NULL;
	UINT8 proposal[1024];
	if( getMajorityVote( nrAddresses, newCascade, proposal ) == E_SUCCESS )
	{
		/** @todo move this to some place else */
		UINT8 buff[1024];
		UINT32 len = 1024;
		if(pglobalOptions->getLastCascadeProposal(buff, len) == E_SUCCESS)
		{
			if(strcmp((char*)proposal, (char*)buff) == 0)
			{
				CAMsg::printMsg(LOG_DEBUG, "New proposal == old proposal:  SKIPPING\n");
				return E_SUCCESS;
			}
		}
		CAMsg::printMsg( LOG_DEBUG, "CADynamicCascadeConfigurator::configure - Vote successful!\n");
		if(newCascade != NULL)
		{
			return reconfigureMix(*newCascade, proposal);
		}
	}
	else
	{
		CAMsg::printMsg(LOG_DEBUG, "CADynamicCascadeConfigurator::configure - Vote not successful, no majority found, remaining as I am\n");
	}
	return E_SUCCESS;
}

/**
  * LERGRUPPE
  * Trys to determine a majority proposal in m_proposals. A majority needs at least
  * 2/3 a_nrInfoServices votes.
  * @return r_elemMajority The cascadeinfo XML structure which had the majority
  * @retval E_SUCCESS if a majority has been reached
  * @retval E_UNKNOWN otherwise
  */
SINT32 CADynamicCascadeConfigurator::getMajorityVote(UINT32 a_nrInfoServices, DOM_Node *&r_elemMajority, UINT8* r_strProposal)
{
	SINT32 ret = E_UNKNOWN;
	r_elemMajority = NULL;
	PROPOSALENTRY *tmp = m_proposals;
	while( tmp != NULL )
	{
		if(tmp->count >= MIN_MAJORITY_QUOTE(a_nrInfoServices))
		{
#ifdef DEBUG
			CAMsg::printMsg( LOG_DEBUG, "CADynamicCascadeConfigurator::getMajorityVote - Proposal (%s) has %i votes, thats the majority (%i were needed)!\n", tmp->proposal, tmp->count,
							 MIN_MAJORITY_QUOTE(a_nrInfoServices));
#endif
			r_elemMajority = &tmp->elem;
			if( r_elemMajority != NULL)
			{
				strcpy((char*)r_strProposal, (char*)tmp->proposal);
				ret = E_SUCCESS;
			}
		}
#ifdef DEBUG
		else
		{
			CAMsg::printMsg( LOG_DEBUG, "CADynamicCascadeConfigurator::getMajorityVote - Proposal (%s) has %i votes, discarting, majority would be (%i)\n", tmp->proposal, tmp->count,
							 MIN_MAJORITY_QUOTE(a_nrInfoServices));
		}
#endif
		tmp = tmp->next;
	}
	return ret;
}

/**
  * LERNGRUPPE
  * Trys to reconfigure this mix according to the cascade information given by a_elemNewCascade. This
  * might include a change of the mix's type and could take some time
  * @retval E_SUCCESS if everything went fine
  * @retval E_UNKNOWN otherwise
  */
SINT32 CADynamicCascadeConfigurator::reconfigureMix(DOM_Node a_elemNewCascade, UINT8* a_strProposal)
{
	DOM_Element mixesElement;
	getDOMChildByName(a_elemNewCascade,(UINT8*)"Mixes", mixesElement, false);
	DOM_Node child = mixesElement.getFirstChild();
	char* mixId;
	UINT8 myMixId[255];
	UINT32 len=255;
	pglobalOptions->getMixId(myMixId, len);
	char* prevMixId = NULL;
	char* myNextMixId = NULL;
	char* myPrevMixId = NULL;
	bool bFoundMix = false;
	while(child!=NULL)
	{
		if(child.getNodeName().equals("Mix") && child.getNodeType() == DOM_Node::ELEMENT_NODE)
		{
			mixId = static_cast<const DOM_Element&>(child).getAttribute("id").transcode();
			if(strcmp(mixId,(char*)myMixId) == 0)
			{
				bFoundMix = true;
				myPrevMixId = prevMixId;
			}
			else if(bFoundMix == true)
			{
				myNextMixId = mixId;
				break;
			}
			prevMixId = mixId;
		}
		child = child.getNextSibling();
	}

	if(!bFoundMix)
	{
		CAMsg::printMsg(LOG_ERR, "CADynamicCascadeConfigurator::reconfigureMix - Unable to find my mix ID in the majority cascade info\n");
		return E_UNKNOWN;
	}

	// Empty cascade info -> I wasn't assigned a new cascade, stop the old one
	/** @todo This prevents usage of cascades of length 1 !!! */
	if(myPrevMixId == NULL && myNextMixId == NULL) 
	{
		// Only stops the cascade, now reconfiguration is done yet
		pglobalOptions->resetPrevMix();
		pglobalOptions->resetNextMix();
		pglobalOptions->setCascadeProposal(a_strProposal, strlen((char*)a_strProposal));
		m_pMix->dynaReconfigure( false );
		return E_SUCCESS;
	}
	m_pMix->setReconfiguring(true);
	SINT32 ret = 0;

	bool typeChanged = false;

	/** First Mix */
	if(myPrevMixId == NULL)
	{

		if(pglobalOptions->isLastMix())
		{
			CAMsg::printMsg( LOG_ERR, "CADynamicCascadeConfigurator::reconfigureMix - I am a LastMix and should be reconfigured as a FirstMix! Won't do that!!\n");
			return E_UNKNOWN;
		}
		else if(pglobalOptions->isMiddleMix())
		{
#ifdef DEBUG
			CAMsg::printMsg( LOG_DEBUG, "CADynamicCascadeConfigurator::reconfigureMix - I am a MiddleMix and should be reconfigured as a FirstMix!\n");
#endif
			pglobalOptions->changeMixType(CAMix::FIRST_MIX);
			typeChanged = true;
		}
		else if(pglobalOptions->isFirstMix())
		{
#ifdef DEBUG
			CAMsg::printMsg( LOG_DEBUG, "CADynamicCascadeConfigurator::reconfigureMix - CADynamicCascadeConfigurator::reconfigureMix - I am (and will remain) a first mix!\n");
#endif
		}
		else
		{
#ifdef DEBUG
			CAMsg::printMsg( LOG_DEBUG, "CADynamicCascadeConfigurator::reconfigureMix - Ugh! I do not have a current mix type! Trying to become a FirstMix nevertheless, but that might fail\n");
#endif
			pglobalOptions->changeMixType(CAMix::FIRST_MIX);
			typeChanged = true;
		}
	}

	/** Last Mix */
	if( myNextMixId == NULL )
	{
		if(!pglobalOptions->isLastMix())
		{
			CAMsg::printMsg( LOG_ERR, "CADynamicCascadeConfigurator::reconfigureMix - I am NOT a LastMix and should be reconfigured as a LastMix! Won't do that!!\n");
			return E_UNKNOWN;
		}
#ifdef DEBUG
		else
		{
			CAMsg::printMsg( LOG_DEBUG, "CADynamicCascadeConfigurator::reconfigureMix - I am (and will remain) a last mix!\n");
		}
#endif
	}

	/** Middle Mix */
	if(myPrevMixId != NULL && myNextMixId != NULL)
	{
		if(pglobalOptions->isLastMix())
		{
			CAMsg::printMsg( LOG_ERR, "CADynamicCascadeConfigurator::reconfigureMix - I am a LastMix and should be reconfigured as a MiddleMix! Won't do that!!\n");
			return E_UNKNOWN;
		}
		else if(pglobalOptions->isFirstMix())
		{
#ifdef DEBUG
			CAMsg::printMsg( LOG_DEBUG, "CADynamicCascadeConfigurator::reconfigureMix - I am a FirstMix and should be reconfigured as a MiddleMix!\n");
#endif
			pglobalOptions->changeMixType(CAMix::MIDDLE_MIX);
			typeChanged = true;
		}
		else if(pglobalOptions->isMiddleMix())
		{
			CAMsg::printMsg( LOG_DEBUG, "CADynamicCascadeConfigurator::reconfigureMix - I am (and will remain) a middle mix!\n");
		}
		else
		{
#ifdef DEBUG
			CAMsg::printMsg( LOG_DEBUG, "CADynamicCascadeConfigurator::reconfigureMix - Ugh! I do not have a current mix type! Trying to become a MiddleMix nevertheless, but that might fail\n");
#endif
			pglobalOptions->changeMixType(CAMix::MIDDLE_MIX);
			typeChanged = true;
		}
	}

	// MiddleMix or LastMox
	if(myPrevMixId != NULL)
	{
		CAMsg::printMsg(LOG_INFO,"CADynamicCascadeConfigurator::reconfigureMix - Asking InfoService about previous mix ...\n");
		char buf[255];
		sprintf(buf, "/mixinfo/%s", (char*)myPrevMixId);
		DOM_Element result;
		/** @todo What if we can't get the mixinfo? is this critical? */
		ret = sendRandomInfoserviceGetRequest( (UINT8*)buf, &result);
		if(ret != E_SUCCESS)
		{
			CAMsg::printMsg(LOG_ERR,"CADynamicCascadeConfigurator::reconfigureMix - Error retrieving mix info from InfoService!\n");
			return ret;
		}
		DOM_Document doc = result.getOwnerDocument();
		ret = pglobalOptions->setPrevMix( doc );
		if(ret != E_SUCCESS)
		{
			CAMsg::printMsg(LOG_ERR,"CADynamicCascadeConfigurator::reconfigureMix - Error setting next mix info!\n");
			pglobalOptions->resetPrevMix();
			return E_UNKNOWN;
		}
	}
	else
	{
		if(pglobalOptions->resetPrevMix() != E_SUCCESS)
		{
			CAMsg::printMsg( LOG_ERR, "CADynamicCascadeConfigurator::reconfigureMix - Unable to reset prev mix information\n");
			return E_UNKNOWN;
		}
	}

	// MiddleMix or FirstMix
	if(myNextMixId != NULL)
	{
		CAMsg::printMsg(LOG_INFO,"CADynamicCascadeConfigurator::reconfigureMix - Asking InfoService about next mix ...\n");
		char buf[255];
		sprintf(buf, "/mixinfo/%s", (char*)myNextMixId);
		DOM_Element result;
		if( sendRandomInfoserviceGetRequest( (UINT8*)buf, &result) != E_SUCCESS )
		{
			CAMsg::printMsg(LOG_ERR,"CADynamicCascadeConfigurator::reconfigureMix - Error retrieving mix info from InfoService!\n");
			return E_UNKNOWN;
		}
		DOM_Document doc = result.getOwnerDocument();
		if(pglobalOptions->setNextMix( doc ) != E_SUCCESS)
		{
			CAMsg::printMsg(LOG_ERR,"CADynamicCascadeConfigurator::reconfigureMix - Error setting next mix info!\n");
			return E_UNKNOWN;
		}
	}
	else
	{
		if(pglobalOptions->resetNextMix() != E_SUCCESS)
		{
			CAMsg::printMsg( LOG_ERR, "CADynamicCascadeConfigurator::reconfigureMix - Unable to reset next mix information\n");
			return E_UNKNOWN;
		}
	}

	pglobalOptions->setCascadeProposal(a_strProposal, strlen((char*)a_strProposal));
/*	if(pglobalOptions->isFirstMix()) 
	{
		pglobalOptions->setCascadeXML((DOM_Element&)a_elemNewCascade);
	}*/
#ifdef DEBUG
	CAMsg::printMsg( LOG_DEBUG, "CADynamicCascadeConfigurator::reconfigureMix - Calling dynaReconfigure on the mix, grab a hold of something!\n");
#endif
	m_pMix->dynaReconfigure( typeChanged );
	return E_SUCCESS;
}

/**
  * LERNGRUPPE
  * Extracts the cascade proposal from the given a_elem and adds it to the list of proposals (m_proposals)
  * A proposal is uniquely defined by the concatenation of its mix ids.
  * If a proposal is already in the list, the count value for the proposal is incremented and the proposing
  * InfoService, identified by the subject key identifier of the appended certificate is noted in the list of proposers
  * the proposal.
  * @param a_elem The XML structure with the cascade info proposal
  * @retval E_SUCCESS if the proposal was successfully added
  * @retval E_UNKNOWN otherwise
  */
SINT32 CADynamicCascadeConfigurator::addProposal(DOM_Element a_elem)
{
	/** First check the signature */
	// FIXME Signature cannot be verified. Why is that?
	/*		if( m_pSignature->verifyXML( a_elem, NULL )	!= E_SUCCESS)
			{
				CAMsg::printMsg( LOG_DEBUG, "Signature was not ok, throwing it away...\n");
				return E_UNKNOWN;
			}*/
	/** Build the proposal string, a concatenation of all mix ids */
	DOM_Element mixesElement;
	getDOMChildByName(a_elem,(UINT8*)"Mixes", mixesElement, false);
	DOM_Node child = mixesElement.getFirstChild();
	char* mixId;
	char* tmpProposal = NULL;
	while(child!=NULL)
	{
		if(child.getNodeName().equals("Mix") && child.getNodeType() == DOM_Node::ELEMENT_NODE)
		{
			mixId = static_cast<const DOM_Element&>(child).getAttribute("id").transcode();
			if(tmpProposal == NULL)
			{
				tmpProposal = new char[strlen(mixId)+1];
				strncpy(tmpProposal, mixId, strlen(mixId)+1);
			}
			else
			{
				char *tmp = new char[strlen(tmpProposal)+1];
				strncpy(tmp, tmpProposal, strlen(tmpProposal)+1);
				delete tmpProposal;
				tmpProposal = new char[ strlen(mixId) + strlen(tmp) + 1];
				strncpy(tmpProposal, tmp, strlen(tmp) + 1);
				delete tmp;
				tmp = NULL;
				strcat(tmpProposal, mixId);
			}
		}
		child = child.getNextSibling();
	}
	UINT8* proposal = NULL;
	UINT32 lenProposal = 0;
	proposal = new UINT8[strlen(tmpProposal) + 1];
	memcpy(proposal, tmpProposal, strlen(tmpProposal) + 1);
	lenProposal = strlen(tmpProposal);
	delete tmpProposal;
	tmpProposal = NULL;

#ifdef DEBUG
// 	CAMsg::printMsg(LOG_DEBUG, "CADynamicCascadeConfigurator::addProposal - next proposal is %s\n", proposal);
#endif

	/** Get the SKI of the InfoService's certificate */
	DOM_Element elemSig, elemCert;
	getDOMChildByName(a_elem,(UINT8*)"Signature",elemSig,false);
	getDOMChildByName(elemSig,(UINT8*)"X509Data",elemCert,true);
	CACertificate *cert = NULL;
	if(elemSig!=NULL)
		cert = CACertificate::decode(elemCert.getFirstChild(),CERT_X509CERTIFICATE);

	UINT8 proposerSki[60];
	UINT32 lenProposerSki = 60;
	if(cert != NULL)
		cert->getSubjectKeyIdentifier( proposerSki, &lenProposerSki );

#ifdef DEBUG
// 	CAMsg::printMsg(LOG_DEBUG, "CADynamicCascadeConfigurator::addProposal - the proposer is %s\n", proposerSki);
#endif

	bool added = false;
	
	/** Now add the entry to the list of our proposals */
	PROPOSALENTRY *tmpEntry;
	if( m_proposals == NULL )
	{
		m_proposals = createProposal( proposal, lenProposal, proposerSki, lenProposerSki, &a_elem );
#ifdef DEBUG
/*		CAMsg::printMsg(LOG_DEBUG,  "CADynamicCascadeConfigurator::addProposal - Adding proposal %s from InfoService %s. Got %i proposing IS for this value\n", m_proposals->proposal,
						m_proposals->proposers->ski, m_proposals->count );*/
#endif
	}
	else
	{
		tmpEntry=m_proposals;
		while(true)
		{
			if(memcmp(tmpEntry->proposal, proposal, lenProposal) == 0)
			{
				PROPOSERENTRY *proposer = tmpEntry->proposers;
				bool ok = true;
				while(true)
				{
					if(memcmp(proposer->ski, proposerSki, lenProposerSki) == 0)
					{
						// Duplicate!
						ok = false;
						added = true;
						break;
					}
					if(proposer->next == NULL)
						break;
					proposer = proposer->next;
				}
				if(ok)
				{
					PROPOSERENTRY *tmpProposer = new PROPOSERENTRY;
					tmpProposer->next = NULL;
					tmpProposer->ski = new UINT8[lenProposerSki+1];
					memcpy(tmpProposer->ski, proposerSki, lenProposerSki+1);
					proposer->next = tmpProposer;
					tmpEntry->count++;
#ifdef DEBUG
/*					CAMsg::printMsg(LOG_DEBUG,  "CADynamicCascadeConfigurator::addProposal - Adding proposal %s from InfoService %s. Got %i proposing IS for this value\n", tmpEntry->proposal,tmpProposer->ski, tmpEntry->count );*/
#endif
					added = true;
					break;
				}
			}
			if(tmpEntry->next == NULL) 
				break;
			tmpEntry = tmpEntry->next;
		}
		if(!added)
		{
			tmpEntry->next = createProposal( proposal, lenProposal, proposerSki, lenProposerSki, &a_elem );
#ifdef DEBUG
/*			CAMsg::printMsg(LOG_DEBUG,  "CADynamicCascadeConfigurator::addProposal - Adding proposal %s from InfoService %s. Got %i proposing IS for this value\n", tmpEntry->next->proposal,tmpEntry->next->proposers->ski, tmpEntry->next->count );*/
#endif
		}
	}
	return E_SUCCESS;
}

/**
  * LERNGRUPPE
  * Helper method to create a new PROPOSALENTRY for the given values
  * @param a_strProposal The proposal (i.e. the concatenated mix ids)
  * @param a_lenProposal The length of the proposal
  * @param a_strProposer The subject key identifier of the proposing InfoService
  * @param a_lenProposer The length of the subject key identifier
  * @param a_elem The XML structure containing the cascade proposal
  * @return The new PROPOSALENTRY
  */
PROPOSALENTRY *CADynamicCascadeConfigurator::createProposal(UINT8* a_strProposal, UINT32 a_lenProposal, UINT8 *a_strProposer, UINT32 a_lenProposer, DOM_Element *a_elem)
{
	PROPOSALENTRY *entry = new PROPOSALENTRY;
	entry->proposal = new UINT8[a_lenProposal+1];
	memcpy(entry->proposal, a_strProposal, a_lenProposal+1);
	entry->count = 1;
	entry->next = NULL;
	PROPOSERENTRY *proposer = new PROPOSERENTRY;
	proposer->next = NULL;
	proposer->ski = new UINT8[a_lenProposer+1];
	memcpy(proposer->ski, a_strProposer, a_lenProposer+1);
	entry->proposers = proposer;
	entry->elem = a_elem->cloneNode( true );
	return entry;
}
#endif // DYNAMIC_MIX
