//
// Created by henry on 2022-12-01.
//

#ifndef LOADBALANCER_INCLUDE_SESSIONS_TCP_SESSION_FACTORY_H_
#define LOADBALANCER_INCLUDE_SESSIONS_TCP_SESSION_FACTORY_H_

#include "session_factory.h"
#include "tcp_sessions.h"

namespace loadbalancer {

class session_manager;

class tcp_session_factory : public session_factory {
  public:
    tcp_session_factory(address_type bind_addr, port_type port,
                        boost::asio::io_context &context,
                        const config::config_property &property);
  public:
    void listen(session_manager*) override;
    void close(session_manager*) override;

  private:
    boost::asio::ip::tcp::acceptor acceptor;
};

}

#endif //LOADBALANCER_INCLUDE_SESSIONS_TCP_SESSION_FACTORY_H_
