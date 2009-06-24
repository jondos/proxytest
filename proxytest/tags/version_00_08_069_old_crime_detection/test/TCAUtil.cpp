#include "StdAfx.h"
#include <cppunit/TestRunner.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include "CAUtil.hpp"
#include "CACmdLnOptions.hpp"
CACmdLnOptions* pglobalOptions;

class TCAUtilTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(TCAUtilTest);
  CPPUNIT_TEST(test_parseU64);
  CPPUNIT_TEST_SUITE_END();

public:
  void setUp(void) {}
  void tearDown(void) {} 

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
