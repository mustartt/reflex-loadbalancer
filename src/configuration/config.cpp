//
// Created by henry on 2022-11-20.
//

#include "configuration/config.h"

#include <optional>

#include "exceptions/exceptions.h"

namespace loadbalancer::config {

namespace yaml = YAML;
namespace po = boost::program_options;

logger::severity_level parse_severity_level(const std::string &s) {
    std::map<std::string, logger::severity_level> map{
        {"debug", logger::debug},
        {"info", logger::info},
        {"notification", logger::notification},
        {"warning", logger::warning},
        {"error", logger::error},
        {"critical", logger::critical},
    };
    if (!map.contains(s)) throw errors::config_error("Invalid log level: " + s);
    return map[s];
}

loadbalance_strategy parse_lb_strategy(const std::string &s) {
    std::map<std::string, loadbalance_strategy> map{
        {"random", loadbalance_strategy::random},
        {"roundrobin", loadbalance_strategy::roundrobin},
        {"weighted", loadbalance_strategy::weighted_rr},
        {"leastconn", loadbalance_strategy::leastconn},
    };
    if (!map.contains(s)) throw errors::config_error("Invalid loadbalance strategy: " + s);
    return map[s];
}

void load_yaml_file_config(config_property &property, const std::string &file) {
    auto root = yaml::LoadFile(file);

    property.server.bind_addr = root["server"]["bind_addr"].as<std::string>("0.0.0.0");
    property.server.port = root["server"]["bind_addr"].as<uint16_t>(4000);

    property.server.config.maxconn
        = root["server"]["config"]["maxconn"].as<uint32_t>(10000);
    property.server.config.transfer_buffer_bytes
        = root["server"]["config"]["transfer_buffer_bytes"].as<uint32_t>(4096);
    property.server.config.socket_queue_depth_bytes
        = root["server"]["config"]["socket_queue_depth_bytes"].as<uint32_t>(8192);
    property.server.config.reuse_address
        = root["server"]["config"]["reuse_address"].as<bool>(true);
    property.server.config.thread_count
        = root["server"]["config"]["thread_count"].as<uint32_t>(1);

    property.server.log.log_file
        = root["server"]["log"]["log_file"].as<std::string>("logs");
    property.server.log.log_level
        = parse_severity_level(root["server"]["log"]["log_level"].as<std::string>("info"));
    property.server.log.rotate_bytes
        = root["server"]["log"]["rotate_bytes"].as<uint32_t>(1048576);
    property.server.log.rotate_duration
        = root["server"]["log"]["rotate_duration"].as<std::string>("24h");
    property.server.log.max_files
        = root["server"]["log"]["max_files"].as<uint32_t>(10);

    property.backend.strategy
        = parse_lb_strategy(root["backend"]["strategy"].as<std::string>("roundrobin"));
    property.backend.default_port = root["backend"]["default_port"].as<uint16_t>(8000);

    const auto &member_node = root["backend"]["members"];
    if (!member_node.IsSequence()) throw errors::config_error("Invalid member property");

    for (const auto &member: member_node) {
        try {
            auto host_string = member.as<std::string>();
            size_t pos = host_string.find(":");
            if (pos != std::string::npos) {
                auto name = host_string.substr(0, pos);
                auto port = host_string.substr(pos + 1);
                try {
                    property.backend.members.emplace_back(name, boost::lexical_cast<std::uint16_t>(port));
                } catch (const boost::bad_lexical_cast &exp) {
                    throw errors::config_error("Cannot parse port: " + port);
                }
            } else {
                property.backend.members.emplace_back(host_string, property.backend.default_port);
            }
        } catch (const yaml::Exception &exp) {
            throw errors::config_error("Failed to parse member");
        }
    }
}

void load_command_line_config(config_property &property, po::variables_map &vm) {
    if (vm.count("bind-addr")) property.server.bind_addr = vm["bind-addr"].as<std::string>();
    if (vm.count("port")) property.server.port = vm["port"].as<uint16_t>();

    if (vm.count("maxconn")) property.server.config.maxconn = vm["maxconn"].as<uint32_t>();
    if (vm.count("transfer-buffer"))
        property.server.config.transfer_buffer_bytes = vm["transfer-buffer"].as<uint32_t>();
    if (vm.count("socket-queue-depth"))
        property.server.config.socket_queue_depth_bytes = vm["socket-queue-depth"].as<uint32_t>();
    if (vm.count("reuse-address")) property.server.config.reuse_address = true;
    if (vm.count("thread")) property.server.config.thread_count = vm["thread"].as<uint32_t>();

    if (vm.count("log-file")) property.server.log.log_file = vm["log-file"].as<std::string>();
    if (vm.count("log-rotate-bytes")) property.server.log.rotate_bytes = vm["log-rotate-bytes"].as<uint32_t>();
    if (vm.count("log-rotate-duration"))
        property.server.log.rotate_duration = vm["log-rotate-duration"].as<std::string>();
    if (vm.count("log-rotate-max-files")) property.server.log.max_files = vm["log-rotate-max-files"].as<uint32_t>();
    if (vm.count("log-level")) property.server.log.log_level = parse_severity_level(vm["log-level"].as<std::string>());

    if (vm.count("strategy")) property.backend.strategy = parse_lb_strategy(vm["strategy"].as<std::string>());
    if (vm.count("hosts")) {
        for (const auto &member: vm["hosts"].as<std::vector<std::string>>()) {
            try {
                size_t pos = member.find(':');
                if (pos != std::string::npos) {
                    auto name = member.substr(0, pos);
                    auto port = member.substr(pos + 1);
                    try {
                        property.backend.members.emplace_back(name, boost::lexical_cast<std::uint16_t>(port));
                    } catch (const boost::bad_lexical_cast &exp) {
                        throw errors::config_error("Cannot parse port: " + port);
                    }
                } else {
                    throw errors::config_error("Failed to parse member missing port");
                }
            } catch (const yaml::Exception &exp) {
                throw errors::config_error("Failed to parse member");
            }
        }
    }
}

}
