#include "ScaleGame.h"
#include <cmath>
#include <raymath.h>

bool ScaleGame::Init()
{
	m_camera.SetPosition({ 6.0f, 4.0f, 6.0f });
	m_camera.LookAt({ 0.0f, 0.5f, 0.0f });
	m_camera.SetPerspective({ 60.0f, 0.05f, 200.0f });
	eng::Engine::GetInstance().GetMainCamera().SetActiveCamera(m_camera);

	// Lit shader for even cubes and the ground; odd cubes keep raylib's unlit default shader.
	// If the shader fails to load, every material falls back to the default shader.
	const Shader* lit = eng::Engine::GetInstance().GetResources().LoadShader("assets/shaders/lit.vs", "assets/shaders/lit.fs");

	// Ring of cubes, each with its own material: hue around the circle, growing size
	m_cubeMaterials.reserve(CUBE_COUNT + 1);
	m_cubeTransforms.reserve(CUBE_COUNT);
	m_cubePositions.reserve(CUBE_COUNT);
	for (int i = 0; i < CUBE_COUNT; ++i)
	{
		const float t		= static_cast<float>(i) / CUBE_COUNT;
		const float angle	= t * 2.0f * PI;
		const float size	= 0.6f + 0.5f * t;

		eng::Material& material = m_cubeMaterials.emplace_back(i % 2 == 0 ? lit : nullptr);
		material.SetTint(ColorFromHSV(t * 360.0f, 0.75f, 0.95f));
		material.SetParameter("specularStrength", 0.6f);

		const Vector3 position = { std::cos(angle) * 4.0f, size * 0.5f, std::sin(angle) * 4.0f };
		m_cubePositions.push_back(position);
		m_cubeTransforms.push_back(MatrixMultiply(
			MatrixMultiply(MatrixScale(size, size, size), MatrixRotateY(angle)),
			MatrixTranslate(position.x, position.y, position.z)));
	}

	// Ground plane (lit), stored after the cubes. The pointer stays valid because of reserve(CUBE_COUNT + 1)
	m_ground = &m_cubeMaterials.emplace_back(lit);
	m_ground->SetTint({ 120, 140, 110, 255 });

	// Minimap: high above the ring looking down (pitch is clamped to -89), pixel size follows the DPI
	m_minimapCamera.SetPosition({ 0.0f, 14.0f, 0.01f });
	m_minimapCamera.SetYawPitch(0.0f, -89.0f);
	m_minimapCamera.SetPerspective({ 45.0f, 0.1f, 50.0f });
	const float dpi = eng::Engine::GetInstance().GetWindow().dpiScale;
	m_minimap.Create(static_cast<int>(MINIMAP_WIDTH * dpi), static_cast<int>(MINIMAP_HEIGHT * dpi));

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
	map.BindAction("ToggleDebug", InputBinding::Key(KEY_F2));
}

void ScaleGame::Update(float deltaTime)
{
	eng::InputManager&		input	= eng::Engine::GetInstance().GetInputManager();
	const eng::InputMap&	map		= input.GetMap();

	if (map.IsActionPressed("ToggleCursor"))
		input.SetCursorLocked(!input.IsCursorLocked());

	if (map.IsActionPressed("ToggleDebug"))
	{
		eng::DebugDraw& debug = eng::Engine::GetInstance().GetDebugDraw();
		debug.SetEnabled(!debug.IsEnabled());
	}

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

	// Cube 0 pulses its "emissive" material parameter (0 = lit, 1 = flat albedo)
	m_pulseTime += deltaTime;
	m_cubeMaterials[0].SetParameter("emissive", 0.5f + 0.5f * std::sin(m_pulseTime * 2.0f));

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
	eng::ResourceManager& resources = eng::Engine::GetInstance().GetResources();

	const ::Mesh* cube = resources.GetPrimitive(eng::Primitive::Cube);
	for (int i = 0; i < CUBE_COUNT; ++i)
		queue.Submit(*cube, m_cubeMaterials[i], m_cubeTransforms[i]);

	// 1 cm below y = 0 so the debug grid doesn't z-fight with it
	queue.Submit(*resources.GetPrimitive(eng::Primitive::Plane), *m_ground,
		MatrixMultiply(MatrixScale(20.0f, 1.0f, 20.0f), MatrixTranslate(0.0f, -0.01f, 0.0f)));

	SubmitDebugShapes();
}

void ScaleGame::SubmitDebugShapes() const
{
	eng::DebugDraw& debug = eng::Engine::GetInstance().GetDebugDraw();
	if (!debug.IsEnabled())
		return;

	debug.Grid(20, 1.0f);
	debug.Axes({ 0.0f, 0.01f, 0.0f }, 1.5f);
	debug.WireSphere({ 0.0f, 0.0f, 0.0f }, 5.2f, Fade(DARKGRAY, 0.4f));
	for (int i = 0; i < CUBE_COUNT; ++i)
		debug.Label(Vector3Add(m_cubePositions[i], { 0.0f, 1.2f, 0.0f }), TextFormat("cube %d", i), BLACK);
}

void ScaleGame::DrawMinimap()
{
	eng::Engine& engine = eng::Engine::GetInstance();

	m_minimap.Begin();
		ClearBackground(Fade(DARKBLUE, 0.85f));
		m_minimapCamera.Begin3D();
			engine.GetRenderQueue().Draw(m_minimapCamera);
			// Where the main camera is
			DrawSphere(m_camera.GetPosition(), 0.4f, YELLOW);
		m_minimapCamera.End3D();
	m_minimap.End();

	const float x = static_cast<float>(engine.GetWindow().screenWidth - MINIMAP_WIDTH - 10);
	m_minimap.DrawToScreen({ x, 10.0f, static_cast<float>(MINIMAP_WIDTH), static_cast<float>(MINIMAP_HEIGHT) });
	DrawRectangleLines(static_cast<int>(x), 10, MINIMAP_WIDTH, MINIMAP_HEIGHT, DARKBLUE);
	DrawText("RenderTarget minimap", static_cast<int>(x) + 6, 14, 10, RAYWHITE);
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
	DrawText(TextFormat("C: %s  |  F2: debug draw  |  close to quit",
		engine.GetInputManager().IsCursorLocked() ? "unlock cursor" : "fly (mouse + WASD)"), 20, 100, 10, DARKGRAY);

	DrawInputPanel();
	DrawMinimap();
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
	m_cubePositions.clear();
	m_ground = nullptr;
	m_minimap.Unload();
	eng::Log::Info("ScaleGame destroyed");
}

const char* ScaleGame::GetTitle() const
{
	return "Build to Scale";
}
