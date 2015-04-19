#pragma once

#include <iostream>

enum rtsp_type
{
	ERRORTYPE,
	OPTIONS,
	DESCRIBE,
	SETUP,
	PLAY,
	PAUSE,
	TEARDOWN
};

typedef struct _rtspinfo
{
	rtsp_type	type;
	std::string	addr;
	std::string file_path;
	std::string	cseq;
	unsigned int session;
    unsigned int streamid;
	unsigned int clientport;

}rtspinfo;

//用于处理Rtsp数据
class rtsp_string
{
public:
	explicit rtsp_string();
	void deal_string(std::string& info);
	~rtsp_string();
	const rtspinfo& deal_requset(std::string& info);
protected:

	bool create_sdp(std::string& info);
	
	bool deal_options(std::string& info);
	bool deal_describe(std::string& info);
	bool deal_setup(std::string& info);
	bool deal_play(std::string& info);
	bool deal_pause(std::string& info);
	bool deal_teardown(std::string& info);

	unsigned get_addr(std::string& info,unsigned pos);
	unsigned get_filepath(std::string& info,unsigned pos);
	unsigned get_cseq(std::string& info,unsigned pos);
	
private:
	//std::string sdpstring
	rtspinfo deal_info;

};
