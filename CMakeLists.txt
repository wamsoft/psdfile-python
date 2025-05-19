cmake_minimum_required(VERSION 3.16)

# UTF-8 source
add_compile_options("$<$<AND:$<C_COMPILER_ID:MSVC>,$<COMPILE_LANGUAGE:C>>:/utf-8>")
add_compile_options("$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<COMPILE_LANGUAGE:CXX>>:/utf-8>")
add_compile_options("$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<COMPILE_LANGUAGE:CXX>>:/Zc:__cplusplus>")

# VCPKG_ROOT を環境変数から設定
if(DEFINED ENV{VCPKG_ROOT})
    string(REPLACE "\\" "/" VCPKG_ROOT $ENV{VCPKG_ROOT})
else()
    message(FATAL_ERROR "VCPKG_ROOT is not set.")
endif() 
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")

# VCPKG_TARGET_TRIPLET を環境変数から設定
if(DEFINED ENV{VCPKG_TARGET_TRIPLET})
    set(VCPKG_TARGET_TRIPLET $ENV{VCPKG_TARGET_TRIPLET})
else()
    message(WARNING "VCPKG_TARGET_TRIPLET is not set.")
endif()

set(CMAKE_CXX_STANDARD 17)

# MSVC static runtime
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

project(${SKBUILD_PROJECT_NAME} LANGUAGES CXX)

set(PYBIND11_FINDPYTHON ON)
find_package(pybind11 CONFIG REQUIRED)

# Add psdparse library
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/psdfile_krkrz/psdparse ${CMAKE_CURRENT_BINARY_DIR}/psdparse)

# Add the module
pybind11_add_module(module src/bindings.cpp)

target_include_directories(module PRIVATE
    ${PYTHON_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/psdfile_krkrz
)

target_link_libraries(module PRIVATE
    psdparse
    ${PYTHON_LIBRARIES}
) 

install(TARGETS module DESTINATION psdfile)
