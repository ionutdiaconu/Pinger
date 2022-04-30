cmake_minimum_required(VERSION 3.16)

set(CMAKE_TOOLCHAIN_FILE  /home/johnny/opt/vcpkg/scripts/buildsystems/vcpkg.cmake
        CACHE STRING "Vcpkg toolchain file")

project(pingger)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)

set( INCLUDE_HEADERS /home/johnny/opt/boost_1_79_0)

find_package(libpqxx CONFIG REQUIRED)
find_package(Boost 1.79.0 REQUIRED system)

FILE(GLOB PROJECT_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

target_link_libraries(${PROJECT_NAME}
		PRIVATE libpqxx::pqxx)

