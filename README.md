# buzz 简洁易用的 C++11 网络库

### 目录结构
 * buzz--------buzz 库源码
 * examples----示例代码
 
### 快速实现 echo 服务
```C++
#include <buzz/Buffer.h>
#include <buzz/Signal.h>
#include <buzz/EventLoop.h>
#include <buzz/TcpServer.h>
#include <buzz/InetAddress.h>
#include <buzz/TcpConnection.h>

using namespace buzz;

int main()
{
    buzz::EventLoop loop;
    buzz::Signal::Register(SIGINT, [&]() { loop.Exit(); });

    buzz::InetAddress listen_address(7);
    TcpServer echo_server(&loop, "echo-serve", listen_address);

    echo_server.OnMessage([](const TcpConnectionPtr& conn, Buffer* buffer, Timestamp time){
        conn->SendMessage(buffer->RetrieveAsString());
    });
    
    echo_server.Start(1);

    loop.Loop();

    return 0;
}
```

部分实现参考自 [chenshuo/muduo](https://github.com/chenshuo/muduo) 在此表示感谢