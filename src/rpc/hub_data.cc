#include "rpc/hub_data.hpp"

namespace indecorous {

std::unordered_map<handler_id_t, handler_callback_t *> hub_data_t::s_handlers;

} // namespace indecorous
