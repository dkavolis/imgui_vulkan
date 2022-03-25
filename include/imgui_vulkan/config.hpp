/**
 * Copyright (c) 2022 <Daumantas Kavolis>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
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

#define IMGUI_VK_NAMESPACE_BEGIN \
  namespace imgui_vulkan {       \
  inline namespace v0 {
#define IMGUI_VK_NAMESPACE_END \
  } /* namespace v0 */         \
  } /* namespace imgui_vulkan */

#if defined(_MSC_VER)
#  define IMGUI_VK_ALWAYS_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#  define IMGUI_VK_ALWAYS_INLINE inline __attribute__((always_inline))
#else
#  define IMGUI_VK_ALWAYS_INLINE inline
#endif

#if defined(_MSC_VER)
#  define IMGUI_VK_NOINLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#  define IMGUI_VK_NOINLINE __attribute__((noinline))
#else
#  define IMGUI_VK_NOINLINE
#endif

#define IMGUI_VK_DO_PRAGMA(x) _Pragma(#x)
#if defined(__GNUC__) && !defined(__clang__)
#  define IMGUI_VK_GCC_DIAGNOSTIC_IGNORED(wrn) \
    IMGUI_VK_DO_PRAGMA(GCC diagnostic push)    \
    IMGUI_VK_DO_PRAGMA(GCC diagnostic ignored wrn)
#  define IMGUI_VK_GCC_DIAGNOSTIC_POP() IMGUI_VK_DO_PRAGMA(GCC diagnostic pop)
#else
#  define IMGUI_VK_GCC_DIAGNOSTIC_IGNORED(wrn)
#  define IMGUI_VK_GCC_DIAGNOSTIC_POP()
#endif

#if defined(__clang__)
#  define IMGUI_VK_CLANG_DIAGNOSTIC_IGNORED(wrn) \
    IMGUI_VK_DO_PRAGMA(clang diagnostic push)    \
    IMGUI_VK_DO_PRAGMA(clang diagnostic ignored wrn)
#  define IMGUI_VK_CLANG_DIAGNOSTIC_POP() IMGUI_VK_DO_PRAGMA(clang diagnostic pop)
#else
#  define IMGUI_VK_CLANG_DIAGNOSTIC_IGNORED(wrn)
#  define IMGUI_VK_CLANG_DIAGNOSTIC_POP()
#endif

#if defined(_MSC_VER)
#  define IMGUI_VK_MSVC_WARNING_DISABLE(wrn) __pragma(warning(push)) __pragma(warning(disable : wrn))
#  define IMGUI_VK_MSVC_WARNING_POP() __pragma(warning(pop))
#else
#  define IMGUI_VK_MSVC_WARNING_DISABLE(wrn)
#  define IMGUI_VK_MSVC_WARNING_POP()
#endif

// https://devblogs.microsoft.com/cppblog/msvc-cpp20-and-the-std-cpp20-switch/
#if _MSC_VER >= 1929  // VS2019 v16.10 and later (_MSC_FULL_VER >= 192829913 for VS 2019 v16.9)
// Works with /std:c++14 and /std:c++17, and performs optimization
#  define IMGUI_VK_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
// no-op in MSVC v14x ABI
#  define IMGUI_VK_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif

#if defined(__cpp_consteval) && (!defined(_MSC_VER) || _MSC_FULL_VER >= 193030704 || defined(__clang__))
#  define IMGUI_VK_CONSTEVAL consteval
#else
#  define IMGUI_VK_CONSTEVAL constexpr
#endif

#ifdef _MSC_VER
#  define IMGUI_VK_ASSUME(cond) __assume(cond)
#elif defined(__clang__)
#  define IMGUI_VK_ASSUME(cond) __builtin_assume(cond)
#elif defined(__GNUC__)
#  define IMGUI_VK_ASSUME(cond)                              \
    {                                                        \
      if (!(cond)) [[unlikely]] { __builtin_unreachable(); } \
    }
#else
#  define IMGUI_VK_ASSUME(cond)
#endif

#ifdef _MSC_VER
#  define IMGUI_VK_UNREACHABLE() __assume(0)
#else
#  define IMGUI_VK_UNREACHABLE() __builtin_unreachable()
#endif
