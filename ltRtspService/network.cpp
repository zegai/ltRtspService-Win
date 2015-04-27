#include "stdafx.h"
#include "network.h"
#include "MediaSession.h"

Network::Network(struct sockaddr_in addr)
{
    sockinfo = addr;
    evnbase = event_base_new();
	
    if(!evnbase)
    {
        //counld not init event base
    }
}

struct evconnlistener* Network::evconnlistener_new_udp_bind(evconnlistener_cb cb)
{
   /* struct evconnlistener *onlistener;
    evutil_socket_t fd;
    fd = socket(sockinfo.sin_family, SOCK_STREAM, 0);
    if(fd == -1)
        return NULL;
    if(evutil_make_socket_nonblocking(fd) < 0 || evutil_make_socket_closeonexec(fd) < 0
            || evutil_make_listen_socket_reuseable(fd) < 0){
        evutil_closesocket(fd);
        return NULL;
    }

    if(bind(fd, (struct sockaddr *)&sockinfo, sizeof(sockinfo)) < 0){
        perror("bad bind:");
        evutil_closesocket(fd);
        return NULL;
    }

    onlistener = evconnlistener_new(evnbase, cb, (void*)this,
                                    LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE, -1, fd);
    if(!onlistener)
    {
        evutil_closesocket(fd);
        return NULL;
    }
    return onlistener;*/
	return NULL;
}

bool Network::NetPrepare(TransType type)
{
    if(type == TCP || type == RTSP)
    {
        listener = evconnlistener_new_bind(evnbase, Listen_Cb,
                                           (void *)this, LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE,
                                           -1, (struct sockaddr*)&sockinfo, sizeof(sockinfo));

        if(!listener)
          {
            printf("error listen\n");
            return false;
          }
		char yes = 1;
		evutil_socket_t lsfd = evconnlistener_get_fd(listener);
		setsockopt(lsfd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes));
        return true;
    }
    else if(type == RTP || type == UDP)
    {
        listener = evconnlistener_new_udp_bind(Listen_Cb);
        if(!listener)
          {
            printf("udp error\n");
            return false;
          }
        return true;
    }
    return false;
}

bool Network::StartServer(bufferevent_data_cb preadcb, bufferevent_data_cb pwritecb)
{
    evn = evsignal_new(evnbase, SIGINT, Signal_Cb, (void *)evnbase);
    if(evn == NULL || event_add(evn, NULL) < 0){ return false; }
    readcb = preadcb;
    writecb = pwritecb;
	

    event_base_dispatch(evnbase);
	return true;
}

void Network::Signal_Cb(evutil_socket_t sig, short events, void *user_data)
{
  struct event_base *base = (struct event_base *)user_data;
  struct timeval delay = { 2, 0 };

  printf("Caught an interrupt signal; exiting cleanly in two seconds.\n");

  event_base_loopexit(base, &delay);
}

void Network::Listen_Cb(evconnlistener *listen, evutil_socket_t fd,
                               sockaddr *sa, int socklen, void *user_data)
{
    printf("a new connect \n");
    Network *net = (Network *)user_data;
    bufferevent *bev;

    bev = bufferevent_socket_new(net->evnbase, fd, BEV_OPT_CLOSE_ON_FREE);

    if(!bev){
        printf("error when create buff sock\n");return ;
    }

	MediaSession* session = new MediaSession(net);
	MediaSessionList::GetInstance()->SessionInsert(session);
	printf("Listen Insert: %I64u\n", session->GetSessionID());
    bufferevent_setcb(bev, net->readcb, net->writecb, Event_Cb, session);
    bufferevent_enable(bev, EV_READ | EV_WRITE);

}



void
Network::Event_Cb(struct bufferevent *bev, short events, void *user_data)
{
    if (events & BEV_EVENT_EOF) {
        printf("Connection closed.\n");
    } else if (events & BEV_EVENT_ERROR) {
        printf("Got an error on the connection: \n");/*XXX win32*/
    }
    /* None of the other events can happen here, since we haven't enabled
     * timeouts */
    bufferevent_free(bev);
	//回收当前MediaSession会话信息资源
    MediaSession * ses = (MediaSession *)user_data;
    MediaSessionList::GetInstance()->SessionDel(ses->GetSessionID());
	//bufferevent_free(bev);
}

//void
//Network::Tcp_Read_Cb(struct bufferevent *bev, void *user_data)
//{
//    assert(bev);
//
//    PQNode newnode = new QNode;
//    newnode->type = TCP;
//    newnode->events = EV_READ;
//    newnode->forbev = bev;
//    newnode->arg = user_data;
//    MesQueue::GetInstance()->InsertQueue(newnode);
//}

//void
//Network::Tcp_Write_Cb(struct bufferevent *bev, void *user_data)
//{
//    /*printf("Write Done!\n");
//    assert(bev);
//    PQNode newnode = new QNode;
//    newnode->type = TCP;
//    newnode->events = EV_WRITE;
//    newnode->forbev = bev;
//    newnode->arg = user_data;
//    MesQueue::GetInstance()->InsertQueue(newnode);*/
//}





//void *
//Network::MultiWork(void* pvoid)
//{
//    printf("Work Thread Create Done\n");
//    TWork* work = (TWork *)pvoid;
//    event_base_dispatch(work->base);
//    event_base_free(work->base);
//    return 0;
//}


//
//void
//MesQueue::MulticastInfo()
//{
//    for (int i = 0; i < MAX_WORK; i++)
//    {
//        if (workfd[i] != 0)
//        {
//            send(workfd[i], "o", sizeof("o") - 1, NULL);
//        }
//    }
//}
//
//void
//MesQueue::InsertQueue(PQNode InsNode)
//{
//    assert(InsNode);
//    MesLock();
//    /*if(QList.size() < MAX_WORK)
//    {
//    MulticastInfo();
//    }*/
//    QList.push(InsNode);
//    //printf("Qlist Ins:%d\n", QList.size());
//
//    MesUnLock();
//    MulticastInfo();
//}
//
//PQNode
//MesQueue::OutQueue()
//{
//    PQNode node = NULL;
//    MesLock();
//    if (QList.size())
//    {
//        node = QList.front();
//        QList.pop();
//        //printf("Qlist Out:%d\n", QList.size());
//    }
//    MesUnLock();
//    return node;
//}
//
//bool
//MesQueue::InsertWork(evutil_socket_t fd)
//{
//    bool flag = false;
//    MesLock();
//    if (fd > 0 && WorkCount < MAX_WORK)
//    {
//        for (int i = 0; i < MAX_WORK ; i++)
//        {
//            if (workfd[i] == 0)
//            {
//                workfd[i] = fd;
//                WorkCount ++;
//                flag = true;
//                break;
//            }
//        }
//    }
//    MesUnLock();
//    return flag;
//}
//
//bool
//MesQueue::DeleteWork(evutil_socket_t fd)
//{
//    bool flag = false;
//    MesLock();
//    for (int i = 0; i < MAX_WORK ; i++)
//    {
//        if (workfd[i] == fd)
//        {
//            workfd[i] = 0;
//            WorkCount --;
//            flag = true;
//        }
//    }
//    MesUnLock();
//    return flag;
//}
//
//MesQueue*
//MesQueue::GetInstance()
//{
//    static MesQueue* que = NULL;
//    if (que == NULL)
//    {
//        que = new MesQueue;
//        return que;
//    }
//    return que;
//}
//
//
//MesQueue::MesQueue(){
//    pthread_mutex_init(&QueueLock, NULL);
//    WorkCount = 0;
//    memset(workfd, 0, sizeof(workfd));
//}
//void MesQueue::MesLock(){pthread_mutex_lock(&QueueLock);}
//void MesQueue::MesUnLock(){pthread_mutex_unlock(&QueueLock);}
//MesQueue::~MesQueue()
//{
//    MesLock();
//    while(QList.size())
//    {
//        delete QList.front();
//        QList.pop();
//    }
//    MesUnLock();
//    pthread_mutex_destroy(&QueueLock);
//}
