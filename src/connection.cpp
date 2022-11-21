#include <boost/asio.hpp>
#include <memory>
#include <iostream>
#include <array>

#include "logger/logger.h"

namespace loadbalancer {

using namespace boost::asio;

class session : public std::enable_shared_from_this<session> {
  public:
    using buffer_type = std::array<char, 4096>;
    explicit session(io_context &ctx, ip::tcp::socket client)
        : seq_ctx(ctx),
          client(std::move(client)),
          member(client.get_executor()) {
        client_buffer = std::make_unique<buffer_type>();
        member_buffer = std::make_unique<buffer_type>();
        BOOST_LOG_SEV(logger::slg, logger::info) << "Connection Accepted";
    }
    ~session() {
        BOOST_LOG_SEV(logger::slg, logger::info) << "Closing Connection";
    }
  public:
    void connect_to_member(ip::tcp::endpoint &endpoint) {
        auto self = shared_from_this();
        member.async_connect(endpoint, [self](const boost::system::error_code &ec) {
          if (ec) {
              BOOST_LOG_SEV(logger::slg, logger::warning)
                  << "Failed to connect to pool member: "
                  << ec.message();
          } else {
              BOOST_LOG_SEV(logger::slg, logger::info) << "Connection Backend Established";
              self->start_transfer();
          }
        });
    }
  private:
    void start_transfer() {
        read_from_client();
        read_from_server();
    }

    void shutdown() {
        if (client.is_open()) { client.shutdown(ip::tcp::socket::shutdown_both); }
        if (member.is_open()) { member.shutdown(ip::tcp::socket::shutdown_both); }
    }

    void read_from_client() {
        auto self = shared_from_this();
        client.async_read_some(
            buffer(*self->client_buffer),
            [self](std::error_code error, std::size_t n) {
              if (!error) {
                  self->write_to_server(n);
              }
            }
        );
    }

    void write_to_server(std::size_t n) {
        auto self = shared_from_this();
        async_write(
            member,
            buffer(*self->client_buffer, n),
            [self](std::error_code ec, std::size_t _) {
              if (!ec) {
                  self->read_from_client();
              }
            }
        );
    }

    void read_from_server() {
        auto self = shared_from_this();
        member.async_read_some(
            buffer(*self->member_buffer),
            [self](std::error_code error, std::size_t n) {
              if (!error) {
                  self->write_to_client(n);
              }
            }
        );
    }

    void write_to_client(std::size_t n) {
        auto self = shared_from_this();
        async_write(
            client,
            buffer(*self->member_buffer, n),
            [self](std::error_code ec, std::size_t _) {
              if (!ec) {
                  self->read_from_server();
              }
            });
    }
  private:
    bool hasFlushedSocket = false;
    ip::tcp::socket client;
    ip::tcp::socket member;
    std::unique_ptr<buffer_type> client_buffer;
    std::unique_ptr<buffer_type> member_buffer;
    io_context::strand seq_ctx;
};

class connection_manager {
  public:
    connection_manager(io_context &ctx, ip::tcp::acceptor &acceptor, ip::tcp::endpoint target)
        : ctx(ctx), acceptor(acceptor), target(std::move(target)) {}
  public:
    void start() {
        BOOST_LOG_SEV(logger::slg, logger::info) << "Accepting Connections";
        listen();
    }

  private:
    void listen() {
        acceptor.async_accept(
            [this](const boost::system::error_code &ec, ip::tcp::socket client) {
              if (!ec) {
                  std::make_shared<session>(ctx, std::move(client))->connect_to_member(target);
              }
              listen();
            });
    }
  private:
    ip::tcp::acceptor &acceptor;
    ip::tcp::endpoint target;
    io_context &ctx;
};

}
//
//int main() {
//    using namespace boost::asio;
//
//    io_context ctx;
//
//    ip::tcp::endpoint endpoint(ip::tcp::v4(), 4000);
//    ip::tcp::acceptor acceptor(ctx);
//    acceptor.open(endpoint.protocol());
//    acceptor.set_option(socket_base::reuse_address(true));
//    acceptor.bind(endpoint);
//    acceptor.listen();
//
//    ip::tcp::endpoint target(ip::tcp::v4(), 8000);
//
//    loadbalancer::connection_manager manager(ctx, acceptor, std::move(target));
//    manager.start();
//
//    std::vector<std::thread> pool;
//
//    for (int i = 0; i < 4; ++i) pool.emplace_back([&]() { ctx.run(); });
//    for (auto &thread: pool) thread.join();
//
//    return 0;
//}


