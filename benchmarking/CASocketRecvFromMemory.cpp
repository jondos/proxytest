#include "../StdAfx.h"
#include "CASocketRecvFromMemory.hpp"
#include "CASingleSocketGroupDummy.hpp"

SINT32 CASocketRecvFromMemory::receive(UINT8 *buff, UINT32 len)
{
//do something
	return E_SUCCESS;
}

SINT32 CASocketRecvFromMemory::setSocket(SOCKET s)
{
	delete m_pSingleSocketGroupRead;
	m_Socket = s;
	if (s != 0)
		{
			m_pSingleSocketGroupRead = new CASingleSocketGroupDummy(false);
			m_pSingleSocketGroupRead->add(m_Socket);
		}
	else
		{
			m_pSingleSocketGroupRead = NULL;
		}
	return E_SUCCESS;
}

SINT32 CASocketRecvFromMemory::create(SINT32 type, bool a_bShowTypicalError)
{
	setSocket(1);
	return E_SUCCESS;
}
