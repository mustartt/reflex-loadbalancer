//
// Created by henry on 2022-11-21.
//

#include <boost/uuid/uuid_io.hpp>

#include "session.h"
#include "session_manager.h"
#include "lb_strategy.h"
#include "logger/logger.h"
#include "exceptions/exceptions.h"

namespace loadbalancer {

using namespace boost::asio;

session_manager::session_manager(io_context &context, ip::tcp::endpoint &endpoint,
                                 lb_strategy<boost::asio::ip::tcp::endpoint> *strategy,
                                 bool reuse_address, size_t maxconn)
    : gen_uuid(), max_session_count(maxconn),
      strategy(strategy), strand(context), acceptor(context),
      timer(context, boost::posix_time::seconds(5)) {
    acceptor.open(endpoint.protocol());
    acceptor.set_option(boost::asio::socket_base::reuse_address(reuse_address));
    acceptor.bind(endpoint);
    boost::system::error_code ec;
    acceptor.listen(10000, ec);
    if (ec) {
        BOOST_LOG_SEV(logger::slg, logger::critical)
            << "Session Manager cannot listen " << ec.message();
        throw errors::config_error(ec.message());
    }
}

void session_manager::start() {
    report_stats();
    listen();
}

void session_manager::report_stats() {
    timer.expires_from_now(boost::posix_time::seconds(5));
    timer.async_wait([this](const boost::system::error_code &ec) {
        if (ec) return;
        post(strand, [this]() {
            BOOST_LOG_SEV(logger::slg, logger::info)
                << "Current Connection Count: "
                << sessions.size() << "/" << max_session_count;
        });
        report_stats();
    });
}

void session_manager::listen() {

    BOOST_LOG_SEV(logger::slg, logger::info)
        << "Accepting connection: "
        << "session_count = " << sessions.size()
        << "/" << max_session_count;

    acceptor.async_accept(
        [this](const boost::system::error_code &ec, ip::tcp::socket client) {
            auto conn_id = gen_uuid();
            if (!ec) {
                if (!strategy->is_backend_alive()) {
                    client.close();
                } else {
                    auto conn = std::make_shared<session>(
                        this, conn_id, strand.context(), std::move(client)
                    );
                    conn->connect_to_member(strategy->next_backend());
                    acquire(conn);
                }
            } else {
                listen();
            }
        }
    );
}

void session_manager::drain() {
    boost::system::error_code ec;
    acceptor.close(ec);
    post(strand, [this]() {
        for (auto &[id, conn]: sessions) {
            conn->shutdown();
        }
    });
}

void session_manager::acquire(std::shared_ptr<session> &conn) {
    post(strand, [conn, this]() {
        sessions[conn->get_connection_id()] = conn;
        if (sessions.size() < max_session_count)
            listen();
    });
}

void session_manager::release(boost::uuids::uuid conn_id) {
    post(strand, [conn_id, this]() {
        bool hasCapacity = sessions.size() < max_session_count;
        sessions.erase(conn_id); // release session lifetime
        if (!hasCapacity && sessions.size() < max_session_count)
            listen(); // requeue listen if more connections
    });
}

std::vector<std::string> session_manager::get_connections() const {
    std::vector<std::string> conns;
    for (const auto &[key, value]: sessions) {
        std::string content;
        content += boost::uuids::to_string(key);
        content += " (";
        content += std::to_string(value->client.is_open());
        content += ",";
        content += std::to_string(value->member.is_open());
        content += ")";
        conns.push_back(std::move(content));
    }
    return conns;
}

}
