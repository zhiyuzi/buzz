#include <buzz/Buffer.h>
#include <buzz/Signal.h>
#include <buzz/EventLoop.h>
#include <buzz/TcpServer.h>
#include <buzz/InetAddress.h>
#include <buzz/TcpConnection.h>

class DaytimeServer
{
public:
    DaytimeServer(buzz::EventLoop* loop, buzz::InetAddress& host, int work_threads = 0)
        : m_server(loop, "daytime", host), m_work_threads(work_threads)
    {
        using namespace std::placeholders;

        m_server.OnStateChange(std::bind(&Daytime::OnSateChange, this, _1));
    }

    void Start()
    {
        m_server.Start(m_work_threads);
    }

    void OnSateChange(const buzz::TcpConnectionPtr& conn)
    {
        if (conn->GetState() == buzz::TcpConnection::kConnected) {
            conn->SendMessage(buzz::Timestamp::Now().ToFormattedString());
            conn->Shutdown();
        }
    }

private:
    buzz::TcpServer m_server;
    int       m_work_threads;
};

int main()
{
    buzz::EventLoop loop;
    buzz::Signal::Register(SIGINT, [&]() { loop.Exit(); });

    buzz::InetAddress listen_address(13);
    DaytimeServer echo_server(&loop, listen_address, 1);
    echo_server.Start();

    loop.Loop();

    return 0;
}