#include "CAConditionVariable.hpp"
class CALookAble
	{
		public:
			CALookAble()
				{
					printf("CALook start\n");
					m_nLookCount=1;
					printf("CALokk end\n");
				}

			SINT32 lock()
				{
					m_ConVar.lock();
					m_nLookCount++;
					m_ConVar.unlock();
					return E_SUCCESS;
				}
			SINT32 unlock()
				{
					m_ConVar.lock();
					m_nLookCount--;
					m_ConVar.unlock();
					return E_SUCCESS;
				}

		protected:
			SINT32 waitForDestroy()
				{
					printf("~CALook start\n");
					m_ConVar.lock();
					while(m_nLookCount>1)
						m_ConVar.wait();
					m_ConVar.unlock();	
					printf("~CALokk end\n");
					return E_SUCCESS;
				}

		private:
			CAConditionVariable m_ConVar;
			UINT32 m_nLookCount;
	};