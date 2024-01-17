#include "cool_offload/cool_offload.hpp"

#include <gtest/gtest.h>

namespace cool_offload::test {

using namespace std::literals;

class CoolOffloadTest : public ::testing::Test {
public:
  void setup(std::size_t buffer_size) {
    m_buffer.resize(buffer_size);
  }

  void SetUp() override {
    setup(1024);
  }

  std::vector<std::uint8_t> m_buffer;
};

TEST_F(CoolOffloadTest, Foo) {
  EXPECT_EQ(answer(), 42);
}

} // namespace cool_offload::test