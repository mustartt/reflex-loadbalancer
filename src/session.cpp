//
// Created by henry on 2022-11-21.
//

#include <boost/uuid/uuid_io.hpp>

#include "session.h"
#include "session_manager.h"

#include "logger/logger.h"

namespace loadbalancer {

using namespace boost::asio;

constexpr int temp_timeout = 30;

session::session(session_manager *manager,
                 boost::uuids::uuid id,
                 io_context &context,
                 ip::tcp::socket client)
    : manager(manager), id(id), strand(context),
      client(std::move(client)),
      member(client.get_executor()),
      client_timeout(context, boost::posix_time::seconds(temp_timeout)),
      member_timeout(context, boost::posix_time::seconds(temp_timeout)) {

    BOOST_LOG_SEV(logger::slg, logger::info)
        << boost::uuids::to_string(id)
        << " - Connection Accepted";

    client_buffer = std::make_unique<buffer_type>();
    member_buffer = std::make_unique<buffer_type>();
}

session::~session() {
    BOOST_LOG_SEV(logger::slg, logger::info)
        << boost::uuids::to_string(id)
        << " - ~Connection Released";
}

void session::connect_to_member(const ip::tcp::endpoint &endpoint) {
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
    close(); // TODO: Temp implementation
}

void session::start_transfer() {
    read_from_client();
    read_from_server();

    push_client_timeout_deadline();
    push_member_timeout_deadline();
}

void session::close() {
    post(
        strand,
        [self = shared_from_this()]() {
            boost::system::error_code ec;
            self->client.close(ec);
            self->member.close(ec);
            self->client_timeout.cancel(ec);
            self->member_timeout.cancel(ec);

            self->manager->release(self->id);
        }
    );
}

void session::push_client_timeout_deadline() {
    client_timeout.expires_from_now(boost::posix_time::seconds(temp_timeout));
    client_timeout.async_wait(
        [self = shared_from_this()](const boost::system::error_code &ec) {
            if (ec) return;

            BOOST_LOG_SEV(logger::slg, logger::info)
                << boost::uuids::to_string(self->id)
                << " - Client Connection Timed out";

            self->close();
        }
    );
}

void session::push_member_timeout_deadline() {
    member_timeout.expires_from_now(boost::posix_time::seconds(temp_timeout));
    member_timeout.async_wait(
        [self = shared_from_this()](const boost::system::error_code &ec) {
            if (ec) return;

            BOOST_LOG_SEV(logger::slg, logger::info)
                << boost::uuids::to_string(self->id)
                << " - Member Connection Timed out ";

            self->close();
        }
    );
}

void session::read_from_client() {
    client.async_read_some(
        buffer(*client_buffer),
        [self = shared_from_this()](std::error_code ec, std::size_t n) {
            if (!ec) {
                self->push_client_timeout_deadline();
                self->write_to_server(n);
            } else {
                self->close();
            }
        }
    );
}

void session::write_to_server(std::size_t n) {
    async_write(
        member,
        buffer(*client_buffer, n),
        [self = shared_from_this()](std::error_code ec, std::size_t) {
            if (!ec) {
                self->read_from_client();
            } else {
                self->close();
            }
        }
    );
}

void session::read_from_server() {
    member.async_read_some(
        buffer(*member_buffer),
        [self = shared_from_this()](std::error_code ec, std::size_t n) {
            if (!ec) {
                self->push_member_timeout_deadline();
                self->write_to_client(n);
            } else {
                self->close();
            }
        }
    );
}

void session::write_to_client(std::size_t n) {
    async_write(
        client,
        buffer(*member_buffer, n),
        [self = shared_from_this()](std::error_code ec, std::size_t) {
            if (!ec) {
                self->read_from_server();
            } else {
                self->close();
            }
        }
    );
}

}
