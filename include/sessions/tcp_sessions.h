//
// Created by henry on 2022-11-30.
//

#ifndef LOADBALANCER_INCLUDE_SESSIONS_TCP_SESSIONS_H_
#define LOADBALANCER_INCLUDE_SESSIONS_TCP_SESSIONS_H_

#include "session.h"

namespace loadbalancer {

using namespace boost::asio;

class tcp_session : public session,
                    public std::enable_shared_from_this<tcp_session> {
  public:
    tcp_session(session_manager *manager,
                boost::uuids::uuid id,
                boost::asio::io_context &context,
                size_t buffer_size,
                boost::posix_time::seconds timeout,
                ip::tcp::socket client
    );
    ~tcp_session() override;

    friend class session_manager;
  public:
    void connect(const ip::address &address, uint16_t port) override;
    void shutdown() override;
    void close() override;
  private:
    void start_transfer();
    void refresh_client_timeout();

    void read_from_client();
    void write_to_server(std::size_t n);
    void read_from_server();
    void write_to_client(std::size_t n);

  private:
    io_context::strand strand_client;
    io_context::strand strand_member;

    ip::tcp::socket client;
    ip::tcp::socket member;
};

}

#endif //LOADBALANCER_INCLUDE_SESSIONS_TCP_SESSIONS_H_
