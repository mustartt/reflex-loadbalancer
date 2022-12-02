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

    property.server.bind_addr = root["server"]["bind_addr"].as<std::string>();
    property.server.port = boost::lexical_cast<uint16_t>(root["server"]["port"].as<int>());

    property.server.config.maxconn = boost::lexical_cast<uint32_t>(
        root["server"]["configs"]["maxconn"].as<int>()
    );
    property.server.config.backlog = boost::lexical_cast<uint32_t>(
        root["server"]["configs"]["backlog"].as<int>()
    );
    property.server.config.transfer_buffer_bytes = boost::lexical_cast<uint32_t>(
        root["server"]["configs"]["transfer_buffer_bytes"].as<int>()
    );
    property.server.config.socket_queue_depth_bytes = boost::lexical_cast<uint32_t>(
        root["server"]["configs"]["socket_queue_depth_bytes"].as<int>()
    );
    property.server.config.reuse_address = root["server"]["configs"]["reuse_address"].as<bool>();
    property.server.config.thread_count = boost::lexical_cast<uint32_t>(
        root["server"]["configs"]["thread_count"].as<int>()
    );

    property.server.log.log_file
        = root["server"]["logs"]["log_file"].as<std::string>();
    property.server.log.log_level
        = parse_severity_level(root["server"]["logs"]["log_level"].as<std::string>());
    property.server.log.rotate_bytes = boost::lexical_cast<uint32_t>(
        root["server"]["logs"]["rotate_bytes"].as<int>()
    );
    property.server.log.rotate_duration
        = root["server"]["logs"]["rotate_duration"].as<std::string>();
    property.server.log.max_files = boost::lexical_cast<uint32_t>(
        root["server"]["logs"]["max_files"].as<int>()
    );

    property.backend.strategy = parse_lb_strategy(root["backend"]["strategy"].as<std::string>());
    property.backend.timeout = boost::lexical_cast<uint32_t>(root["backend"]["timeout"].as<int>());

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
                    throw errors::config_error("Cannot parse port: " + host_string);
                }
            } else {
                throw errors::config_error("Failed to parse port: " + host_string);
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
    if (vm.count("backlog")) property.server.config.backlog = vm["backlog"].as<uint32_t>();
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
    if (vm.count("timeout")) property.backend.timeout = vm["timeout"].as<uint32_t>();
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
