#include "Editor.h"
#include <cmath>
#include <string>
#include <raymath.h>
#include "core/Engine.h"
#include "core/Log.h"
#include "input/InputManager.h"
#include "level/GameActor.h"
#include "level/Level.h"
#include "editor/EditorUI.h"
#include "editor/Picker.h"
#include "physics/Collider.h"
#include "physics/RigidBody.h"
#include "physics/TriggerVolume.h"
#include "render/MeshRenderer.h"
#include "resources/ResourceManager.h"

namespace eng
{
	void Editor::Enter(Level* level)
	{
		m_level		= level;
		m_isActive	= true;

		// Fly from where the player was looking
		ActiveCamera& active = Engine::GetInstance().GetMainCamera();
		m_previousCamera = active.GetActiveCamera();
		if (m_previousCamera)
			m_camera.CopyPoseFrom(*m_previousCamera);
		active.SetActiveCamera(m_camera.GetCamera());

		if (!m_level)
		{
			m_snapshot.Clear();
			Log::Warning("Editor: no active level, nothing to edit");
			return;
		}

		m_selected = nullptr;
		m_snapshot.Capture(*m_level);
		Log::Info("Editor: entered, snapshot of %zu actors", m_snapshot.GetActorCount());
	}

	void Editor::Exit()
	{
		m_isActive = false;
		m_selected = nullptr;
		m_gizmo.CancelDrag();
		if (m_previousCamera)
			Engine::GetInstance().GetMainCamera().SetActiveCamera(*m_previousCamera);
		m_previousCamera = nullptr;
		Log::Info("Editor: exited, edits stay live");
	}

	void Editor::Reset()
	{
		if (!m_level || m_snapshot.IsEmpty())
			return;

		const LevelSnapshot::RestoreResult result = m_snapshot.Restore(*m_level);
		Log::Info("Editor: reset, %d restored, %d new actors removed", result.restored, result.removed);
		if (result.missing > 0)
			Log::Warning("Editor: %d deleted actors cannot be restored yet", result.missing);
	}

	void Editor::Update(InputManager& input, float unscaledDeltaTime)
	{
		if (!m_isActive)
			return;

		ValidateSelection();
		m_camera.Update(input, unscaledDeltaTime);

		const bool isLooking = input.IsCursorLocked();	// RMB fly mode: keys and clicks belong to the camera
		if (isLooking)
		{
			m_gizmo.CancelDrag();
			return;
		}

		const bool overPanels = EditorUI::IsOverPanels(input.GetMousePosition());
		if (!IsTyping())
		{
			if (input.IsKeyPressed(KEY_W))		m_gizmo.SetMode(GizmoMode::Move);
			if (input.IsKeyPressed(KEY_R))		m_gizmo.SetMode(GizmoMode::Scale);
			if (input.IsKeyPressed(KEY_DELETE))	DeleteSelected();
		}

		const bool gizmoHasMouse = !overPanels && m_gizmo.Update(m_camera.GetCamera(), input.GetMousePosition(),
			input.IsMouseButtonPressed(MOUSE_BUTTON_LEFT), input.IsMouseButtonDown(MOUSE_BUTTON_LEFT), m_selected);
		if (m_level && !overPanels && !gizmoHasMouse && input.IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
			Select(Picker::Pick(m_camera.GetCamera(), input.GetMousePosition(), *m_level, Engine::GetInstance().GetPhysics()));

		if (m_selected && !IsTyping() && input.IsKeyPressed(KEY_F))
		{
			const BoundingBox box = Picker::GetActorBounds(*m_selected);
			const Vector3 center	= { (box.min.x + box.max.x) * 0.5f, (box.min.y + box.max.y) * 0.5f, (box.min.z + box.max.z) * 0.5f };
			const Vector3 extent	= { box.max.x - box.min.x, box.max.y - box.min.y, box.max.z - box.min.z };
			m_camera.FocusOn(center, 0.5f * std::sqrt(extent.x * extent.x + extent.y * extent.y + extent.z * extent.z));
		}
	}

	void Editor::ValidateSelection()
	{
		if (!m_selected)
			return;

		// The Level deletes marked actors at the end of the frame: a pointer that is no longer in it is gone
		bool found = false;
		if (m_level && !m_selected->IsMarkedForDestroy())
			for (const auto& owned : m_level->GetActors())
				if (owned.get() == m_selected) { found = true; break; }
		if (!found)
			m_selected = nullptr;
	}

	void Editor::DrawPanels()
	{
		if (!m_isActive || !m_level)
			return;

		// A drag started over a panel must not keep the gizmo alive: panels only react to the free cursor
		m_hierarchy.Draw(*this, *m_level);
	}

	bool Editor::IsTyping() const { return m_hierarchy.IsEditingText(); }

	void Editor::SetDefaultShader(const Shader* shader) { m_defaultShader = shader; }

	GameActor* Editor::AddCube()
	{
		if (!m_level)
			return nullptr;

		const Camera& camera = m_camera.GetCamera();
		GameActor* actor = m_level->CreateGameActor("Cube " + std::to_string(++m_spawnCounter));
		actor->SetPosition(Vector3Add(camera.GetPosition(), Vector3Scale(camera.Forward(), 4.0f)));

		ResourceManager& resources = Engine::GetInstance().GetResources();
		actor->AddComponent<MeshRenderer>(resources.GetPrimitive(Primitive::Cube))
			->CreateOwnMaterial(m_defaultShader).SetTint({ 200, 160, 90, 255 });
		actor->AddComponent<Collider>()->SetLayer(Layer::Prop);
		RigidBodySettings settings;
		settings.mass = 2.0f;
		actor->AddComponent<RigidBody>(settings);

		Select(actor);
		return actor;
	}

	GameActor* Editor::AddTrigger()
	{
		if (!m_level)
			return nullptr;

		const Camera& camera = m_camera.GetCamera();
		GameActor* actor = m_level->CreateGameActor("Trigger " + std::to_string(++m_spawnCounter));
		actor->SetPosition(Vector3Add(camera.GetPosition(), Vector3Scale(camera.Forward(), 4.0f)));
		actor->SetScale(2.0f);

		ResourceManager& resources = Engine::GetInstance().GetResources();
		actor->AddComponent<MeshRenderer>(resources.GetPrimitive(Primitive::Cube))
			->CreateOwnMaterial().SetTint({ 60, 140, 255, 70 });
		actor->AddComponent<Collider>()->SetLayer(Layer::Trigger);
		actor->AddComponent<TriggerVolume>();

		Select(actor);
		return actor;
	}

	void Editor::DeleteSelected()
	{
		if (!m_selected)
			return;

		Log::Info("Editor: deleted '%s'", m_selected->GetName().c_str());
		m_selected->MarkForDestroy();
		m_selected = nullptr;
	}

	void Editor::Select(GameActor* actor)
	{
		m_selected = actor;
		if (actor)
			Log::Info("Editor: selected '%s'", actor->GetName().c_str());
	}

	GameActor* Editor::GetSelected() const	{ return m_selected; }

	void Editor::Draw3D() const
	{
		if (!m_isActive || !m_selected || m_selected->IsMarkedForDestroy())
			return;

		DrawBoundingBox(Picker::GetActorBounds(*m_selected), ORANGE);
		m_gizmo.Draw3D(m_camera.GetCamera(), m_selected);
	}

	EditorCamera& Editor::GetCamera()					{ return m_camera; }
	TransformGizmo& Editor::GetGizmo()					{ return m_gizmo; }
	bool Editor::IsActive() const						{ return m_isActive; }
	Level* Editor::GetLevel() const						{ return m_level; }
	const LevelSnapshot& Editor::GetSnapshot() const	{ return m_snapshot; }
}
