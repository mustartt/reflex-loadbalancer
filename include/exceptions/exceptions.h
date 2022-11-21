//
// Created by henry on 2022-11-21.
//

#ifndef LOADBALANCER_INCLUDE_EXCEPTIONS_EXCEPTIONS_H_
#define LOADBALANCER_INCLUDE_EXCEPTIONS_EXCEPTIONS_H_

#include <exception>
#include <stdexcept>

namespace loadbalancer::errors {

class config_error : public std::runtime_error {
  public:
    explicit config_error(const std::string &arg) : runtime_error(arg) {}
};

}

#endif //LOADBALANCER_INCLUDE_EXCEPTIONS_EXCEPTIONS_H_
