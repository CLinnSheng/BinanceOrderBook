cmake_minimum_required(VERSION 3.12)
project(BlockSys LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages from CMAKE Package
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

# JSONCPP
pkg_check_modules(JSONCPP jsoncpp)

# Fetch FTXUI
include(FetchContent)
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
)
