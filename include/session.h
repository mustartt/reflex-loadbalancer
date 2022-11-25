//
// Created by henry on 2022-11-21.
//

#ifndef LOADBALANCER_SRC_SESSION_H_
#define LOADBALANCER_SRC_SESSION_H_

#include <unordered_map>
#include <memory>

#include <boost/asio.hpp>
#include <boost/uuid/uuid.hpp>

namespace loadbalancer {

class session_manager;

class session : public std::enable_shared_from_this<session> {
  public:
    using buffer_type = std::array<char, 4096>;
  public:
    session(session_manager *manager,
            boost::uuids::uuid id,
            boost::asio::io_context &context,
            boost::asio::ip::tcp::socket client);
    ~session();
  public:
    void connect_to_member(const boost::asio::ip::tcp::endpoint &endpoint);
    void shutdown();

    boost::uuids::uuid get_connection_id() const { return id; }

    friend class session_manager;
  private:
    void start_transfer();
    void close();

    void read_from_client();
    void write_to_server(std::size_t n);
    void read_from_server();
    void write_to_client(std::size_t n);

    void push_client_timeout_deadline();

  private:
    session_manager *manager;
    boost::uuids::uuid id;
    boost::asio::io_context::strand strand;

    boost::asio::ip::tcp::socket client;
    boost::asio::ip::tcp::socket member;

    std::unique_ptr<buffer_type> client_buffer;
    std::unique_ptr<buffer_type> member_buffer;

    boost::asio::deadline_timer client_timeout;
};

} // loadbalancer

#endif //LOADBALANCER_SRC_SESSION_H_
