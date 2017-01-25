#include "../StdAfx.h"
#include "../CAUtil.hpp"
#include "../CACertificate.hpp"
#include "../CALibProxytest.hpp"
#define CERT_DIR "test/certs/"

/*
	public:
		void setUp(void)
		{
			CALibProxytest::init();
		}
		void tearDown(void)
		{
			CALibProxytest::cleanup();
		}
		*/
		TEST(TCACertificate,decodeCerts)
		{
#ifdef _WIN32
			struct _finddata_t c_file;
			intptr_t hFile;
			UINT8 filename[4096];
			strcpy((char*)filename,CERT_DIR);
			strcat((char*)filename,"*.cer");
			printf("\nTesting certs in %s:\n",CERT_DIR);
			hFile = _findfirst( (char*)filename, &c_file );
			do
				{
					strcpy((char*)filename,CERT_DIR);
					strcat((char*)filename,c_file.name);
					printf("Testing cert: %s -- ",filename);
					UINT32 certInBuffLen=0;
					UINT8* certInBuff=readFile(filename,&certInBuffLen);
					CACertificate* cert=CACertificate::decode(certInBuff,certInBuffLen,CERT_DER);
					//CPPUNIT_ASSERT_MESSAGE((char*)filename,cert!=NULL);
					ASSERT_TRUE(cert != NULL);
					if(cert!=NULL)
						printf("ok\n");
					else
						printf("failed!\n");

				}
			while( _findnext( hFile, &c_file ) == 0 );
			_findclose( hFile );
#endif
		}

		TEST(TCACertificate, verifyCerts)
		{
			UINT8 filename[4096];
			strcpy((char*)filename,CERT_DIR);
			strcat((char*)filename,"speedy_public.b64.cer");
			printf("\nLoading cert %s:\n",filename);
			UINT32 certInBuffLen=0;
			UINT8* certInBuff=readFile(filename,&certInBuffLen);
			CACertificate* cert1=CACertificate::decode(certInBuff,certInBuffLen,CERT_DER);
			//CPPUNIT_ASSERT_MESSAGE((char*)filename,cert1!=NULL);
			ASSERT_TRUE( cert1 != NULL);

			strcpy((char*)filename,CERT_DIR);
			strcat((char*)filename,"yatrade_public.b64.cer");
			printf("\nLoading cert %s:\n",filename);
			certInBuff=readFile(filename,&certInBuffLen);
			CACertificate* cert2=CACertificate::decode(certInBuff,certInBuffLen,CERT_DER);
			//CPPUNIT_ASSERT_MESSAGE((char*)filename,cert2!=NULL);
			ASSERT_TRUE(cert2 != NULL);
			SINT32 ret=cert1->verify(cert2);
			//CPPUNIT_ASSERT_MESSAGE("Certificate verification failed!",ret==E_SUCCESS);
			ASSERT_TRUE(ret == E_SUCCESS);

			strcpy((char*)filename,CERT_DIR);
			strcat((char*)filename,"Operator_CA.cer");
			printf("\nLoading cert %s:\n",filename);
			certInBuff=readFile(filename,&certInBuffLen);
			CACertificate* cert3=CACertificate::decode(certInBuff,certInBuffLen,CERT_DER);
			//CPPUNIT_ASSERT_MESSAGE((char*)filename,cert3!=NULL);
			ASSERT_TRUE(cert3 != NULL);
			ret=cert2->verify(cert3);
			//CPPUNIT_ASSERT_MESSAGE("Certificate verification failed!",ret==E_SUCCESS);
			ASSERT_TRUE(ret == E_SUCCESS);
			}


