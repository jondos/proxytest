#include "mxmlInputSource.hpp"
#include "mxmlDOMTypeDef.hpp"

void xercesdomparser_sax_callback (mxml_node_t *, mxml_sax_event_t, void *);
mxml_type_t xercesdomparser_load_callback( mxml_node_t *);
int xercesdomparser_custom_load_callback( mxml_node_t *, const char *);

class XercesDOMParser
	{
		public:
			void parse( const InputSource &  source)
				{
					m_curNode=NULL;
					mxmlSetCustomHandlers( xercesdomparser_custom_load_callback,MXML_TEXT_CALLBACK);
					mxml_node_t* top=mxmlSAXLoadString (NULL,source.getBuff(),xercesdomparser_load_callback,
																							xercesdomparser_sax_callback,this);
				}
				
			UINT32 getErrorCount() const
				{
					return m_uParseErrors;
				}

			XERCES_CPP_NAMESPACE::DOMDocument* getDocument()
				{
					return m_Doc;
				}

		private:
			UINT32 m_uParseErrors;
			XERCES_CPP_NAMESPACE::DOMDocument* m_Doc;
			DOMNode* m_curNode;
			friend void xercesdomparser_sax_callback (mxml_node_t * node, mxml_sax_event_t sax_event, void * data);

	};
