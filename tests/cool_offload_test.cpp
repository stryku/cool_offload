#include "cool_offload/cool_offload.hpp"
#include "fmt/core.h"

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <gtest/gtest.h>
#include <memory>

namespace cool_offload::test {

using namespace std::literals;

class test_work final : public offloaded_work<test_work> {
public:
  std::size_t decode_and_execute(std::span<const std::byte> data) final {
    unsigned answer;
    std::memcpy(&answer, data.data(), sizeof(answer));
    fmt::print("Test work: {}\n", answer);
    return sizeof(answer);
  }

  std::unique_ptr<i_offloaded_work_executor> create_executor() const final {
    return std::make_unique<test_work>();
  }

  constexpr std::size_t required_space_impl() const {
    return sizeof(m_answer);
  }

  int encode_impl(std::span<std::byte> available_space) const {
    std::memcpy(available_space.data(), &m_answer, sizeof(m_answer));
    return sizeof(m_answer);
  }

  unsigned m_answer = 42;
};

struct complex_work_data {

  std::string m_string;
  std::vector<int> m_vector;

  struct {
    bool m_bool = true;
    double m_double = 42.24;
  } m_pod;
};

struct complex_work_data_decoded {
  std::string_view m_string;
  std::span<const int> m_vector;

  struct {
    bool m_bool = true;
    double m_double = 42.24;
  } m_pod;
};

class complex_work_executor final : public i_offloaded_work_executor {
public:
  std::size_t decode_and_execute(std::span<const std::byte> data) final {
    complex_work_data_decoded decoded;

    const std::byte *buffer = data.data();
    std::size_t tmp;
    std::memcpy(&tmp, buffer, sizeof(std::size_t));
    buffer += sizeof(std::size_t);

    decoded.m_string =
        std::string_view(reinterpret_cast<const char *>(buffer), tmp);
    buffer += tmp;

    std::memcpy(&tmp, buffer, sizeof(std::size_t));
    buffer += sizeof(std::size_t);
    decoded.m_vector =
        std::span<const int>(reinterpret_cast<const int *>(buffer), tmp);
    buffer += decoded.m_vector.size_bytes();

    std::memcpy(&decoded.m_pod, buffer, sizeof(decoded.m_pod));
    buffer += sizeof(decoded.m_pod);

    fmt::print("Complex work: str={}, vec={}, pod={{{}, {}}}\n",
               decoded.m_string, decoded.m_vector, decoded.m_pod.m_bool,
               decoded.m_pod.m_double);

    return buffer - data.data();
  }
};

class complex_work final : public i_offloaded_work_encoder<complex_work> {
public:
  std::unique_ptr<i_offloaded_work_executor> create_executor() const final {
    return std::make_unique<complex_work_executor>();
  }

  constexpr std::size_t required_space_impl() const {
    return sizeof(std::size_t) + m_data.m_string.size() + sizeof(std::size_t) +
           m_data.m_vector.size() + sizeof(m_data.m_pod);
  }

  std::size_t encode_impl(std::span<std::byte> available_space) const {
    std::byte *buffer = available_space.data();

    std::size_t tmp = m_data.m_string.size();
    std::memcpy(buffer, &tmp, sizeof(tmp));
    buffer += sizeof(tmp);

    std::memcpy(buffer, m_data.m_string.data(), m_data.m_string.size());
    buffer += m_data.m_string.size();

    tmp = m_data.m_vector.size();
    std::memcpy(buffer, &tmp, sizeof(tmp));
    buffer += sizeof(tmp);

    std::memcpy(buffer, m_data.m_vector.data(),
                m_data.m_vector.size() * sizeof(int));
    buffer += m_data.m_vector.size() * sizeof(int);

    std::memcpy(buffer, &m_data.m_pod, sizeof(m_data.m_pod));

    buffer += sizeof(m_data.m_pod);

    return buffer - available_space.data();
  }

  complex_work_data m_data;
};

TEST(CoolOffloadTest, Basic) {

  offload_worker worker;
  thread_context ctx = worker.create_context();

  test_work work;

  offload(ctx, work);

  complex_work complex;
  complex.m_data.m_string = "Some cool string";
  complex.m_data.m_vector = {1, 3, 3, 7};
  complex.m_data.m_pod.m_bool = true;
  complex.m_data.m_pod.m_double = 13.37;

  offload(ctx, complex);

  worker.execute_work();
}

} // namespace cool_offload::test