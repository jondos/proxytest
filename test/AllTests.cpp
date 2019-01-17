#include "../StdAfx.h"
#include "../CALibProxytest.hpp"

int main(int argc, char **argv)
	{
		::testing::InitGoogleTest(&argc, argv);
		CALibProxytest::init();
		int ret=RUN_ALL_TESTS();
		return ret;
	}