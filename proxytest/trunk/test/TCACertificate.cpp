#include "../StdAfx.h"
#include "../CAUtil.hpp"
#include "../CACertificate.hpp"
#include "../CALibProxytest.hpp"
#define CERT_DIR "test/certs/"

class TCACertificateTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(TCACertificateTest);
  CPPUNIT_TEST(test_decodeCerts);
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp(void) {CALibProxytest::init();}
  void tearDown(void) {CALibProxytest::cleanup();} 

protected:
  void test_decodeCerts(void) 
		{
			UINT32 certInBuffLen=0;
			UINT8* certInBuff=readFile((UINT8*)"test/certs/test.cer",&certInBuffLen);
			CACertificate* cert=CACertificate::decode(certInBuff,certInBuffLen,CERT_DER);
			CPPUNIT_ASSERT_MESSAGE("Failed reading test.cer\n",cert!=NULL);
		}


};

CPPUNIT_TEST_SUITE_REGISTRATION(TCACertificateTest);

