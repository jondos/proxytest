#include "../StdAfx.h"
#include "../CAUtil.hpp"
#include "../CACertificate.hpp"
#include "../CALibProxytest.hpp"
#include "../CAFileSystemDirectory.hpp"
#define CERT_DIR "certs/"

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
			UINT8 filename[8192];
			printf("\nTesting certs in %s:\n",CERT_DIR);
			CAFileSystemDirectory dir((UINT8*)CERT_DIR);
			dir.find((UINT8*)"*.cer");
			while(dir.getNextSearchResult(filename,8192)==E_SUCCESS)
				{
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
		}

		TEST(TCACertificate, verifyCerts)
		{
			UINT8 filename[4096];
			strcpy((char*)filename,CERT_DIR);
			strcat((char*)filename,"cert1.cer");
			printf("\nLoading cert %s:\n",filename);
			UINT32 certInBuffLen=0;
			UINT8* certInBuff=readFile(filename,&certInBuffLen);
			CACertificate* cert1=CACertificate::decode(certInBuff,certInBuffLen,CERT_DER);
			//CPPUNIT_ASSERT_MESSAGE((char*)filename,cert1!=NULL);
			ASSERT_TRUE( cert1 != NULL);

			strcpy((char*)filename,CERT_DIR);
			strcat((char*)filename,"cert2.cer");
			printf("\nLoading cert %s:\n",filename);
			certInBuff=readFile(filename,&certInBuffLen);
			CACertificate* cert2=CACertificate::decode(certInBuff,certInBuffLen,CERT_DER);
			//CPPUNIT_ASSERT_MESSAGE((char*)filename,cert2!=NULL);
			ASSERT_TRUE(cert2 != NULL);
			SINT32 ret=cert1->verify(cert2);
			//CPPUNIT_ASSERT_MESSAGE("Certificate verification failed!",ret==E_SUCCESS);
			ASSERT_TRUE(ret == E_SUCCESS);

			strcpy((char*)filename,CERT_DIR);
			strcat((char*)filename,"cert3.cer");
			printf("\nLoading cert %s:\n",filename);
			certInBuff=readFile(filename,&certInBuffLen);
			CACertificate* cert3=CACertificate::decode(certInBuff,certInBuffLen,CERT_DER);
			//CPPUNIT_ASSERT_MESSAGE((char*)filename,cert3!=NULL);
			ASSERT_TRUE(cert3 != NULL);
			ret=cert2->verify(cert3);
			//CPPUNIT_ASSERT_MESSAGE("Certificate verification failed!",ret==E_SUCCESS);
			ASSERT_TRUE(ret == E_SUCCESS);
			}


