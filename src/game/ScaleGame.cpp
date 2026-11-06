#include "ScaleGame.h"
#include <cmath>
#include <raymath.h>

bool ScaleGame::Init()
{
	m_camera.SetPosition({ 6.0f, 4.0f, 6.0f });
	m_camera.LookAt({ 0.0f, 0.5f, 0.0f });
	m_camera.SetPerspective({ 60.0f, 0.05f, 200.0f });
	eng::Engine::GetInstance().GetMainCamera().SetActiveCamera(m_camera);

	// Ring of cubes, each with its own material: hue around the circle, growing size
	m_cubeMaterials.reserve(CUBE_COUNT);
	m_cubeTransforms.reserve(CUBE_COUNT);
	for (int i = 0; i < CUBE_COUNT; ++i)
	{
		const float t		= static_cast<float>(i) / CUBE_COUNT;
		const float angle	= t * 2.0f * PI;
		const float size	= 0.6f + 0.5f * t;

		eng::Material& material = m_cubeMaterials.emplace_back();
		material.SetTint(ColorFromHSV(t * 360.0f, 0.75f, 0.95f));

		m_cubeTransforms.push_back(MatrixMultiply(
			MatrixMultiply(MatrixScale(size, size, size), MatrixRotateY(angle)),
			MatrixTranslate(std::cos(angle) * 4.0f, size * 0.5f, std::sin(angle) * 4.0f)));
	}

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

	// TODO(Phase 8): cursor is released by Pause (ESC); temporary toggle until then
	map.BindAction("ToggleCursor", InputBinding::Key(KEY_C));
}

void ScaleGame::Update(float deltaTime)
{
	eng::InputManager&		input	= eng::Engine::GetInstance().GetInputManager();
	const eng::InputMap&	map		= input.GetMap();

	if (map.IsActionPressed("ToggleCursor"))
		input.SetCursorLocked(!input.IsCursorLocked());

	if (input.IsCursorLocked())
	{
		// TODO(Phase 10): replaced by FirstPersonLook / FirstPersonController
		const Vector2 look = map.GetAxis2D("Look");
		const Vector2 move = map.GetAxis2D("Move");
		m_camera.Rotate(look.x * LOOK_SENSITIVITY, -look.y * LOOK_SENSITIVITY);	// screen y grows downwards
		m_camera.MoveLocal(move.y * FLY_SPEED * deltaTime, move.x * FLY_SPEED * deltaTime, 0.0f);
	}
	else
	{
		m_orbitAngle += ORBIT_SPEED * deltaTime;
		const float angle = m_orbitAngle * DEG2RAD;
		m_camera.SetPosition({ std::sin(angle) * ORBIT_RADIUS, 4.0f, std::cos(angle) * ORBIT_RADIUS });
		m_camera.LookAt({ 0.0f, 0.5f, 0.0f });
	}

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

void ScaleGame::SubmitRenderables(eng::RenderQueue& queue)
{
	const ::Mesh* cube = eng::Engine::GetInstance().GetResources().GetPrimitive(eng::Primitive::Cube);
	for (int i = 0; i < CUBE_COUNT; ++i)
		queue.Submit(*cube, m_cubeMaterials[i], m_cubeTransforms[i]);
}

void ScaleGame::DrawHUD()
{
	eng::Engine&			engine	= eng::Engine::GetInstance();
	const eng::WindowInfo&	window	= engine.GetWindow();
	const eng::Time&		time	= engine.GetTime();

	DrawRectangle(10, 10, 330, 120, Fade(SKYBLUE, 0.5f));
	DrawRectangleLines(10, 10, 330, 120, BLUE);
	DrawText(TextFormat("%s %s  |  editor %s", eng::GetEngineName(), eng::GetEngineVersion(),
		eng::IsEditorBuild() ? "ON" : "OFF"), 20, 20, 10, BLACK);
	DrawText(TextFormat("FPS %d   dt %.4f s   draws %zu", GetFPS(), time.GetDeltaTime(),
		engine.GetRenderQueue().GetCommandCount()), 20, 40, 10, DARKGRAY);
	DrawText(TextFormat("Fixed steps/s %d (target %d)", m_fixedStepsPerSecond,
		static_cast<int>(1.0f / eng::FIXED_DELTA_TIME)), 20, 60, 10, DARKGRAY);
	DrawText(TextFormat("Screen %d x %d   Render %d x %d   DPI x%.1f", window.screenWidth, window.screenHeight,
		window.renderWidth, window.renderHeight, window.dpiScale), 20, 80, 10, DARKGRAY);
	DrawText(TextFormat("C: %s cursor  |  close the window to quit",
		engine.GetInputManager().IsCursorLocked() ? "unlock" : "lock (mouse look + WASD fly)"), 20, 100, 10, DARKGRAY);

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
	m_cubeMaterials.clear();
	m_cubeTransforms.clear();
	eng::Log::Info("ScaleGame destroyed");
}

const char* ScaleGame::GetTitle() const
{
	return "Build to Scale";
}
