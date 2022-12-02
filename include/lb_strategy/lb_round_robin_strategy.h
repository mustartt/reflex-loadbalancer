//
// Created by henry on 2022-11-30.
//

#ifndef LOADBALANCER_INCLUDE_LB_STRATEGY_LB_ROUND_ROBIN_STRATEGY_H_
#define LOADBALANCER_INCLUDE_LB_STRATEGY_LB_ROUND_ROBIN_STRATEGY_H_

#include "lb_strategy.h"

namespace loadbalancer {

class lb_round_robin_strategy : public lb_strategy {
  public:
    using rr_state_type = typename backend_pool::member_list_type::const_iterator;
  public:
    explicit lb_round_robin_strategy(backend_pool &pool)
        : lb_strategy(pool),
          rr_state(pool.get_members().cbegin()) {}
  public:
    const backend &next_backend() override;
    void client_connect(backend &) noexcept override {}
    void client_disconnect(backend &) noexcept override {}
  private:
    rr_state_type rr_state;
};

const backend &lb_round_robin_strategy::next_backend() {
    const auto &members = lb_strategy::pool.get_members();
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

}

#endif //LOADBALANCER_INCLUDE_LB_STRATEGY_LB_ROUND_ROBIN_STRATEGY_H_
