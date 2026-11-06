#include "Engine.h"
#include "Application.h"
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
			TraceLog(LOG_ERROR, "ENGINE: No Application set, call SetApplication() before Init()");
			return false;
		}

		if (width <= 0 || height <= 0)
		{
			TraceLog(LOG_ERROR, "ENGINE: The window size must be greater than 0");
			return false;
		}

		InitWindow(width, height, m_app->GetTitle());
		if (!IsWindowReady())
		{
			TraceLog(LOG_ERROR, "ENGINE: Window creation failed");
			return false;
		}
		m_isWindowOpen = true;

		m_isAppInitialized = m_app->Init();
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
			m_deltaTime = GetFrameTime();

			// Update game logic
			m_app->Update(m_deltaTime);

			// Render
			BeginDrawing();
				ClearBackground(RAYWHITE);
				m_app->DrawHUD();
			EndDrawing(); // Swaps buffers and polls input events
		}
	}

	void Engine::Shutdown()
	{
		if (m_app && m_isAppInitialized)
		{
			m_app->Destroy(); // logical clean up only
			m_isAppInitialized = false;
		}

		if (m_isWindowOpen)
		{
			CloseWindow();
			m_isWindowOpen = false;
		}
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
		return m_deltaTime;
	}
}
