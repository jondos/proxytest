#include "../StdAfx.h"
#include "../CAUtil.hpp"
#include "../CACertificate.hpp"
#include "../CALibProxytest.hpp"
#define CERT_DIR "test/certs/"

class TCACertificateTest : public CPPUNIT_NS::TestCase
{
  CPPUNIT_TEST_SUITE(TCACertificateTest);
  CPPUNIT_TEST(test_decodeCerts);
  CPPUNIT_TEST(test_verifyCerts);
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

  void test_verifyCerts(void) 
		{
			UINT8 filename[4096];
			strcpy((char*)filename,CERT_DIR);
			strcat((char*)filename,"speedy_public.b64.cer");
			printf("\nLoading cert %s:\n",filename);
			UINT32 certInBuffLen=0;
			UINT8* certInBuff=readFile(filename,&certInBuffLen);
			CACertificate* cert1=CACertificate::decode(certInBuff,certInBuffLen,CERT_DER);
			CPPUNIT_ASSERT_MESSAGE((char*)filename,cert1!=NULL);
			strcpy((char*)filename,CERT_DIR);
			strcat((char*)filename,"yatrade_public.b64.cer");
			printf("\nLoading cert %s:\n",filename);
			certInBuff=readFile(filename,&certInBuffLen);
			CACertificate* cert2=CACertificate::decode(certInBuff,certInBuffLen,CERT_DER);
			CPPUNIT_ASSERT_MESSAGE((char*)filename,cert2!=NULL);
			SINT32 ret=cert1->verify(cert2);
			CPPUNIT_ASSERT_MESSAGE("Certificate verification failed!",ret==E_SUCCESS);
			strcpy((char*)filename,CERT_DIR);
			strcat((char*)filename,"Operator_CA.cer");
			printf("\nLoading cert %s:\n",filename);
			certInBuff=readFile(filename,&certInBuffLen);
			CACertificate* cert3=CACertificate::decode(certInBuff,certInBuffLen,CERT_DER);
			CPPUNIT_ASSERT_MESSAGE((char*)filename,cert3!=NULL);
			ret=cert2->verify(cert3);
			CPPUNIT_ASSERT_MESSAGE("Certificate verification failed!",ret==E_SUCCESS);
		}

};

CPPUNIT_TEST_SUITE_REGISTRATION(TCACertificateTest);

