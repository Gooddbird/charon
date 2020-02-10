cmake_minimum_required(VERSION 2.8)
project(charon)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -g -std=c++11 -lpthread -Wall -Wno-deprecated -Werror -Wno-dev -Wno-unused-function")

set(LIB_SRC
    charon/log.cc
    charon/util.cc
    charon/thread.cc
    charon/coroutine.cc
    charon/scheduler.cc
    charon/io_scheduler.cc
)

add_library(charon SHARED ${LIB_SRC})

set(LIBS

    charon
)

add_executable(test_log tests/test_log.cc)
add_dependencies(test_log charon)
target_link_libraries(test_log ${LIBS})

add_executable(test_thread tests/test_thread.cc)
add_dependencies(test_thread charon)
target_link_libraries(test_thread ${LIBS})

add_executable(test_coroutine tests/test_coroutine.cc)
add_dependencies(test_coroutine charon)
target_link_libraries(test_coroutine ${LIBS})

add_executable(test_scheduler tests/test_scheduler.cc)
add_dependencies(test_scheduler charon)
target_link_libraries(test_scheduler ${LIBS})

add_executable(test_io_scheduler tests/test_io_scheduler.cc)
add_dependencies(test_io_scheduler charon)
target_link_libraries(test_io_scheduler ${LIBS})

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

