#include "../StdAfx.h"
#include "CAMainThread.hpp"

CAMainThread::CAMainThread(UINT32 a_uPort, THREAD_MAIN_TYP a_fncHandleRequest)
{
    m_uPort = a_uPort;
    m_fncHandleRequest = a_fncHandleRequest;
    m_bRun = false;

    if ((m_pThread = new CAThread()) != NULL)
        m_pThread->setMainLoop(thread);
}

CAMainThread::~CAMainThread()
{
    stop();
    join();

    delete m_pThread;
}

SINT32 CAMainThread::start()
{
    SINT32 result = E_SUCCESS;

    m_bRun = true;
    result = m_pThread->start(this);

    return result;
}

SINT32 CAMainThread::stop()
{
    SINT32 result = E_SUCCESS;

    CASocket* pSocket = new CASocket();
    CASocketAddrINet* pAddr = new CASocketAddrINet();

    m_bRun = false;
    if ((pSocket == NULL) || (pAddr == NULL))
        result = E_UNKNOWN;
    else
        result = pSocket->create();
    if (result == E_SUCCESS)
        result = pAddr->setAddr((UINT8*)"127.0.0.1", m_uPort);

    if (result == E_SUCCESS)
        pSocket->connect(*pAddr, 1, 1);

    if (pSocket != NULL) pSocket->close();
    delete pSocket;
    delete pAddr;

    return result;
}

SINT32 CAMainThread::join()
{
    return m_pThread->join();
}

THREAD_RETURN CAMainThread::thread(void* a_pThis)
{
    SINT32 result = E_SUCCESS;
    CAMainThread* ths = (CAMainThread*)a_pThis;
    CASocket* pSocket = new CASocket();

    if (pSocket == NULL)
        result = E_UNKNOWN;
    else
        result = pSocket->create();
    if (result == E_SUCCESS)
        result = pSocket->listen(ths->m_uPort);

    CAMsg::printMsg(LOG_DEBUG, "MainThread startet\n");
    while ((ths->m_bRun) && (result == E_SUCCESS))
    {
        CASocket* pClient = new CASocket();

        if (pClient == NULL)
            result = E_UNKNOWN;
        else
            result = pSocket->accept(*pClient);

        if ((ths->m_bRun) && (result == E_SUCCESS))
        {
            pthread_t pThread;
            UINT32* req = new UINT32[2];
            if (req == NULL)
                result = E_UNKNOWN;
            else
            {
                req[0] = (UINT32)ths;
                req[1] = (UINT32)pClient;
            }

            if (result == E_SUCCESS)
                pthread_create(&pThread, NULL, ths->m_fncHandleRequest, req);
        }
        else
        {
            if (pClient != NULL) pClient->close();
            delete pClient;
        }
    }
    CAMsg::printMsg(LOG_DEBUG, "MainThread finished\n");

    if (pSocket != NULL) pSocket->close();
    delete pSocket;

    THREAD_RETURN_SUCCESS;
}
