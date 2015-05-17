#include "stdafx.h"
#include "MediaBuffer.h"

netbufferpoll* netbufferpoll::single = new netbufferpoll;

Buffer::Buffer(unsigned mtu /* = 1480 */, buftype buft)
{
	if (buft == NORMAL)
	{
		MTU = mtu;
		pbuffer = new unsigned char[mtu];

		memset(pbuffer, 0, mtu);

		pos = mtu;
	}
	else if(buft == NETBUFPOLL)
	{
		 MTU = 1480;
		 pos = 1480;
		 pbuffer = netbufferpoll::get_instance()->get_buf();
	}
	
	
}

Buffer::Buffer(const Buffer& ibuffer)
{
	MTU = ibuffer.MTU;
	pbuffer = new unsigned char[MTU];
	memset(pbuffer, 0, MTU);
	memcpy(pbuffer, ibuffer.pbuffer, MTU);
	pos = ibuffer.pos;
}


Buffer::~Buffer()
{
	//printf("Buffer Del\n");
	delete[] pbuffer;
};

unsigned 
Buffer::GetMtuValue() const
{
	return MTU;
}

unsigned 
Buffer::GetPosValue() const
{
	return pos;
}

unsigned 
Buffer::GetSizeValue() const
{
	return MTU - pos;
}

bool 
Buffer::Clear()
{
	memset(pbuffer, 0, pos);
	pos = MTU;
	return true;
}

bool 
Buffer::FullBuffer(const unsigned char* buffer, unsigned lenth)
{
	assert(buffer&&lenth);
	if (pos >= lenth)
	{
		memcpy(&pbuffer[MTU - pos], buffer, lenth);
		pos -= lenth;
		return true;
	}
	return false;
}

bool 
Buffer::FullBuffer(const Buffer& buf,unsigned start,unsigned length)
{
	if (pos >=length)
	{
		memcpy(&pbuffer[MTU - pos], &buf.pbuffer[start], length);
		pos -= length;
		return true;
	}
	return false;
}

bool 
Buffer::FullBuffer(const buf_share_ptr buf,unsigned start,unsigned length)
{
	assert(buf);
	
	if (pos >=length)
	{
		memcpy(&pbuffer[MTU - pos], &buf->pbuffer[start], length);
		pos -= length;
		return true;
	}
	return false;
}

const unsigned char* 
Buffer::GetBuffer()const
{
	return pbuffer;
}


void 
Buffer::SetByte(unsigned char byteSet, unsigned pos)
{
	pbuffer[pos] = byteSet;
}

unsigned char 
Buffer::GetByte(unsigned pos)
{
	return pbuffer[pos];
}

buf_share_ptr 
Buffer::CreateBuf(unsigned mtu /* = 1480 */)
{
	buf_share_ptr p(new Buffer(mtu));
	return p;
}


const Buffer& 
Buffer::operator=(const Buffer& ibuffer)
{
	if (this != &ibuffer)
	{
		MTU = ibuffer.MTU;
		delete[] pbuffer;
		pbuffer = new unsigned char[MTU];
		memset(pbuffer, 0, MTU);
		if (FullBuffer(ibuffer.pbuffer, ibuffer.pos))
		{
			pos = ibuffer.pos;
			return *this;
		}
		//需要一个log
		pos = MTU;
		return *this;
	}
	return *this;
}
