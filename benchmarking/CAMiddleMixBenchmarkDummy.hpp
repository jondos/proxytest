#ifdef DO_MIDDLE_MIX_BENCHMARK
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
#endif DO_MIDDLE_MIX_BENCHMARK
