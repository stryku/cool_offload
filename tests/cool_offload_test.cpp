#include "cool_offload/cool_offload.hpp"

#include <fmt/format.h>

#include <gtest/gtest.h>
#include <memory>

namespace cool_offload::test {

using namespace std::literals;

class test_work final : public offloaded_work<test_work> {
public:
  std::size_t decode_and_execute(std::span<const std::byte> data) final {
    unsigned answer;
    std::memcpy(&answer, data.data(), sizeof(answer));
    fmt::print("Answer: {}\n", answer);
    return sizeof(answer);
  }

  std::unique_ptr<i_offloaded_work_executor> create_executor() final {
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

class CoolOffloadTest : public ::testing::Test {
public:
  void setup(std::size_t buffer_size) {
    m_buffer.resize(buffer_size);
  }

  void SetUp() override {
    setup(1024);
  }

  std::vector<std::byte> m_buffer;
};

TEST_F(CoolOffloadTest, EncodeDecode) {

  test_work work;
  work.encode(m_buffer);

  work_executor worker;

  worker.execute_work(m_buffer);
}

} // namespace cool_offload::test