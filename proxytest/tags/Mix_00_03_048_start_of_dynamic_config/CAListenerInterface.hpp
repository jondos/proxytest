#ifndef _CALISTENERINTERFAC__
#define _CALISTENERINTERFAC__

#include "typedefs.hpp"
#include "CASocketAddr.hpp"

class CAListenerInterface
	{
		private:
			CAListenerInterface(void);
		
		public:
			CAListenerInterface(const CAListenerInterface& l)//Copy constructor
				{
					m_Type=l.m_Type;
					m_bHidden=l.m_bHidden;
					m_bVirtual=l.m_bVirtual;
					if(l.m_pAddr!=NULL)
						m_pAddr=l.m_pAddr->clone();
					else
						m_pAddr=NULL;
					if(l.m_strHostname!=NULL)
						{
							UINT32 i=strlen((char*)l.m_strHostname);
							m_strHostname=new UINT8[i+1];
							memcpy(m_strHostname,l.m_strHostname,i);
							m_strHostname[i]=0;
						}
					else
						m_strHostname=NULL;
				}
			CAListenerInterface& operator=(const CAListenerInterface&); //Zuweisungsoperator
			static CAListenerInterface* getInstance(const DOM_Node& node);
			static CAListenerInterface* getInstance(NetworkType type,const UINT8* path); //constructs a Unix Domain ListenerInterface
			static CAListenerInterface* getInstance(NetworkType type,const UINT8* hostnameOrIP,UINT16 port); //constructs a TCP/IP ListenerInterface
			~CAListenerInterface(void);

		public:
			NetworkType getType() const
				{
					return m_Type;
				}
			SINT32 getHostName(UINT8* buff,UINT32 bufflen) const;
			CASocketAddr* getAddr() const
				{
					return m_pAddr->clone();
				};
			bool isHidden() const
				{
					return m_bHidden;
				}
			bool isVirtual() const
				{
					return m_bVirtual;
				}
			SINT32 toDOMFragment(DOM_DocumentFragment& fragment,DOM_Document& ownerDoc) const;

		private:
			CASocketAddr* m_pAddr;
			UINT8*				m_strHostname; 
			NetworkType		m_Type;
			bool					m_bHidden;
			bool					m_bVirtual;
	};

#endif