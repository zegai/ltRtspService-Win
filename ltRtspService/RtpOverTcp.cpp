#include "stdafx.h"
#include "RtpOverTcp.h"



buf_share_ptr 
RtpOverTcp::TCPForH264(media_stream_ptr st)
{
	assert(st);
	h264MediaStream* Media = dynamic_cast<h264MediaStream *>(st.get());
	buf_share_ptr node = Media->GetNode();
	if (node->GetMtuValue() == 0)
	{
		//ReadFile
		buf_share_ptr bufget = HardwareIO::GetInstance()->GetBufferFormFile(st->fileindex, st->filepos, 10240);
		if (bufget->GetMtuValue() == 10240)
		{
			//printf("st.pos: %d\n", st->filepos);
			//获取数据之后继续分解成NAL单元
			Media->DevNode(bufget, 0);
			node = Media->GetNode();
		}
		else
		{
			//最后一次readfile
			printf("at the end\n");

		}
	}
	uint8_t ftype = h264MediaStream::GetNalType(node); 
	
	do 
	{
		//SEI和PPS不做处理，PPS已通过SDP发送给VLC
		if (ftype == 6 || ftype == 8)
		{
			buf_share_ptr ret(new Buffer(0));
			return ret;
		}
		if (ftype == 7)
		{
			//SPS
			if (Media->StreamSps == NULL)
			{
				buf_share_ptr sps = Buffer::CreateBuf(node->GetSizeValue());
				sps->FullBuffer(node, 0, node->GetSizeValue());
				//填充SPS结构体
				Media->ParseSqs(sps);
			}
			
			buf_share_ptr ret(new Buffer(0));
			//保存每秒的帧数
			Media->fps = Media->StreamSps->num_units_in_tick / Media->StreamSps->time_scale;
			return ret;
		}
		//用于RTP的M位
		bool IsFinal = false;

		if(node->GetByte(0) == 0)
		{
			//SINGLE NAL 不需要分片 所以M位为1
			//h264_slice_t* slice = new h264_slice_t;
			//分析当前片信息 将会保存至Media的CurSlice中
			Media->ParseSlice(const_cast<unsigned char *>(node->GetBuffer()) + RTP_HEADSIZE + 1,
				(node->GetSizeValue() - RTP_HEADSIZE > 64?64:node->GetSizeValue()), h264MediaStream::GetNalType(node));
			//当前帧数+1
			Media->pframe_count ++;
			
			IsFinal = true;
		}
		else if (node->GetByte(0) == 1)
		{
			//FU-A 被分片的数据
			//
			if (node->GetByte(1) == 1)
			{
				//FU-A的头   S E R TYPE 1 0 0 28
				//由于后续节点中的头个字节不再包含NAL类型，所以需要在这里分析片信息同时保存
				Media->CurNalType = h264MediaStream::GetNalType(node);
				//
				Media->ParseSlice(const_cast<unsigned char *>(node->GetBuffer()) + RTP_HEADSIZE + 2, 64,
					Media->CurNalType);

				//这里组织FU-indicator和FU-header
				node->SetByte(0x00 | node->GetByte(2)&0x60 | 28, RTP_HEADSIZE);
				node->SetByte(0x80 | Media->CurNalType, RTP_HEADSIZE + 1);
				//只有在第一个FU-A帧数才会被+1
				Media->pframe_count ++;
			}
			else if (node->GetByte(1) == 0)
			{
				//FU-A结尾 需要设置M值为1
				node->SetByte(0x00 | node->GetByte(2)&0x60 | 28, RTP_HEADSIZE);
				//S E R 0 1 0
				node->SetByte(0x40 | Media->CurNalType, RTP_HEADSIZE + 1);
				//printf("FIND A FU-A END Type = %d Size = %d\n", Media->CurNalType, node->GetSizeValue());
				IsFinal = true;
			}
			else
			{
				//fu-a中间节点
				node->SetByte(0x00 | node->GetByte(2)&0x60 | 28, RTP_HEADSIZE);
				//S E R 0 0 0
				node->SetByte(Media->CurNalType, RTP_HEADSIZE + 1);
				//printf("FIND A FU-A Mid Type = %d  Size = %d\n", Media->CurNalType, node->GetSizeValue());
			}
		}
		//RTP OVER TCP的头4个字节 $ + 信道0（数据信道）+ 后续RTP数据包的长度
		node->SetByte('$', 0);
		node->SetByte(0x00, 1);
		unsigned short size = node->GetSizeValue() - 4;
		node->SetByte((size >> 8)&0xff, 2);
		node->SetByte(size & 0xff, 3);
		
		//h264要求
		node->SetByte(0x80, 4);
		//设置M值同属与上96，这里认为96代表H264的RTP负载类型
		node->SetByte((IsFinal?0x80:0x00)|96,5);
		//序列
		node->SetByte((Media->sqsnumber >> 8)&0xff, 6);
		node->SetByte((Media->sqsnumber&0xff), 7);
		//每个TCP帧 序列自增
		Media->sqsnumber ++;
		//计算时间戳
		int timestamp;
		//同一关键帧内的frame_count为固定值，每次遇到关键帧时才刷新frame_count的值。这里主要用于FU-A的frame_count更新问题。
		bool framefinal = false;
		if (Media->CurSlice->i_slice_type == 2 || Media->CurSlice->i_slice_type == 7 ||
			Media->CurSlice->i_slice_type == 4 || Media->CurSlice->i_slice_type == 9 )
		{
			//计算关键帧时间戳
			timestamp = Media->frame_count*90000*2*Media->StreamSps->num_units_in_tick / Media->StreamSps->time_scale;
			//printf("I Frame timestamp = %u Frame Count= %u\n", timestamp, Media->frame_count);
			framefinal = true;
		}
		else
		{
			//同一关键帧内，根据P B帧的顺序计算时间戳
			timestamp = (Media->frame_count*2 + Media->CurSlice->i_pic_order_cnt_lsb)*90000*Media->StreamSps->num_units_in_tick / Media->StreamSps->time_scale;
			//printf("P OR B Frame lsb = %d %d %d\n", Media->CurSlice->i_pic_order_cnt_lsb, Media->frame_count,timestamp);
		}
		if (framefinal)
		{
			//关键帧刷新frame_count
			Media->frame_count = Media->pframe_count;
		}
		//
		node->SetByte(timestamp >> 24 & 0xff, 8);
		node->SetByte(timestamp >> 16 & 0xff, 9);
		node->SetByte(timestamp >> 8  & 0xff, 10);
		node->SetByte(timestamp & 0xff, 11);
		//printf("time Stamp = %d %d %d\n", timestamp, Media->CurSlice->i_pic_order_cnt_lsb, Media->frame_count);
		//设置SSRC
		node->SetByte(Media->ssrc >> 24 & 0xff, 12);
		node->SetByte(Media->ssrc >> 16 & 0xff, 13);
		node->SetByte(Media->ssrc >> 8  & 0xff, 14);
		node->SetByte(Media->ssrc & 0xff, 15);
	} while (0);
	return node;
}