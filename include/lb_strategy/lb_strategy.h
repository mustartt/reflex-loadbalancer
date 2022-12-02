//
// Created by henry on 2022-11-23.
//

#ifndef LOADBALANCER_SRC_LB_STRATEGY_H_
#define LOADBALANCER_SRC_LB_STRATEGY_H_

#include "pool.h"
#include "configuration/config.h"
#include "exceptions/exceptions.h"

namespace loadbalancer {

class lb_strategy {
  public:
    explicit lb_strategy(backend_pool &pool) : pool(pool) {}
    virtual ~lb_strategy() = default;
  public:
    virtual const backend &next_backend() = 0;
    virtual bool is_backend_alive() noexcept;

    virtual void client_connect(backend &) noexcept = 0;
    virtual void client_disconnect(backend &) noexcept = 0;

    backend_pool &get_pool() noexcept { return pool; };

  protected:
    backend_pool &pool;
};

} // loadbalancer

#endif //LOADBALANCER_SRC_LB_STRATEGY_H_
