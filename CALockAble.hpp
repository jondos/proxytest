#include "CAConditionVariable.hpp"
class CALockAble
	{
		public:
			CALockAble()
				{
					m_nLockCount=1;
				}

			SINT32 lock()
				{
					m_ConVar.lock();
					m_nLockCount++;
					m_ConVar.unlock();
					return E_SUCCESS;
				}
			SINT32 unlock()
				{
					m_ConVar.lock();
					m_nLockCount--;
					m_ConVar.unlock();
					return E_SUCCESS;
				}

		protected:
			SINT32 waitForDestroy()
				{
					m_ConVar.lock();
					while(m_nLockCount>1)
						m_ConVar.wait();
					m_ConVar.unlock();	
					return E_SUCCESS;
				}

		private:
			CAConditionVariable m_ConVar;
			UINT32 m_nLockCount;
	};
