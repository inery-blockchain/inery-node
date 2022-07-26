#pragma once

#include <inery/chain/webassembly/ine-vm-oc/config.hpp>

#include <boost/asio/local/datagram_protocol.hpp>
#include <inery/chain/webassembly/ine-vm-oc/ipc_helpers.hpp>

namespace inery { namespace chain { namespace inevmoc {

wrapped_fd get_connection_to_compile_monitor(int cache_fd);

}}}