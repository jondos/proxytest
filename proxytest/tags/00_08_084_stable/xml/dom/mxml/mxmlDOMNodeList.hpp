#ifndef __MXML_DOM_NODE_LIST__
#define __MXML_DOM_NODE_LIST__

#include "mxmlDOMNode.hpp"

typedef struct __dom_node_list_entry
	{
		DOMNode* node;
		struct __dom_node_list_entry* next;
	} t_DOMNodeList_entry;

class DOMElement;

class DOMNodeList
	{
		public:   
			DOMNode* item(UINT32 index) const
				{
					t_DOMNodeList_entry* pEntry=m_pHead;
					UINT32 i=0;
					while(pEntry!=NULL)
						{
							if(i==index)
								return pEntry->node;
							i++;
							pEntry=pEntry->next;
						}
					return NULL;
				};

			UINT32 getLength() const
				{
					return m_uSize;
				}
		
		protected:
			DOMNodeList()
				{
					m_pHead=NULL;
					m_pTail=NULL;
					m_uSize=0;
				}
			~DOMNodeList()
				{
					while(m_pHead!=NULL)
						{
							m_pTail=m_pHead->next;
							delete m_pHead;
							m_pHead=m_pTail;
						}
				}

			void add(DOMNode* node)
				{
					t_DOMNodeList_entry* entry=new t_DOMNodeList_entry;
					entry->next=NULL;
					entry->node=node;
					if(m_pTail==NULL)
						{
							m_pHead=m_pTail=entry;
						}
					else
						{
							m_pTail->next=entry;
							m_pTail=entry;
						}
					m_uSize++;
				}

			friend class DOMElement;

		private:
			UINT32 m_uSize;
			t_DOMNodeList_entry* m_pHead;
			t_DOMNodeList_entry* m_pTail;
	};

#endif //__MXML_DOM_NODE_LIST__
