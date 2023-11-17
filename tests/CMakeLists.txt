#
# Copyright (c) 2023 SICK AG, Waldkirch
#
# SPDX-License-Identifier: Unlicense

cmake_minimum_required(VERSION 3.16)

include(GoogleTest)

find_program( GCOVR_PATH gcovr PATHS ${CMAKE_SOURCE_DIR}/scripts/test)

if(VISIONARY_SHARED_ENABLE_CODE_COVERAGE)
  if(NOT GCOVR_PATH)
    message(FATAL_ERROR "gcovr not found! Aborting...")
  endif() # NOT GCOVR_PATH
endif()

set(PRIVATE_SOURCES
  src/NumericConvTest.cpp
  src/EndianTest.cpp
  src/CoLa2ProtocolHandlerTest.cpp
  src/MockTransport.cpp
  src/VisionaryTMiniDataTest.cpp
  src/main.cpp
)

set(TEST_TARGET ${PROJECT_NAME}_tests)

add_executable(${TEST_TARGET} ${PRIVATE_SOURCES})

if(VISIONARY_SHARED_ENABLE_CODE_COVERAGE)
  list(APPEND VISIONARY_SHARED_CFLAGS -fprofile-arcs -ftest-coverage)
  target_link_libraries(${TEST_TARGET} gcov)
  message(STATUS "Enabling coverage for ${TEST_TARGET}")
endif(VISIONARY_SHARED_ENABLE_CODE_COVERAGE)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # strip symbols
  set_target_properties(${TEST_TARGET} PROPERTIES LINK_FLAGS_RELEASE -s)
endif()

target_compile_options(${TEST_TARGET} PRIVATE ${VISIONARY_SHARED_CFLAGS})

target_include_directories(${TEST_TARGET} PRIVATE ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(${TEST_TARGET} sick_visionary_cpp_shared ${GTest_target})

if(CMAKE_CROSSCOMPILING)
  message(STATUS "omitting test discovery since we are cross-compiling")
else()
message(STATUS "discovering tests")
  gtest_discover_tests(${TEST_TARGET} XML_OUTPUT_DIR ${CMAKE_BINARY_DIR}/tests)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if (VISIONARY_SHARED_ENABLE_CODE_COVERAGE)
      add_test(NAME coverage COMMAND ${GCOVR_PATH} --root "${CMAKE_SOURCE_DIR}" --exclude "${CMAKE_SOURCE_DIR}/3pp/.*" --exclude-unreachable-branches --print-summary --xml-pretty -o coverage.xml)
      add_test(NAME coverage-html COMMAND ${GCOVR_PATH} --root "${CMAKE_SOURCE_DIR}" --exclude "${CMAKE_SOURCE_DIR}/3pp/.*" --exclude-unreachable-branches --print-summary --html-details --html coverage.html)
    endif()
  endif()
endif()
