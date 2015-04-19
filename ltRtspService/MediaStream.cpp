#include "stdafx.h"
#include "MediaStream.h"


MediaStream::MediaStream()
{
	filepos = 0;
	OnStream = false;
	sqsnumber = 0;
	frame_count = 0;
	lsb_count = 0;
	pframe_count = 0;
	IsPlay = false;
}

MediaStream::~MediaStream()
{

}

void 
MediaStream::AddNode()
{

}

void 
MediaStream::DevNode(const Buffer& buf, unsigned pos)
{

}

void 
MediaStream::DevNode(buf_share_ptr buf, unsigned pos)
{

}
buf_share_ptr MediaStream::GetNode()
{
    buf_share_ptr p(new Buffer(0));
    return p;
}


const unsigned MediaStream::GetListLen()const
{
	return bufferlist.size();
}

