#include "StdAfx.h"
#include "CASignature.hpp"
CASignature::CASignature()
	{
		dsa=DSA_new();
		BN_hex2bn(&dsa->p,	"fd7f53811d75122952df4a9c2eece4e7f611b7523cef4400c31e3f80b6512669455d402251fb593d8d58fabfc5f5ba30f6cb9b556cd7813b801d346ff26660b76b9950a5a49f9fe8047b1022c24fbba9d7feb7c61bf83b57e7c6a8a6150f04fb83f6d3c51ec3023554135a169132f675f3ae2b61d72aeff22203199dd14801c7");
		BN_hex2bn(&dsa->q,	"9760508f15230bccb292b982a2eb840bf0581cf5");
		BN_hex2bn(&dsa->g,"f7e1a085d69b3ddecbbcab5c36b857b97994afbbfa3aea82f9574c0b3d0782675159578ebad4594fe67107108180b449167123e84c281613b7cf09328cc8a6e13c167a8b547c8d28e0a3ae1e2bb3a675916ea37f0bfa213562f1fb627a01243bcca4f1bea8519089a883dfe15ae59f06928b665e807b552564014c3bfecf492a");
		BN_hex2bn(&dsa->pub_key,"15de2be9c64b6b5ab80a2c5337658b2e729dd112991ad5505eac63caf9f87c5c8a01290286a80cee89bb2084debe33721a7886560fe20d33583328e0b21440dbef1f0ff00c53c873d301d4dfee6cf1129520da9a99969c473f4129b06fc7ade31f61db3cda1c792c8409136fae4e5fddef931e46427161491341c5f5f01e31f3");
		BN_hex2bn(&dsa->priv_key,"1c8d7eeaf834310507c5fa384c13b60f9ec163ed");
	}

CASignature::~CASignature()
	{
		DSA_free(dsa);
	}

int CASignature::sign(unsigned char* in,int inlen,unsigned char* sig,unsigned int* siglen)
	{
		unsigned char dgst[SHA_DIGEST_LENGTH];
		SHA1(in,inlen,dgst);
		return DSA_sign(0,dgst,SHA_DIGEST_LENGTH,sig,siglen,dsa);
	}

int CASignature::getSignatureSize()
	{
		return DSA_size(dsa);
	}
