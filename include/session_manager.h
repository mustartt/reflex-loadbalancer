//
// Created by henry on 2022-11-21.
//

#ifndef LOADBALANCER_SRC_SESSION_MANAGER_H_
#define LOADBALANCER_SRC_SESSION_MANAGER_H_

#include <map>
#include <memory>

#include <boost/asio.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/functional/hash.hpp>

#include "session.h"

namespace loadbalancer {

class session_manager {
  public:
    explicit session_manager(boost::asio::io_context &context,
                             boost::asio::ip::tcp::endpoint &endpoint,
                             bool reuse_address = true,
                             size_t maxconn = 1000);
    ~session_manager() = default;

    friend class session;
  public:
    void start();
    void drain();

  private:
    void listen();
    void release(boost::uuids::uuid conn_id);
    void acquire(std::shared_ptr<session> &conn);

    void report_stats();

    std::vector<std::string> get_connections() const;
  private:
    boost::uuids::random_generator gen_uuid;

    size_t max_session_count;
    std::map<boost::uuids::uuid, std::shared_ptr<session>> sessions{};

    boost::asio::io_context::strand strand;
    boost::asio::ip::tcp::acceptor acceptor;
    boost::asio::deadline_timer timer;
};

}

#endif //LOADBALANCER_SRC_SESSION_MANAGER_H_
