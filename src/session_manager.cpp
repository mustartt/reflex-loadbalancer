//
// Created by henry on 2022-11-21.
//

#include <boost/uuid/uuid_io.hpp>

#include "session.h"
#include "session_manager.h"
#include "logger/logger.h"

namespace loadbalancer {

using namespace boost::asio;

session_manager::session_manager(io_context &context, ip::tcp::endpoint &endpoint,
                                 bool reuse_address, size_t maxconn)
    : max_session_count(maxconn), strand(context),
      acceptor(context), gen_uuid(),
      timer(context, boost::posix_time::seconds(5)) {
    acceptor.open(endpoint.protocol());
    acceptor.set_option(boost::asio::socket_base::reuse_address(reuse_address));
    acceptor.bind(endpoint);
    acceptor.listen();
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
            << sessions.size() << "/" << max_session_count
            << " [ " << get_connections() << "]";
      });
      report_stats();
    });
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
                  this, conn_id, strand.context(), std::move(client)
              );
              conn->connect_to_member(target);

              acquire(conn);
          } else {
              listen();
          }
        }
    );
}

void session_manager::acquire(std::shared_ptr<session> &conn) {
    post(strand, [conn, this]() {
      sessions[conn->get_connection_id()] = conn;
      if (sessions.size() < max_session_count)
          listen();
    });
}

void session_manager::release(boost::uuids::uuid conn_id) {
    post(strand, [&, this]() {
      BOOST_LOG_SEV(logger::slg, logger::info)
          << "Releasing Connection: "
          << to_string(conn_id);

      bool hasCapacity = sessions.size() < max_session_count;
      sessions.erase(conn_id); // release session lifetime
      if (!hasCapacity && sessions.size() < max_session_count)
          listen(); // requeue listen if more connections
    });
}

std::string session_manager::get_connections() const {
    std::string content = "\n";
    for (const auto &[key, value]: sessions) {
        content += "  ";
        content += boost::uuids::to_string(key);
        content += " (";
        content += std::to_string(value->client.is_open());
        content += ",";
        content += std::to_string(value->member.is_open());
        content += ")\n";
    }
    return content;
}

}
