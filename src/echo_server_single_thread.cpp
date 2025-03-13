#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
#include <iostream>
#include <memory>


namespace asio = boost::asio;
namespace beast = boost::beast;
namespace websocket = boost::beast::websocket;

using tcp = asio::ip::tcp;
using error_code = boost::system::error_code;

class ChatSession : public std::enable_shared_from_this<ChatSession>{

    websocket::stream<tcp::socket> ws_;
    beast::flat_buffer buffer_;
    
public:
    explicit ChatSession(tcp::socket&& socket)
        : ws_(std::move(socket)) {

    }
        
    void start() {
        std::shared_ptr<ChatSession> self = shared_from_this();
        ws_.async_accept([self](error_code ec) {
            if (!ec) {
                self->read();
            } else {
                std::cerr << "Error: " << ec.message() << std::endl;
            }
        });
    }
    
private:
    void read() {
        std::shared_ptr<ChatSession> self = shared_from_this();
        ws_.async_read(buffer_, [self](error_code ec, std::size_t bytes_transferred) {
            if (!ec) {
                std::string msg = beast::buffers_to_string(self->buffer_.data());
                std::cout << "Received: " << msg << std::endl;
                        
                self->write(msg);
            } else {
                std::cerr << "Error: " << ec.message() << std::endl;
                self->close();
            }
        });
    }

    void write(const std::string& msg) {
        std::shared_ptr<ChatSession> self = shared_from_this();
        ws_.async_write(asio::buffer(msg),
            [self](error_code ec, std::size_t) {
            if (!ec) {
                self->buffer_.consume(self->buffer_.size());
                self->read();
            } else {
                std::cerr << "Write Error: " << ec.message() << std::endl;
                self->close();
            }
        });
    }

    void close() {
        error_code ec;
        ws_.close(websocket::close_code::normal, ec);
        if (!ec) {
            std::cerr << "Close Error: " << ec.message() << std::endl;
        }
    }
};

class WebSocketServer {

    tcp::acceptor acceptor_;
    
public:
    WebSocketServer(asio::io_context& ioc, short port)
        : acceptor_(ioc, tcp::endpoint(tcp::v4(), port)) {
    }

    void start() {
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept(
            [this](error_code ec, tcp::socket socket) {
            if (!ec) {
                std::make_shared<ChatSession>(std::move(socket))->start();
                std::cout << "Connected!" << std::endl;
            } else {
                std::cerr << "Error: " << ec.message() << std::endl;
            }
            do_accept();
        });
    }
};

int main() {
    try {
        asio::io_context ioc;
        WebSocketServer server(ioc, 8080);
        server.start();
        ioc.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    
    return 0;
}
