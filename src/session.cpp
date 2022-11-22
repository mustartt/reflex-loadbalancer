//
// Created by henry on 2022-11-21.
//

#include <boost/uuid/uuid_io.hpp>

#include "session.h"
#include "session_manager.h"

#include "logger/logger.h"

namespace loadbalancer {

using namespace boost::asio;

session::session(session_manager *manager,
                 boost::uuids::uuid id,
                 io_context &context,
                 ip::tcp::socket client)
    : manager(manager), id(id), strand(context),
      client(std::move(client)),
      member(client.get_executor()),
      client_timeout(context, boost::posix_time::seconds(5)),
      member_timeout(context, boost::posix_time::seconds(5)) {

    BOOST_LOG_SEV(logger::slg, logger::info)
        << boost::uuids::to_string(id)
        << " - Connection Accepted";

    client_buffer = std::make_unique<buffer_type>();
    member_buffer = std::make_unique<buffer_type>();
}

session::~session() {
    BOOST_LOG_SEV(logger::slg, logger::info)
        << boost::uuids::to_string(id)
        << " - Connection Released @";
}

void session::connect_to_member(ip::tcp::endpoint &endpoint) {
    member.async_connect(
        endpoint,
        [this](const boost::system::error_code &ec) {
          if (ec) {
              BOOST_LOG_SEV(logger::slg, logger::warning)
                  << boost::uuids::to_string(id)
                  << " - Failed to connect to pool member: "
                  << ec.message();
          } else {
              BOOST_LOG_SEV(logger::slg, logger::info)
                  << boost::uuids::to_string(id)
                  << " - Backend Connection Established";
              start_transfer();
          }
        }
    );
}

void session::shutdown() {
    post(strand,
         [this]() {
           boost::system::error_code ec;
           if (client.is_open()) { client.shutdown(ip::tcp::socket::shutdown_both, ec); }
           if (member.is_open()) { member.shutdown(ip::tcp::socket::shutdown_both, ec); }
         }
    );
}

void session::start_transfer() {
    read_from_client();
    read_from_server();
}

void session::close() {
    post(
        strand,
        [this]() {
          boost::system::error_code ec;
          client.close(ec);
          member.close(ec);
          client_timeout.cancel(ec);
          member_timeout.cancel(ec);

          manager->release(id);
        }
    );

}

void session::push_client_timeout_deadline() {
    client_timeout.expires_from_now(boost::posix_time::seconds(5));
    client_timeout.async_wait([this](const boost::system::error_code &ec) {
      if (ec) return;
      BOOST_LOG_SEV(logger::slg, logger::info)
          << boost::uuids::to_string(id)
          << " - Client Connection Timed out "
          << ec.message();
      close();
    });
}

void session::push_member_timeout_deadline() {
    member_timeout.expires_from_now(boost::posix_time::seconds(5));
    member_timeout.async_wait([this](const boost::system::error_code &ec) {
      if (ec) return;
      BOOST_LOG_SEV(logger::slg, logger::info)
          << boost::uuids::to_string(id)
          << " - Member Connection Timed out "
          << ec.message();
      close();
    });
}

void session::read_from_client() {
    client.async_read_some(
        buffer(*client_buffer),
        [this](std::error_code ec, std::size_t n) {
          if (!ec) {
              push_client_timeout_deadline();
              write_to_server(n);
          } else {
              close();
          }
        }
    );
}

void session::write_to_server(std::size_t n) {
    async_write(
        member,
        buffer(*client_buffer, n),
        [this](std::error_code ec, std::size_t _) {
          if (!ec) {
              read_from_client();
          } else {
              close();
          }
        }
    );
}

void session::read_from_server() {
    member.async_read_some(
        buffer(*member_buffer),
        [this](std::error_code ec, std::size_t n) {
          if (!ec) {
              push_member_timeout_deadline();
              write_to_client(n);
          } else {
              close();
          }
        }
    );
}

void session::write_to_client(std::size_t n) {
    async_write(
        client,
        buffer(*member_buffer, n),
        [this](std::error_code ec, std::size_t _) {
          if (!ec) {
              read_from_server();
          } else {
              close();
          }
        }
    );
}

}
