//
// Created by henry on 2022-11-23.
//

#ifndef LOADBALANCER_SRC_LB_STRATEGY_H_
#define LOADBALANCER_SRC_LB_STRATEGY_H_

#include <optional>
#include <iostream>
#include <boost/asio.hpp>

#include "configuration/config.h"
#include "exceptions/exceptions.h"

namespace loadbalancer {

template<class EndpointType = boost::asio::ip::tcp::endpoint>
class backend_pool final {
  public:
    using endpoint_type = EndpointType;
    using member_list_type = std::map<endpoint_type, bool>;
  public:
    backend_pool() = default;
  public:
    const endpoint_type &register_backend(const config::host &m);
    const member_list_type &get_members() const;

    void backend_up(const endpoint_type &) noexcept;
    void backend_down(const endpoint_type &) noexcept;

  private:
    member_list_type members;
};

template<class EndpointType>
const typename backend_pool<EndpointType>::endpoint_type &
backend_pool<EndpointType>::register_backend(const config::host &m) {
    using namespace boost::asio;
    boost::system::error_code ec;
    ip::address listen_addr = ip::address::from_string(std::get<0>(m), ec);
    if (ec) {
        throw errors::config_error("Failed to parse member ip: " + std::get<0>(m));
    }

    auto [iter, inserted] = members.emplace(endpoint_type(listen_addr, std::get<1>(m)), false);
    if (!inserted) throw errors::config_error("Cannot register member: " + std::get<0>(m));
    return (*iter).first;
}

template<class EndpointType>
const typename backend_pool<EndpointType>::member_list_type &
backend_pool<EndpointType>::get_members() const {
    return members;
}

template<class EndpointType>
void backend_pool<EndpointType>::backend_up(const endpoint_type &backend) noexcept {
    members[backend] = true;
}

template<class EndpointType>
void backend_pool<EndpointType>::backend_down(const endpoint_type &backend) noexcept {
    members[backend] = false;
}

template<class EndpointType>
class lb_strategy {
  public:
    using pool_type = backend_pool<EndpointType>;
    using endpoint_type = typename pool_type::endpoint_type;
  public:
    explicit lb_strategy(backend_pool<endpoint_type> &pool) : pool(pool) {}
    virtual ~lb_strategy() = default;
  public:
    virtual const endpoint_type &next_backend() = 0;
    virtual bool is_backend_alive() noexcept;

    virtual void client_connect(endpoint_type &) noexcept = 0;
    virtual void client_disconnect(endpoint_type &) noexcept = 0;

    backend_pool<endpoint_type> get_pool() noexcept { return pool; };

  protected:
    backend_pool<endpoint_type> &pool;
};

template<class EndpointType>
bool lb_strategy<EndpointType>::is_backend_alive() noexcept {
    const auto members = pool.get_members();
    return std::ranges::any_of(members.begin(), members.end(), [](const auto &member) {
        return member.second;
    });
}

template<class EndpointType>
class lb_round_robin_strategy : public lb_strategy<EndpointType> {
  public:
    using pool_type = backend_pool<EndpointType>;
    using endpoint_type = typename pool_type::endpoint_type;
    using rr_state_type = typename pool_type::member_list_type::const_iterator;
  public:
    explicit lb_round_robin_strategy(pool_type &pool)
        : lb_strategy<EndpointType>(pool),
          rr_state(pool.get_members().cbegin()) {}
  public:
    const endpoint_type &next_backend() override;
    void client_connect(endpoint_type &) noexcept override {}
    void client_disconnect(endpoint_type &) noexcept override {}
  private:
    rr_state_type rr_state;
};

template<class EndpointType>
const typename lb_round_robin_strategy<EndpointType>::endpoint_type &
lb_round_robin_strategy<EndpointType>::next_backend() {
    const auto &members = lb_strategy<endpoint_type>::pool.get_members();
    for (size_t i = 0; i < members.size(); ++i) {
        ++rr_state;
        if (rr_state == members.cend())
            rr_state = members.cbegin();
        if ((*rr_state).second) {
            return (*rr_state).first;
        }
    }
    rr_state = members.cbegin();
    throw std::runtime_error("Backend is down");
}

} // loadbalancer

#endif //LOADBALANCER_SRC_LB_STRATEGY_H_
