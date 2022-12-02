//
// Created by henry on 2022-12-01.
//

#include "lb_strategy/pool.h"

namespace loadbalancer {

const backend &backend_pool::register_backend(const config::host &m) {
    using namespace boost::asio;
    boost::system::error_code ec;
    ip::address listen_addr = ip::address::from_string(std::get<0>(m), ec);
    if (ec) {
        throw errors::config_error("Failed to parse member ip: " + std::get<0>(m));
    }

    auto [iter, inserted] = members.emplace(backend(listen_addr, std::get<1>(m)), false);
    if (!inserted) throw errors::config_error("Cannot register member: " + std::get<0>(m));
    return (*iter).first;
}

const typename backend_pool::member_list_type &
backend_pool::get_members() const {
    return members;
}

void backend_pool::backend_up(const backend &backend) noexcept {
    members[backend] = true;
}

void backend_pool::backend_down(const backend &backend) noexcept {
    members[backend] = false;
}

bool backend_cmp::operator()(const backend &b1, const backend &b2) const noexcept {
    if (b1.address != b2.address)
        return b1.address < b2.address;
    return b1.port < b2.port;
}

}
