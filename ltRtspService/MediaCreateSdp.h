#pragma once
#include <iostream>
#include <map>
#include "StreamBuilder.h"
#include "bit/h264_sps.h"
using namespace std;

class MediaCreateSdp
{
public:
	static MediaCreateSdp* GetInstance(const string& path);
	bool GetSdp(const string& filename, string* ssdp);
protected:
	static bool H264CreateSdp(const Buffer& buf, media_stream_ptr SdpMedia, string& SdpTemp);
	MediaCreateSdp(const string& path);
	~MediaCreateSdp();
private:
	map<string*, string*> SdpList;
};

