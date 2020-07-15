#include "../StdAfx.h"
#include "../CALibProxytest.hpp"

int main(int argc, char **argv)
	{
		::testing::InitGoogleTest(&argc, argv);
		CALibProxytest::init();
		::testing::GTEST_FLAG(filter) = "XMLTests*";
		int ret=RUN_ALL_TESTS();
		return ret;
	}