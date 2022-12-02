//
// Created by henry on 2022-11-30.
//

#ifndef LOADBALANCER_INCLUDE_SESSIONS_SESSION_FACTORY_H_
#define LOADBALANCER_INCLUDE_SESSIONS_SESSION_FACTORY_H_

#include <boost/asio.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include "session_manager.h"
#include "lb_strategy/lb_strategy.h"
#include "configuration/config.h"
#include "exceptions/exceptions.h"

namespace loadbalancer {

class session_manager;

class session_factory {
  public:
    using address_type = boost::asio::ip::address;
    using port_type = uint16_t;
  public:
    session_factory(address_type bind_addr, port_type port,
                    boost::asio::io_context &context,
                    const config::config_property &property)
        : bind_addr(std::move(bind_addr)), port(port),
          context(context), property(property), gen_uuid() {}
    virtual ~session_factory() = default;

  public:
    virtual void listen(session_manager*) = 0;
    virtual void close(session_manager*) = 0;

  protected:
    boost::uuids::uuid generate_uuid() { return gen_uuid(); }

  protected:
    address_type bind_addr;
    port_type port;
    boost::asio::io_context &context;
    const config::config_property &property;

  private:
    boost::uuids::random_generator gen_uuid;
};

class udp_session_factory : public session_factory {};
class ssl_session_factory : public session_factory {};

}

#endif //LOADBALANCER_INCLUDE_SESSIONS_SESSION_FACTORY_H_
