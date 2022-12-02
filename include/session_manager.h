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

#include "sessions/session.h"
#include "configuration/config.h"

namespace loadbalancer {

class lb_strategy;
class session_factory;

class session_manager {
  public:
    explicit session_manager(boost::asio::io_context &context,
                             lb_strategy *strategy,
                             session_factory *factory,
                             const config::config_property &property);
    ~session_manager() = default;

    friend class tcp_session;
  public:
    void start();
    void drain();

    void acquire(std::shared_ptr<session> &conn);
    void release(boost::uuids::uuid conn_id);

    lb_strategy *get_strategy() { return strategy; }

  private:
    void report_stats();

  private:
    size_t max_session_count;
    std::map<boost::uuids::uuid, std::shared_ptr<session>> sessions;

    lb_strategy *strategy;
    session_factory *factory;

    boost::asio::io_context::strand strand;
    boost::asio::deadline_timer timer;
};

}

#endif //LOADBALANCER_SRC_SESSION_MANAGER_H_
