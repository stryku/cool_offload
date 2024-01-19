#pragma once

#include <cassert>
#include <memory>
#include <mutex>
#include <span>
#include <vector>

namespace cool_offload {

using work_type_id_t = std::size_t;

class i_offloaded_work_executor {
public:
  virtual ~i_offloaded_work_executor() = default;

  // Returns number of consumed bytes
  virtual std::size_t decode_and_execute(std::span<const std::byte> data) = 0;
};

class work_registry {
public:
  work_registry() {
    // To create nullptr at invalid work type id.
    m_work_type_register.resize(1);
  }

  template <typename WorkType> work_type_id_t register_work(WorkType &work) {
    std::lock_guard lg{m_work_type_register_mutex};
    const work_type_id_t id = m_work_type_register.size();
    m_work_type_register.push_back(work.create_executor());
    return id;
  }

  i_offloaded_work_executor &get_executor(work_type_id_t id) {
    std::lock_guard lg{m_work_type_register_mutex};
    return *m_work_type_register[id];
  }

private:
  std::vector<std::unique_ptr<i_offloaded_work_executor>> m_work_type_register;
  std::mutex m_work_type_register_mutex;
};

class pipe {
public:
  pipe() {
    m_buffer.resize(1024);
  }

  std::span<std::byte> get_buffer(std::size_t required_space) {
    (void)required_space;
    return std::span(m_buffer).subspan(m_used);
  }

  void consume(std::size_t number_of_bytes) {
    m_used += number_of_bytes;
  }

  std::span<const std::byte> get_work_buffer() {
    return std::span(m_buffer).subspan(m_work_start, m_used - m_work_start);
  }

  void work_done(std::size_t bytes) {
    m_work_start += bytes;
  }

private:
  std::vector<std::byte> m_buffer;
  std::size_t m_work_start = 0;
  std::size_t m_used = 0;
};

struct thread_context {
  pipe &m_pipe;
  work_registry &m_work_type_registry;
};

template <typename Derived> class i_offloaded_work_encoder {
public:
  virtual ~i_offloaded_work_encoder() = default;
  virtual std::unique_ptr<i_offloaded_work_executor>
  create_executor() const = 0;

  // Returns number of used bytes in the buffer.
  std::size_t encode(const thread_context &context) const {
    if (m_work_type_id == 0) [[unlikely]] {
      m_work_type_id = context.m_work_type_registry.register_work(*this);
    }

    std::span<std::byte> available_space =
        context.m_pipe.get_buffer(required_space());

    assert(available_space.size() >= required_space());

    std::memcpy(available_space.data(), &m_work_type_id,
                sizeof(work_type_id_t));

    available_space = available_space.subspan(sizeof(m_work_type_id));

    return sizeof(work_type_id_t) +
           static_cast<const Derived &>(*this).encode_impl(available_space);
  }

  std::size_t required_space() const {
    return sizeof(work_type_id_t) +
           static_cast<const Derived &>(*this).required_space_impl();
  }

private:
  thread_local static inline work_type_id_t m_work_type_id = 0;
};

template <typename Derived>
class offloaded_work : public i_offloaded_work_encoder<Derived>,
                       public i_offloaded_work_executor {};

template <typename WorkType>
void offload(const thread_context &ctx, const WorkType &work) {
  const std::size_t consumed = work.encode(ctx);
  ctx.m_pipe.consume(consumed);
}

class work_executor {
public:
  void execute_work(const thread_context &ctx) {

    for (std::span<const std::byte> buffer = ctx.m_pipe.get_work_buffer();
         !buffer.empty(); buffer = ctx.m_pipe.get_work_buffer()) {

      work_type_id_t id;
      std::memcpy(&id, buffer.data(), sizeof(work_type_id_t));
      buffer = buffer.subspan(sizeof(work_type_id_t));

      i_offloaded_work_executor &work =
          ctx.m_work_type_registry.get_executor(id);

      const std::size_t consumed =
          sizeof(work_type_id_t) + work.decode_and_execute(buffer);
      ctx.m_pipe.work_done(consumed);
    }
  }
};

class offload_worker {
public:
  thread_context create_context() {
    m_contexts.emplace_back(std::make_unique<context>());
    return thread_context{.m_pipe = m_contexts.back()->m_pipe,
                          .m_work_type_registry =
                              m_contexts.back()->m_registry};
  }

  void execute_work() {
    work_executor executor;

    for (auto &ctx : m_contexts) {
      thread_context thread_ctx{.m_pipe = ctx->m_pipe,
                                .m_work_type_registry = ctx->m_registry};
      executor.execute_work(thread_ctx);
    }
  }

private:
  struct context {
    work_registry m_registry;
    pipe m_pipe;
  };

  std::vector<std::unique_ptr<context>> m_contexts;
};

} // namespace cool_offload
