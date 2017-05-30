#pragma once

#include <memory>
#include <functional>

namespace buzz
{    
    typedef std::function<void()> EventHandler;
    typedef std::function<void()> TaskCallback;

    class Timestamp;
    typedef std::function<void(Timestamp)>  ReadEventHandler;
    
    class TcpConnection;
    typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
    
    typedef std::function<void(const TcpConnectionPtr&)> CloseEventHandler;
    typedef std::function<void(const TcpConnectionPtr&)> StateChangeEventHandler;
    typedef std::function<void(const TcpConnectionPtr&)> WriteCompleteEventHandler;
    typedef std::function<void(const TcpConnectionPtr&, int)> ErrorEventHandler;

    class Buffer;
    typedef std::function<void(const TcpConnectionPtr&, 
                               Buffer*, Timestamp)> MessageEventHandler;
}