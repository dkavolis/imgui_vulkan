# imgui_vulkan

Adaptation of ImGui [SDL2 Vulkan example](https://github.com/ocornut/imgui/blob/master/examples/example_sdl_vulkan/main.cpp)

## Features

* Easy to implement standalone GUI, see [example](test/src/imgui_vulkan_test.cpp) -
  just override one virtual method, setup and cleanup are taken care of

## Install

### Requirements

* [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)
* ImGui with Vulkan and SDL2 bindings
* SDL2 with Vulkan bindings
* fmt

Only Vulkan SDK needs to be installed manually, `vcpkg` can install the rest.

## Usage

Link in CMake

```cmake
target_link_libraries(<TARGET> PUBLIC imgui_vulkan::imgui_vulkan)
```

or embed [config.hpp](include/imgui_vulkan/config.hpp),
[imgui_vulkan.hpp](include/imgui_vulkan/imgui_vulkan.hpp) and
[imgui_vulkan.cpp](src/imgui_vulkan.cpp) into your project and link to
dependencies

```cmake
find_package(fmt CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)

target_compile_features(<TARGET> PUBLIC cxx_std_20)
target_link_libraries(<TARGET> PUBLIC fmt::fmt imgui::imgui PRIVATE SDL2::SDL2 SDL2::SDL2main)
```

## Credits

* [ImGui](https://github.com/ocornut/imgui)
