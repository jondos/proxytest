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
#include "CASignature.hpp"
#include "CABase64.hpp"
#include "CAUtil.hpp"
#include "xml/xmlstream.h"
#include "xml/xmlinput.h"

class BufferInputStream:public XMLInputStream
	{
		public:
			BufferInputStream(UINT8* buff,UINT32 l)
				{
					buffer=buff;
					len=l;
					pos=0;
				}

		int read(XML_Char *buf, size_t bufLen)
			{
				UINT32 size=(UINT32)min(bufLen,len-pos);
				if(size==0)
					return 0;
				memcpy(buf,buffer+pos,size);
				pos+=size;
				return size;
			}

		private:
			UINT8* buffer;
			UINT32 pos;
			UINT32 len;
	};


CASignature::CASignature()
	{
		m_pDSA=NULL;
	}

CASignature::~CASignature()
	{
		DSA_free(m_pDSA);
	}

SINT32 CASignature::setSignKey(UINT8* buff,UINT32 len,UINT32 type)
	{
		if(buff==NULL||len<1||type!=SIGKEY_XML)
			return E_UNKNOWN;
		if(type==SIGKEY_XML)
			return parseSignKeyXML(buff,len);
		return E_UNKNOWN;
	}


//XML Decode...
static void sDSAKeyParamValueHandler(XMLElement &elem, void *userData)
{
	UINT8 buff[4096];
	int len=(int)elem.ReadData((char*)buff,4096);
	
	UINT32 decLen=4096;
	UINT8 decBuff[4096];
	CABase64::decode(buff,len,decBuff,&decLen);
	
	DSA* tmpDSA=(DSA*)userData;
	switch(elem.GetName()[0])
		{
			case 'P':
				if(tmpDSA->p!=NULL)
					BN_free(tmpDSA->p);
				tmpDSA->p=BN_bin2bn((unsigned char*)decBuff,decLen,NULL);
			break;
			case 'Q':
				if(tmpDSA->q!=NULL)
					BN_free(tmpDSA->q);
				tmpDSA->q=BN_bin2bn((unsigned char*)decBuff,decLen,NULL);
			break;
			case 'G':
				if(tmpDSA->g!=NULL)
					BN_free(tmpDSA->g);
				tmpDSA->g=BN_bin2bn((unsigned char*)decBuff,decLen,NULL);
			break;
			case 'X':
				if(tmpDSA->priv_key!=NULL)
					BN_free(tmpDSA->priv_key);
				tmpDSA->priv_key=BN_bin2bn((unsigned char*)decBuff,decLen,NULL);
			break;
			case 'Y':
				if(tmpDSA->pub_key!=NULL)
					BN_free(tmpDSA->pub_key);
				tmpDSA->pub_key=BN_bin2bn((unsigned char*)decBuff,decLen,NULL);
			break;
		}
	
}

static void sDSAKeyValueHandler(XMLElement &elem, void *userData)
{
	XMLHandler handlers[] = {
	XMLHandler("P",sDSAKeyParamValueHandler),
	XMLHandler("Q",sDSAKeyParamValueHandler),
	XMLHandler("G",sDSAKeyParamValueHandler),
	XMLHandler("X",sDSAKeyParamValueHandler),
	XMLHandler("Y",sDSAKeyParamValueHandler),
	XMLHandler::END};
		elem.Process(handlers, userData);
}


static void sKeyValueHandler(XMLElement &elem, void *userData)
{
		XMLHandler handlers[] = {
		XMLHandler("DSAKeyValue",sDSAKeyValueHandler),
			XMLHandler::END};
		elem.Process(handlers, userData);
}

static void sKeyInfoHandler(XMLElement &elem, void *userData)
{
		XMLHandler handlers[] = {
		XMLHandler("KeyValue",sKeyValueHandler),
			XMLHandler::END};
		elem.Process(handlers, userData);
}

SINT32 CASignature::parseSignKeyXML(UINT8* buff,UINT32 len)
	{
		BufferInputStream oStream(buff,len);
		XMLInput input(oStream);

	// set up initial handler for Document
		XMLHandler handlers[] = 
			{
				XMLHandler("KeyInfo",sKeyInfoHandler),
				XMLHandler::END
			};
	
		DSA* tmpDSA=DSA_new();
		try
			{
				input.Process(handlers, tmpDSA);
			}
		catch (const XMLParseException &e)
			{
				DSA_free(tmpDSA);
				return E_UNKNOWN;
			}
		if(DSA_sign_setup(tmpDSA,NULL,&tmpDSA->kinv,&tmpDSA->r)!=1)
			{
				DSA_free(tmpDSA);
				return E_UNKNOWN;
			}
		m_pDSA=tmpDSA;
		return E_SUCCESS;
	}


SINT32 CASignature::sign(UINT8* in,UINT32 inlen,UINT8* sig,UINT32* siglen)
	{
		UINT8 dgst[SHA_DIGEST_LENGTH];
		SHA1(in,inlen,dgst);
		if(DSA_sign(0,dgst,SHA_DIGEST_LENGTH,sig,siglen,m_pDSA)==1)
		 return E_SUCCESS;
		return E_UNKNOWN;
	}

SINT32 CASignature::getSignatureSize()
	{
		return DSA_size(m_pDSA);
	}

const char *XMLSIGINFO_TEMPLATE=
"<SignedInfo>\n\t\t<Reference URI=\"\">\n\t\t<DigestValue>%s</DigestValue>\n\t</Reference>\n\t</SignedInfo>";
const char *XMLSIG_TEMPLATE=
"<Signature>\n\t%s\n<SignatureValue>%s</SignatureValue>\n</Signature>";

SINT32 CASignature::signXML(UINT8* in,UINT32 inlen,UINT8* out,UINT32 *outlen)
	{
		if(in==NULL||inlen<1||out==NULL||*outlen<inlen+getXMLSignatureSize())
			return E_UNKNOWN;
		
		//Calculating the Digest...
		UINT32 len=*outlen;
		if(makeXMLCanonical(in,inlen,out,&len)!=E_SUCCESS)
			return E_UNKNOWN;
		UINT8 dgst[SHA_DIGEST_LENGTH];
		SHA1(out,len,dgst);
		UINT8 tmpBuff[1024];
		len=1024;
		if(CABase64::encode(dgst,SHA_DIGEST_LENGTH,tmpBuff,&len)!=E_SUCCESS)
			return E_UNKNOWN;
		tmpBuff[len]=0;

		//Creating the Sig-InfoBlock....
		sprintf((char*)out,XMLSIGINFO_TEMPLATE,tmpBuff);
		
		// Signing the SignInfo block....
		len=1024;//*outlen;
		if(makeXMLCanonical(out,(UINT32)strlen((char*)out),tmpBuff,&len)!=E_SUCCESS)
			return E_UNKNOWN;
		UINT sigSize=255;
		UINT8 sig[255];
		UINT8* c=sig;
		if(sign(tmpBuff,len,sig,&sigSize)!=E_SUCCESS)
			return E_UNKNOWN;
		
		//Making Base64-Encode r and s
		STACK* a=NULL;
		d2i_ASN1_SET(&a,&c,sigSize,(char *(*)(void))d2i_ASN1_INTEGER,NULL,V_ASN1_SEQUENCE,V_ASN1_UNIVERSAL);
		BIGNUM* s =BN_new();
		ASN1_INTEGER* i=(ASN1_INTEGER*)sk_pop(a);
		ASN1_INTEGER_to_BN(i,s);
		ASN1_INTEGER_free(i);
		BIGNUM* r =BN_new();
		i=(ASN1_INTEGER*)sk_pop(a);
		ASN1_INTEGER_to_BN(i,r);
		ASN1_INTEGER_free(i);
		sk_free(a);

		memset(tmpBuff,0,40); //make first 40 bytes '0' --> if r or s is less then 20 bytes long! 
													//(Due to be compatible to the standarad r and s must be 20 bytes each) 
		BN_bn2bin(r,tmpBuff+20-BN_num_bytes(r)); //so r is 20 bytes with leading '0'...
		BN_bn2bin(s,tmpBuff+40-BN_num_bytes(s));
		BN_free(r);
		BN_free(s);

		sigSize=255;
		if(CABase64::encode(tmpBuff,40,sig,&sigSize)!=E_SUCCESS)
			return E_UNKNOWN;
		sig[sigSize]=0;

		//Makeing the hole Signature-Block....
		sprintf((char*)tmpBuff,XMLSIG_TEMPLATE,out,sig);

		/* Find the last closing tag (</...>) and insert the <Signature> Element just before*/
		int pos=inlen-1;
		while(pos>=0&&in[pos]!='<')
			pos--;
		if(pos<0)
			return E_UNKNOWN;
		*outlen=pos;
		memcpy(out,in,*outlen);
		memcpy(out+(*outlen),tmpBuff,strlen((char*)tmpBuff));
		*outlen+=strlen((char*)tmpBuff);
		memcpy(out+(*outlen),in+pos,inlen-pos);
		(*outlen)+=inlen-pos;
		return E_SUCCESS;
	}

SINT32 CASignature::getXMLSignatureSize()
	{
		return (SINT32)strlen(XMLSIG_TEMPLATE)+strlen(XMLSIGINFO_TEMPLATE)+/*size of DigestValue*/+20+/*sizeof SignatureValue*/+40;
	}

typedef struct
	{
		UINT8* out;
		UINT32 outlen;
		UINT32 pos;
		SINT32 err;
	} XMLCanonicalHandlerData;

static void smakeXMLCanonicalDataHandler(const XML_Char *data, size_t len, void *userData)
	{
		XMLCanonicalHandlerData* pData=(XMLCanonicalHandlerData*)userData;
		if(pData->err!=0)
			return;
		UINT8* buff=new UINT8[len];
		len=memtrim(buff,(UINT8*)data,len);
		if(len==0)
			{
				delete buff;
				return;
			}

		if(pData->outlen-pData->pos<len)
			{
				pData->err=-1;
				delete buff;
				return;
			}
		memcpy(pData->out+pData->pos,buff,len);
		pData->pos+=len;
		delete buff;
	}

static void smakeXMLCanonicalElementHandler(XMLElement &elem, void *userData)
	{
		XMLCanonicalHandlerData* pData=(XMLCanonicalHandlerData*)userData;
		if(pData->err!=0)
			return;
		char* name=(char*)elem.GetName();
		UINT32 namelen=strlen(name);
		if(pData->outlen-pData->pos<2*namelen+5)
			{
				pData->err=-1;
				return;
			}
		pData->out[pData->pos++]='<';
		memcpy(pData->out+pData->pos,name,namelen);
		pData->pos+=namelen;
		if(elem.NumAttributes()>0)
			{
				XMLAttribute attr=elem.GetAttrList();
				while(attr)
					{
						char* attrname=(char*)attr.GetName();
						char* attrvalue=(char*)attr.GetValue();
						pData->out[pData->pos++]=' ';
						int len=(int)strlen(attrname);
						memcpy(pData->out+pData->pos,attrname,len);
						pData->pos+=len;
						pData->out[pData->pos++]='=';
						pData->out[pData->pos++]='\"';
						len=(int)strlen(attrvalue);
						memcpy(pData->out+pData->pos,attrvalue,len);
						pData->pos+=len;
						pData->out[pData->pos++]='\"';
						attr=attr.GetNext();
					}
			}
		pData->out[pData->pos++]='>';
		//procces children...
		static XMLHandler handlers[] = {
		XMLHandler(smakeXMLCanonicalElementHandler),
		XMLHandler(smakeXMLCanonicalDataHandler),
		XMLHandler::END
		};
		elem.Process(handlers,userData);
		//closing tag...
		pData->out[pData->pos++]='<';
		pData->out[pData->pos++]='/';
		memcpy(pData->out+pData->pos,name,namelen);
		pData->pos+=namelen;
		pData->out[pData->pos++]='>';

	}

SINT32 CASignature::makeXMLCanonical(UINT8* in,UINT32 inlen,UINT8* out,UINT32* outlen)
	{
		BufferInputStream oStream(in,inlen);
		XMLInput input(oStream);

	// set up initial handler for Document
		XMLHandler handlers[] =
			{
				XMLHandler(smakeXMLCanonicalElementHandler),
				XMLHandler(smakeXMLCanonicalDataHandler),
				XMLHandler::END
			};
	
		XMLCanonicalHandlerData oData;
		oData.out=out;
		oData.outlen=*outlen;
		oData.pos=0;
		oData.err=0;
		try
			{
				input.Process(handlers, &oData);
			}
		catch (const XMLParseException &e)
			{
				return E_UNKNOWN;
			}
		*outlen=oData.pos;
		return oData.err;
	}
