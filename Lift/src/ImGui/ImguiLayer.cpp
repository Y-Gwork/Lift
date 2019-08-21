﻿#include "pch.h"
#include "ImguiLayer.h"

#include "imgui.h"

#include "Application.h"
#include "examples/imgui_impl_glfw.h"
#include "examples/imgui_impl_opengl3.h"
#include "GLFW/glfw3.h"
#include "glad/glad.h"
#include "imgui_internal.h"
#include <glm/common.hpp>

ivec2 lift::ImGuiLayer::render_window_size_;

// TEMPORARY
ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoMove;

lift::ImGuiLayer::ImGuiLayer()
	: Layer("ImGuiLayer") {

}

lift::ImGuiLayer::~ImGuiLayer() {
	ImGuiLayer::OnDetach();
};

void lift::ImGuiLayer::OnAttach() {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	auto& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows

	// Setup platform/Renderer bindings
	auto& app = Application::Get();
	auto* window_handle = static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow());
	ImGui_ImplGlfw_InitForOpenGL(window_handle, true);
	ImGui_ImplOpenGL3_Init("#version 410");

}

void lift::ImGuiLayer::OnDetach() {
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void lift::ImGuiLayer::OnUpdate() {
}

void lift::ImGuiLayer::OnImguiRender() {
	static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None | ImGuiDockNodeFlags_NoResize;

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->Pos);
	ImGui::SetNextWindowSize(viewport->Size);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
	window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
		window_flags |= ImGuiWindowFlags_NoBackground;

	static bool p_open = true;
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("DockSpace Demo", &p_open, window_flags);
	ImGui::PopStyleVar();

	ImGui::PopStyleVar(2);

	// DockSpace
	ImGuiIO& io = ImGui::GetIO();
	if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
	}
	else {
	}

	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("Docking")) {
			if (ImGui::MenuItem("Flag: NoSplit", "", (dockspace_flags & ImGuiDockNodeFlags_NoSplit) != 0))
				dockspace_flags ^= ImGuiDockNodeFlags_NoSplit;
			if (ImGui::MenuItem("Flag: NoResize", "", (dockspace_flags & ImGuiDockNodeFlags_NoResize) != 0))
				dockspace_flags ^= ImGuiDockNodeFlags_NoResize;
			if (ImGui::MenuItem("Flag: NoDockingInCentralNode", "",
								(dockspace_flags & ImGuiDockNodeFlags_NoDockingInCentralNode) != 0))
				dockspace_flags ^=
					ImGuiDockNodeFlags_NoDockingInCentralNode;
			if (ImGui::MenuItem("Flag: PassthruCentralNode", "",
								(dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) != 0))
				dockspace_flags ^=
					ImGuiDockNodeFlags_PassthruCentralNode;
			if (ImGui::MenuItem("Flag: AutoHideTabBar", "", (dockspace_flags & ImGuiDockNodeFlags_AutoHideTabBar) != 0))
				dockspace_flags ^= ImGuiDockNodeFlags_AutoHideTabBar;
			ImGui::Separator();
			if (ImGui::MenuItem("Close DockSpace", nullptr, false, p_open != false))
				p_open = false;
			ImGui::EndMenu();
		}
		ImGui::EndMenuBar();
	}

	ImGui::End();

	auto& app = Application::Get();
	ImGui::Begin("Editor");
	//ImGui::ColorEdit3("Top color", &app.GetTopColor().x);
	//ImGui::ColorEdit3("Bottom color", &app.GetBottomColor().x);
	if (ImGui::ColorEdit3("Albedo", &app.material_albedo_.x))
		app.RestartAccumulation();

	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
				ImGui::GetIO().Framerate);
	ImGui::End();

	ImGui::Begin("Render");
	const auto window = ImGui::GetCurrentWindow();
	auto size = ivec2(window->Size.x, window->Size.y);
	if(size != render_window_size_) {
		app.Resize(size);
		render_window_size_ = size;
	}
	ImGui::Image(ImTextureID(app.GetFrameTextureId()), 
		{static_cast<float>(size.x), static_cast<float>(size.y - 40)},
		{0.f, 1.f}, {1.f, 0.f});
	is_render_hovered_ = ImGui::IsWindowHovered();
	ImGui::End();
}

void lift::ImGuiLayer::OnEvent(Event& event) {
	const auto& io = ImGui::GetIO();
	if (io.WantCaptureMouse && !is_render_hovered_ && event.GetEventType() == EventType::MouseMoved)
		event.handled_ = true;
}

ivec2 lift::ImGuiLayer::GetRenderWindowSize() {
	return render_window_size_;
}

void lift::ImGuiLayer::Begin() {
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

}

void lift::ImGuiLayer::End() {
	auto& io = ImGui::GetIO();
	auto& app = Application::Get();
	io.DisplaySize = ImVec2(static_cast<float>(app.GetWindow().Width()),
							static_cast<float>(app.GetWindow().Height()));

	// Rendering
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		const auto backup_current_context = glfwGetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		glfwMakeContextCurrent(backup_current_context);
	}
}
