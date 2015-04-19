#include "StreamBuilder.h"
#include "MediaBuffer.h"
#include "HardwareIO.h"
class MediaSession;
class RtpOverTcp
{
public:
	RtpOverTcp();
	~RtpOverTcp();
	//buf_share_ptr GetTransNode(MediaSession* ses);
	static buf_share_ptr TCPForH264(media_stream_ptr st);
protected:
	
};