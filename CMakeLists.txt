cmake_minimum_required(VERSION 3.15)

project(myapp)

set(CMAKE_CXX_STANDARD 17)

find_package(CURL CONFIG REQUIRED)
find_package(libpqxx CONFIG REQUIRED)

add_executable(myapp main.cpp)
target_link_libraries(myapp CURL::libcurl libpqxx::pqxx PostgreSQL::PostgreSQL)