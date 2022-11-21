//
// Created by henry on 2022-11-20.
//

#ifndef LOADBALANCER_SRC_LOGGER_LOGGER_H_
#define LOADBALANCER_SRC_LOGGER_LOGGER_H_

#include <boost/log/sources/basic_logger.hpp>
#include <boost/log/sources/severity_feature.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

namespace loadbalancer::logger {

enum severity_level {
  debug,
  info,
  notification,
  warning,
  error,
  critical
};

extern boost::log::sources::severity_logger<severity_level> slg;

}

#endif //LOADBALANCER_SRC_LOGGER_LOGGER_H_
