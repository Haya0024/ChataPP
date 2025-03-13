#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <iostream>
#include <mutex>
#include <set>
#include <thread>

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = boost::beast::websocket;

using tcp = asio::ip::tcp;
using error_code = boost::system::error_code;

class ChatSession : public std::enable_shared_from_this<ChatSession> {
    
    websocket::stream<tcp::socket> ws_;
    std::set<std::shared_ptr<ChatSession>>& sessions_;
    std::mutex& sessions_mutex_;
    beast::flat_buffer buffer_;

    public:
        ChatSession(tcp::socket socket, std::set<std::shared_ptr<ChatSession>>& sessions, std::mutex& sessions_mutex)
            : ws_(std::move(socket)), sessions_(sessions), sessions_mutex_(sessions_mutex_)
        {
        }

        void start() {
            ws_.async_accept([self = shared_from_this()](error_code ec) {
                if (!ec) {
                    self->join();
                    self->read();
                }
            });
        }

        void join() {
            std::lock_guard<std::mutex> lock(sessions_mutex_);
            sessions_.insert(shared_from_this());
        }
        
        void leave() {
            std::lock_guard<std::mutex> lock(sessions_mutex_);
            sessions_.erase(shared_from_this());
        }

};


class WebSocketServer {

    asio::io_context& io_context_;
    tcp::acceptor acceptor_;
    std::set<std::shared_ptr<ChatSession>> sessions_;
    std::mutex sessions_mutex_;
    
    public:
        WebSocketServer(asio::io_context& ioc, short port)
            : io_context_(ioc), acceptor_(ioc, tcp::endpoint(tcp::v4(), port)) {
        }

        void start() {
            do_accept();
        }
    
    private:
        void do_accept() {
            acceptor_.async_accept([this](error_code ec, tcp::socket socket) {
                if (!ec) {
                    std::make_shared<ChatSession>(std::move(socket))->start();
                } else {
                    std::cerr << "Accept error: " << ec.message() << std::endl;
                }
                do_accept();
            });
        }
};

int main() {
    asio::io_context ioc;
    WebSocketServer server(ioc, 8080);
    server.start();
    ioc.run();
    return 0;
}
