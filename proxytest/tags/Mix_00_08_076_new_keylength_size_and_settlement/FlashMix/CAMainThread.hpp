#ifndef __CAMAINTHREAD__
#define __CAMAINTHREAD__

#include "../CAThread.hpp"
#include "../CASocket.hpp"
#include "../CASocketAddrINet.hpp"
#include "../CAMsg.hpp"

class CAMainThread {
    public:
        CAMainThread(UINT32 a_uPort, THREAD_MAIN_TYP a_fncHandleRequest);
        ~CAMainThread();

        SINT32 start();
        SINT32 stop();
        SINT32 join();

    protected:
        UINT32 m_uPort;

    private:
        bool m_bRun;

        CAThread* m_pThread;
        THREAD_MAIN_TYP m_fncHandleRequest;

        static THREAD_RETURN thread(void* a_pThis);
};

#endif // __CAMAINTHREAD__
