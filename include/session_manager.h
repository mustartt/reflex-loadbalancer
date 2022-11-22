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

namespace loadbalancer {

class session;

class session_manager {
  public:
    explicit session_manager(boost::asio::io_context &context,
                             boost::asio::ip::tcp::endpoint &endpoint,
                             bool reuse_address = true,
                             size_t maxconn = 1000)
        : max_session_count(maxconn), context(context), acceptor(context), gen_uuid() {
        acceptor.open(endpoint.protocol());
        acceptor.set_option(boost::asio::socket_base::reuse_address(reuse_address));
        acceptor.bind(endpoint);
        acceptor.listen();
    }
    ~session_manager() = default;

    friend class session;
  public:
    void start();
    void drain();

  private:
    void listen();
    void release(boost::uuids::uuid conn_id);

    void acquire_conn(std::shared_ptr<session> &conn);

    std::string get_connections() const;
  private:
    boost::uuids::random_generator gen_uuid;

    std::mutex sessions_lock;
    std::map<std::string, std::shared_ptr<session>> sessions;

    size_t max_session_count;
    boost::asio::io_context::strand context;
    boost::asio::ip::tcp::acceptor acceptor;
};

}

#endif //LOADBALANCER_SRC_SESSION_MANAGER_H_
