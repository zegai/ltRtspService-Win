# ltRtspService-Win
一个Win下的Rtsp服务，使用Libevent作为网络模块，暂时只支持rtp over tcp

###服务过程
    1.创建目录下h264文件的Sdp，主要内容为构建一个Sdp模板，填充BASE64加密后的SPS+PPS，然后将其保持至Sdp列表中，
    使用map进行管理，后续内容为将Sdp保存至文件。
    
    2.将地址端口信息作为参数传入网络模块，然后将对应的完成事件回调函数传入绑定。
    
    3.当listen收到一个会话连接时创建一个MediaSession对象，然后将会话对象插入MediaSession的MAP中，键值为这个会话的
    SessionID,保证SessionID不重复。
    
    4.当收到一个RTSP请求时，确认连续的CR LF时开始处理RTSP字符串，DESCRIBE处理字符串之后将会跟着从第一部创建的Sdp信息。
    
    5.SETUP阶段将会使用HardwareIO开始读取文件第一个数据块进行分解，数据块大小自定，暂定为10KB。
    
    6.PLAY阶段，设置MediaStream类中的PLAY标志位为true，代表开始数据传输。
    
    7.数据传输阶段，RtpOverTcp模块将会把从MediaStream缓冲队列取出的数据进行RTP封装，由于使用TCP传输，
    所以头部将有四个字节用于标示后续的RTP数据 $  + 0000(数据信道) + Length(RTP包长度)，同时还需要计算时间戳，
    由于使用与RTSP同一端口，所以该端口也将会收到从客户端发出的RTCP信息。
    
    //
    8.TREADOWN，断开连接，释放对于资源，触发SIGNAL信号。

NetWork 网络模块
====
  网络模块使用Libevent作为网络库，将接受完成事件和发送完成事件回调函数与监听端口绑定。

### 涉及到的代码

    network.h network.cpp
    /*事件回调函数在MediaSession中*/
    MediaSession.h MediaSession.cpp

Buffer 缓冲模块
====
  这里作为Demo还没有使用空数组进行缓冲创建，Buffer用于对数据缓冲的单元，提供了几种创建方式，一种普通的new构造，
这里只有在分解数据为NAL单元时才使用Buffer* p = new Buffer(buffersize)这种形式，而推荐方式是通过Buffer类中的静态成员函数
Createbuffer来创建Buffer，它将返回一个智能指针，这样，这个Buffer对象的生存期就不需要特别的管理。

### 涉及到的代码
    MediaBuffer.h MediaBuffer.cpp

RtspString Rtsp文本处理模块
====
    假设输入的文本为 :
    OPTION rtsp://127.0.0.1:554/1 
    RTSP/1.0 \r\n
    CSeq: 2\r\n
    \r\n
    
    那么通过RtspString的deal_request之后，应当返回
    RTSP/1.0 200 OK\r\n
    Cseq: 2\r\n 
    Public: OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE\r\n
    \r\n

### 涉及到的代码
    RtspString.h RtspString.cpp


MediaStream 流媒体模块
====

      这个Demo暂时只支持h264文件的媒体分析，h264的数据单元非常符合流特性，MediaStream类在此作为一个接口，
    同时也包含了Buffer作为缓冲队列，另外定义了一些媒体共有的特性，而流媒体的具体分包则通过派生类实现，如h264MediaStream。
    创建一个特定的MediaStreams需要通过StreamBuilder这样一个工厂进行创建，返回为指向对应流媒体模块的智能指针。
    
      在具体实现中，如h264MediaStream，主要实现了对从文件读出的数据块进行分解，
    分解后的每一个单元都将作为RTP发送包的一个原始节点，包括STAP-A和FU-A的分解，由于不希望在RTP包组织构成时再创建缓冲区，
    所以这里采用这样的设计，这样Buffer中的缓冲区的头RTP_HEADSIZE个字节将会保留。

### 涉及到的代码
    MediaStream.h MediaStream.cpp
    h264MediaStream.h h264MediaStream.cpp
    StreamBuilder.h StreamBuilder.cpp

MediaSession 会话模块
====

  每一个RTSP连接都作为一个会话处理，会话的生存周期为连接直到断开连接，TCP连接出错或者断开将会触发SIGNAL信号，会话资源将会在
这里释放。会话模块包含一个MediaStream类，DEMO暂时支持单码流，所以并没有用列表。

### 涉及到的代码
    MediaSession.h MediaSession.cpp
    
MediaCreateSdp Sdp信息初始化模块
====

此模块属于初始化模块，使用单例进行全局访问，它将文件的SDP信息保存，同时提供接口返回SDP信息。


HardwareIO 文件读取模块
====

这里将文件读取提取，主要用于以后优化用，如linux和windows的内存映射。
这里也将作为单例全局访问，多个客户端打开同一文件时，将只会有一个文件描述符，对于一个文件是唯一的。





