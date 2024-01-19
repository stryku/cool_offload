#pragma once

#include <cassert>
#include <memory>
#include <mutex>
#include <span>
#include <vector>

namespace cool_offload {

class i_offloaded_work_executor;

using work_type_id_t = std::size_t;

namespace detail {

extern std::vector<std::unique_ptr<i_offloaded_work_executor>>
    g_work_type_register;
extern std::mutex g_work_type_register_mutex;

} // namespace detail

class i_offloaded_work_executor {
public:
  virtual ~i_offloaded_work_executor() = default;

  // Returns number of eaten bytes
  virtual std::size_t decode_and_execute(std::span<const std::byte> data) = 0;
};

template <typename Derived> class i_offloaded_work_encoder {
public:
  using work_type_t = std::remove_cv_t<Derived>;

  virtual ~i_offloaded_work_encoder() = default;

  virtual std::unique_ptr<i_offloaded_work_executor> create_executor() = 0;

  // Returns number of used bytes in the buffer.
  std::size_t encode(std::span<std::byte> available_space) {
    if (m_work_type_id == 0) [[unlikely]] {
      register_work_type();
    }

    assert(available_space.size() >= required_space());

    std::memcpy(available_space.data(), &work_type_t::m_work_type_id,
                sizeof(work_type_id_t));

    available_space =
        available_space.subspan(sizeof(work_type_t::m_work_type_id));

    return static_cast<Derived &>(*this).encode_impl(available_space);
  }

  std::size_t required_space() const {
    return sizeof(work_type_id_t) +
           static_cast<const Derived &>(*this).required_space_impl();
  }

private:
  void register_work_type() {
    std::lock_guard lg{detail::g_work_type_register_mutex};
    m_work_type_id = detail::g_work_type_register.size();
    detail::g_work_type_register.push_back(create_executor());
  }

  static inline work_type_id_t m_work_type_id = 0;
};

template <typename Derived>
class offloaded_work : public i_offloaded_work_encoder<Derived>,
                       public i_offloaded_work_executor {};

class work_executor {
public:
  std::size_t execute_work(std::span<const std::byte> buffer) {
    work_type_id_t id;
    std::memcpy(&id, buffer.data(), sizeof(work_type_id_t));
    buffer = buffer.subspan(sizeof(work_type_id_t));

    i_offloaded_work_executor *work;

    {
      std::lock_guard lg{detail::g_work_type_register_mutex};
      work = detail::g_work_type_register[id].get();
    }

    return sizeof(work_type_id_t) + work->decode_and_execute(buffer);
  }
};

} // namespace cool_offload
