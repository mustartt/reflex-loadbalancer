//
// Created by henry on 2022-11-23.
//

#ifndef LOADBALANCER_SRC_LB_STRATEGY_H_
#define LOADBALANCER_SRC_LB_STRATEGY_H_

#include <boost/asio.hpp>

#include "configuration/config.h"
#include "exceptions/exceptions.h"

namespace loadbalancer {

template<class EndpointType = boost::asio::ip::tcp::endpoint>
class backend_pool final {
  public:
    using endpoint_type = EndpointType;
  public:
    backend_pool() = default;
  public:
    void register_backend(const config::host &m);
    const std::vector<endpoint_type> &get_members() const;

  private:
    std::vector<endpoint_type> members;
};

template<class EndpointType>
void backend_pool<EndpointType>::register_backend(const config::host &m) {
    using namespace boost::asio;
    boost::system::error_code ec;
    ip::address listen_addr = ip::address::from_string(std::get<0>(m), ec);
    if (ec) {
        throw errors::config_error("Failed to parse member ip: " + std::get<0>(m));
    }
    members.emplace_back(listen_addr, std::get<1>(m));
}

template<class EndpointType>
const std::vector<EndpointType> &backend_pool<EndpointType>::get_members() const {
    return members;
}

template<class EndpointType>
class lb_strategy {
  public:
    using endpoint_type = typename backend_pool<EndpointType>::endpoint_type;
  public:
    explicit lb_strategy(backend_pool<endpoint_type> &pool) : pool(pool) {}
    virtual ~lb_strategy() = default;
  public:
    virtual endpoint_type &next_backend() = 0;
    virtual void client_disconnect(endpoint_type &) = 0;

  protected:
    backend_pool<endpoint_type> &pool;
};

} // loadbalancer

#endif //LOADBALANCER_SRC_LB_STRATEGY_H_
