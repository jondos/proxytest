#include "../CASingleSocketGroup.hpp"

class CASingleSocketGroupDummy:public CASingleSocketGroup
{
		public:

			CASingleSocketGroupDummy(bool bWrite)	: CASingleSocketGroup(bWrite)
				{
				}

			SINT32 add(SOCKET s)
				{
					return E_SUCCESS;
				}

				SINT32 add(CAMuxSocket &s)
				{
					return E_SUCCESS;
				}

			SINT32 select()
				{
					return E_SUCCESS;
				}

				SINT32 select(UINT32 time_ms)
				{
					return E_SUCCESS;
				}
};
