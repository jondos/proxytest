#include "StdAfx.h"
#include "CAASymCipher.hpp"

CAASymCipher::CAASymCipher()
	{
		rsa=NULL;
/*		rsa=RSA_new();
		BN_dec2bn(&rsa->n,"146045156752988119086694783791784827226235382817403930968569889520448117142515762490154404168568789906602128114569640745056455078919081535135223786488790643345745133490238858425068609186364886282528002310113020992003131292706048279603244985126945363695371250073851319256901415103802627246986865697725280735339");
		BN_dec2bn(&rsa->e,"65537");
		BN_dec2bn(&rsa->d,"100449081981135731814880969134969450418081177287292668146836998032008168337214710926746722039495350318598616459287747779485859425017265350531079350592317712851494980594232914153427023840292323686802553784291416771584974703478561388467839475898474798517021187817368293846108514686245237357922210697818039852961");
		BN_dec2bn(&rsa->p,"12987122050932746826150318675050441016017480290196436481483831016275193544458430352845567706110422764241659422875175787195135987176410029197001621285742957");
		BN_dec2bn(&rsa->q,"11245382632135887575471281417377089057560682219005124170598788751650268860247740981683175391022053998046084433194418490790049622751379904072048156065321527");

		BIGNUM bn1;
		BN_init(&bn1);
		BN_one(&bn1);
		BIGNUM tbn;
		BN_init(&tbn);
		BN_sub(&tbn,rsa->p,&bn1);
		BN_CTX ctx;
		BN_CTX_init(&ctx);
		rsa->dmp1=BN_new();
		BN_mod(rsa->dmp1,rsa->d,&tbn,&ctx);
		BN_sub(&tbn,rsa->q,&bn1);
		rsa->dmq1=BN_new();
		BN_mod(rsa->dmq1,rsa->d,&tbn,&ctx);
		rsa->iqmp=BN_new();
		BN_mod_inverse(rsa->iqmp,rsa->q,rsa->p,&ctx);
	*///	int i=RSA_check_key(rsa);
	}

CAASymCipher::~CAASymCipher()
	{
		RSA_free(rsa);
	}

int CAASymCipher::decrypt(unsigned char* from,unsigned char* to)
	{
		return RSA_private_decrypt(128,from,to,rsa,RSA_NO_PADDING);		
	}

int CAASymCipher::encrypt(unsigned char* from,unsigned char* to)
	{
		return RSA_public_encrypt(128,from,to,rsa,RSA_NO_PADDING);		
	}

int CAASymCipher::generateKeyPair(int size)
	{
		RSA_free(rsa);
		rsa=RSA_generate_key(size,65537,NULL,NULL);
		if(rsa==NULL)
			return -1;
		else
			return 0;
	}

int CAASymCipher::getPublicKey(unsigned char* buff,int *len)
	{
		if(buff==NULL||*len<getPublicKeySize())
			return -1;
		int aktIndex=0;
		unsigned short size=htons(BN_num_bytes(rsa->n));
		memcpy(buff,&size,sizeof(size));
		aktIndex+=sizeof(size);
		BN_bn2bin(rsa->n,buff+aktIndex);
		aktIndex+=BN_num_bytes(rsa->n);
		size=htons(BN_num_bytes(rsa->e));
		memcpy(buff+aktIndex,&size,sizeof(size));
		aktIndex+=sizeof(size);
		BN_bn2bin(rsa->e,buff+aktIndex);
		aktIndex+=BN_num_bytes(rsa->e);
		*len=aktIndex;
		return 0;
	}

int CAASymCipher::getPublicKeySize()
	{
		if(rsa==NULL||rsa->n==NULL||rsa->e==NULL)
			return -1;
		return BN_num_bytes(rsa->n)+BN_num_bytes(rsa->e)+4;
	}

int CAASymCipher::setPublicKey(unsigned char* key,int* len)
	{
		rsa=RSA_new();
		int aktIndex=0;
		unsigned short size=ntohs(*((unsigned short*)key));
		aktIndex+=2;
		rsa->n=BN_new();
		BN_bin2bn(key+aktIndex,size,rsa->n);
		aktIndex+=size;
		size=ntohs(*((unsigned short*)(key+aktIndex)));
		aktIndex+=2;
		rsa->e=BN_new();
		BN_bin2bn(key+aktIndex,size,rsa->e);
		aktIndex+=size;

	/*	BIGNUM bn1;
		BN_init(&bn1);
		BN_one(&bn1);
		BIGNUM tbn;
		BN_init(&tbn);
		BN_sub(&tbn,rsa->p,&bn1);
		BN_CTX ctx;
		BN_CTX_init(&ctx);
		rsa->dmp1=BN_new();
		BN_mod(rsa->dmp1,rsa->d,&tbn,&ctx);
		BN_sub(&tbn,rsa->q,&bn1);
		rsa->dmq1=BN_new();
		BN_mod(rsa->dmq1,rsa->d,&tbn,&ctx);
		rsa->iqmp=BN_new();
		BN_mod_inverse(rsa->iqmp,rsa->q,rsa->p,&ctx);
	*/
		*len=aktIndex;
	
		//	int i=RSA_check_key(rsa);
		return 0;
	}

	