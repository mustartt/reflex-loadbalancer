//
// Created by henry on 2022-11-21.
//

#include <boost/uuid/uuid_io.hpp>

#include "session.h"
#include "session_manager.h"
#include "logger/logger.h"

namespace loadbalancer {

using namespace boost::asio;

void session_manager::start() {
    listen();
}

void session_manager::drain() {

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
              auto target = ip::tcp::endpoint(ip::tcp::v4(), 8000);
              auto conn = std::make_shared<session>(
                  this, conn_id, context.context(), std::move(client)
              );
              conn->connect_to_member(target);
              acquire_conn(conn);
          } else {
              listen();
          }
        }
    );
}

std::string session_manager::get_connections() const {
    std::string content;
    for (const auto &[key, value]: sessions) {
        content += key;
        content += " ";
    }
    return content;
}

void session_manager::acquire_conn(std::shared_ptr<session> &conn) {
    post(context.context(), context.wrap([this, conn]() {
      std::lock_guard guard(sessions_lock);
      sessions[boost::uuids::to_string(conn->get_connection_id())] = conn;
      if (sessions.size() < max_session_count) { listen(); }
    }));
}

void session_manager::release(boost::uuids::uuid conn_id) {
    post(
        context.context(),
        context.wrap(
            [&, this]() {
              std::lock_guard guard(sessions_lock);
              auto id = boost::uuids::to_string(conn_id);
              if (sessions.contains(id)) {
                  BOOST_LOG_SEV(logger::slg, logger::info)
                      << id << " - Releasing connection";
                  sessions.erase(boost::uuids::to_string(conn_id));
              }
              if (sessions.size() < max_session_count) listen();
            }
        )
    );
}

}
