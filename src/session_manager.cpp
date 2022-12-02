//
// Created by henry on 2022-11-21.
//

#include <boost/uuid/uuid_io.hpp>

#include "session_manager.h"
#include "sessions/session_factory.h"
#include "logger/logger.h"
#include "lb_strategy/lb_strategy.h"

namespace loadbalancer {

using namespace boost::asio;

session_manager::session_manager(io_context &context,
                                 lb_strategy *strategy,
                                 session_factory *factory,
                                 const config::config_property &property)
    : max_session_count(property.server.config.maxconn),
      strategy(strategy), factory(factory), strand(context),
      timer(context, boost::posix_time::seconds(5)) {}

void session_manager::start() {
    report_stats();
    factory->listen(this);
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

void session_manager::drain() {
    post(strand, [this]() {
        factory->close(this);
        for (auto &[id, conn]: sessions) {
            conn->shutdown();
        }
    });
}

void session_manager::acquire(std::shared_ptr<session> &conn) {
    post(strand, [conn, this]() {
        sessions[conn->get_connection_id()] = conn;
        if (sessions.size() < max_session_count) {
            factory->listen(this);
        }
    });
}

void session_manager::release(boost::uuids::uuid conn_id) {
    post(strand, [conn_id, this]() {
        bool hasCapacity = sessions.size() < max_session_count;
        sessions.erase(conn_id); // release session lifetime
        if (!hasCapacity && sessions.size() < max_session_count) {
            factory->listen(this);
        }
    });
}

}
