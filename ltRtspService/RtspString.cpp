#include "stdafx.h"
#include "RtspString.h"
#include "MediaCreateSdp.h"


rtsp_string::rtsp_string(){}

rtsp_string::~rtsp_string(){}

unsigned 
rtsp_string::get_addr(std::string& info,unsigned pos)
{
	const char* decode_p = info.c_str();
	char addr[128] = {0};
	unsigned i,psize = 0;
	for (i = pos ;i < info.size();i++)
	{
		if (decode_p[i] == '/' && decode_p[i + 1] != '/')
		{
			deal_info.addr.clear();
			deal_info.addr.append(addr);
			return i + 1;
		}
		addr[psize ++] = decode_p[i];
	}
	return 0;
}

unsigned 
rtsp_string::get_filepath(std::string& info,unsigned pos)
{
	const char* decode_p = info.c_str();
	unsigned	i,psize  = 0;
	
	char path[128] = {0};
	
	for (i = pos;i < info.size();i++)
	{
		if (decode_p[i] == ' ')
		{
			deal_info.file_path.clear();
			deal_info.file_path.append(path);
			return i + 1;
		}
		path[psize ++] = decode_p[i];
	}
	return 0;
}




unsigned 
rtsp_string::get_cseq(std::string& info,unsigned pos)
{
	const char* decode_p = info.c_str();
	unsigned i,psize = 0;
	for (i = pos;i < info.size(); i++)
	{
		if (i < info.size() - 5		&&
			decode_p[i]		== 'C'	&&
			decode_p[i + 1]	== 'S'	&&
			decode_p[i + 2]	== 'e'  &&
			decode_p[i + 3] == 'q'	&&
			decode_p[i + 4] == ':'
		)
		{ pos = i + 5;break; }
	}
	
	char count[16]  = {0};
	bool startcount = false;

	for (i = pos;i < info.size(); i++)
	{
	    if (decode_p[i] == ' ')
	      continue;
		if (decode_p[i] != ' ' && !startcount)
		{
			startcount = true;
			count[psize ++] = decode_p[i];
			continue;
		}
	    if (decode_p[i] == '\r' || decode_p[i] == '\n')
	    {
	      if (startcount)
	      {
            deal_info.cseq.clear();
            deal_info.cseq.append(count);
            return i;
	      }
	      else
			return 0;
	    }
	    count[psize ++] = decode_p[i];
	}
	return 0;
}




void 
rtsp_string::deal_string(std::string& info)
{
	deal_info.type = ERRORTYPE;
	const char* decode_p = info.c_str();
	unsigned i,pos = 0;
	for (i = 0;i < info.size(); i++)
	{
		if (i < info.size() - 5		&&
			decode_p[i]		== ' '  &&
			decode_p[i + 1]	== 'r'	&&
			decode_p[i + 2]	== 't'  &&
			decode_p[i + 3] == 's'	&&
			decode_p[i + 4] == 'p'  &&
			decode_p[i + 5] == ':')
		{ pos = i;break; }
	}
	
	if(i > info.size() - 5)
		return ;
	for (i = 0;i < pos; i++)
	{
		switch(decode_p[i])
		{
		case 'O':
			if (decode_p[i + 1] == 'P' && decode_p[i + 2] == 'T' &&
			    decode_p[i + 3] == 'I' && decode_p[i + 4] == 'O' &&
			    decode_p[i + 5] == 'N' && decode_p[i + 6] == 'S')
			{
				//OPTIONS
				deal_info.type = OPTIONS;
			}
			break;
		case 'D':
			if (decode_p[i + 1] == 'E' && decode_p[i + 2] == 'S' &&
			    decode_p[i + 3] == 'C' && decode_p[i + 4] == 'R' &&
			    decode_p[i + 5] == 'I' && decode_p[i + 6] == 'B' &&
			    decode_p[i + 7] == 'E')
			{
				//DESCRIBE
				deal_info.type = DESCRIBE;
			}
			break;
		case 'S':
			if (decode_p[i + 1] == 'E' && decode_p[i + 2] == 'T' &&
			    decode_p[i + 3] == 'U' && decode_p[i + 4] == 'P')
			{
				//SETUP
				deal_info.type = SETUP;
			}
			break;
		case 'T':
			if (decode_p[i + 2] == 'E' &&
			    decode_p[i + 3] == 'A' && decode_p[i + 4] == 'R' &&
			    decode_p[i + 5] == 'D' && decode_p[i + 6] == 'O' &&
			    decode_p[i + 7] == 'W' && decode_p[i + 8] == 'N')
			{
				deal_info.type = TEARDOWN;
			}
			break;
		case 'P':
			if (decode_p[i + 1] == 'L' && decode_p[i + 2] == 'A' &&
			    decode_p[i + 3] == 'Y')
			{
				deal_info.type = PLAY;
			}
			else if (decode_p[i + 1] == 'A' && decode_p[i + 1] == 'U' && 
					 decode_p[i + 2] == 'S' && decode_p[i + 3] == 'E')
			{
				deal_info.type = PAUSE;
			}
			break;
		default:
			continue;
		}
	}

	if (deal_info.type == ERRORTYPE)
		return;
	pos = get_addr(info, pos + 8);
	if (!pos)
		return;
	pos = get_filepath(info, pos);
	if (!pos)
		return;
	pos = get_cseq(info, pos);
	if (!pos)
		return;
	

	
}



bool 
rtsp_string::create_sdp(std::string& info)
{
	return true;
}

bool 
rtsp_string::deal_options(std::string& info)
{
	info.clear();

	info.append("RTSP/1.0 200 OK\r\nCSeq: ");
    info.append(deal_info.cseq.c_str());
    info.append("\r\nPublic: DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE\r\n");

	return true;

}



bool 
rtsp_string::deal_describe(std::string& info)
{
	info.clear();
	info.append("RTSP/1.0 200 OK\r\n");
	info.append("CSeq: ");
	info.append(deal_info.cseq);
	info.append("\r\n");
	info.append("Server: ltRtspService\r\n");

	char MoreDiscribe[256] = {0};
    _snprintf_s(MoreDiscribe, 256, "Content-Base: rtsp://%s/%s\r\n", deal_info.addr.c_str(), deal_info.file_path.c_str());
	info.append(MoreDiscribe);
	
	info.append("Content-Type: application/sdp\r\n");
	info.append("Content-Length: content_count\r\n\r\n");
	
	std::string SdpInfo;
	
	if (MediaCreateSdp::GetInstance(deal_info.file_path)->GetSdp(deal_info.file_path, &SdpInfo))
	{
		//printf("\n\n\nSdpInfo:\n%s\n", SdpInfo.c_str());
		info.append(SdpInfo);
		return true;
	}
	else
	{
		printf("Rtsp Sdp Info Not Found!\n");
		info.clear();
		info.append("RTSP/1.0 405 Method Not Allowd\r\n");
		info.append("CSeq: ");
		info.append(deal_info.cseq);
		info.append("\r\n");
		deal_info.type = ERRORTYPE;
		return false;
	}
}

bool 
rtsp_string::deal_setup(std::string& info)
{
    const char* decode_p = info.c_str();
    int pos;
    deal_info.streamid = 0;
    deal_info.clientport = 0;
    pos = info.find("trackID=");
    if(pos != std::string::npos)
    {
        //int end = info.find_first_of(" ", pos);
        int i = pos + 8;
        while(decode_p[i] >= '0' && decode_p[i] <= '9')
        {
            deal_info.streamid = deal_info.streamid*10 + decode_p[i] - '0';
            i++;
        }
    }
    printf("StreamId = %d\n", deal_info.streamid);
    pos = info.find("client_port=");
    if(pos != std::string::npos)
    {
        int i = pos + 12;
        while(decode_p[i] >= '0' && decode_p[i] <= '9')
        {
            //printf("%c\n", decode_p[i]);
            deal_info.clientport = deal_info.clientport*10 + decode_p[i] - '0';
            i++;
        }
    }
    printf("client_port = %d\n", deal_info.clientport);
	if (info.find("RTP/AVP/TCP") == std::string::npos)
	{
		printf("不支持RTPoverTCP模式");
		info.clear();
		info.append("RTSP/1.0 405 Method Not Allowd\r\n");
		info.append("CSeq: ");
		info.append(deal_info.cseq);
		info.append("\r\n");
	}
	info.clear();
	info.append("RTSP/1.0 200 OK\r\n");
	info.append("CSeq: ");
	info.append(deal_info.cseq);
	info.append("\r\n");
	info.append("Transport: RTP/AVP/TCP;interleaved=0-1\r\n");
	char tmp[64] = {0};
	_snprintf_s(tmp, 64, "Session: %u\r\n", deal_info.session);
	info.append(tmp);
	return true;
}

bool 
rtsp_string::deal_play(std::string& info)
{
	info.clear();
	info.append("RTSP/1.0 200 OK\r\n");
	info.append("CSeq: ");
	info.append(deal_info.cseq);
	info.append("\r\n");
	char tmp[64] = {0};
	_snprintf_s(tmp, 64, "Session: %u\r\n", deal_info.session);
	info.append(tmp);

	info.append("RTP-Info: url=rtsp://127.0.0.1:1000/test/track=1;seq=0\r\n");
	return true;
}

bool 
rtsp_string::deal_pause(std::string& info)
{
	return true;
}

bool 
rtsp_string::deal_teardown(std::string& info)
{
	return true;
}


const rtspinfo& 
rtsp_string::deal_requset(std::string& info)
{
	deal_string(info);

	switch(deal_info.type)
	{
	case OPTIONS:
		deal_options(info);
		break;
	case DESCRIBE:
		deal_describe(info);
		break;
	case SETUP:
		deal_setup(info);
		break;
	case PLAY:
		deal_play(info);
		break;
	case TEARDOWN:
		deal_teardown(info);
		break;
	case PAUSE:
		deal_pause(info);
		break;
	case ERRORTYPE:
	default:
		break;
	}
	return deal_info;
}
