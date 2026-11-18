#include "Engine.h"
#include "Application.h"
#include "EngineInfo.h"
#include "Log.h"
#include "level/Level.h"
#include "physics/Collider.h"
#include "physics/RigidBody.h"
#include "physics/TriggerVolume.h"
#include "physics/CharacterMotor.h"
#include "render/MeshRenderer.h"
#include "ui/UIStyle.h"
#include "console/EngineCommands.h"
#include "console/EditorCommands.h"
#include <raylib.h>

namespace eng
{
	Engine& Engine::GetInstance()
	{
		static Engine instance;
		return instance;
	}

	Engine::~Engine()
	{
		delete m_app;
		m_app = nullptr;
	}

	bool Engine::Init(int width, int height)
	{
		if (!m_app)
		{
			Log::Error("No Application set, call SetApplication() before Init()");
			return false;
		}

		if (width <= 0 || height <= 0)
		{
			Log::Error("The window size must be greater than 0, got %d x %d", width, height);
			return false;
		}

		// Window hints must be set before the window exists
		SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI);

#if ENG_DEV_CONSOLE
		m_console.Init();	// before the window, so the startup log lines reach the scroll-back
#endif

		InitWindow(width, height, m_app->GetTitle());
		if (!IsWindowReady())
		{
			Log::Error("Window creation failed");
			return false;
		}
		m_isWindowOpen = true;

		// ESC is used for Pause (Phase 8), raylib closes the window on ESC by default
		SetExitKey(KEY_NULL);

		// Assets are copied next to the executable, so resolve paths from there,
		// no matter which directory the app was launched from
		ChangeDirectory(GetApplicationDirectory());

		SetTargetFPS(60);

		UIStyle::Apply();

		// A missing audio device is not fatal: the game runs silently
		m_audio.Init();

		m_componentRegistry.Register<MeshRenderer>("MeshRenderer");
		m_componentRegistry.Register<Collider>("Collider");
		m_componentRegistry.Register<RigidBody>("RigidBody");
		m_componentRegistry.Register<TriggerVolume>("TriggerVolume");
		m_componentRegistry.Register<CharacterMotor>("CharacterMotor");

		m_physics.Init(m_world.gravity);

		m_defaultCamera.SetPosition({ 0.0f, 2.0f, 5.0f });
		m_defaultCamera.LookAt({ 0.0f, 0.0f, 0.0f });
		m_activeCamera.SetActiveCamera(m_defaultCamera);

		m_window.Refresh();
		Log::Info("%s %s | screen %d x %d | render %d x %d | editor %s | console %s | audio %s",
			GetEngineName(), GetEngineVersion(),
			m_window.screenWidth, m_window.screenHeight, m_window.renderWidth, m_window.renderHeight,
			IsEditorBuild() ? "ON" : "OFF", IsDevConsoleBuild() ? "ON" : "OFF", m_audio.IsReady() ? "ON" : "OFF");

#if ENG_DEV_CONSOLE
		RegisterEngineCommands(*this, m_console);	// before the game, which adds its own
#if ENG_EDITOR
		RegisterEditorCommands(*this, m_console);
#endif
#endif

		m_isAppInitialized = m_app->Init();
		if (!m_isAppInitialized)
			Log::Error("Application::Init() failed");
		else
			m_modes.Begin(m_app->GetStartMode());

		return m_isAppInitialized;
	}

	void Engine::Run()
	{
		if (!m_app || !m_isAppInitialized)
			return;

		///////////////////////////////////////////////////////////////////////
		// THE GAME LOOP
		///////////////////////////////////////////////////////////////////////
		while (!WindowShouldClose() && !m_app->NeedsToBeClosed())
		{
			// 1. Time, window state and input snapshot
			m_time.BeginFrame(GetFrameTime());
			m_window.Refresh();
			m_inputManager.Poll(); // State polled by the previous EndDrawing()

#if ENG_DEV_CONSOLE
			m_console.HandleToggle(m_inputManager);	// grave opens / closes in every mode
#endif

			// Mode changes requested last frame (menus) or by ESC / TAB now take effect before anything updates
			m_modes.HandleInput(m_inputManager);
			m_modes.HandleFocus(m_window.isFocused);
			m_modes.ApplyPendingTransition();

			// 2. Fixed step (physics rate), then variable step. The level runs before the game
			Level* level = m_app->GetActiveLevel();

			bool isConsoleOpen = false;
#if ENG_DEV_CONSOLE
			isConsoleOpen = m_console.IsOpen();	// it owns the keyboard and the mouse: the editor's hotkeys stay quiet
#endif

			// Only Play simulates: in menus, Paused and the editor the world stays exactly as it was
			const bool isSimulating = m_modes.IsSimulating();
			if (isSimulating)
				m_time.AdvanceAccumulator();
			while (m_time.ConsumeFixedStep())
			{
				if (level) level->FixedUpdate(m_time.GetFixedDeltaTime());
				m_app->FixedUpdate(m_time.GetFixedDeltaTime());
				m_physics.Step(m_time.GetFixedDeltaTime());
			}

			if (isSimulating)
			{
				if (level) level->Update(m_time.GetDeltaTime());
				m_particles.Update(m_time.GetDeltaTime());
			}
			if (m_modes.GetMode() == EngineMode::Editor && !isConsoleOpen)
				m_editor.Update(m_inputManager, m_time.GetUnscaledDeltaTime());
			m_app->Update(m_time.GetDeltaTime());	// always runs: menu backdrop, editor camera, input

			// UI (unscaled: menus work while the game is frozen)
			m_ui.Update(m_time.GetUnscaledDeltaTime());

			// 3. Audio (unscaled: fades and ducking keep running while the game is paused)
			if (const Camera* listener = m_activeCamera.GetActiveCamera())
				m_audio.SetListener(listener->GetPosition(), listener->Right());
			m_audio.Update(m_time.GetUnscaledDeltaTime());

			// 4. Render
			BeginDrawing();
				ClearBackground(m_world.skyColor);

				// The game may have switched levels during Update
				level = m_app->GetActiveLevel();
				if (level) level->SubmitRenderables(m_renderQueue);
				m_app->SubmitRenderables(m_renderQueue);
				m_renderQueue.SetLighting({ m_world.sunDirection, m_world.sunColor, m_world.ambientColor });
				m_activeCamera.Begin3D();
					m_renderQueue.Draw(*m_activeCamera.GetActiveCamera());
					m_particles.Draw(*m_activeCamera.GetActiveCamera());	// transparent, after the opaque pass
					m_debugDraw.Draw3D();
					m_physics.DebugDraw();
					m_editor.Draw3D();
				m_activeCamera.End3D();
				m_debugDraw.DrawLabels(*m_activeCamera.GetActiveCamera());

				m_app->DrawViewModel();
				m_app->DrawHUD();
				if (m_modes.GetMode() == EngineMode::Editor)
					m_editor.DrawPanels();
#if ENG_DEV_CONSOLE
				m_console.DrawOverlay();
#endif
				m_ui.Draw();	// menus and dialogs on top of everything
			EndDrawing(); // Swaps buffers and polls input events

			// 5. End of frame: queued events (handlers may still look at actors marked this frame),
			//    then actors marked for destroy are deleted (nothing refers to them anymore)
			m_events.Dispatch();
			m_renderQueue.Clear();
			m_debugDraw.EndFrame(m_time.GetUnscaledDeltaTime());
			if (level) level->FlushDestroyed();
		}
	}

	void Engine::Shutdown()
	{
		m_ui.Clear();	// screens may refer to the game

		if (m_app && m_isAppInitialized)
		{
			m_app->Destroy(); // logical clean up only
			m_isAppInitialized = false;
			Log::Info("Application destroyed");
		}

		if (m_isWindowOpen)
		{
			// Bodies were removed by the game's Destroy (level cleared); then the world itself.
			// Aliases before the sounds they share data with; GPU/audio resources while the GL context
			// and the audio device still exist
			m_physics.Shutdown();
			m_audio.ReleaseVoices();
			m_resources.UnloadAll();
			m_audio.Shutdown();
			CloseWindow();
			m_isWindowOpen = false;
			Log::Info("Engine shut down");
		}

#if ENG_DEV_CONSOLE
		m_console.Shutdown();
#endif
	}

	void Engine::SetApplication(Application* app)
	{
		if (app == m_app)
			return;

		delete m_app;	// Delete previous
		m_app = app;	// Takes ownership
	}

	Application* Engine::GetApplication() const
	{
		return m_app;
	}

	float Engine::GetDeltaTime() const
	{
		return m_time.GetDeltaTime();
	}

	InputManager& Engine::GetInputManager()
	{
		return m_inputManager;
	}

	ResourceManager& Engine::GetResources()
	{
		return m_resources;
	}

	ActiveCamera& Engine::GetMainCamera()
	{
		return m_activeCamera;
	}

	RenderQueue& Engine::GetRenderQueue()
	{
		return m_renderQueue;
	}

	DebugDraw& Engine::GetDebugDraw()
	{
		return m_debugDraw;
	}

	AudioManager& Engine::GetAudio()
	{
		return m_audio;
	}

	PhysicsWorld& Engine::GetPhysics()
	{
		return m_physics;
	}

	ParticleSystem& Engine::GetParticles()
	{
		return m_particles;
	}

	EventBus& Engine::GetEvents()
	{
		return m_events;
	}

	ComponentRegistry& Engine::GetComponentRegistry()
	{
		return m_componentRegistry;
	}

	Editor& Engine::GetEditor()
	{
		return m_editor;
	}

	ModeMachine& Engine::GetModes()
	{
		return m_modes;
	}

	UIStack& Engine::GetUI()
	{
		return m_ui;
	}

#if ENG_DEV_CONSOLE
	DevConsole& Engine::GetConsole()
	{
		return m_console;
	}
#endif

	Time& Engine::GetTime()
	{
		return m_time;
	}

	const WindowInfo& Engine::GetWindow() const
	{
		return m_window;
	}

	const World& Engine::GetWorld() const
	{
		return m_world;
	}
}
