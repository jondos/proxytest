#include "CAMuxSocket.hpp"
class CASocketGroup
	{
		public:
			CASocketGroup();
			int add(CASocket&s);
			int add(CAMuxSocket&s);
			int remove(CASocket&s);
			int select();
			bool isSignaled(CASocket&s);
			bool isSignaled(CAMuxSocket&s);
		protected:
			fd_set m_fdset;
			fd_set m_signaled_set;
			#ifndef _WIN32
			    int max;
			#endif
	};
