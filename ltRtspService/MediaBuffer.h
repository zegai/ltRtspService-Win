#pragma once
#include <iostream>
#include <boost/shared_ptr.hpp>


class Buffer;
typedef boost::shared_ptr<Buffer> buf_share_ptr;

enum BUFLOG
{
	DONE,		//正常
	NONEDATA,	//空
	HALFSIZE,	//到达阈值
	HAVEDATA,   //已存在数据
	FULLBUFFER,	//用于已存在数据单元，但下一个单元过大故不填充
	LARGEPACKET,//用于数据单元较大，大于MTU需要分包
};

class Buffer
{
public:
	explicit Buffer(unsigned mtu = 1480);
	Buffer(const Buffer& ibuffer);
	
	static buf_share_ptr CreateBuf(unsigned mtu = 1480);
	const unsigned char* GetBuffer() const;


	unsigned GetMtuValue() const;
	unsigned GetPosValue() const;
	unsigned GetSizeValue()const;

	unsigned char GetByte(unsigned pos);
	void SetByte(unsigned char byteSet, unsigned pos);

	bool CopyData(unsigned char* dst, int length);
	bool Clear();
	
	inline bool FullBuffer(const unsigned char* buffer,unsigned lenth);
	bool FullBuffer(const Buffer& buf,unsigned start,unsigned length);
	bool FullBuffer(const buf_share_ptr buf,unsigned start,unsigned length);

	virtual ~Buffer();
protected:
	const Buffer& operator=(const Buffer& ibuffer);

private:
	unsigned char *pbuffer;
	unsigned short pos;
	unsigned MTU;
};




