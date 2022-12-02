//
// Created by henry on 2022-11-20.
//

#ifndef LOADBALANCER_SRC_CONFIGURATION_CONFIG_H_
#define LOADBALANCER_SRC_CONFIGURATION_CONFIG_H_

#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>
#include <boost/program_options.hpp>
#include <logger/logger.h>

namespace loadbalancer::config {

enum class protocol {
  tcp, udp, http
};

enum class loadbalance_strategy {
  random, roundrobin, weighted_rr, leastconn
};

using host = std::tuple<std::string, uint16_t>;

class backend_property {
  public:
    loadbalance_strategy strategy;
    uint16_t default_port;
    std::vector<host> members;
};

class config_property {
  public:
    class server_property {
      public:
        class server_config_property {
          public:
            uint32_t maxconn;
            uint32_t backlog;
            uint32_t transfer_buffer_bytes;
            uint32_t socket_queue_depth_bytes;
            bool reuse_address;
            uint32_t thread_count;
        };
        class server_log_property {
          public:
            std::string log_file;
            logger::severity_level log_level;
            uint32_t rotate_bytes;
            std::string rotate_duration;
            uint32_t max_files;
        };
      public:
        std::string bind_addr;
        uint16_t port;
        server_config_property config;
        server_log_property log;
    };
    class backend_property {
      public:
        loadbalance_strategy strategy;
        uint32_t timeout;
        std::vector<host> members;
    };
  public:
    server_property server;
    backend_property backend;
};

void load_yaml_file_config(config_property &property, const std::string &file);
void load_command_line_config(config_property &property, boost::program_options::variables_map &vm);

}

#endif //LOADBALANCER_SRC_CONFIGURATION_CONFIG_H_
