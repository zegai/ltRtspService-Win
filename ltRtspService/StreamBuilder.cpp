#include "stdafx.h"
#include "StreamBuilder.h"

media_stream_ptr 
MediaStreamBuild::CreateNew(const string& MediaExt, MediaType& Type)
{
	MediaStream* Builder = NULL;
	Type = UNDEFINED;
	if (!MediaExt.compare(".h264") || !MediaExt.compare(".H264"))
	{
		Builder =  new h264MediaStream();
		Type = H264;
		
	}


	media_stream_ptr p(Builder);
	return p;

}
