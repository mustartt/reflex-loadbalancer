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
    : manager(manager), id(id),
      context(context),
      client(std::move(client)),
      member(client.get_executor()) {
    client_buffer = std::make_unique<buffer_type>();
    member_buffer = std::make_unique<buffer_type>();

    BOOST_LOG_SEV(logger::slg, logger::info)
        << boost::uuids::to_string(id)
        << " - Connection Accepted";
}

session::~session() {
    BOOST_LOG_SEV(logger::slg, logger::info)
        << boost::uuids::to_string(id)
        << " - Connection Released @";
}

void session::connect_to_member(ip::tcp::endpoint &endpoint) {
    member.async_connect(
        endpoint,
        [self = shared_from_this()](const boost::system::error_code &ec) {
          if (ec) {
              BOOST_LOG_SEV(logger::slg, logger::warning)
                  << boost::uuids::to_string(self->id)
                  << " - Failed to connect to pool member: "
                  << ec.message();
          } else {
              BOOST_LOG_SEV(logger::slg, logger::info)
                  << boost::uuids::to_string(self->id)
                  << " - Backend Connection Established";
              self->start_transfer();
          }
        }
    );
}

void session::shutdown() {
    BOOST_LOG_SEV(logger::slg, logger::info)
        << boost::uuids::to_string(id)
        << " - Connection Shutdown";
    post(
        context.context(),
        context.wrap(
            [self = shared_from_this()]() {
              boost::system::error_code ec;
              if (self->client.is_open()) {
                  self->client.shutdown(ip::tcp::socket::shutdown_both, ec);
                  if (ec) {
                      BOOST_LOG_SEV(logger::slg, logger::info)
                          << boost::uuids::to_string(self->id)
                          << " client " << ec.message();
                  }
              }
              if (self->member.is_open()) {
                  self->member.shutdown(ip::tcp::socket::shutdown_both, ec);
                  if (ec) {
                      BOOST_LOG_SEV(logger::slg, logger::info)
                          << boost::uuids::to_string(self->id)
                          << " member " << ec.message();
                  }
              }
            }
        )
    );
    manager->release(id);
}

void session::start_transfer() {
    read_from_client();
    read_from_server();
}

void session::read_from_client() {
    auto self = shared_from_this();
    client.async_read_some(
        buffer(*self->client_buffer),
        [self](std::error_code error, std::size_t n) {
          if (!error) {
              self->write_to_server(n);
          } else {
              self->shutdown();
          }
        }
    );
}

void session::write_to_server(std::size_t n) {
    auto self = shared_from_this();
    async_write(
        member,
        buffer(*self->client_buffer, n),
        [self](std::error_code ec, std::size_t _) {
          if (!ec) {
              self->read_from_client();
          } else {
              self->shutdown();
          }
        }
    );
}

void session::read_from_server() {
    auto self = shared_from_this();
    member.async_read_some(
        buffer(*self->member_buffer),
        [self](std::error_code error, std::size_t n) {
          if (!error) {
              self->write_to_client(n);
          } else {
              self->shutdown();
          }
        }
    );
}

void session::write_to_client(std::size_t n) {
    auto self = shared_from_this();
    async_write(
        client,
        buffer(*self->member_buffer, n),
        [self](std::error_code ec, std::size_t _) {
          if (!ec) {
              self->read_from_server();
          } else {
              self->shutdown();
          }
        }
    );
}

}
