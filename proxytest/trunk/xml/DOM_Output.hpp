#include "../CAQueue.hpp"
class MemFormatTarget: public XMLFormatTarget
	{
#define MEM_FORMART_TARGET_SPACE 1024
		public:
			MemFormatTarget()
				{
					pQueue=new CAQueue();
					m_Buff=new UINT8[MEM_FORMART_TARGET_SPACE];
					m_aktIndex=0;
				}

			~MemFormatTarget()
				{
					delete pQueue;
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
									pQueue->add(m_Buff,MEM_FORMART_TARGET_SPACE);
									m_aktIndex=0;
								}
						}
				}
			SINT32 dumpMem(UINT8* buff,UINT32* size)
				{
					if(	pQueue->add(m_Buff,m_aktIndex)!=E_SUCCESS||
							pQueue->get(buff,size)!=E_SUCCESS)
						return E_UNKNOWN;
					return E_SUCCESS;
				}

		private:
			CAQueue* pQueue;
			UINT8* m_Buff;
			UINT32 m_aktIndex;
	};

class DOM_Output
	{
		public:
			static void dumpToMem(DOM_Node& node,UINT8* buff, UINT32* size);
			static SINT32 makeCanonical(DOM_Node& node,UINT8* buff,UINT32* size);
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
