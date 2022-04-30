cmake_minimum_required(VERSION 3.16)

#[[set(CMAKE_TOOLCHAIN_FILE  C:/opt/vcpkg/scripts/buildsystems/vcpkg.cmake
        CACHE STRING "Vcpkg toolchain file")]]

set(CMAKE_TOOLCHAIN_FILE  /home/johnny/opt/vcpkg/scripts/buildsystems/vcpkg.cmake
        CACHE STRING "Vcpkg toolchain file")

project("pinger" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/bin)


set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.79.0 REQUIRED system)
find_package(libpqxx CONFIG REQUIRED)

FILE(GLOB PROJECT_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

target_include_directories(pinger PRIVATE ${Boost_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME}
                PRIVATE libpqxx::pqxx
                ${BOOST_LIBRARIES})

target_compile_options(pinger PRIVATE -Wall -Wextra -Wunreachable-code -Wpedantic)
