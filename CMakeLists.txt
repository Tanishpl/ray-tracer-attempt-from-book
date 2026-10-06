cmake_minimum_required(VERSION 3.16)
project(raytracer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Threads REQUIRED)

if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

add_executable(raytracer src/main.cpp)
target_include_directories(raytracer PRIVATE src)
target_include_directories(raytracer SYSTEM PRIVATE third_party)
target_link_libraries(raytracer PRIVATE Threads::Threads)

enable_testing()
add_executable(tests tests/tests.cpp)
target_include_directories(tests PRIVATE src)
target_link_libraries(tests PRIVATE Threads::Threads)
add_test(NAME tests COMMAND tests)
