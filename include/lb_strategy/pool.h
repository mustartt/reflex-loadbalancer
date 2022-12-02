//
// Created by henry on 2022-11-30.
//

#ifndef LOADBALANCER_INCLUDE_LB_STRATEGY_POOL_H_
#define LOADBALANCER_INCLUDE_LB_STRATEGY_POOL_H_

#include <optional>
#include <iostream>
#include <boost/asio.hpp>
#include <utility>

#include "configuration/config.h"
#include "exceptions/exceptions.h"

namespace loadbalancer {

class backend final {
  public:
    using address_type = boost::asio::ip::address;
    using port_type = uint16_t;
  public:
    backend(address_type address, port_type port)
        : address(std::move(address)), port(port) {}

  public:
    address_type address;
    port_type port;
};

struct backend_cmp {
    bool operator()(const backend &b1, const backend &b2) const noexcept;
};

class backend_pool final {
  public:
    using member_list_type = std::map<backend, bool, backend_cmp>;
  public:
    backend_pool() = default;
  public:
    const backend &register_backend(const config::host &m);
    const member_list_type &get_members() const;

    void backend_up(const backend &) noexcept;
    void backend_down(const backend &) noexcept;

  private:
    member_list_type members;
};

}
#endif //LOADBALANCER_INCLUDE_LB_STRATEGY_POOL_H_
