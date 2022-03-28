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

#include "imgui_vulkan/imgui_vulkan.hpp"

#include <cstdlib>
#include <vector>

#include <SDL.h>
#include <SDL_vulkan.h>
#include <fmt/core.h>
#include <imgui_impl_sdl.h>
#include <imgui_impl_vulkan.h>
#include <vulkan/vulkan.h>

IMGUI_VK_NAMESPACE_BEGIN

#if defined(_MSC_VER)
#  define IMGUI_VK_NOINLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#  define IMGUI_VK_NOINLINE __attribute__((noinline))
#else
#  define IMGUI_VK_NOINLINE
#endif

#ifdef _DEBUG
#  define IMGUI_VK_DEBUG_REPORT
#endif

#ifdef IMGUI_VK_DEBUG_REPORT
auto VKAPI_CALL debug_report(VkDebugReportFlagsEXT flags [[maybe_unused]], VkDebugReportObjectTypeEXT objectType,
                             uint64_t object [[maybe_unused]], size_t location [[maybe_unused]],
                             int32_t messageCode [[maybe_unused]], const char* pLayerPrefix [[maybe_unused]],
                             const char* pMessage, void* pUserData) -> VkBool32 {
  fmt::print(stderr, "[vulkan] Debug report from ObjectType: {}\nMessage: {}\n\n", objectType, pMessage);
  return VK_FALSE;
}
#endif

template <class F>
class scope_guard {
 public:
  explicit scope_guard(F f) noexcept : f_(std::move(f)) {}
  ~scope_guard() noexcept { f_(); }

 private:
  F f_;
};

[[noreturn]] IMGUI_VK_NOINLINE static void vthrow_gui_error(
    std::string const& msg, int code = -1, std::source_location src = std::source_location::current()) {
  throw GUIError(msg, code, src);
}

[[noreturn]] IMGUI_VK_NOINLINE static void vthrow_gui_error(
    char const* msg, int code = -1, std::source_location src = std::source_location::current()) {
  throw GUIError(msg, code, src);
}

template <class... Args>
struct FormatString {
  using self = FormatString;

  fmt::format_string<Args...> str;
  std::source_location src;

  consteval FormatString(fmt::format_string<Args...> fmt,
                         std::source_location s = std::source_location::current()) noexcept
      : str(fmt), src(s) {}

  template <class S>
    requires(std::constructible_from<fmt::format_string<Args...>, S> && !std::same_as<S, self>)
  consteval FormatString(S const& fmt, std::source_location s = std::source_location::current()) noexcept
      : str(fmt), src(s) {}
};

template <class... Args>
[[noreturn]] void throw_gui_error(typename FormatString<Args...>::self fmt, Args&&... args) {
  vthrow_gui_error(fmt::vformat(fmt.str, fmt::make_format_args(args...)), -1, fmt.src);
}

template <class... Args>
[[noreturn]] void throw_gui_error(int code, typename FormatString<Args...>::self fmt, Args&&... args) {
  vthrow_gui_error(fmt::vformat(fmt.str, fmt::make_format_args(args...)), code, fmt.src);
}

class Vulkan {
 public:
  explicit Vulkan(std::span<char const* const> extensions);

  static void check_vk_result(VkResult err);
  void setup_window(ImGui_ImplVulkanH_Window* wd, VkSurfaceKHR surface, int width, int height);
  void init(SDL_Window* window);
  void cleanup();
  void cleanup_window();
  void render_frame(ImGui_ImplVulkanH_Window* wd, ImDrawData* draw_data);
  void present_frame(ImGui_ImplVulkanH_Window* wd);
  void create_surface(SDL_Window* wd, VkSurfaceKHR* surface);
  void create_framebuffers(SDL_Window* wd, VkSurfaceKHR surface);
  void shutdown();
  void upload_fonts();
  void maybe_resize_swap_chain(SDL_Window* window);

  [[nodiscard]] auto main_window_data() const noexcept -> ImGui_ImplVulkanH_Window const&;
  [[nodiscard]] auto main_window_data() noexcept -> ImGui_ImplVulkanH_Window&;

 private:
  VkAllocationCallbacks* allocator_ = nullptr;
  VkInstance instance_ = nullptr;
  VkPhysicalDevice physical_device_ = nullptr;
  VkDevice device_ = nullptr;
  std::uint32_t queue_family_ = std::numeric_limits<std::uint32_t>::max();
  VkQueue queue_ = nullptr;
  VkDebugReportCallbackEXT debug_report_ = nullptr;
  VkPipelineCache pipeline_cache_ = nullptr;
  VkDescriptorPool descriptor_pool_ = nullptr;

  ImGui_ImplVulkanH_Window main_window_data_;
  std::uint32_t min_image_count_ = 2;
  bool swap_chain_rebuild_ = false;
};

[[nodiscard]] inline auto Vulkan::main_window_data() const noexcept -> ImGui_ImplVulkanH_Window const& {
  return main_window_data_;
}
[[nodiscard]] inline auto Vulkan::main_window_data() noexcept -> ImGui_ImplVulkanH_Window& { return main_window_data_; }

static auto vk_result_string(VkResult err) noexcept -> char const* {
  switch (err) {
#define VK_ERR_STR(e) \
  case VK_##e: return "VK_" #e
    VK_ERR_STR(SUCCESS);
    VK_ERR_STR(ERROR_OUT_OF_HOST_MEMORY);
    VK_ERR_STR(ERROR_DEVICE_LOST);
    VK_ERR_STR(ERROR_EXTENSION_NOT_PRESENT);
    VK_ERR_STR(ERROR_TOO_MANY_OBJECTS);
    VK_ERR_STR(ERROR_UNKNOWN);
    VK_ERR_STR(ERROR_FRAGMENTATION);
    VK_ERR_STR(ERROR_SURFACE_LOST_KHR);
    VK_ERR_STR(SUBOPTIMAL_KHR);
    VK_ERR_STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
    VK_ERR_STR(ERROR_INVALID_SHADER_NV);
    VK_ERR_STR(ERROR_NOT_PERMITTED_EXT);
    VK_ERR_STR(THREAD_IDLE_KHR);
    VK_ERR_STR(OPERATION_NOT_DEFERRED_KHR);
    VK_ERR_STR(ERROR_OUT_OF_POOL_MEMORY_KHR);
    VK_ERR_STR(ERROR_INVALID_EXTERNAL_HANDLE_KHR);
    VK_ERR_STR(ERROR_INVALID_DEVICE_ADDRESS_EXT);
    VK_ERR_STR(ERROR_PIPELINE_COMPILE_REQUIRED_EXT);
    VK_ERR_STR(RESULT_MAX_ENUM);
#undef VK_ERR_STR
    default: return "VK_UNKNOWN_ERROR";
  }
}

void Vulkan::check_vk_result(VkResult err) {
  if (err == 0) return;
  auto str = vk_result_string(err);
  if (err < 0) {
    vthrow_gui_error(str, err);
  } else {
    std::fprintf(stderr, "[vulkan] Error: VkResult = %s\n", str);
  }
}

Vulkan::Vulkan(std::span<char const* const> extensions) {
  VkResult err;

  // Create Vulkan Instance
  {
    VkInstanceCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .enabledExtensionCount = static_cast<std::uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
    };
#ifdef IMGUI_VK_DEBUG_REPORT
    // Enabling validation layers
    constexpr static std::array<const char*, 1> layers{"VK_LAYER_KHRONOS_validation"};
    create_info.enabledLayerCount = static_cast<std::uint32_t>(layers.size());
    create_info.ppEnabledLayerNames = layers.data();

    // Enable debug report extension (we need additional storage, so we duplicate the user array to add our new
    // extension to it)
    std::vector<char const*> extensions_ext(extensions.size() + 1);
    std::ranges::copy(extensions, extensions_ext.begin());
    extensions_ext.back() = "VK_EXT_debug_report";
    create_info.enabledExtensionCount = static_cast<std::uint32_t>(extensions_ext.size());
    create_info.ppEnabledExtensionNames = extensions_ext.data();

    // Create Vulkan Instance
    err = vkCreateInstance(&create_info, allocator_, &instance_);
    check_vk_result(err);

    // Get the function pointer (required for any extensions)
    auto vkCreateDebugReportCallbackEXT = static_cast<PFN_vkCreateDebugReportCallbackEXT>(
        vkGetInstanceProcAddr(instance_, "vkCreateDebugReportCallbackEXT"));
    if (vkCreateDebugReportCallbackEXT == nullptr) [[unlikely]]
      vthrow_gui_error("Could not get vkCreateDebugReportCallbackEXT");

    // Setup the debug report callback
    VkDebugReportCallbackCreateInfoEXT debug_report_ci{
        .sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
        .flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT |
                 VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
        .pfnCallback = debug_report,
        .pUserData = nullptr,
    };
    err = vkCreateDebugReportCallbackEXT(instance_, &debug_report_ci, allocator_, &debug_report_);
    check_vk_result(err);
#else
    // Create Vulkan Instance without any debug feature
    err = vkCreateInstance(&create_info, allocator_, &instance_);
    check_vk_result(err);
#endif
  }

  // Select GPU
  {
    uint32_t gpu_count;
    err = vkEnumeratePhysicalDevices(instance_, &gpu_count, nullptr);
    check_vk_result(err);
    if (gpu_count == 0) [[unlikely]] { vthrow_gui_error("Could not find any GPUs!"); }

    std::vector<VkPhysicalDevice> gpus(gpu_count);
    err = vkEnumeratePhysicalDevices(instance_, &gpu_count, gpus.data());
    check_vk_result(err);

    // If a number >1 of GPUs got reported, find discrete GPU if present, or use first one available. This covers
    // most common cases (multi-gpu/integrated+dedicated graphics). Handling more complicated setups (multiple
    // dedicated GPUs) is out of scope of this sample.
    for (auto const& gpu : gpus) {
      VkPhysicalDeviceProperties properties;
      vkGetPhysicalDeviceProperties(gpu, &properties);
      if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        physical_device_ = gpu;
        break;
      }
    }
  }

  // Select graphics queue family
  {
    uint32_t count;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &count, nullptr);
    std::vector<VkQueueFamilyProperties> queues(count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device_, &count, queues.data());
    auto queue_it =
        std::ranges::find_if(queues, [](auto const& queue) { return queue.queueFlags & VK_QUEUE_GRAPHICS_BIT; });

    if (queue_it == queues.end()) [[unlikely]] { vthrow_gui_error("Could not find graphics queue"); }
    queue_family_ = static_cast<std::uint32_t>(queue_it - queues.begin());
  }

  // Create Logical Device (with 1 queue)
  {
    constexpr static std::array<const char*, 1> device_extensions{"VK_KHR_swapchain"};
    std::array<float, 1> queue_priority{1.0f};
    VkDeviceQueueCreateInfo queue_info{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queue_family_,
        .queueCount = static_cast<std::uint32_t>(queue_priority.size()),
        .pQueuePriorities = queue_priority.data(),
    };
    VkDeviceCreateInfo create_info{
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queue_info,
        .enabledExtensionCount = static_cast<std::uint32_t>(device_extensions.size()),
        .ppEnabledExtensionNames = device_extensions.data(),
    };
    err = vkCreateDevice(physical_device_, &create_info, allocator_, &device_);
    check_vk_result(err);
    vkGetDeviceQueue(device_, queue_family_, 0, &queue_);
  }

  // Create Descriptor Pool
  {
    constexpr static auto pool_sizes = std::to_array<VkDescriptorPoolSize>({
        {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000},
    });
    VkDescriptorPoolCreateInfo pool_info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = 1000 * static_cast<std::uint32_t>(pool_sizes.size()),
        .poolSizeCount = static_cast<std::uint32_t>(pool_sizes.size()),
        .pPoolSizes = pool_sizes.data(),
    };
    err = vkCreateDescriptorPool(device_, &pool_info, allocator_, &descriptor_pool_);
    check_vk_result(err);
  }
}

void Vulkan::setup_window(ImGui_ImplVulkanH_Window* wd, VkSurfaceKHR surface, int width, int height) {
  wd->Surface = surface;

  // Check for WSI support
  VkBool32 res;
  vkGetPhysicalDeviceSurfaceSupportKHR(physical_device_, queue_family_, wd->Surface, &res);
  if (res != VK_TRUE) [[unlikely]] { vthrow_gui_error("Error no WSI support on physical device 0"); }

  // Select Surface Format
  constexpr static auto requestSurfaceImageFormat = std::to_array<VkFormat>({
      VK_FORMAT_B8G8R8A8_UNORM,
      VK_FORMAT_R8G8B8A8_UNORM,
      VK_FORMAT_B8G8R8_UNORM,
      VK_FORMAT_R8G8B8_UNORM,
  });
  constexpr static VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
  wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(
      physical_device_, wd->Surface, requestSurfaceImageFormat.data(),
      static_cast<int>(requestSurfaceImageFormat.size()), requestSurfaceColorSpace);

  // Select Present Mode
  constexpr static auto present_modes = std::to_array<VkPresentModeKHR>({
#ifdef IMGUI_UNLIMITED_FRAME_RATE
      VK_PRESENT_MODE_MAILBOX_KHR,
      VK_PRESENT_MODE_IMMEDIATE_KHR,
      VK_PRESENT_MODE_FIFO_KHR,
#else
      VK_PRESENT_MODE_FIFO_KHR
#endif
  });
  wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(physical_device_, wd->Surface, present_modes.data(),
                                                        static_cast<int>(present_modes.size()));
  // printf("[vulkan] Selected PresentMode = %d\n", wd->PresentMode);

  // Create SwapChain, RenderPass, Framebuffer, etc.
  if (min_image_count_ < 2) [[unlikely]] {
    throw_gui_error("Need at least 2 frame buffers for swapping, current: {}", min_image_count_);
  }
  ImGui_ImplVulkanH_CreateOrResizeWindow(instance_, physical_device_, device_, wd, queue_family_, allocator_, width,
                                         height, min_image_count_);
}

void Vulkan::init(SDL_Window* window) {
  ImGui_ImplSDL2_InitForVulkan(window);
  ImGui_ImplVulkan_InitInfo init_info{
      .Instance = instance_,
      .PhysicalDevice = physical_device_,
      .Device = device_,
      .QueueFamily = queue_family_,
      .Queue = queue_,
      .PipelineCache = pipeline_cache_,
      .DescriptorPool = descriptor_pool_,
      .Subpass = 0,
      .MinImageCount = min_image_count_,
      .ImageCount = main_window_data_.ImageCount,
      .MSAASamples = VK_SAMPLE_COUNT_1_BIT,
      .Allocator = allocator_,
      .CheckVkResultFn = check_vk_result,
  };
  ImGui_ImplVulkan_Init(&init_info, main_window_data_.RenderPass);
}

void Vulkan::present_frame(ImGui_ImplVulkanH_Window* wd) {
  if (swap_chain_rebuild_) return;
  VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
  VkPresentInfoKHR info{
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &render_complete_semaphore,
      .swapchainCount = 1,
      .pSwapchains = &wd->Swapchain,
      .pImageIndices = &wd->FrameIndex,
  };
  VkResult err = vkQueuePresentKHR(queue_, &info);
  if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) {
    swap_chain_rebuild_ = true;
    return;
  }
  check_vk_result(err);
  wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->ImageCount;  // Now we can use the next set of semaphores
}

void Vulkan::render_frame(ImGui_ImplVulkanH_Window* wd, ImDrawData* draw_data) {
  VkResult err;

  VkSemaphore image_acquired_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
  VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
  err = vkAcquireNextImageKHR(device_, wd->Swapchain, std::numeric_limits<std::uint64_t>::max(),
                              image_acquired_semaphore, nullptr, &wd->FrameIndex);
  if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR) {
    swap_chain_rebuild_ = true;
    return;
  }
  check_vk_result(err);

  ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex];
  {
    // wait indefinitely instead of periodically checking
    err = vkWaitForFences(device_, 1, &fd->Fence, VK_TRUE, std::numeric_limits<std::uint64_t>::max());
    check_vk_result(err);

    err = vkResetFences(device_, 1, &fd->Fence);
    check_vk_result(err);
  }
  {
    err = vkResetCommandPool(device_, fd->CommandPool, 0);
    check_vk_result(err);
    VkCommandBufferBeginInfo info{
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VkCommandBufferUsageFlags{} | VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    err = vkBeginCommandBuffer(fd->CommandBuffer, &info);
    check_vk_result(err);
  }
  {
    VkRenderPassBeginInfo info{
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = wd->RenderPass,
        .framebuffer = fd->Framebuffer,
        .renderArea =
            {
                .extent =
                    {
                        .width = static_cast<std::uint32_t>(wd->Width),
                        .height = static_cast<std::uint32_t>(wd->Height),
                    },
            },
        .clearValueCount = 1,
        .pClearValues = &wd->ClearValue,
    };
    vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
  }

  // Record dear imgui primitives into command buffer
  ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

  // Submit command buffer
  vkCmdEndRenderPass(fd->CommandBuffer);
  {
    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo info{
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &image_acquired_semaphore,
        .pWaitDstStageMask = &wait_stage,
        .commandBufferCount = 1,
        .pCommandBuffers = &fd->CommandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &render_complete_semaphore,
    };

    err = vkEndCommandBuffer(fd->CommandBuffer);
    check_vk_result(err);
    err = vkQueueSubmit(queue_, 1, &info, fd->Fence);
    check_vk_result(err);
  }
}

void Vulkan::cleanup() {
  vkDestroyDescriptorPool(device_, descriptor_pool_, allocator_);

#ifdef IMGUI_VK_DEBUG_REPORT
  // Remove the debug report callback
  auto vkDestroyDebugReportCallbackEXT = static_cast<PFN_vkDestroyDebugReportCallbackEXT>(
      vkGetInstanceProcAddr(instance_, "vkDestroyDebugReportCallbackEXT"));
  vkDestroyDebugReportCallbackEXT(instance_, debug_report_, allocator_);
#endif  // IMGUI_VK_DEBUG_REPORT

  vkDestroyDevice(device_, allocator_);
  vkDestroyInstance(instance_, allocator_);
}

void Vulkan::shutdown() {
  auto err = vkDeviceWaitIdle(device_);
  check_vk_result(err);
  ImGui_ImplVulkan_Shutdown();
}

void Vulkan::upload_fonts() {
  // Use any command queue
  auto* wd = &main_window_data_;
  VkCommandPool command_pool = wd->Frames[wd->FrameIndex].CommandPool;
  VkCommandBuffer command_buffer = wd->Frames[wd->FrameIndex].CommandBuffer;

  auto err = vkResetCommandPool(device_, command_pool, 0);
  check_vk_result(err);
  VkCommandBufferBeginInfo begin_info{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VkCommandBufferUsageFlags{} | VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };
  err = vkBeginCommandBuffer(command_buffer, &begin_info);
  check_vk_result(err);

  ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

  VkSubmitInfo end_info{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers = &command_buffer,
  };
  err = vkEndCommandBuffer(command_buffer);
  check_vk_result(err);
  err = vkQueueSubmit(queue_, 1, &end_info, nullptr);
  check_vk_result(err);

  err = vkDeviceWaitIdle(device_);
  check_vk_result(err);
  ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void Vulkan::maybe_resize_swap_chain(SDL_Window* window) {
  if (!swap_chain_rebuild_) { return; }
  int width, height;
  SDL_GetWindowSize(window, &width, &height);
  if (width > 0 && height > 0) {
    ImGui_ImplVulkan_SetMinImageCount(min_image_count_);
    ImGui_ImplVulkanH_CreateOrResizeWindow(instance_, physical_device_, device_, &main_window_data_, queue_family_,
                                           allocator_, width, height, min_image_count_);
    main_window_data_.FrameIndex = 0;
    swap_chain_rebuild_ = false;
  }
}

void Vulkan::create_framebuffers(SDL_Window* window, VkSurfaceKHR surface) {
  int w, h;
  SDL_GetWindowSize(window, &w, &h);
  setup_window(&main_window_data_, surface, w, h);
}

void Vulkan::create_surface(SDL_Window* window, VkSurfaceKHR* surface) {
  if (SDL_Vulkan_CreateSurface(window, instance_, surface) == 0) [[unlikely]] {
    throw_gui_error("Failed to create Vulkan surface: {}.", SDL_GetError());
  }
}

void Vulkan::cleanup_window() { ImGui_ImplVulkanH_DestroyWindow(instance_, device_, &main_window_data_, allocator_); }

Window::Window(std::string name, int width, int height) noexcept : name_(std::move(name)), size_({width, height}) {}

Window::~Window() noexcept = default;

void Window::show() {
  // Setup window
  constexpr static SDL_WindowFlags window_flags =
      (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  window_ =
      SDL_CreateWindow(name_.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, size_[0], size_[1], window_flags);
  if (window_ == nullptr) [[unlikely]] {
    throw_gui_error("Failed to create SDL window for '{}': {}", name_, SDL_GetError());
  }

  // Setup Vulkan
  uint32_t extensions_count = 0;
  SDL_Vulkan_GetInstanceExtensions(window_, &extensions_count, nullptr);
  std::vector<char const*> extensions(extensions_count);
  SDL_Vulkan_GetInstanceExtensions(window_, &extensions_count, extensions.data());
  vulkan_ = std::make_unique<Vulkan>(extensions);

  // Create Window Surface
  VkSurfaceKHR surface;
  vulkan_->create_surface(window_, &surface);

  // Create Framebuffers
  vulkan_->create_framebuffers(window_, surface);

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io [[maybe_unused]] = ImGui::GetIO();
  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  // ImGui::StyleColorsClassic();

  // Setup Platform/Renderer backends
  vulkan_->init(window_);

  // Load Fonts
  // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use
  // ImGui::PushFont()/PopFont() to select them.
  // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
  // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g.
  // use an assertion, or display an error and quit).
  // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling
  // ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
  // - Read 'docs/FONTS.md' for more instructions and details.
  // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double
  // backslash \\ !
  // io.Fonts->AddFontDefault();
  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
  // io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
  // ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL,
  // io.Fonts->GetGlyphRangesJapanese()); IM_ASSERT(font != NULL);

  // Upload Fonts
  vulkan_->upload_fonts();

  running_ = true;

  // cleanup after we're done
  scope_guard guard([this]() {
    vulkan_->shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    vulkan_->cleanup_window();
    vulkan_->cleanup();

    SDL_DestroyWindow(window_);

    window_ = nullptr;
    vulkan_ = nullptr;
  });

  while (running_) { draw(); }
}

void Window::before_render_frame(ImGui_ImplVulkanH_Window* wd [[maybe_unused]],
                                 ImDrawData* draw_data [[maybe_unused]]) {}

void Window::draw() {
  // Poll and handle events (inputs, window resize, etc.)
  // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
  // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite
  // your copy of the mouse data.
  // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or
  // clear/overwrite your copy of the keyboard data. Generally you may always pass all inputs to dear imgui, and hide
  // them from your application based on those two flags.
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL2_ProcessEvent(&event);
    running_ =
        event.type != SDL_QUIT && !(event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
                                    event.window.windowID == SDL_GetWindowID(window_));

    // Resize swap chain?
    vulkan_->maybe_resize_swap_chain(window_);

    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    on_gui();

    // Rendering
    ImGui::Render();
    ImDrawData* draw_data = ImGui::GetDrawData();
    const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
    if (!is_minimized) [[likely]] {
      auto* wd = &vulkan_->main_window_data();
      before_render_frame(wd, draw_data);
      vulkan_->render_frame(wd, draw_data);
      vulkan_->present_frame(wd);
    }
  }
}

Application::Application(Window* window) : window_(window) {
  // Setup SDL
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) [[unlikely]] {
    throw_gui_error("Failed to initialize SDL2: {}\n", SDL_GetError());
  }
}

Application::~Application() noexcept { SDL_Quit(); }

auto Application::run() noexcept -> int {
  if (window_ == nullptr) {
    fmt::print(stderr, "No window given\n");
    return -3;
  }

  try {
    window_->show();
    return 0;
  } catch (GUIError const& e) {
    fmt::print("{:s}({:d}) in {:s}:\n\t{:s}\n", e.source().file_name(), e.source().line(), e.source().function_name(),
               e.what());
    return e.error();
  } catch (std::exception const& e) {
    fmt::print("Exception caught: {:s}\n", e.what());
    return -1;
  } catch (...) {
    fmt::print(stderr, "Unknown thrown object!\n");
    return -2;
  }
}

IMGUI_VK_NAMESPACE_END
