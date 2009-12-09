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
#ifndef __CATHREAD__
#define __CATHREAD__

#ifndef ONLY_LOCAL_PROXY

#include "CAMsg.hpp"

class CAThreadList;

#ifdef PRINT_THREAD_STACK_TRACE
	#define INIT_STACK CAThread::METHOD_STACK* _stack
	#define SAVE_STACK(methodName, methodPosition) \
	_stack = new CAThread::METHOD_STACK; \
	_stack->strMethodName = (methodName); \
	_stack->strPosition = (methodPosition); \
	CAThread::setCurrentStack(_stack)

	#define FINISH_STACK(methodName) SAVE_STACK(methodName, CAThread::METHOD_END)
	#define BEGIN_STACK(methodName) SAVE_STACK(methodName, CAThread::METHOD_BEGIN)
#else
	#define INIT_STACK
	#define BEGIN_STACK(methodName)
	#define FINISH_STACK(methodName)
	#define SAVE_STACK(methodName, methodPosition)
#endif

/** Defines the type of the main function of the thread. The main function has one argument of type void*.
	*	The exit points of the main function should be THREAD_RETURN_SUCCESS or THREAD_RETRUN_ERROR. 
	*
	*	Example:
	* @code
	*	
	*	THREAD_RETURN myMainFunction(void* param)
	*		{
	*				doSomething;
	*				THREAD_RETURN_SUCCESS;
	*   }
	* @endcode
	**/
typedef THREAD_RETURN(*THREAD_MAIN_TYP)(void *);

///Type of an ID for a thread which can be used to identify the current and other threads
typedef unsigned long thread_id_t;

/** @defgroup threading Classes for multithreaded programming
	*
	* There exists several classes to support multi-threading. Some of these classes deal with creating and executing
	* of threads, where others a for synchronisation among the threads.
	*/
/**
	* @ingroup threading
	*
	* This class could be used for creating a new thread. The function which should be executed within this thread could be set 
	* be using the setMainLoop() method.
	*
*
*	Some example on using CAThread:

* First one needs to define a function which should be executed within the thread:
@code
	THREAD_RETURN doSomeThing(void* param)
		{
			THREAD_RETURN_SUCCESS
		}
	@endcode
*
* Now we can create the thread, set the main function, start the thread and wait for the thread to finish execution:

	@code
	CAThread* pThread=new CAThread();
	pThread->setMainLoop(doSomeThing);
	pThread->start(theParams);
	pThread->join();
	delete pThread;
	@endcode
	*/
class CAThread
	{
		public:
#ifdef PRINT_THREAD_STACK_TRACE
			struct METHOD_STACK
			{
				const char* strMethodName;
				const char* strPosition;
			};
#endif
			/** Creates a CAThread object but no actual thread.
				*/
			CAThread();

			/** Creates a CAThread object but no actual thread.
				* @param strName a name for this thread, useful mostly for debugging
				*/
			CAThread(const UINT8* strName);
			
			~CAThread()
				{
					#ifdef OS_TUDOS
					m_Thread = L4THREAD_INVALID_ID;
					#else
					delete m_pThread;
					m_pThread = NULL;
					#endif
					delete[] m_strName;
					m_strName = NULL;
				}
			
#ifdef PRINT_THREAD_STACK_TRACE			
			static void setCurrentStack(METHOD_STACK* a_stack);	
			static METHOD_STACK* getCurrentStack();
#endif			
			
			/** Sets the main function which will be executed within this thread.
				*
				* @param fnc the fuction to be executed
				*	@retval E_SUCCESS
				*/
			SINT32 setMainLoop(THREAD_MAIN_TYP fnc)
				{
					m_fncMainLoop=fnc;
					return E_SUCCESS;
				}

			/** Starts the execution of the main function of this thread. The main function could be set
			  * with setMainLoop().
				*
				* @param param a pointer which is used as argument to the main function
				* @param bDaemon true, if this thread should be a deamon thread. A daemon thread is a dettached thread, which will not
				* @param bSilent if true, no (log) messages about thats going on are produced. This is especially helpful to avoid any blocking on any mutex during a call to start().
				* preserve a join state. A daemon thread will automatically release resources which 
				*	are associated with the thread. Normaly this is done by calling join().
				*						The default value is false.
				* @retval E_SUCCESS if the thread could be started successfully
				* @retval E_UNKNOWN otherwise
				*/								 
			SINT32 start(void* param,bool bDaemon=false,bool bSilent=false);
			
			/** Waits for the main function to finish execution. A call of this method will block until the main
				* function exits.
				* @retval E_SUCCESS if successful
				* @retval E_UNKNOWN otherwise
				*/
			SINT32 join();

/*			SINT32 sleep(UINT32 msSeconds)
				{
					m_CondVar.lock();
					m_CondVar.wait(msSeconds);
					m_CondVar.unlock();
					return E_SUCCESS;
				}

			SINT32 wakeup()
				{
					m_CondVar.lock();
					m_CondVar.unlock();
					m_CondVar.signal();
					return E_SUCCESS;
				}
*/
			UINT8* getName() const
			{
				return m_strName;
			}

			UINT32 getID() const
				{
					return m_Id;
				}

			static thread_id_t getSelfID()
				{
					#ifdef _WIN32
						return (unsigned long) pthread_self().p;
					#else
							return (unsigned long) pthread_self();
					#endif
				}

#ifdef PRINT_THREAD_STACK_TRACE
			static const char* METHOD_BEGIN;
			static const char* METHOD_END;
#endif
		private:
#ifdef PRINT_THREAD_STACK_TRACE
			static void destroyValue(void* a_stack);
			static void initKey();
		
		
			static pthread_key_t ms_threadKey; 
			static pthread_once_t ms_threadKeyInit;
#endif
			THREAD_MAIN_TYP m_fncMainLoop;
#ifdef OS_TUDOS
			l4thread_t m_Thread;
#else	
	 		pthread_t* m_pThread;
#endif
			UINT8* m_strName; //< a name mostly for debugging purpose...
			UINT32 m_Id; // some unique identifier
			static UINT32 ms_LastId;
#if defined _DEBUG && ! defined(ONLY_LOCAL_PROXY)
			static CAThreadList* m_pThreadList;
		public:
			static void setThreadList(CAThreadList* pThreadList)
				{
					m_pThreadList=pThreadList;
				}
#endif
	};
#endif

#endif //ONLY_LOCAL_PROXY
