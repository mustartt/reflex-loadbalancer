//
// Created by henry on 2022-11-21.
//

#include "sessions/session.h"

namespace loadbalancer {

using namespace boost::asio;

session::session(session_manager *manager,
                 boost::uuids::uuid id,
                 boost::asio::io_context &context,
                 size_t buffer_size,
                 boost::posix_time::seconds timeout)
    : id(id), manager(manager), session_strand(context),
      client_timeout(context, boost::posix_time::seconds(timeout)),
      timeout(timeout) {
    client_buffer.resize(buffer_size);
    member_buffer.resize(buffer_size);
}

}
