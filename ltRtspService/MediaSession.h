#pragma once
#include "network.h"
#include "RtspString.h"
#include "StreamBuilder.h"
#include "RtpOverTcp.h"
/************************************************************************/
/*               媒体会话                                               */
/*	
/************************************************************************/
typedef long long INT64;
class MediaSessionList;
class MediaSession
{
public:
	
    static void Send(struct bufferevent*, void *arg);
    static void Recv(struct bufferevent*, void *arg);

    void DealRtsp(struct bufferevent* bev);
	void SetDescribe();
    static long long GenSessionID();
	explicit MediaSession(Network *pwork):work(pwork){};
	friend class MediaSessionList;
	DWORD tick;
	bool IsPlay();
protected:
	virtual ~MediaSession();
	rtsp_string rs;
private:
	Network *work;
    INT64 sessionid;
	std::string rtspinc;
	media_stream_ptr streamobj;
	MediaType type;
	rtsp_type stype;
	
};


//待完善 

class MediaSessionList
{
public:
	static MediaSessionList* GetInstance();
protected:
	bool SessionInsert();
	bool SessionDel();
	bool SessionGet();
	MediaSessionList();
	~MediaSessionList();
};