#include "../StdAfx.h"
#include "../CAUtil.hpp"
#include "../xml/DOM_Output.hpp"
TEST(XMLTests,ReadWriteDocument)
{
	initDOMParser();
	UINT32 inlen = 0;
	UINT8* inbuff=readFile((UINT8*)"xml/testdocument.xml", &inlen);
	ASSERT_TRUE(inbuff != NULL);
	if (inbuff == NULL)
		return;
	XERCES_CPP_NAMESPACE::DOMDocument *pDOC = parseDOMDocument(inbuff,inlen);
	UINT32 len = 0;
	UINT8 *buff = DOM_Output::dumpToMem(pDOC, &len);
	saveFile((UINT8*)"out.xml", buff,len);
	ASSERT_EQ(inlen, len);
	if (inlen==len)
		{
			for (UINT32 i=0;i<len;i++)
				{
					ASSERT_EQ(buff[i], inbuff[i]);
				}
		}
}