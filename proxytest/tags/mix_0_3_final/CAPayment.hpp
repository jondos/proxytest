#ifdef PAYMENT_SUPPORT

class CAPayment
	{
		public:
			CAPayment(){m_mysqlCon=NULL;}
			SINT32 init(UINT8* host,SINT32 port,UINT8* user,UINT8* passwd);
			SINT32 checkAccess(UINT8* code,UINT8 codeLen,UINT32* accessUntil);
		private:
			MYSQL* m_mysqlCon;
	};
#endif