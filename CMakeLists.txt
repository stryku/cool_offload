cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

project(cool_offload)

add_compile_options(-stdlib=libc++)
add_link_options(-stdlib=libc++ -lc++abi)

include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.13.0)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 9.1.0
)
FetchContent_MakeAvailable(fmt)

set(BENCHMARK_ENABLE_TESTING OFF)

FetchContent_Declare(
  googlebenchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.7.1)
FetchContent_MakeAvailable(googlebenchmark)

set(CMAKE_CXX_STANDARD 23)
add_compile_options(-Wall -Wextra -pedantic -Werror -Wno-invalid-offsetof)

option(COOL_OFFLOAD_SANITIZE "Do sanitize?" OFF)

if(COOL_OFFLOAD_SANITIZE)
  add_compile_options(-fsanitize=undefined,address -fno-omit-frame-pointer -fno-sanitize=alignment -g)
  add_link_options(-fsanitize=undefined,address -fno-omit-frame-pointer -g)
endif()

add_subdirectory(cool_offload)
add_subdirectory(tests)
