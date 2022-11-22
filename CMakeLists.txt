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

add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)

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
