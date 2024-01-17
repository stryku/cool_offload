add_library(cool_offload STATIC
  cool_offload.hpp
)

target_include_directories(cool_offload INTERFACE ..)
target_link_libraries(cool_offload fmt::fmt)
