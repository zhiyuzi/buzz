set(SRCS
    Buffer.cpp
    Channel.cpp
    CurrentThread.cpp
    EventLoop.cpp
    EventLoopThreadPoll.cpp
    InetAddress.cpp
    Logger.cpp
    PollerEpoll.cpp
    Signal.cpp
    Socket.cpp
    TcpConnection.cpp
    TcpServer.cpp
    Timer.cpp
    Timestamp.cpp
)

add_library(buzz STATIC ${SRCS})

install(TARGETS buzz DESTINATION lib)

set(HEADERS
    any.h
    BlockingQueue.h
    Buffer.h
    Callbacks.h
    Channel.h
    CurrentThread.h
    EventLoop.h
    EventLoopThreadPoll.h
    InetAddress.h
    Logger.h
    noncopyable.h
    Poller.h
    Signal.h
    Socket.h
    TcpConnection.h
    TcpServer.h
    Timer.h
    Timestamp.h
)

install(FILES ${HEADERS} DESTINATION include/buzz)