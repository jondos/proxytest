/*
Copyright (c) 2000, The JAP-Team 
All rights reserved.
Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

	- Redistributions of source code must retain the above copyright notice, 
	  this list of conditions and the following disclaimer.

	- Redistributions in binary form must reproduce the above copyright notice, 
	  this list of conditions and the following disclaimer in the documentation and/or 
		other materials provided with the distribution.

	- Neither the name of the University of Technology Dresden, Germany nor the names of its contributors 
	  may be used to endorse or promote products derived from this software without specific 
		prior written permission. 

	
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS 
OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY 
AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER 
IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE
*/

#include "StdAfx.h"
#ifdef PAYMENT
#include "CASSLContext.hpp"
#include "CACertificate.hpp"
#include "CACmdLnOptions.hpp"
#include "CAMsg.hpp"

extern CACmdLnOptions options;
// WARNING: Implementation in this file might be INCOMPLETE

// the pointer to the singleton instance
CASSLContext * CASSLContext::m_pInstance = 0;


/**
 * Initialize the context, load certificates, keys, etc
 */
CASSLContext::CASSLContext(){

	// get X509 Mix Certificate
	CACertificate * mixCert = options.getOwnCertificate();
	if(mixCert == 0) {
		CAMsg::printMsg(LOG_ERR, "CASSLContext could not get MixCertificate (1)\n");
		return;
	}
	X509 * mixCertX509 = mixCert->getX509();
	if(mixCertX509 == 0) {
		CAMsg::printMsg(LOG_ERR, "CASSLContext could not get MixCertificate (2)\n");
		return;
	}

	// get mix private signkey and convert it to evp format
	// Note: The documentation of these OpenSSL functions is quite poor
	// so there might be some errors left here :-(
	EVP_PKEY pEvpKey;
	CASignature * pSignKey = options.getSignKey();
	if(pSignKey == 0) {
		CAMsg::printMsg(LOG_ERR, "CASSLContext could not get Mix Signkey\n");
		return;
	}
	DSA * pDsaKey = pSignKey->getDSA();

  // is this the correct method to convert dsa -> evp ???
  if(!EVP_PKEY_set1_DSA(&pEvpKey,pDsaKey)) {
		CAMsg::printMsg(LOG_ERR, "CASSLContext could not convert DSA->EVP\n");
		return;
	}

 	// initialize ssl context object
	SSL_METHOD * meth;
	meth = SSLv23_client_method();
	m_SSLCTX = SSL_CTX_new( meth );
	if(m_SSLCTX==0) {
//		CAMsg::printMsg(LOG_ERROR, "CASSLContext Could not init SSL Context\n");
		return;
	}

	// add mix Certificate
	// TODO: use new RootCA structure here
	if(!SSL_CTX_use_certificate(m_SSLCTX, mixCert->getX509())) {
		CAMsg::printMsg(LOG_ERR, "CASSLContext: Error setting Mix Certificate\n");
		return;
	}

	// add private evp key
	if(!SSL_CTX_use_PrivateKey(m_SSLCTX, &pEvpKey)) {
		CAMsg::printMsg(LOG_ERR, "CASSLContext: Error setting EVP PrivateKey\n");
		return;
	}

  /* this is the code from LinuxJournal:
  SSL_METHOD *meth;
  SSL_CTX *ctx;
    if(!bio_err){
      //Global system initialization
      SSL_library_init();
      SSL_load_error_strings();
      // An error write context
      bio_err=BIO_new_fp(stderr,BIO_NOCLOSE);
    }

    // Create our context
    meth=SSLv23_method();
    ctx=SSL_CTX_new(meth);
    // Load our keys and certificates
    if(!(SSL_CTX_use_certificate_chain_file(ctx,
      keyfile)))
      berr_exit("Can't read certificate file");
    pass=password;
    SSL_CTX_set_default_passwd_cb(ctx,
      password_cb);
    if(!(SSL_CTX_use_PrivateKey_file(ctx,
      keyfile,SSL_FILETYPE_PEM)))
      berr_exit("Can't read key file");
    // Load the CAs we trust
    if(!(SSL_CTX_load_verify_locations(ctx,
       CA_LIST,0)))
       berr_exit("Can't read CA list");
#if (OPENSSL_VERSION_NUMBER < 0x0090600fL)
    SSL_CTX_set_verify_depth(ctx,1);
#endif
    return ctx;
  }
  end LinuxJournal sample code */
}

CASSLContext::~CASSLContext(){
}


SSL_CTX * CASSLContext::getSSLContext()
{
	if(m_pInstance == 0) {
		m_pInstance = new CASSLContext();
	}
	return m_pInstance->m_SSLCTX;
}
#endif
