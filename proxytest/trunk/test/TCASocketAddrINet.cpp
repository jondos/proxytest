#include "../StdAfx.h"
#include "../CASocketAddrINet.hpp"

TEST(TCASocketAddrINet, getIPForString)
	{
		UINT8 ip[4];
		SINT32 ret=CASocketAddrINet::getIPForString((UINT8*)"1.2.3.4", ip);
		ASSERT_EQ(ret, E_SUCCESS);
		ASSERT_EQ(ip[0],1);
		ASSERT_EQ(ip[1], 2);
		ASSERT_EQ(ip[2], 3);
		ASSERT_EQ(ip[3], 4);

		ret = CASocketAddrINet::getIPForString((UINT8*)"1.2.3.400", ip);
		ASSERT_NE(ret, E_SUCCESS);

	}

