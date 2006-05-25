#ifndef __CAABSTRACTXMLSIGNABLE__
#define __CAABSTRACTXMLSIGNABLE__

#include "CAAbstractXMLEncodable.hpp"
#include "CACertStore.hpp"
#include "CASignature.hpp"
#include "CAMsg.hpp"
#include "xml/DOM_Output.hpp"

/**
 * An abstract base class for signable XML structures. 
 *
 * @author Bastian Voigt
 */
class CAAbstractXMLSignable : public CAAbstractXMLEncodable
{
public:
	CAAbstractXMLSignable() : CAAbstractXMLEncodable()
		{
			m_signature = NULL;
		}
	
	virtual ~CAAbstractXMLSignable() {}
	
	/** TODO: implement */
	SINT32 sign(CASignature &signer)
		{
			return E_UNKNOWN;
		}

	/**
	 * Verifies the signature. Returns E_SUCCESS if the signature is valid.
	 */
	SINT32 verifySignature(CASignature &verifier)
		{
			//ASSERT(verifier!=NULL, "sigVerifier is NULL");
			DOM_Document doc;
			toXmlDocument(doc);
			DOM_Element elemRoot = doc.getDocumentElement();
			SINT32 rc = verifier.verifyXML( (DOM_Node &)elemRoot, (CACertStore *)NULL );
			return rc;
		}

	/** 
	 * sets the internal signature representation. 
	 * Should be called from derived class constructors.
	 */
	SINT32 setSignature(DOM_Element &elemSig)
		{
			ASSERT(!elemSig.isNull(), "Signature element is NULL")
			m_signature = DOM_Document::createDocument();
			m_signature.appendChild(m_signature.importNode((DOM_Node&)elemSig, true));
			return E_SUCCESS;
		}

	/** returns nonzero, if this structure is already signed */
	SINT32 isSigned()
		{
			if(!m_signature.isNull())
			{
				return true;
			}
			else
			{
				return false;
			}
		}

protected:
	DOM_Document m_signature;
};

#endif
