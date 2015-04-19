#include "stdafx.h"
#include "MediaCreateSdp.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#endif // _WIN32



MediaCreateSdp* 
MediaCreateSdp::GetInstance(const string& path)
{
	static MediaCreateSdp* sdp = NULL;
	if (sdp == NULL)
	{
		sdp = new MediaCreateSdp(path);
		return sdp;
	}
	return sdp;
}

MediaCreateSdp::~MediaCreateSdp()
{
	map<string*, string*>::iterator iter = SdpList.begin();
	while (iter != SdpList.end())
	{
		string *key = static_cast<string *>(iter->first);
		string *svalue = static_cast<string *>(iter->second);
		delete key;
		delete svalue;
		iter ++;
	}
}

MediaCreateSdp::MediaCreateSdp(const string& path)
{
    char filename[256] = {0};
    char ext[256] = {0};

    _splitpath_s(path.c_str(), NULL, 0, NULL, 0, filename, 256, ext, 256);
	
	HANDLE m_hVideoFile = CreateFile(path.c_str(), 
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
        
	do 
	{
        
		if (m_hVideoFile == INVALID_HANDLE_VALUE) 
		{
			perror(filename);
			break;
		}
        
		//只读取前4096个字节做分析
		unsigned char valuetmp[4096] = {0};
        //FILE* filefd = fopen("test.h264","r");

        //mmap(valuetmp, 4096, PROT_READ, MAP_SHARED, filefd, 0);
        /*filename[0] = 'T';
        filename[1] = 'e';
        filename[2] = 's';
        filename[3] = 't';

        ext[0] = '.';
        ext[1] = 'h';
        ext[2] = '2';
        ext[3] = '6';
        ext[4] = '4';*/
		string sdpall;
		sdpall.append("v=0\r\n");
		sdpall.append("o=- sessionforreplace sessionforreplace IN IP4 127.0.0.1\r\n");

		sdpall += "s=";
		sdpall += filename;
		sdpall += "\r\n";

		sdpall.append("c=IN IP4 127.0.0.1\r\n");
		sdpall.append("t=0 0\r\n");
		sdpall.append("a=control:*\r\n");
		sdpall.append("m=video 0 RTP/AVP loadtype\r\n");
		sdpall.append("b=AS:bandwidth\r\n");
		sdpall.append("a=rtpmap:loadtype H264/90000\r\n");
		sdpall.append("a=fmtp:loadtype profile-level-id=profile_idc; sprop-parameter-sets=tempforsps; packtization-mode=1;\r\n");
		sdpall.append("a=cliprect:rectofvideo\r\n");
		sdpall.append("a=mpeg4-esid:tmpforesid\r\n");
		sdpall.append("a=control:trackID=tmpforstid\r\n");

        DWORD SizeGet = 0;
        //int fresult = 0;
		ReadFile(m_hVideoFile, valuetmp, 4096, &SizeGet, NULL);
        //SizeGet = fread(valuetmp, sizeof(char), 4096, filefd);
        printf("Get File %d\n", SizeGet);
        if (!SizeGet)
		{
			perror(filename);
			break;
		}

		MediaType tp = UNDEFINED;
		media_stream_ptr st = MediaStreamBuild::CreateNew(ext, tp);

		if (!st)
		{
			printf("Create %s Stream Error\n", ext);
			break;
		}

		Buffer buf(SizeGet);
		buf.FullBuffer(valuetmp, SizeGet);

		switch (tp)
		{
		case H264:
			H264CreateSdp(buf, st, sdpall);
			break;
		case AAC:
			break;
		case UNDEFINED:
			break;
		default:
			break;
		}

        CloseHandle(m_hVideoFile);
        //fclose(filefd);
        
		HANDLE wf = CreateFile("\\MySession.sdp", 
			GENERIC_WRITE,	
			FILE_SHARE_WRITE, NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

		if (INVALID_HANDLE_VALUE == wf)
		{
			perror("Error When Write Sdp File!\n");
			CloseHandle(wf);
			break;
		}

		DWORD dwSize = 0;

		if(!WriteFile(wf, sdpall.c_str(), sdpall.size(), &dwSize, NULL))
		{
			perror("Error When Write Sdp File!\n");
			CloseHandle(wf);
			break;
		}

		CloseHandle(wf);
        
        //fclose(filefd);
		string *KeyName = new string(filename);
		string *KeyValue = new string(sdpall);

		SdpList.insert(pair<string *, string *>(KeyName, KeyValue));

	} while (0);
	//_CrtDbgBreak();
}

bool 
MediaCreateSdp::GetSdp(const string& filename, string* ssdp)
{
	assert(ssdp);

	map<string*, string*>::iterator iter = SdpList.begin();

	while (iter != SdpList.end())
	{
		string *key = static_cast<string *>(iter->first);
		if (key->compare(filename) == 0)
		{
			string *svalue = static_cast<string *>(iter->second);
			ssdp->clear();
			ssdp->append(svalue->c_str());
			return true;
		}
		iter ++;
	}
	return false;
	
}


bool 
MediaCreateSdp::H264CreateSdp(const Buffer &buf, media_stream_ptr SdpMedia, string& SdpTemp)
{
	h264MediaStream *h264SdpMedia = dynamic_cast<h264MediaStream *>(SdpMedia.get());

	h264SdpMedia->DevNode(buf, 0);

	buf_share_ptr DataNode = h264SdpMedia->GetNode();

    buf_share_ptr SPS;
    buf_share_ptr PPS;

	string SpsString = "";
	string PpsString = "";

	unsigned char stcode[] = {0x00, 0x00, 0x01}; 

	while (DataNode->GetMtuValue())
	{
		uint8_t p = h264MediaStream::GetNalType(DataNode);
		
		if (p == 0x07) //SPS
		{
			h264SdpMedia->ParseSqs(DataNode);
			SPS = Buffer::CreateBuf(DataNode->GetSizeValue() + 3);
			SPS->FullBuffer(stcode, 3);
			SPS->FullBuffer(DataNode, 0, DataNode->GetSizeValue());
			h264SdpMedia->h264Base64Ps(SPS, SpsString);
		}
		if (p == 0x08) //PPS
		{
			PPS = Buffer::CreateBuf(DataNode->GetSizeValue() + 3);
			PPS->FullBuffer(stcode, 3);
			PPS->FullBuffer(DataNode, 0, DataNode->GetSizeValue());
			h264SdpMedia->h264Base64Ps(PPS, PpsString);
		}

		DataNode = h264SdpMedia->GetNode();
	}
	
	if (!SpsString.size() || !PpsString.size())
	{
		printf("Counldn't Find Sps And Pps\n");
		return false;
	}

	string ParSet;

	char forchange[256] = {0};

	//替换自定义模版中的tempforsps
	ParSet.append(SpsString.c_str());
	ParSet.append(",");
	ParSet.append(PpsString.c_str());
	SdpTemp.replace(SdpTemp.find("tempforsps"), 10, ParSet);

    _snprintf_s(forchange, 256, "%06X", h264SdpMedia->StreamSps->profile_idc);
	SdpTemp.replace(SdpTemp.find("profile_idc"), 11, forchange);

    _snprintf_s(forchange, 256, "0,0,%d,%d", h264SdpMedia->StreamSps->mb_height, h264SdpMedia->StreamSps->mb_width);
	SdpTemp.replace(SdpTemp.find("rectofvideo"), 11, forchange);

	return true;
}
