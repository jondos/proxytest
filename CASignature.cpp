#include "StdAfx.h"
#include "CASignature.hpp"
#include "CABase64.hpp"
#include "CAUtil.hpp"
#include "xml/xmlstream.h"
#include "xml/xmlinput.h"
class BufferInputStream:public XML::InputStream
	{
		public:
			BufferInputStream(char* buff,unsigned int l)
				{
					buffer=buff;
					len=l;
					pos=0;
				}
		int read(XML_Char *buf, size_t bufLen)
			{
				unsigned int size=min(bufLen,len-pos);
				if(size==0)
					return 0;
				memcpy(buf,buffer+pos,size);
				pos+=size;
				return size;
			}

		private:
			char* buffer;
			unsigned int pos;
			unsigned int len;
	};


CASignature::CASignature()
	{
/*		dsa=DSA_new();
		BN_hex2bn(&dsa->p,	"fd7f53811d75122952df4a9c2eece4e7f611b7523cef4400c31e3f80b6512669455d402251fb593d8d58fabfc5f5ba30f6cb9b556cd7813b801d346ff26660b76b9950a5a49f9fe8047b1022c24fbba9d7feb7c61bf83b57e7c6a8a6150f04fb83f6d3c51ec3023554135a169132f675f3ae2b61d72aeff22203199dd14801c7");
		BN_hex2bn(&dsa->q,	"9760508f15230bccb292b982a2eb840bf0581cf5");
		BN_hex2bn(&dsa->g,"f7e1a085d69b3ddecbbcab5c36b857b97994afbbfa3aea82f9574c0b3d0782675159578ebad4594fe67107108180b449167123e84c281613b7cf09328cc8a6e13c167a8b547c8d28e0a3ae1e2bb3a675916ea37f0bfa213562f1fb627a01243bcca4f1bea8519089a883dfe15ae59f06928b665e807b552564014c3bfecf492a");
		BN_hex2bn(&dsa->pub_key,"15de2be9c64b6b5ab80a2c5337658b2e729dd112991ad5505eac63caf9f87c5c8a01290286a80cee89bb2084debe33721a7886560fe20d33583328e0b21440dbef1f0ff00c53c873d301d4dfee6cf1129520da9a99969c473f4129b06fc7ade31f61db3cda1c792c8409136fae4e5fddef931e46427161491341c5f5f01e31f3");
		BN_hex2bn(&dsa->priv_key,"1c8d7eeaf834310507c5fa384c13b60f9ec163ed");
*/
		dsa=NULL;
	}

CASignature::~CASignature()
	{
		DSA_free(dsa);
	}

int CASignature::setSignKey(char* buff,int len,int type)
	{
		if(buff==NULL||len<1||type!=SIGKEY_XML)
			return -1;
		if(type==SIGKEY_XML)
			return parseSignKeyXML(buff,len);
		return -1;
	}


//XML Decode...
static void sDSAKeyParamValueHandler(XML::Element &elem, void *userData)
{
	char buff[4096];
	int len=elem.ReadData(buff,4096);
	
	unsigned int decLen=4096;
	char decBuff[4096];
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

static void sDSAKeyValueHandler(XML::Element &elem, void *userData)
{
	XML::Handler handlers[] = {
	XML::Handler("P",sDSAKeyParamValueHandler),
	XML::Handler("Q",sDSAKeyParamValueHandler),
	XML::Handler("G",sDSAKeyParamValueHandler),
	XML::Handler("X",sDSAKeyParamValueHandler),
	XML::Handler("Y",sDSAKeyParamValueHandler),
	XML::Handler::END};
		elem.Process(handlers, userData);
}


static void sKeyValueHandler(XML::Element &elem, void *userData)
{
		XML::Handler handlers[] = {
		XML::Handler("DSAKeyValue",sDSAKeyValueHandler),
			XML::Handler::END};
		elem.Process(handlers, userData);
}

static void sKeyInfoHandler(XML::Element &elem, void *userData)
{
		XML::Handler handlers[] = {
		XML::Handler("KeyValue",sKeyValueHandler),
			XML::Handler::END};
		elem.Process(handlers, userData);
}

int CASignature::parseSignKeyXML(char* buff,int len)
{
	BufferInputStream oStream(buff,len);
	XML::Input input(oStream);

	// set up initial handler for Document
		XML::Handler handlers[] = {
		XML::Handler("KeyInfo",sKeyInfoHandler),
		XML::Handler::END
	};
	
	DSA* tmpDSA=DSA_new();
	try {
		input.Process(handlers, tmpDSA);
	}
	catch (const XML::ParseException &e)
	{
		printf("ERROR: %s (line %d, column %d)\n", e.What(), e.GetLine(), e.GetColumn());
	}
	if(DSA_sign_setup(tmpDSA,NULL,&tmpDSA->kinv,&tmpDSA->r)!=1)
		{
			DSA_free(tmpDSA);
			return -1;
		}
	dsa=tmpDSA;
	return 0;
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

const char *XMLSIGINFO_TEMPLATE=
"<SignedInfo>\n\t\t<Reference URI=\"\">\n\t\t<DigestValue>%s</DigestValue>\n\t</Reference>\n\t</SignedInfo>";
const char *XMLSIG_TEMPLATE=
"<Signature>\n\t%s\n<SignatureValue>%s</SignatureValue>\n</Signature>";

int CASignature::signXML(char* in,unsigned int inlen,char* out,unsigned int *outlen)
	{
		if(in==NULL||inlen<1||out==NULL||*outlen<inlen+getXMLSignatureSize())
			return -1;
		
		//Calculating the Digest...
		unsigned int len=*outlen;
		makeXMLCanonical(in,inlen,out,&len);
		unsigned char dgst[SHA_DIGEST_LENGTH];
		SHA1((unsigned char*)out,len,dgst);
		char tmpBuff[1024];
		len=1024;
		CABase64::encode((char*)dgst,SHA_DIGEST_LENGTH,tmpBuff,&len);
		tmpBuff[len]=0;

		//Creating the Sig-InfoBlock....
		sprintf(out,XMLSIGINFO_TEMPLATE,tmpBuff);
		
		// Signing the SignInfo block....
		len=1024;//*outlen;
		makeXMLCanonical(out,strlen(out),tmpBuff,&len);
		unsigned int sigSize=255;
		unsigned char sig[255];
		unsigned char* c=sig;
// tmp
//		tmpBuff[len]=0;
//		printf("CanSigInfo: %s\n",tmpBuff);
//		printf("CanSigInfoSize: %u\n",len);
//tmp end
		sign((unsigned char*)tmpBuff,len,sig,&sigSize);
		
		//Making Base64-Encode r and s
		STACK* a=NULL;
//		d2i_ASN1_SET(&a,&c,sigSize,(char *(__cdecl *)(void))d2i_ASN1_INTEGER,NULL,V_ASN1_SEQUENCE,V_ASN1_UNIVERSAL);
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

//tmp
//		printf("r: %s\n",BN_bn2dec(r));
//		printf("s: %s\n",BN_bn2dec(s));
//tmp-End
		memset(tmpBuff,0,40); //make first 40 bytes '0' --> if r or s is less then 20 bytes long! 
													//(Due to be compatible to the standarad r and s must be 20 bytes each) 
		BN_bn2bin(r,(unsigned char*)tmpBuff+20-BN_num_bytes(r)); //so r is 20 bytes with leading '0'...
		BN_bn2bin(s,(unsigned char*)tmpBuff+40-BN_num_bytes(s));
		sigSize=255;
		CABase64::encode(tmpBuff,40,(char*)sig,&sigSize);
		sig[sigSize]=0;

		//Makeing the hole Signature-Block....
		sprintf(tmpBuff,XMLSIG_TEMPLATE,out,sig);
		BN_free(r);
		BN_free(s);

		/* Find the last closing tag (</...>) and insert the <Signature> Element just before*/
		int pos=inlen-1;
		while(pos>=0&&in[pos]!='<')
			pos--;
		if(pos<0)
			return -1;
		*outlen=pos;
		memcpy(out,in,*outlen);
		memcpy(out+(*outlen),tmpBuff,strlen(tmpBuff));
		*outlen+=strlen(tmpBuff);
		memcpy(out+(*outlen),in+pos,inlen-pos);
		(*outlen)+=inlen-pos;
		return 0;
	}

int CASignature::getXMLSignatureSize()
	{
		return strlen(XMLSIG_TEMPLATE)+strlen(XMLSIGINFO_TEMPLATE)+/*size of DigestValue*/+20+/*sizeof SignatureValue*/+40;
	}

typedef struct
	{
		char* out;
		unsigned int outlen;
		unsigned int pos;
		int err;
	} XMLCanonicalHandlerData;

static void smakeXMLCanonicalDataHandler(const XML_Char *data, size_t len, void *userData)
	{
		XMLCanonicalHandlerData* pData=(XMLCanonicalHandlerData*)userData;
		if(pData->err!=0)
			return;
		char* buff=new char[len];
		len=memtrim(data,buff,len);
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

static void smakeXMLCanonicalElementHandler(XML::Element &elem, void *userData)
	{
		XMLCanonicalHandlerData* pData=(XMLCanonicalHandlerData*)userData;
		if(pData->err!=0)
			return;
		char* name=(char*)elem.GetName();
		int namelen=strlen(name);
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
				XML::Attribute attr=elem.GetAttrList();
				while(attr)
					{
						char* attrname=(char*)attr.GetName();
						char* attrvalue=(char*)attr.GetValue();
						pData->out[pData->pos++]=' ';
						int len=strlen(attrname);
						memcpy(pData->out+pData->pos,attrname,len);
						pData->pos+=len;
						pData->out[pData->pos++]='=';
						pData->out[pData->pos++]='\"';
						len=strlen(attrvalue);
						memcpy(pData->out+pData->pos,attrvalue,len);
						pData->pos+=len;
						pData->out[pData->pos++]='\"';
						attr=attr.GetNext();
					}
			}
		pData->out[pData->pos++]='>';
		//procces children...
		static XML::Handler handlers[] = {
		XML::Handler(smakeXMLCanonicalElementHandler),
		XML::Handler(smakeXMLCanonicalDataHandler),
		XML::Handler::END
		};
		elem.Process(handlers,userData);
		//closing tag...
		pData->out[pData->pos++]='<';
		pData->out[pData->pos++]='/';
		memcpy(pData->out+pData->pos,name,namelen);
		pData->pos+=namelen;
		pData->out[pData->pos++]='>';

	}

int CASignature::makeXMLCanonical(char* in,unsigned int inlen,char* out,unsigned int* outlen)
	{
		BufferInputStream oStream(in,inlen);
		XML::Input input(oStream);

	// set up initial handler for Document
		XML::Handler handlers[] = {
		XML::Handler(smakeXMLCanonicalElementHandler),
		XML::Handler(smakeXMLCanonicalDataHandler),
		XML::Handler::END
	};
	
	XMLCanonicalHandlerData oData;
	oData.out=out;
	oData.outlen=*outlen;
	oData.pos=0;
	oData.err=0;
	try {
		input.Process(handlers, &oData);
	}
	catch (const XML::ParseException &e)
	{
		printf("ERROR: %s (line %d, column %d)\n", e.What(), e.GetLine(), e.GetColumn());
	}
		*outlen=oData.pos;
		return oData.err;
	}