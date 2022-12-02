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

class session {
  public:
    using buffer_type = std::vector<char>;
  public:
    session(session_manager *manager,
            boost::uuids::uuid id,
            boost::asio::io_context &context,
            size_t buffer_size,
            boost::posix_time::seconds timeout);
    virtual ~session() = default;
  public:
    virtual void connect(const boost::asio::ip::address &address, uint16_t port) = 0;
    virtual void shutdown() = 0;
    virtual void close() = 0;

    boost::uuids::uuid get_connection_id() const { return id; }

    friend class session_manager;
  private:
    boost::uuids::uuid id;
  protected:
    session_manager *manager;
    buffer_type client_buffer;
    buffer_type member_buffer;
    boost::asio::io_context::strand session_strand;
    boost::asio::deadline_timer client_timeout;
    boost::posix_time::seconds timeout;
};

} // loadbalancer

#endif //LOADBALANCER_SRC_SESSION_H_
