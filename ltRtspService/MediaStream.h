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


	uint64_t filepos;	 //��ǰ�������Ӧ���ļ�ƫ��ֵ
	int64_t frame_count; //��ǰ�ؼ�֡
	int64_t pframe_count;//��ǰ֡
	int32_t ssrc;		 //ÿ��ý������Ӧ��SSRC ������
	
	bool OnStream;		//�����ж�ʱ���ڲ���״̬
	bool IsPlay;
	
	unsigned short sqsnumber; //��ǰ����TCP֡������
	
	int fileindex;			  //��Ӧ�ļ�����
	int lsb_count;			  //��ʱ����
	int fps;				  //֡��
	string filename;		  //�ļ���
	
	
	
	
	
	
	
	
protected:
	//ý�����������
	queue<Buffer *> bufferlist;
	
};

