#include <buzz/Buffer.h>
#include <buzz/Signal.h>
#include <buzz/EventLoop.h>
#include <buzz/TcpServer.h>
#include <buzz/InetAddress.h>
#include <buzz/TcpConnection.h>

#include <iostream>

class EchoServer
{
public:
    EchoServer(buzz::EventLoop* loop, buzz::InetAddress& host, int work_threads = 0)
        : m_server(loop, "echo-serve", host), m_work_threads(work_threads)
    {
        using namespace std::placeholders;

        m_server.OnStateChange(std::bind(&EchoServer::OnSateChange, this, _1));
        m_server.OnMessage(std::bind(&EchoServer::OnMessage, this, _1, _2, _3));
    }

    void Start()
    {
        m_server.Start(m_work_threads);
    }

    void OnSateChange(const buzz::TcpConnectionPtr& conn)
    {
        if (conn->GetState() == buzz::TcpConnection::kConnected) {
            std::cout << '[' << conn->Name() << "] connection." << std::endl;
        } else if (conn->GetState() == buzz::TcpConnection::kDisconnected) {
            std::cout << '[' << conn->Name() << "] closed." << std::endl;
        }
    }

    void OnMessage(const buzz::TcpConnectionPtr& conn, buzz::Buffer* buffer, buzz::Timestamp time)
    {
        std::string messsage(buffer->RetrieveAsString());
        conn->SendMessage(messsage);
    }
private:
    buzz::TcpServer m_server;
    int       m_work_threads;
};

int main()
{
    buzz::EventLoop loop;
    buzz::Signal::Register(SIGINT, [&]() { loop.Exit(); });

    buzz::InetAddress listen_address(7);
    EchoServer echo_server(&loop, listen_address, 1);
    echo_server.Start();

    loop.Loop();

    return 0;
}