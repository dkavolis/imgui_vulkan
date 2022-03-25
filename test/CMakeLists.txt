cmake_minimum_required(VERSION 3.15)

project(imgui_vulkan_test LANGUAGES CXX)

set(test_sources src/imgui_vulkan_test.cpp)

add_executable(imgui_vulkan_test ${test_sources})

include(../cmake/CompilerWarnings.cmake)
set_project_warnings(imgui_vulkan_test PRIVATE true)

find_package(Vulkan REQUIRED)
target_link_libraries(imgui_vulkan_test PUBLIC imgui_vulkan::imgui_vulkan)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
  target_compile_options(imgui_vulkan_test PRIVATE "-fconcepts-diagnostics-depth=4")
endif()

add_test(NAME imgui_vulkan_test COMMAND imgui_vulkan_test)
