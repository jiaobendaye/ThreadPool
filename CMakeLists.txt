cmake_minimum_required(VERSION 2.8)
project(sylar)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")

include_directories(.)


set(LIB_SRC
    src/Thread.cpp
    src/ThreadPool.cpp 
    )

add_library(threadpool SHARED ${LIB_SRC})

set(LIB_LIB
    threadpool
    pthread
    )

add_executable(test tests/test.cpp)
add_dependencies(test threadpool)
target_link_libraries(test ${LIB_LIB})

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
