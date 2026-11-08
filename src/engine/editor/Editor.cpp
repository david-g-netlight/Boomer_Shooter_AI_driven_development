#include "Editor.h"
#include <cmath>
#include <string>
#include <raymath.h>
#include "core/Engine.h"
#include "core/Log.h"
#include "input/InputManager.h"
#include "level/GameActor.h"
#include "level/Level.h"
#include "core/Application.h"
#include "editor/ActorEdit.h"
#include "editor/EditorUI.h"
#include "level/LevelSerializer.h"
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

		// Bullet wireframes are part of the editor overlay
		PhysicsWorld& physics = Engine::GetInstance().GetPhysics();
		m_physicsDebugWasOn = physics.IsDebugDrawEnabled();
		physics.SetDebugDrawEnabled(true);

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
		Engine::GetInstance().GetPhysics().SetDebugDrawEnabled(m_physicsDebugWasOn);
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

		if (m_isLoadPending && m_level)
			LoadNow();

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
			if (input.IsKeyPressed(KEY_D) && (input.IsKeyDown(KEY_LEFT_SUPER) || input.IsKeyDown(KEY_LEFT_CONTROL)))
				DuplicateSelected();
			if (input.IsKeyPressed(KEY_S) && (input.IsKeyDown(KEY_LEFT_SUPER) || input.IsKeyDown(KEY_LEFT_CONTROL)))
				Save();
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
		m_toolbar.Draw(*this);
		m_hierarchy.Draw(*this, *m_level);
		m_inspector.Draw(*this);
	}

	std::string Editor::GetLevelPath() const
	{
		const Application* app = Engine::GetInstance().GetApplication();
		return app ? app->GetLevelPath() : std::string();
	}

	bool Editor::CanSaveLoad() const	{ return m_level && !GetLevelPath().empty(); }

	void Editor::Save()
	{
		if (!CanSaveLoad())
			return;

		const std::string path = GetLevelPath();
		LevelSerializer::Save(*m_level, path);
#if defined(ENG_SOURCE_DIR)
		// The build copies assets/ over the executable's copy: keep a copy where the developer commits it
		LevelSerializer::Save(*m_level, std::string(ENG_SOURCE_DIR) + "/" + path);
#endif
	}

	void Editor::RequestLoad()
	{
		if (CanSaveLoad())
			m_isLoadPending = true;
	}

	void Editor::LoadNow()
	{
		m_isLoadPending = false;
		m_selected = nullptr;
		m_gizmo.CancelDrag();

		if (!LevelSerializer::Load(*m_level, GetLevelPath()).ok)
			return;

		if (Application* app = Engine::GetInstance().GetApplication())
			app->OnLevelLoaded();
		m_snapshot.Capture(*m_level);
	}

	GameActor* Editor::DuplicateSelected()
	{
		if (!m_selected || !m_level)
			return nullptr;

		GameActor* copy = LevelSerializer::Duplicate(*m_level, *m_selected);
		if (!copy)
			return nullptr;

		// Step aside so the copy doesn't hide the original
		::Transform transform = copy->GetTransform();
		transform.translation.x += 1.0f;
		ActorEdit::SetLocalTransform(*copy, transform);
		Select(copy);
		return copy;
	}

	bool Editor::IsTyping() const { return m_hierarchy.IsEditingText() || m_inspector.IsEditingText(); }

	GameActor* Editor::AddCube()
	{
		if (!m_level)
			return nullptr;

		const Camera& camera = m_camera.GetCamera();
		GameActor* actor = m_level->CreateGameActor("Cube " + std::to_string(++m_spawnCounter));
		actor->SetPosition(Vector3Add(camera.GetPosition(), Vector3Scale(camera.Forward(), 4.0f)));

		ResourceManager& resources = Engine::GetInstance().GetResources();
		actor->AddComponent<MeshRenderer>(resources.GetPrimitive(Primitive::Cube))
			->CreateOwnMaterial(resources.GetDefaultLitShader()).SetTint({ 200, 160, 90, 255 });
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

	void Editor::SetSpawnHere()
	{
		if (!m_level)
			return;

		const Camera& camera = m_camera.GetCamera();
		Vector3 feet = camera.GetPosition();
		if (const auto hit = Engine::GetInstance().GetPhysics().Raycast(feet, { 0.0f, -1.0f, 0.0f }, 200.0f, Layer::Ground | Layer::Prop))
			feet = hit->point;

		m_level->SetSpawn(feet, camera.GetYaw());
		Log::Info("Editor: spawn set to %.2f %.2f %.2f yaw %.0f", static_cast<double>(feet.x), static_cast<double>(feet.y),
			static_cast<double>(feet.z), static_cast<double>(camera.GetYaw()));
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
		if (!m_isActive)
			return;

		// Grid at y = 0 (the game's debug view draws its own when it is on)
		if (!Engine::GetInstance().GetDebugDraw().IsEnabled())
			DrawGrid(80, 1.0f);

		// Trigger volumes: translucent box from the collider, so they show even without a mesh
		if (m_level)
			for (const auto& owned : m_level->GetActors())
			{
				const GameActor& actor = *owned;
				const Collider* collider = actor.GetComponent<Collider>();
				if (actor.IsMarkedForDestroy() || !collider || !actor.GetComponent<TriggerVolume>() || actor.GetComponent<MeshRenderer>())
					continue;

				const BoundingBox box = collider->GetWorldBounds();
				const Vector3 size = { box.max.x - box.min.x, box.max.y - box.min.y, box.max.z - box.min.z };
				const Vector3 center = { (box.min.x + box.max.x) * 0.5f, (box.min.y + box.max.y) * 0.5f, (box.min.z + box.max.z) * 0.5f };
				DrawCubeV(center, size, Fade(SKYBLUE, 0.25f));
				DrawCubeWiresV(center, size, Fade(BLUE, 0.8f));
			}

		// Spawn point: the player's capsule and the way it faces
		if (m_level && m_level->GetSpawn().isSet)
		{
			const SpawnPoint& spawn = m_level->GetSpawn();
			const float yaw = spawn.yawDegrees * DEG2RAD;
			const Vector3 feet = spawn.position;
			DrawCapsuleWires({ feet.x, feet.y + 0.5f, feet.z }, { feet.x, feet.y + 1.5f, feet.z }, 0.5f, 8, 4, GREEN);
			DrawLine3D({ feet.x, feet.y + 1.5f, feet.z }, { feet.x + std::sin(yaw) * 1.5f, feet.y + 1.5f, feet.z - std::cos(yaw) * 1.5f }, GREEN);
		}

		if (!m_selected || m_selected->IsMarkedForDestroy())
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
