#include "imgui_layer.h"

#include <array>

#include "core.h"
#include "imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_vulkan.h"
#include "scene_list.h"
#include "user_settings.h"
#include "vulkan/descriptor_pool.h"
#include "vulkan/device.h"
#include "vulkan/frame_buffer.h"
#include "vulkan/instance.h"
#include "vulkan/render_pass.h"
#include "vulkan/single_time_commands.h"
#include "vulkan/surface.h"
#include "vulkan/swap_chain.h"
#include "vulkan/window.h"

void checkVulkanResultCallback(const VkResult err) {
    LF_ASSERT(err == VK_SUCCESS, "ImGui Vulkan Error: {0}", vulkan::toString(err));
}

ImguiLayer::ImguiLayer(vulkan::CommandPool& command_pool,
                       const vulkan::SwapChain& swap_chain,
                       const vulkan::DepthBuffer& depth_buffer,
                       UserSettings& user_settings) :
    user_settings_(user_settings) {
    const auto& device = swap_chain.device();
    const auto& window = device.surface().instance().window();

    // Initialise descriptor pool and render pass for ImGui.
    const std::vector<vulkan::DescriptorBinding> descriptor_bindings = {
        {0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0},
    };
    descriptor_pool_ = std::make_unique<vulkan::DescriptorPool>(device, descriptor_bindings, 1);
    render_pass_ = std::make_unique<vulkan::RenderPass>(swap_chain, depth_buffer, false, false);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    if (!ImGui_ImplGlfw_InitForVulkan(window.handle(), false)) {
        LF_ASSERT(false, "Failed to initialize Glfw -> Imgui");
        return;
    }

    // Initialise ImGui vulkan adapter
    ImGui_ImplVulkan_InitInfo vulkan_init = {};
    vulkan_init.Instance = device.surface().instance().handle();
    vulkan_init.PhysicalDevice = device.physicalDevice();
    vulkan_init.Device = device.handle();
    vulkan_init.QueueFamily = device.graphicsFamilyIndex();
    vulkan_init.Queue = device.graphicsQueue();
    vulkan_init.PipelineCache = nullptr;
    vulkan_init.DescriptorPool = descriptor_pool_->handle();
    vulkan_init.MinImageCount = swap_chain.minImageCount();
    vulkan_init.ImageCount = static_cast<uint32_t>(swap_chain.images().size());
    vulkan_init.Allocator = nullptr;
    vulkan_init.CheckVkResultFn = checkVulkanResultCallback;

    if (!ImGui_ImplVulkan_Init(&vulkan_init, render_pass_->handle())) {
        LF_ASSERT(false, "Failed to initialize Vulkan -> Imgui");
        return;
    }

    auto& io = ImGui::GetIO();

    io.IniFilename = nullptr;

    const auto scale_factor = window.contentScale();

    ImGui::StyleColorsDark();
    ImGui::GetStyle().ScaleAllSizes(scale_factor);

    const std::string& font_path = "../resources/fonts/Cousine-Regular.ttf";
    if (!io.Fonts->AddFontFromFileTTF(font_path.c_str(), 13 * scale_factor)) {
        LF_ASSERT(false, "Failed to load font {0}", font_path);
        return;
    }

    vulkan::SingleTimeCommands::submit(command_pool, [](VkCommandBuffer command_buffer) {
        if (!ImGui_ImplVulkan_CreateFontsTexture(command_buffer)) {
            LF_ASSERT(false, "Failed create ImGUi font textures");
            return;
        }
    });

    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

ImguiLayer::~ImguiLayer() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImguiLayer::render(VkCommandBuffer command_buffer,
                        const vulkan::FrameBuffer& frame_buffer,
                        const Statistics& statistics) {
    ImGui_ImplGlfw_NewFrame();
    ImGui_ImplVulkan_NewFrame();
    ImGui::NewFrame();

    drawSettings();
    drawOverlay(statistics);
    //    ImGui::ShowStyleEditor();
    ImGui::Render();

    std::array<VkClearValue, 2> clear_values = {};
    clear_values[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
    clear_values[1].depthStencil = {1.0f, 0};

    VkRenderPassBeginInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_info.renderPass = render_pass_->handle();
    render_pass_info.framebuffer = frame_buffer.handle();
    render_pass_info.renderArea.offset = {0, 0};
    render_pass_info.renderArea.extent = render_pass_->swapChain().extent();
    render_pass_info.clearValueCount = 0;  // static_cast<uint32_t>(clearValues.size());
    render_pass_info.pClearValues = clear_values.data();

    vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), command_buffer);
    vkCmdEndRenderPass(command_buffer);
}

bool ImguiLayer::wantsToCaptureKeyboard() {
    return ImGui::GetIO().WantCaptureKeyboard;
}

bool ImguiLayer::wantsToCaptureMouse() {
    return ImGui::GetIO().WantCaptureMouse;
}

void ImguiLayer::drawSettings() {
    if (!settings().show_settings) {
        return;
    }

    const float distance = 10.0f;
    const ImVec2 pos = ImVec2(distance, distance);
    const ImVec2 pos_pivot = ImVec2(0.0f, 0.0f);
    ImGui::SetNextWindowPos(pos, ImGuiCond_Always, pos_pivot);
    ImGui::SetNextWindowBgAlpha(0.8f);

    const auto flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;

    if (ImGui::Begin("Settings", &settings().show_settings, flags)) {
        std::vector<const char*> scenes;
        scenes.reserve(SceneList::allScenes.size());
        for (const auto& scene : SceneList::allScenes) { scenes.push_back(scene.first.c_str()); }

        ImGui::Text("scene");
        ImGui::Separator();
        ImGui::PushItemWidth(-1);
        ImGui::Combo("", &settings().scene_index, scenes.data(), static_cast<int>(scenes.size()));
        ImGui::PopItemWidth();
        ImGui::NewLine();

        if (ImGui::CollapsingHeader("ray tracing")) {
            ImGui::Separator();
            ImGui::Checkbox("enable denoising", &settings().is_denoised);
            ImGui::Checkbox("accumulate rays between frames", &settings().accumulate_rays);
            uint32_t min = 1, max = 16;
            ImGui::SliderScalar("samples per pixel", ImGuiDataType_U32, &settings().number_of_samples, &min, &max);
            min = 1, max = 32;
            ImGui::SliderScalar("path length", ImGuiDataType_U32, &settings().number_of_bounces, &min, &max);
            ImGui::NewLine();
        }

        if (ImGui::CollapsingHeader("camera settings")) {
            float fmin = 0.1f, fmax = 1000.0f;
            ImGui::SliderScalar("camera move speed",
                                ImGuiDataType_Float,
                                &settings().camera_move_speed,
                                &fmin,
                                &fmax,
                                "%.1f",
                                2.0f);
            ImGui::SliderScalar("camera mouse speed",
                                ImGuiDataType_Float,
                                &settings().camera_mouse_speed,
                                &fmin,
                                &fmax,
                                "%.1f");
            ImGui::Checkbox("gamma correction", &settings().gamma_correction);
            ImGui::Checkbox("show statistics", &settings().show_overlay);
            ImGui::NewLine();
        }
    }
    ImGui::End();
}

void ImguiLayer::drawOverlay(const Statistics& statistics) {
    if (!settings().show_overlay) {
        return;
    }

    const auto& io = ImGui::GetIO();
    const float distance = 10.0f;
    const ImVec2 pos = ImVec2(io.DisplaySize.x - distance, distance);
    const ImVec2 pos_pivot = ImVec2(1.0f, 0.0f);
    ImGui::SetNextWindowPos(pos, ImGuiCond_Always, pos_pivot);
    ImGui::SetNextWindowBgAlpha(0.3f);

    const auto flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDecoration
                       | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

    if (ImGui::Begin("Statistics", &settings().show_overlay, flags)) {
        ImGui::Text("Statistics");
        ImGui::Separator();
        ImGui::Text("frame size: (%dx%d)", statistics.framebufferSize.width, statistics.framebufferSize.height);
        ImGui::Text("frame rate: %.1f fps", statistics.frameRate);
        ImGui::Text("total samples:  %u", statistics.totalSamples);
    }
    ImGui::End();
}
