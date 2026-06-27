cmake_minimum_required(VERSION 3.20)

project(NovaBrowser VERSION 1.0.0 LANGUAGES CXX)

# =====================================================
# C++ Settings
# =====================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

# =====================================================
# Qt
# =====================================================
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    WebEngineWidgets
    WebEngineCore
    Network
    NetworkAuth
    Sql
    PrintSupport
    Concurrent
)

qt_standard_project_setup()

# =====================================================
# Dependencies
# =====================================================
find_package(OpenSSL REQUIRED)
find_package(ZLIB REQUIRED)

# =====================================================
# Sources
# =====================================================
file(GLOB_RECURSE SOURCES
    app/*.cpp
    app/*.h
)

# =====================================================
# Executable
# =====================================================
qt_add_executable(NovaBrowser
    MACOSX_BUNDLE
    WIN32

    main.cpp
    ${SOURCES}
    resources/resources.qrc
)

# =====================================================
# Includes
# =====================================================
target_include_directories(NovaBrowser PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/app
    ${CMAKE_CURRENT_SOURCE_DIR}/app/browser
    ${CMAKE_CURRENT_SOURCE_DIR}/app/network
    ${CMAKE_CURRENT_SOURCE_DIR}/app/storage
    ${CMAKE_CURRENT_SOURCE_DIR}/app/security
    ${CMAKE_CURRENT_SOURCE_DIR}/app/reader
    ${CMAKE_CURRENT_SOURCE_DIR}/app/utils
)

# =====================================================
# Libraries
# =====================================================
target_link_libraries(NovaBrowser PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::WebEngineWidgets
    Qt6::WebEngineCore
    Qt6::Network
    Qt6::NetworkAuth
    Qt6::Sql
    Qt6::PrintSupport
    Qt6::Concurrent

    OpenSSL::SSL
    OpenSSL::Crypto
    ZLIB::ZLIB
)

# =====================================================
# Compiler Warnings
# =====================================================
target_compile_options(NovaBrowser PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

# =====================================================
# macOS Bundle
# =====================================================
set_target_properties(NovaBrowser PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "NovaBrowser"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.nova.browser"
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
)

# =====================================================
# Install
# =====================================================
install(TARGETS NovaBrowser
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

# =====================================================
# Tests
# =====================================================
option(BUILD_TESTS "Build tests" OFF)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()