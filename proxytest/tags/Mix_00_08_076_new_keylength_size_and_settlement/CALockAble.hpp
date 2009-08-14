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
#ifndef ONLY_LOCAL_PROXY
#include "CAConditionVariable.hpp"
/** From this class other classes could be derived, which need some kind from "locking" in memory. Imagine for
  * instance that thread t1 creates an object o of class c and uses it. During that a second thread t2 destroies o by calling 
	* delete o. Clearly this would be desasterous. To solve this problem cc should be derived from CALockAble. Than
	* t1 calls o.lock() before using o and o.unlock() if t1 finsihed using o. If t2 calls delete o between the 
	* lock() / unlock() calls, it would block until all references of o are unlocked.
	* Probablly this class is usefull for other tings, too...
	*/ 
class CALockAble
	{
		public:
			CALockAble()
				{
					m_nLockCount=1;
				}
				
			virtual ~CALockAble()
				{
				}

			/** Locks the lockable object by threadsafe incrementing a reference counter.
			  * @retval E_SUCCESS
				*/
			SINT32 lock()
				{
					m_ConVar.lock();
					m_nLockCount++;
					m_ConVar.unlock();
					return E_SUCCESS;
				}
				
			/** Unlocks the lockable object by threadsafe decrementing a reference counter. The counter would never become
			  * less than zero. Every thread, which waits in waitForDestroy() will be signaled.
			  * @retval E_SUCCESS
				*/	
			SINT32 unlock()
				{
					m_ConVar.lock();
					if(m_nLockCount>0)
						m_nLockCount--;
					m_ConVar.unlock();
					m_ConVar.signal();
					return E_SUCCESS;
				}

		protected:
		  /** If called checks if the reference counter equals zero. If the reference counter is greater than
			  * zero, the call blocks.
				* @retval E_SUCCESS
				*/
			SINT32 waitForDestroy()
				{
					m_ConVar.lock();
					while(m_nLockCount>1)
						m_ConVar.wait();
					m_ConVar.unlock();	
					return E_SUCCESS;
				}

		private:
		  /** A conditional variable, which would be signaled if the rerference counter reaches zero.*/
			CAConditionVariable m_ConVar;
			/** The reference counter*/
			UINT32 m_nLockCount;
	};
#endif //ONLY_LOCAL_PROXY
