/*!
    \file ws_chat_client.cpp
    \brief WebSocket chat client example
    \author Ivan Shynkarenka
    \date 22.05.2019
    \copyright MIT License
*/

#include "asio_service.h"

#include "server/ws/ws_client.h"
#include "threads/thread.h"

#include <atomic>
#include <iostream>

class ChatClient : public CppServer::WS::WSClient
{
public:
    using CppServer::WS::WSClient::WSClient;

    void DisconnectAndStop()
    {
        _stop = true;
        DisconnectAsync();
        while (IsConnected())
            CppCommon::Thread::Yield();
    }

protected:
    void onWSConnecting(CppServer::HTTP::HTTPRequest& request) override
    {
        request.SetHeader("Host", "echo.websocket.org");
        request.SetHeader("Origin", "http://websocket.org");
    }

    void onWSConnected(const CppServer::HTTP::HTTPResponse& response) override
    {
        std::cout << "Chat WebSocket client connected a new session with Id " << id() << std::endl;
    }

    void onWSDisconnected() override
    {
        std::cout << "Chat WebSocket client disconnected a session with Id " << id() << std::endl;
    }

    void onDisconnected() override
    {
        WSClient::onDisconnected();

        // Wait for a while...
        CppCommon::Thread::Sleep(1000);

        // Try to connect again
        if (!_stop)
            ConnectAsync();
    }

    void onReceived(const void* buffer, size_t size) override
    {
        WSClient::onReceived(buffer, size);
        std::cout << "Incoming: " << std::string((const char*)buffer, size) << std::endl;
    }

    void onError(int error, const std::string& category, const std::string& message) override
    {
        std::cout << "Chat WebSocket client caught an error with code " << error << " and category '" << category << "': " << message << std::endl;
    }

private:
    std::atomic<bool> _stop{false};
};

int main(int argc, char** argv)
{
    // WebSocket server address
    std::string address = "174.129.224.73";
    if (argc > 1)
        address = argv[1];

    // WebSocket server port
    int port = 80;
    if (argc > 2)
        port = std::atoi(argv[2]);

    std::cout << "WebSocket server address: " << address << std::endl;
    std::cout << "WebSocket server port: " << port << std::endl;

    std::cout << std::endl;

    // Create a new Asio service
    auto service = std::make_shared<AsioService>();

    // Start the Asio service
    std::cout << "Asio service starting...";
    service->Start();
    std::cout << "Done!" << std::endl;

    // Create a new WebSocket chat client
    auto client = std::make_shared<ChatClient>(service, address, port);

    // Connect the client
    std::cout << "Client connecting...";
    client->ConnectAsync();
    std::cout << "Done!" << std::endl;

    std::cout << "Press Enter to stop the client or '!' to reconnect the client..." << std::endl;

    // Perform text input
    std::string line;
    while (getline(std::cin, line))
    {
        if (line.empty())
            break;

        // Reconnect the client
        if (line == "!")
        {
            std::cout << "Client reconnecting...";
            client->ReconnectAsync();
            std::cout << "Done!" << std::endl;
            continue;
        }

        // Send the entered text to the chat server
        client->SendAsync(line);
    }

    // Disconnect the client
    std::cout << "Client disconnecting...";
    client->DisconnectAndStop();
    std::cout << "Done!" << std::endl;

    // Stop the Asio service
    std::cout << "Asio service stopping...";
    service->Stop();
    std::cout << "Done!" << std::endl;

    return 0;
}
