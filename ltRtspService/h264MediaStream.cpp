#include "stdafx.h"
#include "h264MediaStream.h"
#include "BaseEncoder.h"

h264MediaStream::h264MediaStream()
{
	DevCount = 0;
	StreamSps = NULL;
	CurSlice = NULL;
	CurNalType = NULL;
}

h264MediaStream::~h264MediaStream()
{
	printf("Release H264 Buffer\n");
	delete StreamSps;
}

bool 
h264MediaStream::h264_decode_hrd_parameters(bs_t& s, h264_sps_t* p_sps)
{
	int cpb_count, i;
	cpb_count = bs_read_ue(&s) + 1;
	bs_read(&s, 4); /* bit_rate_scale */
	bs_read(&s, 4); /* cpb_size_scale */
	for(i=0; i<cpb_count; i++){
		bs_read_ue(&s); /* bit_rate_value_minus1 */
		bs_read_ue(&s); /* cpb_size_value_minus1 */
		bs_read(&s, 1);     /* cbr_flag */
	}
	bs_read(&s, 5); /* initial_cpb_removal_delay_length_minus1 */
	bs_read(&s, 5); /* cpb_removal_delay_length_minus1 */
	bs_read(&s, 5); /* dpb_output_delay_length_minus1 */
	bs_read(&s, 5); /* time_offset_length */
	return true;
}

bool 
h264MediaStream::ParseSqs(const buf_share_ptr SqsNode)
{
	assert(SqsNode);

	if (StreamSps == NULL)
		StreamSps = new h264_sps_t;
	uint8_t *pb_dec = NULL;
	int     i_dec = 0;
	bs_t s;
	int i_sps_id;
	i_dec = SqsNode->GetSizeValue(); 
	int nal_hrd_parameters_present_flag, vcl_hrd_parameters_present_flag;
	pb_dec = const_cast<uint8_t *>(SqsNode->GetBuffer());

	//转化00 00 03为00 00


	uint8_t *p_nal = new uint8_t[i_dec];
	int pcount = 0;
	for (int tmp = 1; tmp < i_dec ; tmp++)
	{
		if (tmp < i_dec - 3 && *(pb_dec + tmp) == 0x00 && 
			*(pb_dec + tmp + 1) == 0x00 && *(pb_dec + tmp + 2) == 0x03)
		{
			p_nal[pcount++] = 0x00;
			p_nal[pcount++] = 0x00;
			//printf("count = %d %2X\n",tmp, *(p_nal + tmp));
			tmp += 2;
			continue;
		}
		p_nal[pcount++] = *(pb_dec + tmp);
	}
	pb_dec = p_nal;

	//以下内容根据H264协议解读，属于直接复制粘贴代码 同parseslice

	bs_init( &s, pb_dec, i_dec );
	// profile(8)
	StreamSps->profile_idc = bs_read( &s, 8);

	/* constraint_set012, reserver(5), level(8) */
	bs_skip( &s, 1+1+1 + 5 + 8 );
	/* sps id */
	i_sps_id = bs_read_ue( &s );
	if( i_sps_id >= 32/*SPS_MAX*/ )
	{
		printf("invalid SPS (sps_id=%d)", i_sps_id );
		delete pb_dec;
		return false;
	}

	StreamSps->scaling_matrix_present = 0;
	if(StreamSps->profile_idc >= 100)		//high profile
	{ 
		if(bs_read_ue(&s) == 3)			//chroma_format_idc
			bs_read(&s, 1);				//residual_color_transform_flag
		bs_read_ue(&s);					//bit_depth_luma_minus8
		bs_read_ue(&s);					//bit_depth_chroma_minus8
		StreamSps->transform_bypass = bs_read(&s, 1);
		bs_skip(&s, 1); //decode_scaling_matrices(h, sps, NULL, 1, sps->scaling_matrix4, sps->scaling_matrix8);
	}

	/* Skip i_log2_max_frame_num */
	StreamSps->log2_max_frame_num = bs_read_ue( &s );
	if( StreamSps->log2_max_frame_num > 12)
		StreamSps->log2_max_frame_num = 12;
	/* Read poc_type */
	StreamSps->poc_type/*->i_pic_order_cnt_type*/ = bs_read_ue( &s );
	if( StreamSps->poc_type == 0 )
	{
		/* skip i_log2_max_poc_lsb */
		StreamSps->log2_max_poc_lsb/*->i_log2_max_pic_order_cnt_lsb*/ = bs_read_ue( &s );
		if( StreamSps->log2_max_poc_lsb > 12 )
			StreamSps->log2_max_poc_lsb = 12;
	}
	else if( StreamSps->poc_type/*p_sys->i_pic_order_cnt_type*/ == 1 )
	{
		int i_cycle;
		/* skip b_delta_pic_order_always_zero */
		StreamSps->delta_pic_order_always_zero_flag/*->i_delta_pic_order_always_zero_flag*/ = bs_read( &s, 1 );
		/* skip i_offset_for_non_ref_pic */
		bs_read_se( &s );
		/* skip i_offset_for_top_to_bottom_field */
		bs_read_se( &s );
		/* read i_num_ref_frames_in_poc_cycle */
		i_cycle = bs_read_ue( &s );
		if( i_cycle > 256 ) i_cycle = 256;
		while( i_cycle > 0 )
		{
			/* skip i_offset_for_ref_frame */
			bs_read_se(&s );
			i_cycle--;
		}
	}
	/* i_num_ref_frames */
	bs_read_ue( &s );
	/* b_gaps_in_frame_num_value_allowed */
	bs_skip( &s, 1 );

	/* Read size */
	StreamSps->mb_width/*->fmt_out.video.i_width*/  = 16 * ( bs_read_ue( &s ) + 1 );
	StreamSps->mb_height/*fmt_out.video.i_height*/ = 16 * ( bs_read_ue( &s ) + 1 );

	/* b_frame_mbs_only */
	StreamSps->frame_mbs_only_flag/*->b_frame_mbs_only*/ = bs_read( &s, 1 );
	if( StreamSps->frame_mbs_only_flag == 0 )
	{
		bs_skip( &s, 1 );
	}
	/* b_direct8x8_inference */
	bs_skip( &s, 1 );

	/* crop */
	StreamSps->crop = bs_read( &s, 1 );
	if( StreamSps->crop )
	{
		/* left */
		bs_read_ue( &s );
		/* right */
		bs_read_ue( &s );
		/* top */
		bs_read_ue( &s );
		/* bottom */
		bs_read_ue( &s );
	}

	/* vui */
	StreamSps->vui_parameters_present_flag = bs_read( &s, 1 );
	if( StreamSps->vui_parameters_present_flag )
	{
		int aspect_ratio_info_present_flag = bs_read( &s, 1 );
		if( aspect_ratio_info_present_flag )
		{
			static const struct { int num, den; } sar[17] =
			{
				{ 0,   0 }, { 1,   1 }, { 12, 11 }, { 10, 11 },
				{ 16, 11 }, { 40, 33 }, { 24, 11 }, { 20, 11 },
				{ 32, 11 }, { 80, 33 }, { 18, 11 }, { 15, 11 },
				{ 64, 33 }, { 160,99 }, {  4,  3 }, {  3,  2 },
				{  2,  1 },
			};

			int i_sar = bs_read( &s, 8 );

			if( i_sar < 17 )
			{
				StreamSps->sar.num = sar[i_sar].num;
				StreamSps->sar.den = sar[i_sar].den;
			}
			else if( i_sar == 255 )
			{
				StreamSps->sar.num = bs_read( &s, 16 );
				StreamSps->sar.den = bs_read( &s, 16 );
			}
			else
			{
				StreamSps->sar.num = 0;
				StreamSps->sar.den = 0;
			}

			//if( den != 0 )
			//	p_dec->fmt_out.video.i_aspect = (int64_t)VOUT_ASPECT_FACTOR *
			//	( num * p_dec->fmt_out.video.i_width ) /
			//	( den * p_dec->fmt_out.video.i_height);
			//else
			//	p_dec->fmt_out.video.i_aspect = VOUT_ASPECT_FACTOR;
		}
		else
		{
			StreamSps->sar.num = 0;
			StreamSps->sar.den = 0;
		}

		if(bs_read(&s, 1))		/* overscan_info_present_flag */
		{
			bs_read(&s, 1);     /* overscan_appropriate_flag */
		}

		if(bs_read(&s, 1))		/* video_signal_type_present_flag */
		{      
			bs_read(&s, 3);		/* video_format */
			bs_read(&s, 1);     /* video_full_range_flag */

			if(bs_read(&s, 1))  /* colour_description_present_flag */
			{
				bs_read(&s, 8);	/* colour_primaries */
				bs_read(&s, 8); /* transfer_characteristics */
				bs_read(&s, 8); /* matrix_coefficients */
			}
		}

		if(bs_read(&s, 1))		/* chroma_location_info_present_flag */
		{
			bs_read_ue(&s);		/* chroma_sample_location_type_top_field */
			bs_read_ue(&s);		/* chroma_sample_location_type_bottom_field */
		}

		StreamSps->timing_info_present_flag = bs_read(&s, 1);
		if(StreamSps->timing_info_present_flag)
		{
			StreamSps->num_units_in_tick = bs_read(&s, 32);
			StreamSps->time_scale = bs_read(&s, 32);
			StreamSps->fixed_frame_rate_flag = bs_read(&s, 1);
		}

		nal_hrd_parameters_present_flag = bs_read(&s, 1);
		if(nal_hrd_parameters_present_flag)
			h264_decode_hrd_parameters(s, StreamSps);
		vcl_hrd_parameters_present_flag = bs_read(&s, 1);
		if(vcl_hrd_parameters_present_flag)
			h264_decode_hrd_parameters(s, StreamSps);
		if(nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag)
			bs_read(&s, 1);     /* low_delay_hrd_flag */
		bs_read(&s, 1);         /* pic_struct_present_flag */

		StreamSps->bitstream_restriction_flag = bs_read(&s, 1);
		if(StreamSps->bitstream_restriction_flag)
		{
			unsigned int num_reorder_frames;
			bs_read(&s, 1);     /* motion_vectors_over_pic_boundaries_flag */
			bs_read_ue(&s); /* max_bytes_per_pic_denom */
			bs_read_ue(&s); /* max_bits_per_mb_denom */
			bs_read_ue(&s); /* log2_max_mv_length_horizontal */
			bs_read_ue(&s); /* log2_max_mv_length_vertical */
			num_reorder_frames= bs_read_ue(&s);
			bs_read_ue(&s); /*max_dec_frame_buffering*/

			if(num_reorder_frames > 16 /*max_dec_frame_buffering || max_dec_frame_buffering > 16*/){
				printf("illegal num_reorder_frames %d\n", num_reorder_frames);
				delete pb_dec;
				return true;
			}

			StreamSps->num_reorder_frames= num_reorder_frames;
		}
	}
	delete pb_dec;
	return true;
}



void 
h264MediaStream::ParseSlice(uint8_t* p_nal,  int n_nal_size, int i_nal_type)
{	
	
	if(CurSlice == NULL)
		CurSlice = new h264_slice_t;
	//转化00 00 03为00 00
	int icount = 0;
	uint8_t p_slice[64] = {0};
	int parcount = (n_nal_size>64?64:n_nal_size);
	for (int t = 0; t < parcount; t++)
	{
		if (t < parcount-2 && p_nal[t] == 0x00 && p_nal[t + 1] == 0x00 && p_nal[t + 2] == 0x03)
		{
			p_slice[icount++] = 0x00;
			p_slice[icount++] = 0x00;
			t += 2;
			continue;
		}
		p_slice[icount++] = p_nal[t];
	}
	
	


	bs_t s;
	bs_init(&s, p_slice, icount);

	bs_read_ue( &s );	// first_mb_in_slice
	CurSlice->i_slice_type = bs_read_ue( &s );	// slice type

	int i_pic_parameter_set_id = bs_read_ue( &s );
	CurSlice->i_frame_num = bs_read( &s, StreamSps->log2_max_frame_num + 4 );

	int i_field_pic_flag = 0;
	int i_bottom_field_flag = -1;
	if( !StreamSps->frame_mbs_only_flag)
	{
		/* field_pic_flag */
		i_field_pic_flag = bs_read( &s, 1 );
		if( i_field_pic_flag )
			i_bottom_field_flag = bs_read( &s, 1 );
	}

	int i_idr_pic_id;
	if( i_nal_type == 5/*NAL_SLICE_IDR*/ )
		i_idr_pic_id = bs_read_ue( &s );

	int i_delta_pic_order_cnt_bottom = -1;
	int i_delta_pic_order_cnt0 = 0;
	int i_delta_pic_order_cnt1 = 0;

	CurSlice->i_pic_order_cnt_lsb = 0;

	if( StreamSps->poc_type == 0 )
	{
		CurSlice->i_pic_order_cnt_lsb = bs_read( &s, StreamSps->log2_max_poc_lsb + 4 );
		//if( g_pic_order_present_flag && !i_field_pic_flag )
		//	i_delta_pic_order_cnt_bottom = bs_read_se( &s );
	}
	else if( (StreamSps->poc_type == 1) &&
		(!StreamSps->delta_pic_order_always_zero_flag) )
	{
		i_delta_pic_order_cnt0 = bs_read_se( &s );
		//if( g_pic_order_present_flag && !i_field_pic_flag )
		//	i_delta_pic_order_cnt1 = bs_read_se( &s );
	}
}



//分解单元规则
//SPS PPS SEI不参与传输
//如果单帧能够结束的SINGLE NAL
//RTP_HEADSIZE + DATA ，标记为头一个字节为0， 即分解后得到node[0] = 0 代表这是个SINGLE NAL使用STAP-A封装
//如果单帧比MTU大的，需要分片，那么头一个字节为1，第二个字节 为1代表这一长帧的头，0为尾，
//其它为次序，第三个字节为NAL的头。
//RTP_HEADSIZE + FU-A + DATA,
//分解后得到
//node[0] = 1，代表FU-A，
//node[1] = 1代表FU-A第一个数据包, 2-~代表FU-A中间数据包,0代表FU-A尾数据包
//node[2] = NAL HEAD



void
h264MediaStream::DevLargeNode(const Buffer& buf, unsigned pos, int length, int dpage)
{	
	unsigned sta = pos + length;
	const unsigned char* pdata = buf.GetBuffer();
	Buffer RtpHead(RTP_HEADSIZE);
	bool IsFU_A = dpage > 0?true:false;
	unsigned char fu[2] = {0};
	unsigned char nal_type = pdata[pos];
	while (sta - pos > NETMTU - RTP_HEADSIZE - 2)
	{
		//数据大于MTU-RTP_HEADSIZE，需要分包
		IsFU_A = true;
		DevCount ++;
		Buffer *NewNode = new Buffer(NETMTU);
		NewNode->FullBuffer(RtpHead, 0, RTP_HEADSIZE);	//RTP
		NewNode->FullBuffer(fu, 2);						//FU-A头
		//RTP FU-A的第一个字节不应该是NAL头，这里跳过NAL头一个字节进行填充。 
		NewNode->FullBuffer(buf, pos + 1, NETMTU - RTP_HEADSIZE - 3);
		
		
		NewNode->SetByte(0x01, 0);
		NewNode->SetByte(DevCount, 1);
		NewNode->SetByte(nal_type ,2);
		pos += (NETMTU - RTP_HEADSIZE - 2);

		//printf("节点%d的数据长度为: %d\n",DevCount, NewNode->GetSizeValue());
		bufferlist.push(NewNode);
	}
	if (IsFU_A)
	{
		//FU-A最后一个节点
		if (dpage == 1)
			DevCount++;
		else
			DevCount = 0;
		
		Buffer *NewNode = new Buffer(sta - pos + RTP_HEADSIZE + 2);
		NewNode->FullBuffer(RtpHead, 0, RTP_HEADSIZE);
		NewNode->FullBuffer(fu, 2);
		if(dpage == 1 && DevCount == 1)
			NewNode->FullBuffer(buf, pos + 1, sta - pos - 1);
		else
			NewNode->FullBuffer(buf, pos, sta - pos);
		NewNode->SetByte(0x01, 0);
		NewNode->SetByte(DevCount, 1);
		NewNode->SetByte(nal_type ,2);
		bufferlist.push(NewNode);
		//printf("节点%d的数据长度为: %d pos :%d  sta :%d\n",DevCount, NewNode->GetSizeValue(), pos, sta);
		return ;
		
	}
	//SINGLE NAL处理
	Buffer *NewNode = new Buffer(sta - pos + RTP_HEADSIZE);
	NewNode->FullBuffer(RtpHead, 0, RTP_HEADSIZE);
	NewNode->FullBuffer(buf, pos, sta - pos);
	NewNode->SetByte(DevCount, 0);
	bufferlist.push(NewNode);
	DevCount = 0;
}

//分离出NALU（1），同时对0x00 0x00 0x03进行处理（3），处理之后将每个NALU单元插入队列中。
//int CountForPos = pos;
//for (int p = pos; p < sta; p++)
//{
//	//（2）分离出与Start Code和关键标志位重复的数据 如 0x00 0x00 0x03 0x01 为 0x00 0x00 0x01
//	if (p < sta - 3 && pdata[p] == 0x00 && pdata[p + 1] == 0x00 && pdata[p + 2] == 0x03 )
//	{
//		NewNode->FullBuffer(&pdata[CountForPos], p - CountForPos + 1);
//		p += 2;
//		CountForPos = p;
//	}
//}
//NewNode->FullBuffer(buf, CountForPos, sta - CountForPos);





//




void 
h264MediaStream::DevNode(buf_share_ptr buf, unsigned pos)
{
	Buffer sbuf(buf->GetSizeValue() - pos);
	sbuf.FullBuffer(buf->GetBuffer() + pos, buf->GetSizeValue() - pos);
	DevNode(sbuf, 0);
}

void 
h264MediaStream::DevNode(const Buffer& buf, unsigned pos)
{
	unsigned sta = pos;
	const unsigned char* pdata = buf.GetBuffer();

	bool NalFlag = false;
	
	for (;sta < buf.GetSizeValue() - 4; sta ++)
	{		
		//（1）判断Start Code
		if (pdata[sta] == 0x00 && pdata[sta + 1] == 0x00 && 
			(pdata[sta + 2] ==0x01 || (pdata[sta + 2] == 0x00 && pdata[sta + 3] == 0x01)))
		{
			//NAL头或尾
			if (!NalFlag)
			{
				NalFlag = true;
				if (DevCount > 0 )
				{
					//DevCount = 0;
					if (sta == 0)
						DevLargeNode(buf, 0, 0, 2);
					else
						DevLargeNode(buf, 0, sta - 1, 2);
				}
				sta += (pdata[sta + 2]==0?3:2);
				pos += (sta + 1);
			}
			else
			{
				if ((pdata[pos] & 0x1f) >= 6)
				{
					int type = (pdata[pos] & 0x1f);
					Buffer *NewNode = new Buffer(sta - pos);
					NewNode->FullBuffer(buf, pos, sta - pos);
					bufferlist.push(NewNode);
					pos += sta - pos + (pdata[sta + 2]==0?4:3);
					sta += (pdata[sta + 2]==0?4:3);
					//printf("找到一个Type大于6的节点 :%d 插入位置为: %d\n", type, bufferlist.size());
					continue;
				}
				DevLargeNode(buf, pos, sta - pos);
				pos += sta - pos + (pdata[sta + 2]==0?4:3);
				sta += (pdata[sta + 2]==0?4:3);
				
			}
		}
	}
	//对剩下的字节做分包处理
	DevLargeNode(buf, pos, buf.GetSizeValue() - pos, 1);
	//printf("剩下的字节：%d 队列中的元素个数 = %d\n", buf.GetSizeValue() - pos, bufferlist.size());
	filepos +=buf.GetSizeValue();
}

bool 
h264MediaStream::h264Base64Ps(buf_share_ptr buf, std::string& psbase)
{
	assert(buf);
	unsigned short t = buf->GetPosValue();
    BaseEncoder::Base64Encode((uint8_t*)buf->GetBuffer(), buf->GetSizeValue(), &psbase);
	return true;
}

buf_share_ptr 
h264MediaStream::GetNode()
{
	if (bufferlist.size())
	{
		Buffer* ret = bufferlist.front();
		bufferlist.pop();
		buf_share_ptr retptr(ret);
		return retptr;
	}
    buf_share_ptr ret(new Buffer(0));
    return ret;
}

void 
h264MediaStream::PullNode(const Buffer& Pulldata)
{
	
}

uint8_t 
h264MediaStream::GetNalType(buf_share_ptr buf)
{
	if (buf->GetByte(0) == 0)//SINGL NAL
	{
		return (buf->GetByte(RTP_HEADSIZE)&0x1f);
	}
	else if (buf->GetByte(0) == 1)//FU-A
	{
		if (buf->GetByte(1) == 1)
		{
			return (buf->GetByte(2)&0x1f);
		}
		else
			return 0;
		
	}
	return (buf->GetByte(0)&0x1f);
	
}




