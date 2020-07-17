#pragma once
#include "../CAMiddleMix.hpp"

class CAMiddleMixBenchmarkDummy : public CAMiddleMix
{
		public:
			CAMiddleMixBenchmarkDummy()	: CAMiddleMix()
			{
			}

			private:
				SINT32 init();
				SINT32 initOnce();
				SINT32 clean();
};
