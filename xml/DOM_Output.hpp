#include "../CAQueue.hpp"
class MemFormatTarget: public XMLFormatTarget
	{
#define MEM_FORMART_TARGET_SPACE 1024
		public:
			MemFormatTarget()
				{
					m_pQueue=new CAQueue();
					m_Buff=new UINT8[MEM_FORMART_TARGET_SPACE];
					m_aktIndex=0;
				}

			~MemFormatTarget()
				{
					delete m_pQueue;
					delete[] m_Buff;
				}

			virtual void writeChars(const XMLByte* const toWrite, const unsigned int count,
															XMLFormatter* const formatter)
				{
					const XMLByte* write=toWrite;
					UINT32 c=count;
					while(c>0)
						{
							UINT32 space=MEM_FORMART_TARGET_SPACE-m_aktIndex;
							if(space>=c)
								{
									memcpy(m_Buff+m_aktIndex,write,c);
									m_aktIndex+=c;
									return;
								}
							else
								{
									memcpy(m_Buff+m_aktIndex,write,space);
									write+=space;
									c-=space;
									m_pQueue->add(m_Buff,MEM_FORMART_TARGET_SPACE);
									m_aktIndex=0;
								}
						}
				}

			/** Copys the XML-chars into buff.
				* @param buff buffer in which to copy the XML-chars
				* @param size contains the size of buff, on return contains the number of XML-CHars copied
				* @return E_SUCCESS, if successful
				* @return E_SPACE, if buff is to small
				* @return E_UNKNOWN, if an error occurs
				*/
			SINT32 dumpMem(UINT8* buff,UINT32* size)
				{
					if(buff==NULL||size==NULL)
						return E_UNKNOWN;
					if(m_aktIndex>0&&m_pQueue->add(m_Buff,m_aktIndex)!=E_SUCCESS)
						return E_UNKNOWN;
					m_aktIndex=0;
					if(*size<m_pQueue->getSize())
						return E_SPACE;
					if(m_pQueue->peek(buff,size)!=E_SUCCESS)
						return E_UNKNOWN;
					return E_SUCCESS;
				}

			/** Returns a Copy of the XML-chars.
				* @param size, on return contains the number of XML-Chars copied
				* @return a pointer to a newls allocated buff, which must be delete[] by the caller
				* @return NULL, if an error occurs
				*/
			UINT8* dumpMem(UINT32* size)
				{
					if(size==NULL)
						return NULL;
					if(m_aktIndex>0&&m_pQueue->add(m_Buff,m_aktIndex)!=E_SUCCESS)
						return NULL;
					m_aktIndex=0;
					*size=m_pQueue->getSize();
					UINT8* tmp=new UINT8[*size];
					if(m_pQueue->peek(tmp,size)!=E_SUCCESS)
						{
							delete[] tmp;
							return NULL;
						}
					return tmp;
				}

		private:
			CAQueue* m_pQueue;
			UINT8* m_Buff;
			UINT32 m_aktIndex;
	};

class DOM_Output
	{
		public:
			/** Dumps the node and all childs into buff.
				* @param node, Node to dump
				* @param buff buffer in which to copy the XML-chars
				* @param size contains the size of buff, on return contains the number of XML-CHars copied
				* @return E_SUCCESS, if successful
				* @return E_SPACE, if buff is to small
				* @return E_UNKNOWN, if an error occurs
				*/
			static SINT32 dumpToMem(DOM_Node& node,UINT8* buff, UINT32* size)
				{
					DOM_Output out;
					if(	out.dumpNode(node,false)!=E_SUCCESS)
						return E_UNKNOWN;
					return out.m_pFormatTarget->dumpMem(buff,size);
				}

			
			/** Dumps the Node an returns a pointer to the memory.
				* @param node, Node to dump
				* @param size, on return contains the number of XML-Chars copied
				* @return a pointer to a newls allocated buff, which must be delete[] by the caller
				* @return NULL, if an error occurs
				*/
			static UINT8* dumpToMem(DOM_Node& node,UINT32* size)
				{
					DOM_Output out;
					if(	out.dumpNode(node,false)!=E_SUCCESS)
						return NULL;
					return out.m_pFormatTarget->dumpMem(size);
				}

			
			/** Dumps the node and all childs in a 'cannonical form' into buff.
				* @param node, Node to dump
				* @param buff buffer in which to copy the XML-chars
				* @param size contains the size of buff, on return contains the number of XML-CHars copied
				* @return E_SUCCESS, if successful
				* @return E_SPACE, if buff is to small
				* @return E_UNKNOWN, if an error occurs
				*/
			static SINT32 makeCanonical(DOM_Node& node,UINT8* buff,UINT32* size)
				{
					DOM_Output out;
					if(	out.dumpNode(node,true)!=E_SUCCESS)
						return E_UNKNOWN;
					return out.m_pFormatTarget->dumpMem(buff,size);
				}

		
			/** Dumps the Node in a cannonical form an returns a pointer to the memory.
				* @param node, Node to dump
				* @param size, on return contains the number of XML-Chars copied
				* @return a pointer to a newls allocated buff, which must be delete[] by the caller
				* @return NULL, if an error occurs
				*/
			static UINT8* makeCanonical(DOM_Node& node,UINT32* size)
				{
					DOM_Output out;
					if(	out.dumpNode(node,true)!=E_SUCCESS)
						return NULL;
					return out.m_pFormatTarget->dumpMem(size);
				}

		private:
			DOM_Output()
				{
					m_pFormatTarget=new MemFormatTarget();
					m_pFormatter=new XMLFormatter(m_UTF8, m_pFormatTarget,
                                          XMLFormatter::NoEscapes, XMLFormatter::UnRep_Fail);
				}
			~DOM_Output()
				{
					delete m_pFormatTarget;
					delete m_pFormatter;
				}

			SINT32 dumpNode(DOM_Node& toWrite,bool bCanonical);
			XMLFormatter* m_pFormatter;
			MemFormatTarget* m_pFormatTarget;
			static const XMLCh  m_XML[39]; 
			static const XMLCh  m_UTF8[6]; 
};
