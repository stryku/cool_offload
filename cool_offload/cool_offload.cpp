#include "cool_offload.hpp"

namespace cool_offload {

namespace detail {

std::vector<std::unique_ptr<i_offloaded_work_executor>> g_work_type_register;
std::mutex g_work_type_register_mutex;

} // namespace detail

} // namespace cool_offload
