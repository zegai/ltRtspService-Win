#ifndef NETWORK_H
#define NETWORK_H


#include <errno.h>

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
#include <queue>
#define MAX_WORK 4
//
enum TransType
{
    TCP,
    UDP,
    RTSP,
    RTP
};



class Network
{
public:
    explicit Network(struct sockaddr_in addr);
    bool NetPrepare(TransType type = TCP);
    bool StartServer(bufferevent_data_cb readcb, bufferevent_data_cb writecb);
    ~Network();
private:
    static void Signal_Cb(evutil_socket_t sig, short events, void *user_data);
    static void Listen_Cb(evconnlistener *listen, evutil_socket_t fd,
                          sockaddr *sa, int socklen, void *user_data);
    static void Event_Cb(struct bufferevent *bev, short events, void *user_data);
    struct evconnlistener* evconnlistener_new_udp_bind(evconnlistener_cb cb);

    struct event_base* evnbase;
    struct evconnlistener* listener;
    struct event* evn;

    bufferevent_data_cb readcb,writecb;
    int sockfd;
    struct sockaddr_in sockinfo;
};


#endif // NETWORK_H
