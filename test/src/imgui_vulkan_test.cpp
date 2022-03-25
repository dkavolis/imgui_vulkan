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

#include <imgui_vulkan/imgui_vulkan.hpp>

IMGUI_VK_MSVC_WARNING_DISABLE(4005)
#include <imgui_impl_vulkan.h>
IMGUI_VK_MSVC_WARNING_POP()

IMGUI_VK_NAMESPACE_BEGIN

class TestWindow : public Window {
 public:
  using Window::Window;

 protected:
  void on_gui() override {
    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to
    // learn more about Dear ImGui!).
    if (show_demo_) ImGui::ShowDemoWindow(&show_demo_);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
    {
      static float f = 0.0f;
      static int counter = 0;

      ImGui::Begin("Hello, world!");  // Create a window called "Hello, world!" and append into it.

      ImGui::Text("This is some useful text.");     // Display some text (you can use a format strings too)
      ImGui::Checkbox("Demo Window", &show_demo_);  // Edit bools storing our window open/close state
      ImGui::Checkbox("Another Window", &show_another_);

      ImGui::SliderFloat("float", &f, 0.0f, 1.0f);              // Edit 1 float using a slider from 0.0f to 1.0f
      ImGui::ColorEdit3("clear color", (float*)&clear_color_);  // Edit 3 floats representing a color

      if (ImGui::Button("Button"))  // Buttons return true when clicked (most widgets return true when edited/activated)
        counter++;
      ImGui::SameLine();
      ImGui::Text("counter = %d", counter);

      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                  ImGui::GetIO().Framerate);
      ImGui::End();
    }

    // 3. Show another simple window.
    if (show_another_) {
      ImGui::Begin("Another Window",
                   &show_another_);  // Pass a pointer to our bool variable (the window will have a closing button
                                     // that will clear the bool when clicked)
      ImGui::Text("Hello from another window!");
      if (ImGui::Button("Close Me")) show_another_ = false;
      ImGui::End();
    }
  }

  void before_render_frame(ImGui_ImplVulkanH_Window* wd, ImDrawData* draw_data [[maybe_unused]]) override {
    wd->ClearValue.color.float32[0] = clear_color_.x * clear_color_.w;
    wd->ClearValue.color.float32[1] = clear_color_.y * clear_color_.w;
    wd->ClearValue.color.float32[2] = clear_color_.z * clear_color_.w;
    wd->ClearValue.color.float32[3] = clear_color_.w;
  }

 private:
  ImVec4 clear_color_ = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
  bool show_demo_ = false;
  bool show_another_ = false;
};

IMGUI_VK_NAMESPACE_END

auto main(int argc [[maybe_unused]], char* argv[] [[maybe_unused]]) -> int {
  imgui_vulkan::TestWindow window("Example", 1280, 720);
  imgui_vulkan::Application app(&window);
  return app.run();
}
