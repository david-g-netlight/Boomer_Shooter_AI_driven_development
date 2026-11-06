#include "ScaleGame.h"

bool ScaleGame::Init()
{
	m_camera.position	= { 6.0f, 4.0f, 6.0f };
	m_camera.target		= { 0.0f, 0.5f, 0.0f };
	m_camera.up			= eng::Engine::GetInstance().GetWorld().worldUp;
	m_camera.fovy		= 60.0f;
	m_camera.projection	= CAMERA_PERSPECTIVE;

	eng::Log::Info("ScaleGame initialized");
	return true;
}

void ScaleGame::Update(float deltaTime)
{
	UpdateCamera(&m_camera, CAMERA_ORBITAL); // No input yet (Phase 2): the camera orbits by itself

	m_secondTimer += deltaTime;
	if (m_secondTimer >= 1.0f)
	{
		m_fixedStepsPerSecond	= m_fixedStepsThisSecond;
		m_fixedStepsThisSecond	= 0;
		m_secondTimer			-= 1.0f;
	}
}

void ScaleGame::FixedUpdate(float /*fixedDeltaTime*/)
{
	++m_fixedStepsThisSecond;
}

void ScaleGame::DrawHUD()
{
	// TODO(Phase 3): the 3D pass moves into the Engine (RenderQueue inside BeginMode3D)
	BeginMode3D(m_camera);
		DrawCube({ 0.0f, 0.5f, 0.0f }, 1.0f, 1.0f, 1.0f, RED);
		DrawCubeWires({ 0.0f, 0.5f, 0.0f }, 1.0f, 1.0f, 1.0f, MAROON);
		DrawGrid(10, 1.0f);
	EndMode3D();

	eng::Engine&			engine	= eng::Engine::GetInstance();
	const eng::WindowInfo&	window	= engine.GetWindow();
	const eng::Time&		time	= engine.GetTime();

	DrawRectangle(10, 10, 330, 120, Fade(SKYBLUE, 0.5f));
	DrawRectangleLines(10, 10, 330, 120, BLUE);
	DrawText(TextFormat("%s %s  |  editor %s", eng::GetEngineName(), eng::GetEngineVersion(),
		eng::IsEditorBuild() ? "ON" : "OFF"), 20, 20, 10, BLACK);
	DrawText(TextFormat("FPS %d   dt %.4f s", GetFPS(), time.GetDeltaTime()), 20, 40, 10, DARKGRAY);
	DrawText(TextFormat("Fixed steps/s %d (target %d)", m_fixedStepsPerSecond,
		static_cast<int>(1.0f / eng::FIXED_DELTA_TIME)), 20, 60, 10, DARKGRAY);
	DrawText(TextFormat("Screen %d x %d   Render %d x %d   DPI x%.1f", window.screenWidth, window.screenHeight,
		window.renderWidth, window.renderHeight, window.dpiScale), 20, 80, 10, DARKGRAY);
	DrawText("Close the window to quit (ESC is reserved for Pause)", 20, 100, 10, DARKGRAY);
}

void ScaleGame::Destroy()
{
	eng::Log::Info("ScaleGame destroyed");
}

const char* ScaleGame::GetTitle() const
{
	return "Build to Scale";
}
