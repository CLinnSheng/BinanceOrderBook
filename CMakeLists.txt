cmake_minimum_required(VERSION 3.12)
project(BlockSys LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages from CMAKE Package
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
find_package(Boost REQUIRED)

# JSONCPP
pkg_check_modules(JSONCPP jsoncpp)

# Setup FetchContent for automatic dependency downloading
include(FetchContent)

# WebSocketPP (header-only library)
find_path(WEBSOCKETPP_INCLUDE_DIR
    NAMES websocketpp/config/asio_client.hpp
    PATHS /usr/include /usr/local/include
    DOC "WebSocketPP include directory"
)

# Check if WebSocketPP was found
if(NOT WEBSOCKETPP_INCLUDE_DIR)
    message(FATAL_ERROR "WebSocketPP not found! Please install libwebsocketpp-dev (Ubuntu/Debian) or websocketpp (other systems)")
endif()

# Fetch FTXUI
FetchContent_Declare(ftxui
  GIT_REPOSITORY https://github.com/ArthurSonzogni/ftxui
  GIT_TAG v6.1.9
)
FetchContent_MakeAvailable(ftxui)

file(GLOB SRC_FILES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

# Add executable
add_executable(main ${SRC_FILES})

# Include directories
target_include_directories(main
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${OPENSSL_INCLUDE_DIR}
        ${JSONCPP_INCLUDE_DIRS}
        ${CURL_INCLUDE_DIRS}
        ${WEBSOCKETPP_INCLUDE_DIR}
        ${Boost_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(main
    PRIVATE
        ftxui::component
        ftxui::dom
        ftxui::screen
        ${CMAKE_THREAD_LIBS_INIT}
        ${OPENSSL_LIBRARIES}
        ${JSONCPP_LIBRARIES}
        ${CURL_LIBRARIES}
        ${Boost_LIBRARIES}
)
