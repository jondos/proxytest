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
			m_pSignature = NULL;
		}
	
	virtual ~CAAbstractXMLSignable() 
		{
			if(m_psignature!=NULL)
				{
					m_pSiganture->release();
					delete m_pSignature();
				}
		}
	
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
			XERCES_CPP_NAMESPACE::DOMDocument* pDoc;
			toXmlDocument(doc);
			DOMElement* pElemRoot = pDoc->getDocumentElement();
			SINT32 rc = verifier.verifyXML( pElemRoot, (CACertStore *)NULL );
			return rc;
		}

	/** 
	 * sets the internal signature representation. 
	 * Should be called from derived class constructors.
	 */
	SINT32 setSignature(DOMElement* elemSig)
		{
			ASSERT(!elemSig.isNull(), "Signature element is NULL")
			m_pSignature = createDOMDocument();
			m_pSignature->appendChild(m_signature->importNode(elemSig, true));
			return E_SUCCESS;
		}

	/** returns nonzero, if this structure is already signed */
	SINT32 isSigned()
		{
			if(m_pSignature!=NULL)
			{
				return true;
			}
			else
			{
				return false;
			}
		}

protected:
	XERCES_CPP_NAMESPACE::DOM_Document* m_pSignature;
};

#endif
