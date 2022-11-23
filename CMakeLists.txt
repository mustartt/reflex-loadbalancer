cmake_minimum_required(VERSION 3.16)
project(loadbalancer)

set(CMAKE_FIND_DEBUG_MODE TRUE)
find_package(Boost COMPONENTS filesystem system log program_options REQUIRED)
find_package(yaml-cpp REQUIRED)
set(CMAKE_FIND_DEBUG_MODE FALSE)

set(CMAKE_CXX_STANDARD 20)

set(INCLUDE_DIRECTORY include)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${YAML_CPP_INCLUDE_DIRS})
include_directories(${INCLUDE_DIRECTORY})


set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -fsanitize=address -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_executable(loadbalancer
        main.cpp
        src/connection.cpp
        include/logger/logger.h
        src/configuration/config.cpp
        include/configuration/config.h
        src/logger/logger.cpp
        include/exceptions/exceptions.h
        src/session_manager.cpp
        include/session_manager.h
        src/session.cpp
        include/session.h)

target_link_libraries(loadbalancer ${Boost_LIBRARIES} ${YAML_CPP_LIBRARIES})
