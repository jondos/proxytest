class CASocketGroup
	{
		public:
			CASocketGroup();
			int add(CASocket&s);
			int select();
			bool isSignaled(CASocket&s);
		protected:
			fd_set m_fdset;
			fd_set m_signaled_set;
	};