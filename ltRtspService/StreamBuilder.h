#pragma once

#include "h264MediaStream.h"


typedef boost::shared_ptr<h264MediaStream> h264_stream_ptr;
typedef boost::shared_ptr<MediaStream>	   media_stream_ptr;

enum MediaType
{
	H264,
	AAC,
	UNDEFINED,
};

class MediaStreamBuild
{
public:
	static media_stream_ptr CreateNew(const string& MediaExt, MediaType& Type);
protected:
	~MediaStreamBuild();
private:

};