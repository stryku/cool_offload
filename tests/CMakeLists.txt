enable_testing()

add_executable(cool_offload_test
  cool_offload_test.cpp
)

target_link_libraries(cool_offload_test cool_offload gtest_main)
