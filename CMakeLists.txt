cmake_minimum_required (VERSION 2.8)
################################################################################
set(CMAKE_BUILD_TYPE Debug)
set(GLOB BOOST_VERSION 1.55)
include(ExternalProject)

externalproject_add(yaml
    URL https://github.com/jbeder/yaml-cpp/archive/yaml-cpp-0.5.3.tar.gz
    CMAKE_ARGS -DCMAKE_CXX_FLAGS=-fPIC
    UPDATE_COMMAND ""
    INSTALL_COMMAND ""
    PREFIX "yaml")

set(YAML_DIR "${CMAKE_BINARY_DIR}/yaml/src")
################################################################################
project (banjax)

find_package(Boost ${BOOST_VERSION} COMPONENTS REQUIRED)
find_package(Threads)

# TODO: yaml uses deprecated auto_ptr and this gives me a lot of warnings
# when compiling banjax. That caused me to miss some important warnings in
# banjax so the yaml warnings are currently disabled. Fixing yaml to not
# use auto_ptr would be preferred.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread -Wall -ggdb -Wno-deprecated-declarations")

include_directories(
  "${Boost_INCLUDE_DIR}"
  "${YAML_DIR}/yaml/include")

file(GLOB sources
  "${CMAKE_SOURCE_DIR}/src/*.cpp")

set(BANJAX_LIB_DEPS
  ssl
  crypto
  re2
  zmq
  ${YAML_DIR}/yaml-build/libyaml-cpp.a)

if(NOT DEFINED PRINT_DEBUG)
  set(PRINT_DEBUG false)
endif()
add_definitions(-DPRINT_DEBUG=${PRINT_DEBUG})

add_library(banjax_static STATIC ${sources})
set_target_properties(banjax_static PROPERTIES OUTPUT_NAME banjax)
target_link_libraries(banjax_static ${BANJAX_LIB_DEPS})


add_library(banjax_module MODULE ${sources})
set_target_properties(banjax_module PROPERTIES OUTPUT_NAME banjax)
target_link_libraries(banjax_module ${BANJAX_LIB_DEPS})
set_target_properties(banjax_module PROPERTIES PREFIX "")

add_dependencies(banjax_static yaml)
add_dependencies(banjax_module yaml)

################################################################################
project (unit-tests)

find_package(Boost ${BOOST_VERSION} COMPONENTS system unit_test_framework REQUIRED)
find_package(Threads)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread -ggdb")

include_directories(
  "${Boost_INCLUDE_DIR}"
  "${CMAKE_SOURCE_DIR}/src"
  "${CMAKE_SOURCE_DIR}/test")

file(GLOB sources
  "${CMAKE_SOURCE_DIR}/test/unittest_main.cpp"
  "${CMAKE_SOURCE_DIR}/test/challenge_manager_unittest.cpp"
  "${CMAKE_SOURCE_DIR}/test/regex_manager_unittest.cpp"
  "${CMAKE_SOURCE_DIR}/test/unittest_common.cpp"
  "${CMAKE_SOURCE_DIR}/test/white_lister_unittest.cpp"
  "${CMAKE_SOURCE_DIR}/test/cookie_unittest.cpp"
)

add_executable(unit-tests ${sources})
target_link_libraries(unit-tests banjax_static ${Boost_LIBRARIES})

################################################################################
