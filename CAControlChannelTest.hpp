#ifdef WITH_CONTROL_CHANNELS_TEST
#include "CASyncControlChannel.hpp"

/** A Control channel for testing if the control channels are working. 
	* This channel simply reflects every message it receives.
	*/
class CAControlChannelTest :
	public CASyncControlChannel
{
public:
	CAControlChannelTest(void);
	~CAControlChannelTest(void);

	/** Reflects the incoming message back to the sender*/
	SINT32 processXMLMessage(DOM_Document& doc)
	{
		return sendMessage(doc);
	}
};
#endif
