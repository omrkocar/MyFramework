#include "EditorPCH.h"
#include "SceneEditor.h"

#include "Editor/Application.h"

#include <Saz/Utils/PlatformUtils.h>
#include <Saz/SceneSerializer.h>
#include <Saz/Rendering/Renderer.h>
#include <Saz/Rendering/Renderer2D.h>
#include <Saz/Rendering/Framebuffer.h>
#include <Saz/RenderComponents.h>
#include <Saz/WindowResizedOneFrameComponent.h>
#include <Saz/CameraComponent.h>

#include <imgui/imgui.h>
#include "Saz/SceneComponent.h"
#include "Saz/InputComponent.h"

namespace ecs
{
	SceneEditor::SceneEditor()
	{
		
	}

	void SceneEditor::Init()
	{
		Entity m_FrameBufferEntity = m_World->CreateEntity();
		auto& frameBufferComp = m_World->AddComponent<component::FrameBufferComponent>(m_FrameBufferEntity);
		Saz::FrameBufferSpecification fbSpec;
		fbSpec.Width = 1280;
		fbSpec.Height = 720;
		frameBufferComp.FrameBuffer = Saz::FrameBuffer::Create(fbSpec);
		m_FrameBuffer = frameBufferComp.FrameBuffer;

		m_World->m_Registry.on_construct<component::CameraComponent>().connect<&SceneEditor::OnCameraComponentAdded>(this);
	}

	void SceneEditor::Update(const Saz::GameTime& gameTime)
	{
		auto& registry = m_World->m_Registry;

		const auto windowResizeView = m_World->GetAllEntitiesWith<const component::WindowResizedOneFrameComponent>();
		for (auto& ent : windowResizeView)
		{
			m_World->DestroyEntity(ent);
		}

		const auto frameBufferView = m_World->GetAllEntitiesWith<component::FrameBufferComponent>();
		for (const auto& frameBufferEntity : frameBufferView)
		{
			const auto& frameBufferComp = m_World->m_Registry.get<component::FrameBufferComponent>(frameBufferEntity);
			Saz::FrameBufferSpecification spec = m_FrameBuffer->GetSpecification();
			if (m_SceneSize.x > 0.0f && m_SceneSize.y > 0.0f && // zero sized framebuffer is invalid
				(spec.Width != m_SceneSize.x || spec.Height != m_SceneSize.y))
			{
				uint32_t width = (uint32_t)m_SceneSize.x;
				uint32_t height = (uint32_t)m_SceneSize.y;
				m_FrameBuffer->Resize(width, height);

				auto& world = Saz::Application::Get().GetWorld();
				ecs::Entity entity = world.CreateEntity();
				world.AddComponent<component::WindowResizedOneFrameComponent>(entity, width, height);
			}
		}

		const auto inputView = m_World->GetAllEntitiesWith<component::InputComponent>();
		for (const auto& inputEntity : inputView)
		{
			const auto& inputComp = m_World->m_Registry.get<component::InputComponent>(inputEntity);

			bool control = inputComp.IsKeyHeld(Input::KeyCode::LeftControl) || inputComp.IsKeyHeld(Input::KeyCode::RightControl);
			bool shift = inputComp.IsKeyHeld(Input::KeyCode::LeftShift) || inputComp.IsKeyHeld(Input::KeyCode::RightShift);

			if (inputComp.IsKeyHeld(Input::KeyCode::S))
			{
				if (control)
				{
					if (shift)
						SaveSceneAs();
					else
						SaveScene();
				}
			}
			
			if (inputComp.IsKeyHeld(Input::KeyCode::N))
			{
				if (control)
				{
					NewScene();
				}
			}

			if (inputComp.IsKeyHeld(Input::KeyCode::O))
			{
				if (control)
				{
					OpenScene();
				}
			}
		}
	}

	void SceneEditor::ImGuiRender()
	{
		DrawScene();
		DrawMenuBar();
		DrawProfiler();
	}

	void SceneEditor::DrawScene()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });

		// Scene
		ImGui::Begin("Scene");
		m_ViewportFocused = ImGui::IsWindowFocused();
		m_ViewPortHovered = ImGui::IsWindowHovered();
		m_World->GetSingleComponent<component::LoadedSceneComponent>().IsFocused = m_ViewportFocused;
		m_World->GetSingleComponent<component::LoadedSceneComponent>().IsHovered = m_ViewPortHovered;

		ImVec2 scenePanelSize = ImGui::GetContentRegionAvail();
		if (m_SceneSize != *((glm::vec2*)&scenePanelSize))
		{
			m_SceneSize = { scenePanelSize.x, scenePanelSize.y };
		}

		uint32_t textureId = m_FrameBuffer->GetColorAttachmentRendererID();
		ImGui::Image((void*)textureId, ImVec2{ m_SceneSize.x, m_SceneSize.y }, ImVec2{ 0,1 }, ImVec2{ 1, 0 });

		ImGui::End();

		ImGui::PopStyleVar();
	}

	void SceneEditor::DrawProfiler()
	{
		ImGui::Begin("Profiler");
		auto stats = Saz::Renderer2D::GetStats();
		ImGui::Text("Renderer2D Stats: ");
		ImGui::Text("DrawCalls: %d ", stats.DrawCalls);
		ImGui::Text("Quads: %d", stats.QuadCount);
		ImGui::Text("Vertices: %d", stats.GetTotalVertexCount());
		ImGui::Text("Indices: %d", stats.GetTotalIndexCount());

		ImGui::End();
	}

	void SceneEditor::DrawMenuBar()
	{
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("New", "Ctrl+N"))
				{
					NewScene();
				}

				ImGui::Separator();

				if (ImGui::MenuItem("Open...", "Ctrl+O"))
				{
					OpenScene();
				}

				ImGui::Separator();

				if (ImGui::MenuItem("Save Scene", "Ctrl+S"))
				{
					SaveScene();
				}

				ImGui::Separator();

				if (ImGui::MenuItem("Save As...", "Ctrl+Shift+S"))
				{
					SaveSceneAs();
				}

				ImGui::Separator();

				if (ImGui::MenuItem("Exit"))
				{
					Application::Get().Close();
				}

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}
	}

	void SceneEditor::OnCameraComponentAdded(entt::registry& registry, entt::entity entity)
	{
		if (m_SceneSize.x == 0 || m_SceneSize.y == 0)
			return;

		m_World->GetComponent<component::CameraComponent>(entity).Camera.SetViewportSize(m_SceneSize.x, m_SceneSize.y);
	}

	void SceneEditor::NewScene()
	{
		auto entity = m_World->CreateEntity();
		auto& sceneComponent = m_World->AddComponent<component::NewSceneRequestOneFrameComponent>(entity);

		Saz::Renderer::OnWindowResize((uint32_t)m_SceneSize.x, (uint32_t)m_SceneSize.y);
	}

	void SceneEditor::OpenScene()
	{
		const String& path = Saz::FileDialogs::OpenFile("Saz Scene (*.saz)\0*.saz\0");
		if (!path.empty())
		{
			auto entity = m_World->CreateEntity();
			auto& sceneComponent = m_World->AddComponent<component::LoadSceneRequestOneFrameComponent>(entity);
			sceneComponent.Path = path;
			Saz::Renderer::OnWindowResize((uint32_t)m_SceneSize.x, (uint32_t)m_SceneSize.y);
		}
	}

	void SceneEditor::SaveScene()
	{
		String scenePath;
		scenePath = m_World->GetSingleComponent<component::LoadedSceneComponent>().Path;
		if (scenePath.empty())
		{
			scenePath = Saz::FileDialogs::SaveFile("Saz Scene (*.saz)\0*.saz\0");
		}

		auto entity = m_World->CreateEntity();
		auto& sceneComponent = m_World->AddComponent<component::SaveSceneRequestOneFrameComponent>(entity);
		sceneComponent.Path = scenePath;
	}

	void SceneEditor::SaveSceneAs()
	{
		const String& path = Saz::FileDialogs::SaveFile("Saz Scene (*.saz)\0*.saz\0");
		if (!path.empty())
		{
			auto entity = m_World->CreateEntity();
			auto& sceneComponent = m_World->AddComponent<component::SaveSceneRequestOneFrameComponent>(entity);
			sceneComponent.Path = path;
		}
	}

}