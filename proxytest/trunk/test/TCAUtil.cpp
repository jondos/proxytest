#include "../StdAfx.h"
#include "../CAUtil.hpp"
#include "../CACmdLnOptions.hpp"
#include "../TypeA/CALastMixA.hpp"
#include "../CALibProxytest.hpp"

class CparseDomainFromPayloadHelper :public CALastMix
	{
		public:
			CparseDomainFromPayloadHelper(char* payload,char* expectedDomain)
				{
					m_payload=(UINT8*)payload;
					m_expectedDomain=(UINT8*)expectedDomain;
				}

			void doTest();
			virtual SINT32 loop(){return 0;}
	private:
		UINT8* m_payload;
		UINT8* m_expectedDomain;
		
	};

class TCAUtilTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(TCAUtilTest);
  CPPUNIT_TEST(test_parseU64);
#ifdef LOG_CRIME
  CPPUNIT_TEST(test_parseDomainFromPayload);
#endif
	CPPUNIT_TEST(test_regexp_payload);
	CPPUNIT_TEST_SUITE_END();

public:
	void setUp(void) {CALibProxytest::init();}
  void tearDown(void) {CALibProxytest::cleanup();} 

protected:
  void test_parseU64(void) 
		{
			UINT64 u64;
			UINT64 u64Ref;
			SINT32 ret;
			set64(u64Ref,(UINT32)1);
			set64(u64,(UINT32)0);
			ret=parseU64((UINT8*)"1",u64);
			CPPUNIT_ASSERT_EQUAL(ret,E_SUCCESS);
			CPPUNIT_ASSERT(isEqual64(u64Ref,u64));

			set64(u64Ref,(UINT32)0);
			ret=parseU64((UINT8*)"0",u64);
			CPPUNIT_ASSERT_EQUAL(ret,E_SUCCESS);
			CPPUNIT_ASSERT(isEqual64(u64Ref,u64));

			ret=parseU64((UINT8*)"-1",u64);
			CPPUNIT_ASSERT(ret!=E_SUCCESS);

			ret=parseU64((UINT8*)"-a",u64);
			CPPUNIT_ASSERT(ret!=E_SUCCESS);

			ret=parseU64((UINT8*)"d",u64);
			CPPUNIT_ASSERT(ret!=E_SUCCESS);

			set64(u64Ref,(UINT32)34564566);
			ret=parseU64((UINT8*)"34564566",u64);
			CPPUNIT_ASSERT_EQUAL(ret,E_SUCCESS);
			CPPUNIT_ASSERT(isEqual64(u64Ref,u64));
	}

	void test_parseDomainFromPayload()
		{
			CparseDomainFromPayloadHelper arTest[]=
			{
				CparseDomainFromPayloadHelper("GET http://www.bild.de HTTP/1.1","www.bild.de"),
				CparseDomainFromPayloadHelper("GET http://www.bild.de","www.bild.de"),
				CparseDomainFromPayloadHelper("CONNECT www.bild.de:443","www.bild.de")
			};
			for(UINT32 i=0;i<sizeof(arTest)/sizeof(CparseDomainFromPayloadHelper);i++)
				arTest[i].doTest();
		}

	void test_regexp_payload()
		{
			regex_t regex;
			regcomp(&regex,"HALLO",REG_EXTENDED|REG_ICASE|REG_NOSUB);
			int ret=regexec(&regex,"dfdsfdsf\n\rdsfdsfdsf\nfgfdgdfgfdg\r\ndfdfdfdsfHaLlofdsfdsf",0,NULL,0);
			CPPUNIT_ASSERT_EQUAL(ret,0);
			regfree(&regex);
		}

};

CPPUNIT_TEST_SUITE_REGISTRATION(TCAUtilTest);

int main( int ac, char **av )
{
  //--- Create the event manager and test controller
  CPPUNIT_NS::TestResult controller;

  //--- Add a listener that colllects test result
  CPPUNIT_NS::TestResultCollector result;
  controller.addListener( &result );        

  //--- Add a listener that print dots as test run.
  CPPUNIT_NS::BriefTestProgressListener progress;
  controller.addListener( &progress );      

  //--- Add the top suite to the test runner
  CPPUNIT_NS::TestRunner runner;
  runner.addTest( CPPUNIT_NS::TestFactoryRegistry::getRegistry().makeTest() );
  runner.run( controller );

  return result.wasSuccessful() ? 0 : 1;
}

#ifdef LOG_CRIME
void CparseDomainFromPayloadHelper::doTest()
	{
		UINT8* domain=parseDomainFromPayload(m_payload,strlen((char*)m_payload));
		CPPUNIT_ASSERT(domain!=NULL);
		if(domain!=NULL)
			{
				int ret=strcmp((char*)domain,(char*)m_expectedDomain);
				CPPUNIT_ASSERT(ret==0);
			}
	}
#endif
