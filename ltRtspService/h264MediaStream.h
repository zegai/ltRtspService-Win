#pragma once
#include <vector>
#include "MediaStream.h"
#include "bit/vlc_bits.h"


using namespace std;

#define MAXDATABUF		10
#define RTP_HEADSIZE	16
#define BUFFERSIZE		1024*1024
#define NETMTU			1480
class h264MediaStream: public MediaStream
{
public:

	/**
	* Rational number num/den.
	*/
	typedef struct AVRational_t{
		int num; ///< numerator
		int den; ///< denominator
	} AVRational_t;

	/**
	* Sequence parameter set
	*/
	typedef struct h264_sps_t{

		int profile_idc;
		int level_idc;
		int transform_bypass;              ///< qpprime_y_zero_transform_bypass_flag
		int log2_max_frame_num;            ///< log2_max_frame_num_minus4 + 4
		int poc_type;                      ///< pic_order_cnt_type
		int log2_max_poc_lsb;              ///< log2_max_pic_order_cnt_lsb_minus4
		int delta_pic_order_always_zero_flag;
		int offset_for_non_ref_pic;
		int offset_for_top_to_bottom_field;
		int poc_cycle_length;              ///< num_ref_frames_in_pic_order_cnt_cycle
		int ref_frame_count;               ///< num_ref_frames
		int gaps_in_frame_num_allowed_flag;
		int mb_width;                      ///< frame_width_in_mbs_minus1 + 1
		int mb_height;                     ///< frame_height_in_mbs_minus1 + 1
		int frame_mbs_only_flag;
		int mb_aff;                        ///<mb_adaptive_frame_field_flag
		int direct_8x8_inference_flag;
		int crop;                   ///< frame_cropping_flag
		int crop_left;              ///< frame_cropping_rect_left_offset
		int crop_right;             ///< frame_cropping_rect_right_offset
		int crop_top;               ///< frame_cropping_rect_top_offset
		int crop_bottom;            ///< frame_cropping_rect_bottom_offset
		int vui_parameters_present_flag;
		AVRational_t sar;
		int timing_info_present_flag;
		uint32_t num_units_in_tick;
		uint32_t time_scale;
		int fixed_frame_rate_flag;
		short offset_for_ref_frame[256]; //FIXME dyn aloc?
		int bitstream_restriction_flag;
		int num_reorder_frames;
		int scaling_matrix_present;
		uint8_t scaling_matrix4[6][16];
		uint8_t scaling_matrix8[2][64];
	}h264_sps_t;


	typedef struct h264_slice_t
	{
		int i_slice_type;
		int i_frame_num;
		int i_pic_order_cnt_lsb;

	}h264_slice_t;


	h264MediaStream();
	virtual ~h264MediaStream();



	virtual void DevNode(const Buffer& buf, unsigned pos);
	virtual void DevNode(buf_share_ptr buf, unsigned pos);

	void DealBuf(const char*, unsigned);
	void DevLargeNode(const Buffer& buf, unsigned pos, int length, int dpage = 0);
	virtual buf_share_ptr GetNode();
	virtual void	PullNode(const Buffer& Pulldata);


	bool ParseSqs(const buf_share_ptr SqsNode);

	
	static bool h264Base64Ps(buf_share_ptr buf, std::string& sps);
	static uint8_t GetNalType(buf_share_ptr buf);
	void ParseSlice(uint8_t* p_nal,  int n_nal_size, int i_nal_type);

	h264_sps_t* StreamSps;
	h264_slice_t* CurSlice;

	unsigned char CurNalType;
	unsigned short poc;


protected:
	static bool h264_decode_hrd_parameters(bs_t& s, h264_sps_t* p_sps);
	
private:
	int DevCount;
};