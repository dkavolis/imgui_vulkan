/**
 * MIT License
 *
 * Copyright (c) 2022, Daumantas Kavolis
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <numeric>
#include <source_location>
#include <span>
#include <stdexcept>
#include <string>

#include "config.hpp"

// enable unlimited framerate for smoother display
#define IMGUI_UNLIMITED_FRAME_RATE

// installing imgui through vcpkg with vulkan/sdl2 will also add the macros to command line and imconfig.h...
IMGUI_VK_MSVC_WARNING_DISABLE(4005)
#include <imgui.h>
IMGUI_VK_MSVC_WARNING_POP()

struct SDL_Window;
struct ImGui_ImplVulkanH_Window;

IMGUI_VK_NAMESPACE_BEGIN

class GUIError : public std::runtime_error {
 public:
  explicit GUIError(std::string const& msg, int code = -1, std::source_location src = std::source_location::current())
      : std::runtime_error(msg), src_(src), error_(code) {}
  explicit GUIError(char const* msg, int code = -1, std::source_location src = std::source_location::current())
      : std::runtime_error(msg), src_(src), error_(code) {}

  [[nodiscard]] auto error() const noexcept -> int;

  [[nodiscard]] auto source() const noexcept -> std::source_location;

 private:
  std::source_location src_;
  int error_ = -1;
};

class Vulkan;
class Window;

/**
 * @brief SDL2 GUI application.
 *
 */
class Application {
 public:
  /**
   * @brief Construct a new Application object with main window. Sets up SDL2 environment
   *
   * @param window main window
   */
  Application(Window* window);

  /**
   * @brief Destroy the Application object. Cleans up SDL2 environment.
   */
  ~Application() noexcept;

  /**
   * @brief Display the main window. Caught exceptions are printed to stderr.
   *
   * @return int Exit code
   */
  auto run() noexcept -> int;

 private:
  Window* window_;
};

/**
 * @brief Vulkan/SDL2 ImGui window. Pass to `Application` to run.
 *
 */
class Window {
 public:
  /**
   * @brief Construct a new Window object
   *
   * @param name Window name
   * @param width Window width in pixels
   * @param height Window height in pixels
   */
  Window(std::string name, int width, int height) noexcept;

  virtual ~Window() noexcept;

  /**
   * @brief Close the window
   */
  void close() noexcept;

  [[nodiscard]] auto name() const noexcept -> std::string_view;
  [[nodiscard]] auto is_running() const noexcept -> bool;
  [[nodiscard]] auto width() const noexcept -> int;
  [[nodiscard]] auto height() const noexcept -> int;

 protected:
  /**
   * @brief Main entry point for ImGui implementation, everything is setup by this point. Called once per draw loop
   */
  virtual void on_gui() = 0;

  /**
   * @brief Change window parameters before rendering, called every loop just before rendering the frame in Vulkan and
   * after the draw data has been prepared. Requires `#include <imgui_impl_vulkan.h>`
   *
   * @param wd Vulkan window parameters
   * @param draw_data ImGui draw data
   */
  virtual void before_render_frame(ImGui_ImplVulkanH_Window* wd, ImDrawData* draw_data);

 private:
  std::string name_;
  std::array<int, 2> size_;
  bool running_ = false;
  // noop deleter, cleaned up after the window is closed
  SDL_Window* window_ = nullptr;
  // PIMPL for all Vulkan related stuff, cleaned up with the window
  std::unique_ptr<Vulkan> vulkan_ = nullptr;

  friend class Application;

  /**
   * @brief Draw GUI once, called in show() loop
   */
  void draw();

  /**
   * @brief Display the window, blocks until the window is closed
   */
  void show();
};

inline void Window::close() noexcept { running_ = false; }

[[nodiscard]] inline auto Window::name() const noexcept -> std::string_view { return name_; }

[[nodiscard]] inline auto Window::is_running() const noexcept -> bool { return running_; }

[[nodiscard]] inline auto Window::width() const noexcept -> int { return size_[0]; }

[[nodiscard]] inline auto Window::height() const noexcept -> int { return size_[1]; }

[[nodiscard]] inline auto GUIError::error() const noexcept -> int { return error_; }

[[nodiscard]] inline auto GUIError::source() const noexcept -> std::source_location { return src_; }

IMGUI_VK_NAMESPACE_END
