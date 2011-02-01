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
			struct _finddata_t c_file;
			intptr_t hFile;
			UINT8 filename[4096];
			strcpy((char*)filename,CERT_DIR);
			strcat((char*)filename,"*.cer");
			printf("\nTesting certs in %s:\n",CERT_DIR);
			hFile = _findfirst( (char*)filename, &c_file );
		   do {
						strcpy((char*)filename,CERT_DIR);
						strcat((char*)filename,c_file.name);
						printf("Testing cert: %s -- ",filename);
						UINT32 certInBuffLen=0;
						UINT8* certInBuff=readFile(filename,&certInBuffLen);
						CACertificate* cert=CACertificate::decode(certInBuff,certInBuffLen,CERT_DER);
						CPPUNIT_ASSERT_MESSAGE((char*)filename,cert!=NULL);
						if(cert!=NULL)
							printf("ok\n");
						else
							printf("failed!\n");

      } while( _findnext( hFile, &c_file ) == 0 );
      _findclose( hFile );	
			
		}


};

CPPUNIT_TEST_SUITE_REGISTRATION(TCACertificateTest);

