#ifndef __CAFLASHMIX__
#define __CAFLASHMIX__

#include "../CAThread.hpp"
#include "../CASocket.hpp"
#include "../CASocketAddrINet.hpp"
#include "../CAMsg.hpp"

#include "elgamal.hpp"
#include "CABNSend.hpp"
#include "FlashMIXGlobal.hpp"
#include "CAMainThread.hpp"

static const UINT16 ROLE_PERM_SRV = 0;
static const UINT16 ROLE_DECR_SRV = 1;

class CAFlashMIX : public CAMainThread
{
    public:
        CAFlashMIX(
                UINT8* a_uLocalIP,
                UINT32 a_uPort,
                UINT32 a_uPortKeyShare,
                UINT32 a_uPortDummy,
                UINT32 a_uPortBlind,
                UINT32 a_uPortBlindOK,
                UINT8* a_uBBIp,
                UINT32 a_uBBPort);
        ~CAFlashMIX();

        SINT32 init();
        SINT32 logout();

    private:
        UINT8* m_uLocalIP;
        UINT32 m_uPortKeyShare;
        UINT32 m_uPortDummy;
        UINT32 m_uPortBlind;
        UINT32 m_uPortDecrypt;
        UINT32 m_uKeyLength;

        UINT32 m_uMIXCnt;
        UINT32 m_uMIXPermCnt;
        UINT32 m_uMIXDecryptCnt;

        ELGAMAL* m_elSignKey;
        ELGAMAL* m_elEncKey;

        ELGAMAL* m_elBBSignKey;
        ELGAMAL* m_elBBEncKey;

        ELGAMAL* m_elGroupKey;

        CASocketAddrINet* m_pAddrBB;

        // MIX Data
        MIXDATA** m_vMIXData;
        CAMutex* m_pMutexMIXData;

        // fills the m_vMIXData-List
        SINT32 receiveMIXData(SINT32& ar_sResult, CASocket* a_pClient);

        // receives the F and s Values of all mixes
        SINT32 receiveMIXValues(SINT32& ar_sResult, BIGNUM*** r_bnAllF, BIGNUM*** r_bnAllSj, BIGNUM** a_bnF, BIGNUM** a_bnSi);
        // sends the F and s values of this mix
        SINT32 sendMIXValues(SINT32&ar_sResult, BIGNUM*** r_bnAllF, BIGNUM*** r_bnAllSj, BIGNUM** a_bnF, BIGNUM** a_bnSi);

        static THREAD_RETURN handleRequest(void* a_req);

        static SINT32 startKeyShare(CAFlashMIX* a_ths, CASocket* a_pClient);
        static SINT32 createDummies(CAFlashMIX* a_ths, CASocket* a_pClient, CASocket* a_pPrevMIX, CASocket* a_pNextMIX, UINT32 a_uSelfIdx, UINT32* a_uPermList);
        static SINT32 blind(UINT32* r_uInpListsSize, BIGNUM*** r_bnInpList, UINT32** r_uInpPermLists, BIGNUM*** r_bnExp, CAFlashMIX* a_ths, CASocket* a_pClient, CASocket* a_pPrevMIX, CASocket* a_pNextMIX, UINT32 a_uSelfIdx, UINT32* a_uPermList);
        static SINT32 mixProcess(CAFlashMIX* a_ths, CASocket* a_pClient);
        static SINT32 blindReceiveInpList(SINT32& ar_sResult, UINT32& r_uListSize, BIGNUM*** r_bnInpList, CAFlashMIX* a_ths, CASocket* a_pBBClient, CASocket* a_pPrevMIX, UINT32 a_uSelfIdx, UINT32* a_uPermList);
        static SINT32 blindSendInpList(SINT32& ar_sResult, CAFlashMIX* a_ths, CASocket* a_pBBClient, CASocket* a_pNextMIX, UINT32 a_uSelfIdx, UINT32* a_uPermList, UINT32 a_uListSize, BIGNUM** a_bnList);
        static SINT32 blindReenc(SINT32& ar_sResult, BIGNUM*** r_bnExp, BIGNUM** ar_bnList, ELGAMAL* a_elGroupKey, UINT32 a_uListSize);
        static SINT32 permutate(SINT32& ar_sResult, BIGNUM** ar_bnList, UINT32 a_uListSize, UINT32* a_uPerm);
        static SINT32 unblind(CAFlashMIX* a_ths, CASocket* a_pBB, CASocket* a_pPrevMIX, CASocket* a_pNextMIX, UINT32 a_uSelfIdx, UINT32* a_uPermList, UINT32* a_uInpListsSize, BIGNUM*** a_bnInpList, UINT32** a_uInpPerm, BIGNUM*** a_bnExp);
        static SINT32 unblindSendReceiveDummyData(SINT32& ar_sResult, UINT32* ar_uDummies, CASocket* a_pPrevMIX, CASocket* a_pNextMIX, UINT32 a_uSelfIdx, UINT32 MIXCNT, UINT32 LISTCNT, UINT32 DUMMYCNT, UINT32 REENCCNT, UINT32* a_uInpPermSize, UINT32** a_uInpPerm, UINT32 a_rReEnc);
        static SINT32 unblindSendReceiveDummyExp2ReEnc(SINT32& ar_sResult, BIGNUM*** r_bnExp2ReEnc, CASocket* a_pPrevMIX, CASocket* a_pNextMIX, UINT32 a_uSelfIdx, BIGNUM*** a_bnExp, UINT32* a_uDummyPerm, UINT32 MIXCNT, UINT32 LISTCNT, UINT32 DUMMYCNT, UINT32 REENCCNT, UINT32 explistsize);
        static SINT32 switchEncListElements(UINT32 a_uListSize, BIGNUM** ar_bnList, UINT32 a_uMsgIdx1, UINT32 a_uMsgIdx2);
        static SINT32 unblindCheck2Dummy(SINT32& ar_sResult, CAFlashMIX* a_ths, BIGNUM** a_bnL0R0, BIGNUM** a_bnL1R0, BIGNUM** a_bnL0R1, BIGNUM** a_bnL1R1, UINT32* a_uDummyPerm, BIGNUM** a_bnDummyExp, UINT32 MIXCNT, UINT32 MIXFAC, UINT32 LISTFAC, UINT32 DUMMYFAC, UINT32 REENCFAC);
        static SINT32 unblindCheckProd(SINT32& ar_sResult, CAFlashMIX* a_ths, CASocket* a_pPrevMIX, CASocket* a_pNextMIX, UINT32 a_uSelfIdx, UINT32* a_uInpListsSize, BIGNUM** a_bnInpFirstReEncList, BIGNUM** a_bnInpSecondReEncList, BIGNUM*** a_bnOutLists, BIGNUM*** a_bnExp, UINT32 MIXCNT, UINT32 LISTCNT, UINT32 DUMMYCNT, UINT32 REENCCNT);
        static SINT32 unblindCheckFirstReEnc(SINT32& ar_sResult, CAFlashMIX* a_ths, CASocket* a_pPrevMIX, CASocket* a_pNextMIX, UINT32 a_uSelfIdx, UINT32 a_uInpListSize, BIGNUM** a_bnInpList, BIGNUM** a_bnFistList, BIGNUM** a_bnSecondList, UINT32** a_uInpPerm, BIGNUM*** a_bnExp, UINT32 MIXCNT);
        static SINT32 unblindFirstReEncPermData(SINT32& ar_sResult, UINT32** r_uPermList, CASocket* a_pPrevMIX, CASocket* a_pNextMIX, UINT32 a_uSelfIdx, UINT32 LISTSIZE, UINT32** a_uInpPerm, UINT32 MIXCNT);
        static SINT32 unblindFirstReEncExpList(SINT32& ar_sResult, BIGNUM*** r_bnExpList, CASocket* a_pPrevMix, CASocket* a_pNextMIX, UINT32 a_uSelfIdx, UINT32 LISTSIZE, BIGNUM*** a_bnExp, UINT32 MIXCNT);
        static SINT32 unblindPermutate(SINT32& ar_sResult, BIGNUM** ar_bnList, UINT32 a_uListSize, UINT32* a_uPerm);
        static SINT32 unblindCalcAkkPerm(SINT32& ar_sResult, UINT32** r_uPerm0, UINT32** r_uPerm1, UINT32* a_uPermList, UINT32 MIXCNT, UINT32 LISTSIZE);

        static SINT32 decrypt(CAFlashMIX* a_ths, CASocket* a_pBB);
        static SINT32 decryptConnectMIX(CASocket** r_pPrevMIX, CASocket** r_pNextMIX, CAFlashMIX* a_ths, UINT32 a_uSelfIdx, UINT32* a_uDecryptList);
        static SINT32 decryptSendMessage(SINT32& ar_sResult, BIGNUM* a_bnMsg);

        static SINT32 unblindCalcAkk(SINT32& ar_sResult, UINT32** r_uPerm0, UINT32** r_uPerm1, UINT32 LISTSIZE, UINT32* a_uPermList, UINT32 MIXCNT);
        static SINT32 unblindGetFirstL1NotInL2(SINT32& ar_sResult, UINT32& r_uElement, UINT32 a_uList1Size, UINT32* a_uList1, UINT32 a_uList2Size, UINT32* a_uList2);
        static SINT32 unblindPermSetPtr(UINT32& ar_uPtrL, UINT32& ar_uPtrH, UINT32 a_uListSize, UINT32* a_uPerm);
        static SINT32 unblindPermutateMsg(SINT32& ar_sResult, BIGNUM** ar_bnList, UINT32 a_uListSize, UINT32* a_uPerm);
};

#endif // __CAFLASHMIX__
