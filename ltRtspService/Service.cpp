// Sort.cpp : 定义控制台应用程序的入口点。
//
#pragma once
#include "stdafx.h"
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>
#ifndef WIN32
#include <netinet/in.h>
# ifdef _XOPEN_SOURCE_EXTENDED
#  include <arpa/inet.h>
# endif
#include <sys/socket.h>
#endif

#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/event.h>
#include <Windows.h>
#include <iostream>
#include "network.h"
#include "RtspString.h"
#include "MediaSession.h"
#include "MediaCreateSdp.h"
#include <queue>
#include <boost/shared_ptr.hpp>
//#include "RtspString.h"


int _tmain(int argc, _TCHAR* argv[])
{

#ifdef WIN32
	WSADATA wsa_data;
	WSAStartup(0x0202, &wsa_data);
#endif
	MediaCreateSdp::GetInstance("\\test.h264");

	struct sockaddr_in sa;
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_port = htons(1000);
	sa.sin_addr.s_addr = htonl(0);
	const char* ver = event_get_version();
	printf("%s\n", ver);
	Network* work = new Network(sa);
	work->NetPrepare(TCP);
	work->StartServer(MediaSession::Recv, MediaSession::Send);
	return 0;
}







