//
// Created by henry on 2022-11-23.
//

#include "lb_strategy/lb_strategy.h"

namespace loadbalancer {

bool lb_strategy::is_backend_alive() noexcept {
    const auto members = pool.get_members();
    return std::ranges::any_of(
        members.begin(), members.end(),
        [](const auto &member) -> bool {
            return member.second;
        }
    );
}

} // loadbalancer
