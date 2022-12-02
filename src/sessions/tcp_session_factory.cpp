//
// Created by henry on 2022-12-01.
//

#include "sessions/tcp_session_factory.h"

namespace loadbalancer {

using namespace boost::asio;

tcp_session_factory::tcp_session_factory(session_factory::address_type bind_addr,
                                         session_factory::port_type port,
                                         io_context &context,
                                         const config::config_property &property)
    : session_factory(std::move(bind_addr), port, context, property),
      acceptor(context) {
    boost::system::error_code ec;

    ip::tcp::endpoint endpoint(this->bind_addr, port);
    acceptor.open(endpoint.protocol());
    acceptor.set_option(socket_base::reuse_address(property.server.config.reuse_address));
    acceptor.bind(endpoint);

    acceptor.listen(property.server.config.backlog, ec);
    if (ec) {
        BOOST_LOG_SEV(logger::slg, logger::critical)
            << "Cannot create tcp session factory " << ec.message();
        throw errors::config_error(ec.message());
    }
}

void tcp_session_factory::listen(session_manager *manager) {
    using namespace boost::asio;

    acceptor.async_accept(
        [this, manager](const boost::system::error_code &ec, ip::tcp::socket client) {
            if (ec) {
                BOOST_LOG_SEV(logger::slg, logger::warning)
                    << "Session Factory failed to accept connection: " << ec.message();
                listen(manager);
            } else {
                if (!manager->get_strategy()->is_backend_alive()) {
                    BOOST_LOG_SEV(logger::slg, logger::warning)
                        << "Session Factory backend is down";
                    client.close();
                } else {
                    auto conn_id = generate_uuid();
                    std::shared_ptr<session> conn = std::make_shared<tcp_session>(
                        manager, conn_id, context,
                        property.server.config.transfer_buffer_bytes,
                        boost::posix_time::seconds(property.backend.timeout),
                        std::move(client)
                    );
                    auto &backend = manager->get_strategy()->next_backend();
                    conn->connect(backend.address, backend.port);
                    manager->acquire(conn);
                }
            }
        }
    );
}

void tcp_session_factory::close(session_manager *) {
    acceptor.close();
}

}
