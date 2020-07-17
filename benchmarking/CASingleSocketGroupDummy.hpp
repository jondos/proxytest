#include "../CASingleSocketGroup.hpp"

class CASingleSocketGroupDummy:public CASingleSocketGroup
{
		public:

			CASingleSocketGroupDummy(bool bWrite)	: CASingleSocketGroup(bWrite)
				{
				}
};
