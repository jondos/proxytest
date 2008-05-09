/*
Copyright (c) The JAP-Team, JonDos GmbH

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation and/or
       other materials provided with the distribution.
    * Neither the name of the University of Technology Dresden, Germany, nor the name of
       the JonDos GmbH, nor the names of their contributors may be used to endorse or
       promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#ifndef CATHREADLIST_H_
#define CATHREADLIST_H_

/**
 * @author Simon Pecher, JonDos GmbH
 */
#if defined (DEBUG) && !defined(ONLY_LOCAL_PROXY)
struct thread_list_entry
{
	CAThread* tle_thread;
	struct thread_list_entry *tle_next;
};

typedef struct thread_list_entry thread_list_entry_t;

/* Just a simple list implementation to get hold of all created threads */
class CAThreadList
{

public:
	CAThreadList();
	virtual ~CAThreadList();
	
	SINT32	put(const CAThread* const thread);
	SINT32	remove(const CAThread* const thread);
	//CAThread* get(CAThread *thread);
	void showAll() const;
	UINT32 getSize() const
		{
			return m_Size;
		}
	
private:
	
	void			removeAll();
	
	UINT32 m_Size;
	thread_list_entry_t* m_pHead;
	CAMutex* m_pListLock;
};

#endif /*CATHREADLIST_H_*/
#endif //DEBUG
