//
// Created by henry on 2022-11-30.
//

#include <boost/uuid/uuid_io.hpp>
#include "sessions/tcp_sessions.h"
#include "session_manager.h"
#include "logger/logger.h"

namespace loadbalancer {

using namespace boost::asio;

tcp_session::tcp_session(session_manager *manager,
                         boost::uuids::uuid id,
                         boost::asio::io_context &context,
                         size_t buffer_size,
                         boost::posix_time::seconds timeout,
                         ip::tcp::socket client
) : session(manager, id, context, buffer_size, timeout),
    strand_client(context),
    strand_member(context),
    client(std::move(client)),
    member(client.get_executor()) {
    BOOST_LOG_SEV(logger::slg, logger::info)
        << to_string(id) << " - Session Created";
}

void tcp_session::connect(const ip::address &address, uint16_t port) {
    member.async_connect(
        ip::tcp::endpoint(address, port),
        [self = shared_from_this(), &address, port](const boost::system::error_code &ec) {
            if (ec) {
                BOOST_LOG_SEV(logger::slg, logger::warning)
                    << boost::uuids::to_string(self->get_connection_id())
                    << " - Failed to connect to pool member: "
                    << ec.message();
            } else {
                BOOST_LOG_SEV(logger::slg, logger::info)
                    << to_string(self->get_connection_id())
                    << " - Backend connection established "
                    << address << ":" << port;
                self->start_transfer();
            }
        }
    );
}

void tcp_session::start_transfer() {
    read_from_client();
    read_from_server();
    refresh_client_timeout();
}

void tcp_session::shutdown() {
    close();
}

void tcp_session::close() {
    post(
        session_strand,
        [self = shared_from_this()]() {
            boost::system::error_code ec;

            self->client.close(ec);
            self->member.close(ec);
            self->client_timeout.cancel(ec);

            self->manager->release(self->get_connection_id());
        }
    );
}

void tcp_session::refresh_client_timeout() {
    client_timeout.expires_from_now(boost::posix_time::seconds(timeout));
    client_timeout.async_wait(
        bind_executor(
            session_strand,
            [self = shared_from_this()](const boost::system::error_code &ec) {
                if (ec) return;
                BOOST_LOG_SEV(logger::slg, logger::info)
                    << to_string(self->get_connection_id())
                    << " - Connection timeout";
                self->close();
            }
        )
    );
}

void tcp_session::read_from_client() {
    client.async_read_some(
        buffer(client_buffer),
        bind_executor(
            strand_client,
            [self = shared_from_this()](std::error_code ec, std::size_t n) {
                if (!ec) {
                    self->refresh_client_timeout();
                    self->write_to_server(n);
                } else {
                    self->close();
                }
            }
        )
    );
}

void tcp_session::write_to_server(std::size_t n) {
    async_write(
        member,
        buffer(client_buffer, n),
        bind_executor(
            strand_member,
            [self = shared_from_this()](std::error_code ec, std::size_t) {
                if (!ec) {
                    self->read_from_client();
                } else {
                    self->close();
                }
            })
    );
}

void tcp_session::read_from_server() {
    member.async_read_some(
        buffer(member_buffer),
        bind_executor(
            strand_client,
            [self = shared_from_this()](std::error_code ec, std::size_t n) {
                if (!ec) {
                    self->refresh_client_timeout();
                    self->write_to_client(n);
                } else {
                    self->close();
                }
            })
    );
}

void tcp_session::write_to_client(std::size_t n) {
    async_write(
        client,
        buffer(member_buffer, n),
        bind_executor(
            strand_member,
            [self = shared_from_this()](std::error_code ec, std::size_t) {
                if (!ec) {
                    self->read_from_server();
                } else {
                    self->close();
                }
            })
    );
}

tcp_session::~tcp_session() {
    BOOST_LOG_SEV(logger::slg, logger::info)
        << to_string(get_connection_id())
        << " - ~Connection Released";
}

}
