#include "ScaleGame.h"
#include "components/Rotator.h"
#include "components/SelfDestructTimer.h"
#include <algorithm>
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
	eng::ResourceManager& resources = eng::Engine::GetInstance().GetResources();
	m_litShader = resources.LoadShader("assets/shaders/lit.vs", "assets/shaders/lit.fs");

	m_sfxStretch	= resources.LoadSound("assets/audio/sfx_stretch.wav");
	m_sfxShrink		= resources.LoadSound("assets/audio/sfx_shrink.wav");
	m_ping			= resources.LoadSound("assets/audio/sfx_ping.wav");

	m_themeMenu = resources.LoadMusic("assets/audio/music_menu.wav");
	m_themeGame = resources.LoadMusic("assets/audio/music_game.wav");
	if (m_themeMenu)
	{
		eng::MusicPlayer& music = eng::Engine::GetInstance().GetAudio().GetMusic();
		music.SetVolume(0.7f);
		music.Play(*m_themeMenu, MUSIC_FADE_IN);
	}

	BuildScene();

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
	// TODO(Phase 8): replaced by the real Pause (ESC); simulates its audio side
	map.BindAction("TogglePauseAudio", InputBinding::Key(KEY_P));
	map.BindAction("PingCube", InputBinding::Key(KEY_K));
	map.BindAction("SwitchTheme", InputBinding::Key(KEY_M));

	// Scene demo
	map.BindAction("DestroyCube", InputBinding::Key(KEY_X));
	map.BindAction("RebuildRing", InputBinding::Key(KEY_R));
}

//////////////////////////////////////////
/// Scene
//////////////////////////////////////////
void ScaleGame::BuildScene()
{
	eng::ResourceManager& resources = eng::Engine::GetInstance().GetResources();

	// Ground (lit), 1 cm below y = 0 so the debug grid doesn't z-fight with it
	eng::GameActor* ground = m_level.CreateGameActor("Ground");
	ground->SetPosition({ 0.0f, -0.01f, 0.0f });
	ground->SetScale({ 20.0f, 1.0f, 20.0f });
	ground->AddComponent<eng::MeshRenderer>(resources.GetPrimitive(eng::Primitive::Plane))
		->CreateOwnMaterial(m_litShader).SetTint({ 120, 140, 110, 255 });

	BuildRing();
}

void ScaleGame::BuildRing()
{
	eng::ResourceManager& resources = eng::Engine::GetInstance().GetResources();
	const ::Mesh* cube		= resources.GetPrimitive(eng::Primitive::Cube);
	const ::Mesh* sphere	= resources.GetPrimitive(eng::Primitive::Sphere);

	++m_ringBuilds;
	m_ring = m_level.CreateGameActor(TextFormat("Ring %d", m_ringBuilds));
	m_ring->AddComponent<Rotator>(RING_SPEED);

	m_cubes.clear();
	m_cubes.reserve(CUBE_COUNT);
	m_lastPingedCube = -1;

	// Ring of cubes, each with its own material: hue around the circle, growing size
	for (int i = 0; i < CUBE_COUNT; ++i)
	{
		const float t		= static_cast<float>(i) / CUBE_COUNT;
		const float angle	= t * 2.0f * PI;
		const float size	= 0.6f + 0.5f * t;

		eng::GameActor* actor = m_level.CreateGameActor(TextFormat("Cube %d", i), m_ring);
		actor->SetPosition({ std::cos(angle) * RING_RADIUS, size * 0.5f, std::sin(angle) * RING_RADIUS });
		actor->SetRotationEuler({ 0.0f, -angle * RAD2DEG, 0.0f });
		actor->SetScale(size);

		eng::Material& material = actor->AddComponent<eng::MeshRenderer>(cube)->CreateOwnMaterial(i % 2 == 0 ? m_litShader : nullptr);
		material.SetTint(ColorFromHSV(t * 360.0f, 0.75f, 0.95f));
		material.SetParameter("specularStrength", 0.6f);

		CubeEntry& entry = m_cubes.emplace_back();
		entry.actor = actor;
		entry.source.SetSound(m_ping);
		entry.source.SetSpatial(true);
		entry.source.SetRange(2.0f, 25.0f);
		entry.source.SetPitch(0.8f + 0.5f * t);		// each cube has its own note
		entry.source.SetPitchVariation(0.02f);
	}

	// Cube 0 spins fast and carries a moon: Ring -> Cube 0 -> Moon (scaled by both parents)
	m_pulseCube = m_cubes[0].actor;
	m_pulseCube->AddComponent<Rotator>(90.0f);
	eng::GameActor* moon = m_level.CreateGameActor("Moon", m_pulseCube);
	moon->SetPosition({ 1.4f, 0.3f, 0.0f });
	moon->SetScale(0.4f);
	moon->AddComponent<eng::MeshRenderer>(sphere)->CreateOwnMaterial(m_litShader).SetTint(LIGHTGRAY);
}

void ScaleGame::PruneDestroyedCubes()
{
	const size_t before = m_cubes.size();
	m_cubes.erase(std::remove_if(m_cubes.begin(), m_cubes.end(),
		[](const CubeEntry& entry) { return entry.actor->IsMarkedForDestroy(); }), m_cubes.end());

	if (m_cubes.size() != before)
		m_lastPingedCube = -1;
	if (m_pulseCube && m_pulseCube->IsMarkedForDestroy())
		m_pulseCube = nullptr;
	if (m_ring && m_ring->IsMarkedForDestroy())
		m_ring = nullptr;
}

void ScaleGame::UpdateSceneDemo()
{
	const eng::InputMap& map = eng::Engine::GetInstance().GetInputManager().GetMap();

	// X: a random cube destroys itself from inside its own Update next frame (with its children)
	if (map.IsActionPressed("DestroyCube") && !m_cubes.empty())
	{
		const int index = GetRandomValue(0, static_cast<int>(m_cubes.size()) - 1);
		m_cubes[index].actor->AddComponent<SelfDestructTimer>(0.0f);
	}

	// R: destroy the whole ring (children follow) and build a new one in the same frame
	if (map.IsActionPressed("RebuildRing"))
	{
		if (m_ring)
			m_ring->MarkForDestroy();
		BuildRing();
	}

	PruneDestroyedCubes();

	// Cube 0 pulses its "emissive" material parameter (0 = lit, 1 = flat albedo)
	if (m_pulseCube)
	{
		if (eng::Material* material = m_pulseCube->GetComponent<eng::MeshRenderer>()->GetOwnMaterial())
			material->SetParameter("emissive", 0.5f + 0.5f * std::sin(m_pulseTime * 2.0f));
	}
}

//////////////////////////////////////////
/// Frame
//////////////////////////////////////////
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

	m_pulseTime += deltaTime;
	UpdateSceneDemo();
	UpdateAudioDemo();

	m_secondTimer += deltaTime;
	if (m_secondTimer >= 1.0f)
	{
		m_fixedStepsPerSecond	= m_fixedStepsThisSecond;
		m_fixedStepsThisSecond	= 0;
		m_secondTimer			-= 1.0f;
	}
}

void ScaleGame::UpdateAudioDemo()
{
	eng::AudioManager&		audio	= eng::Engine::GetInstance().GetAudio();
	const eng::InputMap&	map		= eng::Engine::GetInstance().GetInputManager().GetMap();

	// Wheel picks the scale factor (Unity PlayerState::ChangeFactor)
	const float scroll = map.GetAxis("Scroll");
	if (scroll > 0.0f && m_scaleFactorIndex < 2)	++m_scaleFactorIndex;
	if (scroll < 0.0f && m_scaleFactorIndex > 0)	--m_scaleFactorIndex;
	const float factor = SCALE_FACTORS[m_scaleFactorIndex];

	// Unity ScalingAudioFX: stretch pitch = (1 + factor) + 0.02, shrink pitch = (1 - factor) - 0.02
	if (map.IsActionPressed("Fire") && m_sfxStretch)
	{
		m_lastPitch = 1.0f + factor + 0.02f;
		audio.Play(*m_sfxStretch, 0.8f, m_lastPitch);
	}
	if (map.IsActionPressed("AltFire") && m_sfxShrink)
	{
		m_lastPitch = 1.0f - factor - 0.02f;
		audio.Play(*m_sfxShrink, 0.8f, m_lastPitch);
	}

	// The cubes move with the ring: keep each source on its cube
	for (CubeEntry& entry : m_cubes)
	{
		entry.source.SetPosition(entry.actor->GetWorldPosition());
		entry.source.Update();
	}

	// Spatial ping from a random cube: listen to pan (left / right) and distance falloff
	if (map.IsActionPressed("PingCube") && !m_cubes.empty())
	{
		m_lastPingedCube = GetRandomValue(0, static_cast<int>(m_cubes.size()) - 1);
		m_cubes[m_lastPingedCube].source.Play();
	}

	// Cross-fade between the menu and gameplay themes
	if (map.IsActionPressed("SwitchTheme") && m_themeMenu && m_themeGame)
	{
		eng::MusicPlayer& music = audio.GetMusic();
		music.Play(music.GetCurrent() == m_themeMenu ? *m_themeGame : *m_themeMenu, MUSIC_CROSSFADE);
	}

	if (map.IsActionPressed("TogglePauseAudio"))
	{
		m_isAudioPaused = !m_isAudioPaused;
		audio.Duck(m_isAudioPaused);
		if (m_isAudioPaused) audio.PauseAll(); else audio.ResumeAll();
	}
}

void ScaleGame::FixedUpdate(float /*fixedDeltaTime*/)
{
	++m_fixedStepsThisSecond;
}

void ScaleGame::SubmitRenderables(eng::RenderQueue& /*queue*/)
{
	// The level's MeshRenderers submit themselves; only debug shapes are added here
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
	for (const CubeEntry& entry : m_cubes)
	{
		// World bounds follow the ring's rotation (axis aligned, so they grow while a cube is turned)
		const BoundingBox bounds = entry.actor->GetComponent<eng::MeshRenderer>()->GetWorldBounds();
		debug.WireBox(Vector3Scale(Vector3Add(bounds.min, bounds.max), 0.5f), Vector3Subtract(bounds.max, bounds.min), Fade(BLACK, 0.3f));
		debug.Label(Vector3Add(entry.actor->GetWorldPosition(), { 0.0f, 1.2f, 0.0f }), entry.actor->GetName(), BLACK);
	}
}

//////////////////////////////////////////
/// HUD
//////////////////////////////////////////
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
	DrawAudioPanel();
	DrawScenePanel();
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

void ScaleGame::DrawAudioPanel() const
{
	const eng::AudioManager& audio = eng::Engine::GetInstance().GetAudio();

	const int x = 10, y = 270;
	DrawRectangle(x, y, 330, 115, Fade(BEIGE, 0.6f));
	DrawRectangleLines(x, y, 330, 115, BROWN);
	DrawText(TextFormat("Audio %s  |  voices %d active / %d", audio.IsReady() ? "ON" : "OFF (no device)",
		audio.GetActiveVoiceCount(), audio.GetVoiceCount()), x + 10, y + 10, 10, BLACK);
	DrawText(TextFormat("LMB stretch / RMB shrink  factor x%.2f (wheel)  pitch %.2f",
		SCALE_FACTORS[m_scaleFactorIndex], m_lastPitch), x + 10, y + 28, 10, DARKGRAY);
	DrawText(TextFormat("P: %s  |  duck %.2f", m_isAudioPaused ? "resume audio" : "pause audio (demo of Pause)",
		audio.GetDuckFactor()), x + 10, y + 46, 10, DARKGRAY);

	if (m_lastPingedCube >= 0)
	{
		const CubeEntry& entry = m_cubes[m_lastPingedCube];
		DrawText(TextFormat("K: ping  |  %s  dist %.1f  gain %.2f  pan %+.2f", entry.actor->GetName().c_str(),
			Vector3Distance(entry.source.GetPosition(), audio.GetListenerPosition()), entry.source.ComputeGain(),
			entry.source.ComputePan()), x + 10, y + 64, 10, DARKGRAY);
	}
	else
	{
		DrawText("K: ping a random cube (spatial)", x + 10, y + 64, 10, DARKGRAY);
	}

	const eng::MusicPlayer&	music	= audio.GetMusic();
	const char*				track	= music.GetCurrent() == m_themeMenu ? "menu"
									: music.GetCurrent() == m_themeGame ? "game" : "none";
	DrawText(TextFormat("M: switch theme  |  %s  fade %.2f  %.1f s", track, music.GetFade(), music.GetTimePlayed()),
		x + 10, y + 82, 10, DARKGRAY);
}

void ScaleGame::DrawScenePanel() const
{
	const int x = 10, y = 395;
	DrawRectangle(x, y, 330, 75, Fade(LIME, 0.35f));
	DrawRectangleLines(x, y, 330, 75, DARKGREEN);
	DrawText(TextFormat("Scene  |  actors %zu  |  cubes %zu / %d  |  ring #%d", m_level.GetActorCount(),
		m_cubes.size(), CUBE_COUNT, m_ringBuilds), x + 10, y + 10, 10, BLACK);
	DrawText("Ring -> Cube 0 -> Moon: children follow their parents", x + 10, y + 28, 10, DARKGRAY);
	DrawText("X: a random cube destroys itself mid-update  |  R: rebuild ring", x + 10, y + 46, 10, DARKGRAY);
}

void ScaleGame::Destroy()
{
	// Actors (and their materials) go first; resources are unloaded by the Engine afterwards
	m_cubes.clear();
	m_level.Clear();
	m_ring			= nullptr;
	m_pulseCube		= nullptr;
	m_sfxStretch	= nullptr;
	m_sfxShrink		= nullptr;
	m_ping			= nullptr;
	m_themeMenu		= nullptr;
	m_themeGame		= nullptr;
	m_litShader		= nullptr;
	m_minimap.Unload();
	eng::Log::Info("ScaleGame destroyed");
}

const char* ScaleGame::GetTitle() const
{
	return "Build to Scale";
}

eng::Level* ScaleGame::GetActiveLevel()
{
	return &m_level;
}
