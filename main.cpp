#include <iostream>

#include <boost/asio.hpp>
#include <boost/program_options.hpp>

#include "configuration/config.h"
#include "exceptions/exceptions.h"
#include "session_manager.h"
#include "lb_strategy/lb_strategy.h"
#include "lb_strategy/lb_round_robin_strategy.h"
#include "sessions/tcp_session_factory.h"

using namespace loadbalancer;

namespace po = boost::program_options;
using namespace boost::asio;

std::pair<po::options_description, po::positional_options_description> get_option_desc() {
    po::options_description generic("Generic Options");
    generic.add_options()
        ("help,h", "produce help message")
        ("version,v", "print version string")
        ("config-file,f", po::value<std::string>()->required(), "configuration file");

    po::options_description options("Loadbalancer Server Configuration");
    options.add_options()
        ("bind-addr,addr", po::value<std::string>(), "address to listen on")
        ("port,p", po::value<uint16_t>(), "port to listen on")
        ("maxconn,C", po::value<std::uint32_t>(), "maximum number of connections")
        ("backlog", po::value<std::uint32_t>(), "maximum number queued connections")
        ("transfer-buffer", po::value<std::uint32_t>(), "transfer buffer size in bytes")
        ("socket-queue-depth", po::value<std::uint32_t>(), "socket queue depth in bytes")
        ("reuse-address", "allow address reuse")
        ("thread,T", po::value<std::uint32_t>(), "number of threads to use");

    po::options_description logging("Logging Configuration");
    logging.add_options()
        ("log-file", po::value<std::string>(), "log filename prefix")
        ("log-rotate-bytes", po::value<uint32_t>(), "max size of a log")
        ("log-rotate-duration", po::value<std::string>(), "max duration a log should be written to")
        ("log-rotate-max-files", po::value<uint32_t>(), "maximum number of logs to keep")
        ("log-level", po::value<std::string>(), "level of logging to keep");

    po::options_description backend("Backend Configuration");
    backend.add_options()
        ("strategy", po::value<std::string>(), "load balance strategy")
        ("timeout", po::value<std::uint32_t>(), "connection timeout")
        ("hosts", po::value<std::vector<std::string>>(), "list of host:port");
    po::positional_options_description hosts;
    hosts.add("hosts", -1);

    po::options_description all("List of all commands");
    all.add(generic)
        .add(options)
        .add(logging)
        .add(backend);

    return {all, hosts};
}

void init(const config::config_property &) {
//    std::map<int, bool> test{{1, false}, {2, false}, {3, false}, {4, false}, {5, false}};
//    auto curr = test.cbegin();
//    for (int i = 0; i < 50; ++i) {
//        std::cout << "Before: " << (*curr).first << std::endl;
//        ++curr;
//        if (curr == test.cend()) {
//            curr = test.cbegin();
//        }
//        std::cout << "After: " << (*curr).first << std::endl;
//    }
}

int start(const config::config_property &config) {

    io_context context;

    boost::system::error_code ec;
    ip::address listen_addr = ip::address::from_string(config.server.bind_addr, ec);
    if (ec) throw errors::config_error(ec.message());

    backend_pool backend_pool;
    for (auto &member: config.backend.members) {
        auto &endpoint = backend_pool.register_backend(member);
        backend_pool.backend_up(endpoint);
    }
    auto loadbalancer_strategy = std::make_unique<lb_round_robin_strategy>(backend_pool);
    auto factory = std::make_unique<tcp_session_factory>(listen_addr, config.server.port, context, config);

    session_manager manager(context, loadbalancer_strategy.get(), factory.get(), config);
    manager.start();

    BOOST_LOG_SEV(logger::slg, logger::info)
        << "Starting asio::io_context with "
        << config.server.config.thread_count
        << " threads";

    signal_set signals(context, SIGINT);
    deadline_timer exit_timer(context);
    signals.async_wait([&](const boost::system::error_code &ec, int signal_number) {
        if (ec) {
            BOOST_LOG_SEV(logger::slg, logger::error)
                << "Signal Handler Error: " << ec.message();
        } else {
            BOOST_LOG_SEV(logger::slg, logger::info)
                << "Received Signal: (" << signal_number << ") starting shutdown";
            manager.drain();
            exit_timer.expires_from_now(boost::posix_time::seconds(5));
            exit_timer.async_wait([&](const boost::system::error_code &ec) {
                if (!ec) context.stop();
            });
        }
    });

    std::vector<std::thread> pool;
    for (size_t i = 0; i < config.server.config.thread_count; ++i) {
        pool.emplace_back([&]() { context.run(); });
    }

    BOOST_LOG_SEV(logger::slg, logger::info)
        << "Started thread pool";

    exit_timer.async_wait([&](const boost::system::error_code &) {
        context.stop();
    });

    for (auto &thread: pool)
        if (thread.joinable())
            thread.join();

    return 0;
}

int main(int argc, char *argv[]) {
    auto [all, hosts] = get_option_desc();
    try {
        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv)
                      .options(all)
                      .positional(hosts)
                      .run(), vm);
        if (vm.count("help")) {
            std::cout << all << std::endl;
            return 0;
        }
        if (vm.count("version")) {
            std::cout << "version: 0.0.1" << std::endl;
            return 0;
        }
        po::notify(vm); // raise exception for invalid argument

        config::config_property property; // load configuration
        config::load_yaml_file_config(property, vm["config-file"].as<std::string>());
        config::load_command_line_config(property, vm);

        BOOST_LOG_SEV(logger::slg, logger::notification)
            << "Starting loadbalancer " << socket_base::max_connections;

        init(property);
        return start(property);

    } catch (const po::error &err) {
        std::cout << "\nError when parsing commandline argument: "
                  << err.what() << "\n" << std::endl;
        std::cout << all << std::endl;
    }
}
