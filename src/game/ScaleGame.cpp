#include "ScaleGame.h"

bool ScaleGame::Init()
{
	m_camera.position	= { 6.0f, 4.0f, 6.0f };
	m_camera.target		= { 0.0f, 0.5f, 0.0f };
	m_camera.up			= eng::Engine::GetInstance().GetWorld().worldUp;
	m_camera.fovy		= 60.0f;
	m_camera.projection	= CAMERA_PERSPECTIVE;

	BindInput();

	eng::Log::Info("ScaleGame initialized");
	return true;
}

void ScaleGame::BindInput()
{
	using eng::InputBinding;
	using eng::AxisBinding;

	eng::InputMap& map = eng::Engine::GetInstance().GetInputManager().GetMap();

	// Movement: WASD and arrows, clamped so diagonals are not faster
	map.BindAxis("MoveX", AxisBinding::Keys(KEY_A, KEY_D));
	map.BindAxis("MoveX", AxisBinding::Keys(KEY_LEFT, KEY_RIGHT));
	map.BindAxis("MoveY", AxisBinding::Keys(KEY_S, KEY_W));
	map.BindAxis("MoveY", AxisBinding::Keys(KEY_DOWN, KEY_UP));
	map.BindAxis2D("Move", "MoveX", "MoveY", true);

	// Look: mouse delta in screen points
	map.BindAxis("LookX", AxisBinding::MouseX());
	map.BindAxis("LookY", AxisBinding::MouseY());
	map.BindAxis2D("Look", "LookX", "LookY", false);

	// Scaling gun (Phase 11): stretch, shrink, change factor
	map.BindAction("Jump",		InputBinding::Key(KEY_SPACE));
	map.BindAction("Fire",		InputBinding::Mouse(MOUSE_BUTTON_LEFT));
	map.BindAction("AltFire",	InputBinding::Mouse(MOUSE_BUTTON_RIGHT));
	map.BindAxis("Scroll",		AxisBinding::Wheel());
}

void ScaleGame::Update(float deltaTime)
{
	UpdateCamera(&m_camera, CAMERA_ORBITAL); // TODO(Phase 2, cursor lock): mouse look

	const eng::InputMap& map = eng::Engine::GetInstance().GetInputManager().GetMap();
	if (map.IsActionPressed("Jump"))	++m_jumpPresses;
	if (map.IsActionPressed("Fire"))	++m_firePresses;
	if (map.IsActionPressed("AltFire"))	++m_altFirePresses;
	m_scrollTotal += map.GetAxis("Scroll");

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

	DrawInputPanel();
}

void ScaleGame::DrawInputPanel() const
{
	const eng::InputMap& map = eng::Engine::GetInstance().GetInputManager().GetMap();
	const Vector2 move = map.GetAxis2D("Move");
	const Vector2 look = map.GetAxis2D("Look");

	auto state = [&map](const char* action)
	{
		return map.IsActionDown(action) ? "DOWN" : "up";
	};

	const int x = 10, y = 140;
	DrawRectangle(x, y, 330, 120, Fade(LIGHTGRAY, 0.6f));
	DrawRectangleLines(x, y, 330, 120, GRAY);
	DrawText("Input (InputMap actions / axes)", x + 10, y + 10, 10, BLACK);
	DrawText(TextFormat("Move  (WASD / arrows)  x %+.2f  y %+.2f", move.x, move.y), x + 10, y + 30, 10, DARKGRAY);
	DrawText(TextFormat("Look  (mouse delta)    x %+.1f  y %+.1f", look.x, look.y), x + 10, y + 45, 10, DARKGRAY);
	DrawText(TextFormat("Jump    (Space) %-4s  presses %d", state("Jump"), m_jumpPresses), x + 10, y + 60, 10, DARKGRAY);
	DrawText(TextFormat("Fire    (LMB)   %-4s  presses %d", state("Fire"), m_firePresses), x + 10, y + 75, 10, DARKGRAY);
	DrawText(TextFormat("AltFire (RMB)   %-4s  presses %d", state("AltFire"), m_altFirePresses), x + 10, y + 90, 10, DARKGRAY);
	DrawText(TextFormat("Scroll  (wheel) total %+.1f", m_scrollTotal), x + 10, y + 105, 10, DARKGRAY);
}

void ScaleGame::Destroy()
{
	eng::Log::Info("ScaleGame destroyed");
}

const char* ScaleGame::GetTitle() const
{
	return "Build to Scale";
}
