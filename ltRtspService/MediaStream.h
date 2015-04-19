#pragma once
#include "MediaBuffer.h"
#include <iostream>
#include <queue>

using namespace std;

class MediaStream
{
public:
	MediaStream();

	virtual void	DevNode(const Buffer& buf, unsigned pos);
	virtual void	DevNode(buf_share_ptr buf, unsigned pos);
	virtual buf_share_ptr GetNode();
	const unsigned GetListLen()const;
	virtual void	AddNode();

	virtual ~MediaStream();


	uint64_t filepos;	 //当前流传输对应的文件偏移值
	int64_t frame_count; //当前关键帧
	int64_t pframe_count;//当前帧
	int32_t ssrc;		 //每个媒体流对应的SSRC 待完善
	
	bool OnStream;		//用于判断时候处于播放状态
	bool IsPlay;
	
	unsigned short sqsnumber; //当前发送TCP帧的数量
	
	int fileindex;			  //对应文件索引
	int lsb_count;			  //暂时无用
	int fps;				  //帧率
	string filename;		  //文件名
	
	
	
	
	
	
	
	
protected:
	//媒体流缓冲队列
	queue<Buffer *> bufferlist;
	
};


