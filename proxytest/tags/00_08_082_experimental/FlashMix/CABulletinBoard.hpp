#ifndef __CABULLETINBOARD__
#define __CABULLETINBOARD__

#include "../CAThread.hpp"
#include "../CASocket.hpp"
#include "../CASocketAddrINet.hpp"
#include "../CAMutex.hpp"
#include "../CAMsg.hpp"

#include "uBubbleSort.hpp"
#include "elgamal.hpp"
#include "CABNSend.hpp"
#include "FlashMIXGlobal.hpp"
#include "CAMainThread.hpp"

class CABulletinBoard : public CAMainThread
{
    public:
        CABulletinBoard(
                UINT32 a_uPort,
                UINT32 a_uKeyLength,
                UINT32 a_uMIXCnt,
                UINT32 a_uMIXPermCnt,
                UINT32 a_uMIXDecryptCnt,
                UINT32 a_uAnonCnt);
        ~CABulletinBoard();

        SINT32 init();

    private:
        UINT32 m_uKeyLength;

        UINT32 m_uMIXCnt;
        UINT32 m_uMIXPermCnt;
        UINT32 m_uMIXDecryptCnt;

        ELGAMAL* m_elSignKey;
        ELGAMAL* m_elEncKey;

        ELGAMAL* m_elGroupKey;

        // MIX Data
        MIXDATA** m_vMIXData;
        CAMutex* m_pMutexMIXData;

        // user messages
        BIGNUM** m_vBNUserMsg;
        UINT32 m_uAnonCnt;          // size of anonymity-set
        UINT32 m_uAnonListSize;     // size of the msg-list
        UINT32 m_uUserMsgListPtr;   // pointer to next free place
        CAMutex* m_pMutexUserMsg;   // the mutex for the array

        SINT32 insertToUserList(BIGNUM* a_bnA, BIGNUM* a_bnB);
        SINT32 insertToUserList(SINT32& ar_sResult, BIGNUM* a_bnA, BIGNUM* a_bnB);
        SINT32 insertDummy(BIGNUM** ar_bnList, BIGNUM* a_bnDummyA, BIGNUM* a_bnDummyB);
        SINT32 insertDummy(SINT32& ar_sResult, BIGNUM** ar_bnList, BIGNUM* a_bnDummyA, BIGNUM* a_bnDummyB);

        static THREAD_RETURN handleRequest(void* a_req);
        static THREAD_RETURN keyShareing(void* a_thisPtr);
        static THREAD_RETURN mixProcess(void* a_thisPtr);

        static SINT32 initMix(CABulletinBoard* a_ths, CASocket* a_pClient);
        static SINT32 removeMix(CABulletinBoard* a_ths, CASocket* a_pClient);
        static SINT32 startKeyShareing(CABulletinBoard* a_ths);
        static SINT32 receiveUserData(CABulletinBoard* a_ths, CASocket* a_pClient);
        static SINT32 sendSignKey(CABulletinBoard* a_ths, CASocket* a_pClient);
        static SINT32 sendEncKey(CABulletinBoard* a_ths, CASocket* a_pClient);
        static SINT32 sendGroupKey(CABulletinBoard* a_ths, CASocket* a_pClient);
        static SINT32 createDummies(BIGNUM*** r_bnDummyList, CABulletinBoard* a_ths, CASocket** a_pSockets, UINT32* a_uPermList, UINT32 DUMMYCNT = 2);
        static SINT32 blind(BIGNUM*** r_bnFirstListFirstReenc, BIGNUM*** r_bnSecondListFirstReenc, BIGNUM*** r_bnFirstListSecondReenc, BIGNUM*** r_bnSecondListSecondReenc, CABulletinBoard* a_ths, CASocket** a_pSockets, UINT32* a_uPermList, BIGNUM** a_bnInputList);
};

#endif // __CABULLETINBOARD__
