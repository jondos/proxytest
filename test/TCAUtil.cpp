#include "../StdAfx.h"
#include "../CAUtil.hpp"
#include "../CACmdLnOptions.hpp"
#include "../TypeA/CALastMixA.hpp"
#include "../CALibProxytest.hpp"


#ifdef LOG_CRIME
class CparseDomainFromPayloadHelper :public CALastMix
	{
		public:
			CparseDomainFromPayloadHelper(const char* payload,const char* expectedDomain)
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

void CparseDomainFromPayloadHelper::doTest()
	{
	UINT8* domain = parseDomainFromPayload(m_payload, strlen((char*)m_payload));
	ASSERT_TRUE(domain != NULL);
	if (domain != NULL)
		{
		int ret = strcmp((char*)domain, (char*)m_expectedDomain);
		ASSERT_TRUE(ret == 0);
		}
	}


/*
public:
	void setUp(void) {CALibProxytest::init();}
  void tearDown(void) {CALibProxytest::cleanup();} 
*/

TEST(TCAUtil, parseDomainFromPayload)
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


#endif

TEST(TCAUtil, parseU64)
	{
	UINT64 u64;
	UINT64 u64Ref;
	SINT32 ret;
	set64(u64Ref, (UINT32)1);
	set64(u64, (UINT32)0);
	ret = parseU64((UINT8*)"1", u64);
	ASSERT_EQ(ret, E_SUCCESS);
	ASSERT_TRUE(isEqual64(u64Ref, u64));

	set64(u64Ref, (UINT32)0);
	ret = parseU64((UINT8*)"0", u64);
	ASSERT_EQ(ret, E_SUCCESS);
	ASSERT_TRUE(isEqual64(u64Ref, u64));

	ret = parseU64((UINT8*)"-1", u64);
	ASSERT_TRUE(ret != E_SUCCESS);

	ret = parseU64((UINT8*)"-a", u64);
	ASSERT_TRUE(ret != E_SUCCESS);

	ret = parseU64((UINT8*)"d", u64);
	ASSERT_TRUE(ret != E_SUCCESS);

	set64(u64Ref, (UINT32)34564566);
	ret = parseU64((UINT8*)"34564566", u64);
	ASSERT_EQ(ret, E_SUCCESS);
	ASSERT_TRUE(isEqual64(u64Ref, u64));
	}

TEST(TCAUtil, regex)
	{
	tre_regex_t regex;
	tre_regcomp(&regex, "HALLO", REG_EXTENDED | REG_ICASE | REG_NOSUB);
	int ret = tre_regexec(&regex, "dfdsfdsf\n\rdsfdsfdsf\nfgfdgdfgfdg\r\ndfdfdfdsfHaLlofdsfdsf", 0, NULL, 0);
	ASSERT_EQ(ret, 0);
	tre_regfree(&regex);
	}