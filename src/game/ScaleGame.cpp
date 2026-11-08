#include "ScaleGame.h"
#include "components/TriggerCounter.h"
#include "components/SelfDestructTimer.h"
#include <algorithm>
#include <cmath>
#include <raymath.h>

bool ScaleGame::Init()
{
	m_camera.SetPerspective({ 60.0f, 0.05f, 300.0f });
	eng::Engine::GetInstance().GetMainCamera().SetActiveCamera(m_camera);

	eng::ResourceManager& resources = eng::Engine::GetInstance().GetResources();
	m_litShader		= resources.LoadShader("assets/shaders/lit.vs", "assets/shaders/lit.fs");
	resources.SetDefaultLitShader(m_litShader);	// level files (lit=1) and editor-made cubes use it
	eng::Engine::GetInstance().GetComponentRegistry().Register<TriggerCounter>("TriggerCounter");
	eng::Engine::GetInstance().GetComponentRegistry().Register<Scalable>("Scalable");
	eng::Engine::GetInstance().GetComponentRegistry().Register<WinTrigger>("WinTrigger");
	eng::Engine::GetInstance().GetComponentRegistry().Register<LoseTrigger>("LoseTrigger");
	eng::Engine::GetInstance().GetComponentRegistry().Register<NextLevelTrigger>("NextLevelTrigger");
	GameContext::SetGameState(&m_gameState);
	eng::Engine::GetInstance().GetEditor().SetPropDecorator([](eng::GameActor& prop) { prop.AddComponent<Scalable>(); });
	SoundFXBank::Instance().Load(resources);

	m_themeMenu = resources.LoadMusic("assets/audio/music_menu.wav");
	m_themeGame = resources.LoadMusic("assets/audio/music_game.wav");
	if (m_themeMenu)
	{
		eng::MusicPlayer& music = eng::Engine::GetInstance().GetAudio().GetMusic();
		music.SetVolume(0.7f);
		music.Play(*m_themeMenu, MUSIC_FADE_IN);
	}

	m_gameplay.Init(m_gameState, m_playerState, LevelList::Default(), [this](const std::string& path) { return LoadLevelFile(path); });
	m_gameplay.LoadNow(0);

	m_beams.Init(&m_level);	// every gun shot: muzzle burst, beam, impact, sound

	m_gameUI.Init(m_input, m_gameplay);	// win / lose dialogs

	// Minimap: high above the sandbox looking down (pitch is clamped to -89), pixel size follows the DPI
	m_minimapCamera.SetPosition({ 0.0f, 32.0f, 0.01f });
	m_minimapCamera.SetYawPitch(0.0f, -89.0f);
	m_minimapCamera.SetPerspective({ 60.0f, 0.1f, 100.0f });
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

	// Walk, look, jump, fire, wheel
	PlayerInput::BindDefaults(map);

	map.BindAction("ToggleDebug", InputBinding::Key(KEY_F2));
	map.BindAction("SwitchTheme", InputBinding::Key(KEY_M));

	// Physics sandbox
	map.BindAction("DeleteAimed", InputBinding::Key(KEY_X));
	map.BindAction("TogglePhysicsDebug", InputBinding::Key(KEY_F3));
	map.BindAction("ThrowPellet", InputBinding::Key(KEY_T));
}

//////////////////////////////////////////
/// Sandbox
//////////////////////////////////////////
eng::GameActor* ScaleGame::SpawnBox(const BoxDesc& desc)
{
	eng::ResourceManager& resources = eng::Engine::GetInstance().GetResources();

	eng::GameActor* actor = m_level.CreateGameActor(desc.name);
	actor->SetPosition(desc.position);
	actor->SetRotationEuler(desc.rotationDegrees);
	actor->SetScale(desc.size);		// unit cube mesh and 1 m box collider, scaled by the actor

	eng::Material& material = actor->AddComponent<eng::MeshRenderer>(resources.GetPrimitive(eng::Primitive::Cube))
		->CreateOwnMaterial(m_litShader);
	material.SetTint(desc.color);
	material.SetParameter("specularStrength", desc.type == eng::BodyType::Static ? 0.1f : 0.4f);

	actor->AddComponent<eng::Collider>()->SetLayer(desc.layer);	// default box: half extents 0.5 = unit cube
	eng::RigidBodySettings settings;
	settings.type = desc.type;
	settings.mass = desc.mass;
	actor->AddComponent<eng::RigidBody>(settings);
	if (desc.type == eng::BodyType::Dynamic && desc.isScalable)
		actor->AddComponent<Scalable>();
	return actor;
}

void ScaleGame::BuildSandbox()
{
	using eng::BodyType;
	constexpr Color GROUND	= { 120, 140, 110, 255 };
	constexpr Color RAMP	= { 150, 150, 160, 255 };
	constexpr Color STEEP	= { 170, 130, 150, 255 };
	constexpr Color HEAVY	= { 139,  90,  43, 255 };
	constexpr Color LIGHT	= { 235, 205,  80, 255 };
	constexpr Color DROPPED	= { 230, 120,  40, 255 };

	// Static level geometry. Ground top at y = -0.01 so the debug grid (y = 0) doesn't z-fight with it
	SpawnBox({ "Ground", { 0.0f, -0.51f, 0.0f }, { 40.0f, 1.0f, 40.0f }, {}, BodyType::Static, 0.0f, GROUND, eng::Layer::Ground });

	// Ramps rotated around Z: +angle raises the +X end. Centered so the low end sits on the ground
	const float rampLength = 6.0f;
	auto rampHeight = [rampLength](float degrees) { return 0.5f * rampLength * std::sin(degrees * DEG2RAD); };
	SpawnBox({ "Ramp 30", { -9.0f, rampHeight(30.0f), -6.0f }, { rampLength, 0.4f, 4.0f }, { 0.0f, 0.0f, 30.0f },
			   BodyType::Static, 0.0f, RAMP, eng::Layer::Ground });
	SpawnBox({ "Ramp 50", {  9.0f, rampHeight(50.0f), -6.0f }, { rampLength, 0.4f, 4.0f }, { 0.0f, 0.0f, 50.0f },
			   BodyType::Static, 0.0f, STEEP, eng::Layer::Ground });

	// 20 dynamic crates: a heavy pyramid (10), a light tower (5), dropped crates (4), a low crate to jump on (1)
	m_crates.clear();
	for (int row = 0, count = 4; count > 0; ++row, --count)
		for (int i = 0; i < count; ++i)
			m_crates.push_back(SpawnBox({ TextFormat("Heavy %d.%d", row, i),
				{ -1.53f + 1.02f * (i + 0.5f * row), 0.5f + 1.0f * row, -4.0f }, { 1.0f, 1.0f, 1.0f }, {},
				BodyType::Dynamic, HEAVY_MASS, HEAVY, eng::Layer::Prop, true }));

	for (int i = 0; i < 5; ++i)
		m_crates.push_back(SpawnBox({ TextFormat("Light %d", i), { 5.0f, 0.45f + 0.9f * i, 2.0f }, { 0.9f, 0.9f, 0.9f }, {},
			BodyType::Dynamic, LIGHT_MASS, LIGHT, eng::Layer::Prop, true }));

	for (int i = 0; i < 4; ++i)
		m_crates.push_back(SpawnBox({ TextFormat("Dropped %d", i), { -4.0f + 0.4f * i, 6.0f + 2.0f * i, 2.0f + 0.3f * i },
			{ 0.8f, 0.8f, 0.8f }, { 15.0f * i, 30.0f * i, 10.0f }, BodyType::Dynamic, 5.0f, DROPPED, eng::Layer::Prop, true }));

	m_crates.push_back(SpawnBox({ "Jump crate", { -3.0f, 0.4f, 7.0f }, { 1.6f, 0.8f, 1.6f }, {},
		BodyType::Dynamic, 60.0f, RED, eng::Layer::Prop, true }));

	// Trigger zone: translucent, objects pass through; turns green while something is inside
	eng::ResourceManager& resources = eng::Engine::GetInstance().GetResources();
	eng::GameActor* zone = m_level.CreateGameActor("Trigger Zone");
	zone->SetPosition({ 2.0f, 1.0f, 5.0f });
	zone->SetScale({ 3.0f, 2.0f, 3.0f });
	zone->AddComponent<eng::MeshRenderer>(resources.GetPrimitive(eng::Primitive::Cube))
		->CreateOwnMaterial().SetTint({ 60, 140, 255, 70 });
	zone->AddComponent<eng::Collider>()->SetLayer(eng::Layer::Trigger);
	zone->AddComponent<eng::TriggerVolume>();
	m_triggerCounter = zone->AddComponent<TriggerCounter>();

	// Thin wall behind the pyramid, target for the CCD pellets
	SpawnBox({ "Thin Wall", { 0.0f, 1.5f, WALL_Z }, { 6.0f, 3.0f, 0.05f }, {}, BodyType::Static, 0.0f,
			   { 200, 200, 210, 255 }, eng::Layer::Ground });

}

void ScaleGame::ThrowPellet()
{
	// Oldest out when the cap is reached
	if (m_pellets.size() >= MAX_PELLETS)
	{
		if (eng::GameActor* oldest = m_pellets.front().Get())
			oldest->MarkForDestroy();
		m_pellets.erase(m_pellets.begin());
	}

	eng::GameActor* pellet = SpawnBox({ "Pellet", Vector3Add(m_camera.GetPosition(), Vector3Scale(m_camera.Forward(), 1.0f)),
		{ PELLET_SIZE, PELLET_SIZE, PELLET_SIZE }, {}, eng::BodyType::Dynamic, 0.05f, RAYWHITE });
	pellet->GetComponent<eng::RigidBody>()->SetLinearVelocity(Vector3Scale(m_camera.Forward(), PELLET_SPEED));
	m_pellets.push_back(pellet);
	++m_pelletsThrown;
}

void ScaleGame::UpdatePellets()
{
	// A pellet "tunnelled" if it ends up behind the wall's footprint (counted once, then removed)
	for (const eng::ActorHandle& handle : m_pellets)
	{
		eng::GameActor* pellet = handle.Get();
		if (!pellet)
			continue;

		const Vector3 p = pellet->GetWorldPosition();
		if (p.z < WALL_Z - 0.3f && std::fabs(p.x) < 3.0f && p.y > 0.0f && p.y < 3.0f)
		{
			++m_tunnelled;
			pellet->MarkForDestroy();
		}
	}
	m_pellets.erase(std::remove_if(m_pellets.begin(), m_pellets.end(),
		[](const eng::ActorHandle& pellet) { return !pellet.IsAlive(); }), m_pellets.end());
}

void ScaleGame::SpawnPlayer()
{
	// From the level's spawn line; a level without one uses the sandbox spot
	const eng::SpawnPoint& spawn = m_level.GetSpawn();
	m_player = PlayerActor::Create(m_level, { &m_input, &m_playerState, &m_camera },
		spawn.isSet ? spawn.position : SANDBOX_SPAWN, spawn.isSet ? spawn.yawDegrees : SANDBOX_SPAWN_YAW, true);
}

bool ScaleGame::IsCrate(const eng::GameActor* actor) const
{
	return actor && std::find(m_crates.begin(), m_crates.end(), eng::ActorHandle(actor)) != m_crates.end();
}

void ScaleGame::UpdateAim()
{
	const eng::InputMap& map = eng::Engine::GetInstance().GetInputManager().GetMap();

	// Center-screen ray, like the scaling gun (Unity ScalingProvider): Ground | Prop, ignores player and triggers
	m_aimHit = eng::Engine::GetInstance().GetPhysics().Raycast(m_camera.GetPosition(), m_camera.Forward(), AIM_REACH,
		eng::Layer::Ground | eng::Layer::Prop);

	if (!m_aimHit || !IsCrate(m_aimHit->actor))
		return;

	// X: delete it mid-simulation (the body leaves the world at the end of the frame)
	if (map.IsActionPressed("DeleteAimed"))
	{
		m_aimHit->actor->MarkForDestroy();
		++m_deletedCrates;
	}
}

void ScaleGame::ClearSandbox()
{
	m_aimHit.reset();
	m_triggerCounter	= nullptr;
	m_player			= nullptr;
	m_pellets.clear();
	m_crates.clear();
	m_level.Clear();
}

void ScaleGame::OnStartRequested()
{
	m_gameplay.LoadNow(0);		// between frames: safe to clear the level; the mode is Play, so the game starts
}

void ScaleGame::OnReturnToMainMenu()
{
	m_gameState.Reset();
}

void ScaleGame::OnRestartRequested()
{
	m_gameplay.LoadNow(m_gameplay.GetLevelIndex());
}

bool ScaleGame::LoadLevelFile(const std::string& path)
{
	ClearSandbox();

	bool isFile = eng::LevelSerializer::Load(m_level, path).ok;
	if (!isFile)
	{
		eng::Log::Warning("ScaleGame: '%s' not found, building the code sandbox", path.c_str());
		BuildSandbox();
	}

	RebindActors();
	SpawnPlayer();
	m_deletedCrates = m_pelletsThrown = m_tunnelled = 0;
	return isFile;
}

void ScaleGame::OnLevelLoaded()
{
	// The editor replaced every actor
	RebindActors();
	SpawnPlayer();
}

void ScaleGame::RebindActors()
{
	// Find the actors the sandbox logic works with again
	m_aimHit.reset();
	m_triggerCounter	= nullptr;
	m_player			= nullptr;
	m_pellets.clear();
	m_crates.clear();

	for (const auto& owned : m_level.GetActors())
	{
		eng::GameActor* actor = owned.get();
		if (actor->GetComponent<eng::CharacterMotor>())
			actor->MarkForDestroy();	// an old file saved before the player was spawned by the game: the game spawns its own
		else if (TriggerCounter* counter = actor->GetComponent<TriggerCounter>())
			m_triggerCounter = counter;
		else if (const eng::RigidBody* body = actor->GetComponent<eng::RigidBody>();
				 body && body->GetType() == eng::BodyType::Dynamic && actor->GetName() != "Pellet")
			m_crates.push_back(actor);
	}

	eng::Log::Info("ScaleGame: level loaded, %zu crates", m_crates.size());
}

std::string ScaleGame::GetLevelPath() const
{
	return m_gameplay.GetCurrentPath();
}

void ScaleGame::PruneDestroyedCrates()
{
	m_crates.erase(std::remove_if(m_crates.begin(), m_crates.end(),
		[](const eng::ActorHandle& crate) { return !crate.IsAlive(); }), m_crates.end());
}

//////////////////////////////////////////
/// Frame
//////////////////////////////////////////
void ScaleGame::Update(float deltaTime)
{
	m_gameplay.Update();	// a requested level load (next level trigger, retry) happens here, between frames

	eng::InputManager&		input	= eng::Engine::GetInstance().GetInputManager();
	const eng::InputMap&	map		= input.GetMap();

	if (map.IsActionPressed("ToggleDebug"))
	{
		eng::DebugDraw& debug = eng::Engine::GetInstance().GetDebugDraw();
		debug.SetEnabled(!debug.IsEnabled());
	}

	if (map.IsActionPressed("TogglePhysicsDebug"))
	{
		eng::PhysicsWorld& physics = eng::Engine::GetInstance().GetPhysics();
		physics.SetDebugDrawEnabled(!physics.IsDebugDrawEnabled());
	}

	// Menus, pause and the editor only move the camera: clicks there belong to the UI
	const bool isPlaying = eng::Engine::GetInstance().GetModes().IsSimulating();
	if (isPlaying && map.IsActionPressed("ThrowPellet"))
		ThrowPellet();

	m_input.Update(deltaTime, map);
	UpdateCamera(deltaTime);
	if (isPlaying)
	{
		UpdateAim();
		UpdateAudioDemo();
	}
	UpdatePellets();
	PruneDestroyedCrates();

	m_secondTimer += deltaTime;
	if (m_secondTimer >= 1.0f)
	{
		m_fixedStepsPerSecond	= m_fixedStepsThisSecond;
		m_fixedStepsThisSecond	= 0;
		m_secondTimer			-= 1.0f;
	}
}

void ScaleGame::UpdateCamera(float deltaTime)
{
	// Play: the FirstPersonLook on the player owns the camera. The menu gets a slow orbit around the sandbox
	if (eng::Engine::GetInstance().GetModes().GetMode() == eng::EngineMode::MainMenu)
	{
		m_orbitAngle += ORBIT_SPEED * deltaTime;
		const float angle = m_orbitAngle * DEG2RAD;
		m_camera.SetPosition({ std::sin(angle) * ORBIT_RADIUS, 10.0f, std::cos(angle) * ORBIT_RADIUS });
		m_camera.LookAt({ 0.0f, 1.0f, -1.0f });
	}
}

void ScaleGame::UpdateAudioDemo()
{
	eng::AudioManager&		audio	= eng::Engine::GetInstance().GetAudio();
	const eng::InputMap&	map		= eng::Engine::GetInstance().GetInputManager().GetMap();

	// Cross-fade between the menu and gameplay themes
	if (map.IsActionPressed("SwitchTheme") && m_themeMenu && m_themeGame)
	{
		eng::MusicPlayer& music = audio.GetMusic();
		music.Play(music.GetCurrent() == m_themeMenu ? *m_themeGame : *m_themeMenu, MUSIC_CROSSFADE);
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

	debug.Grid(40, 1.0f);
	debug.Axes({ 0.0f, 0.01f, 0.0f }, 1.5f);
	debug.Label({ -9.0f, 3.5f, -6.0f }, "30 deg ramp", BLACK);
	debug.Label({  9.0f, 5.0f, -6.0f }, "50 deg ramp", BLACK);

	if (m_aimHit)
	{
		debug.WireSphere(m_aimHit->point, 0.08f, RED);
		debug.Line(m_aimHit->point, Vector3Add(m_aimHit->point, Vector3Scale(m_aimHit->normal, 0.6f)), RED);
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
			DrawSphere(m_camera.GetPosition(), 0.6f, YELLOW);	// where the main camera is
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
	DrawText(TextFormat("FPS %d  dt %.4f s  draws %zu  lines %zu  particles %zu", GetFPS(), time.GetDeltaTime(),
		engine.GetRenderQueue().GetCommandCount(), engine.GetRenderQueue().GetLineCount(),
		engine.GetParticles().GetAliveCount()), 20, 40, 10, DARKGRAY);
	DrawText(TextFormat("Fixed steps/s %d (target %d)", m_fixedStepsPerSecond,
		static_cast<int>(1.0f / eng::FIXED_DELTA_TIME)), 20, 60, 10, DARKGRAY);
	DrawText(TextFormat("Screen %d x %d   Render %d x %d   DPI x%.1f", window.screenWidth, window.screenHeight,
		window.renderWidth, window.renderHeight, window.dpiScale), 20, 80, 10, DARKGRAY);
	DrawText(TextFormat("WASD walk  Space jump  mouse look  |  ESC pause  |  F2 / F3 debug  |  %s",
		eng::ModeName(engine.GetModes().GetMode())), 20, 100, 10, DARKGRAY);

	DrawPhysicsPanel();
	DrawAudioPanel();
	DrawMinimap();

	// Crosshair
	const int cx = window.screenWidth / 2, cy = window.screenHeight / 2;
	const Color crosshair = (m_aimHit && IsCrate(m_aimHit->actor)) ? RED : RAYWHITE;
	DrawLine(cx - 8, cy, cx + 8, cy, crosshair);
	DrawLine(cx, cy - 8, cx, cy + 8, crosshair);
}

void ScaleGame::DrawPhysicsPanel() const
{
	int sleeping = 0;
	for (const eng::ActorHandle& handle : m_crates)
		if (const eng::GameActor* crate = handle.Get())
			if (const eng::RigidBody* body = crate->GetComponent<eng::RigidBody>(); body && body->IsSleeping())
				++sleeping;

	const int x = 10, y = 140;
	DrawRectangle(x, y, 330, 126, Fade(LIME, 0.35f));
	DrawRectangleLines(x, y, 330, 126, DARKGREEN);
	DrawText(TextFormat("Physics (Bullet)  |  bodies %d  |  actors %zu", eng::Engine::GetInstance().GetPhysics().GetObjectCount(),
		m_level.GetActorCount()), x + 10, y + 10, 10, BLACK);
	DrawText(TextFormat("Crates %zu  |  sleeping %d  |  deleted %d", m_crates.size(), sleeping, m_deletedCrates),
		x + 10, y + 28, 10, DARKGRAY);
	DrawText("heavy 20 kg (brown)  light 2 kg (yellow)  jump crate (red)", x + 10, y + 44, 10, DARKGRAY);
	if (m_aimHit)
		DrawText(TextFormat("Aim: %s  %.1f m%s", m_aimHit->actor->GetName().c_str(), m_aimHit->distance,
			IsCrate(m_aimHit->actor) ? "  (X delete)" : ""), x + 10, y + 60, 10, BLACK);
	else
		DrawText("Aim: nothing within 30 m", x + 10, y + 60, 10, DARKGRAY);

	if (m_triggerCounter)
		DrawText(TextFormat("Trigger: inside %zu  |  enters %d  exits %d  |  %s", m_triggerCounter->GetInside(),
			m_triggerCounter->GetEnters(), m_triggerCounter->GetExits(), m_triggerCounter->GetLast().c_str()),
			x + 10, y + 76, 10, DARKGRAY);

	if (const eng::CharacterMotor* motor = m_player ? m_player->GetComponent<eng::CharacterMotor>() : nullptr)
	{
		const Vector3 v = motor->GetVelocity();
		DrawText(TextFormat("Player: %s  slope %.0f deg%s  speed %.1f m/s  feet y %.2f",
			motor->IsGrounded() ? "grounded" : "airborne", motor->GetGroundSlopeDegrees(),
			motor->IsOnSteepSlope() ? " (too steep)" : "", std::sqrt(v.x * v.x + v.z * v.z),
			m_player->GetWorldPosition().y - 1.0f), x + 10, y + 92, 10, BLACK);
	}

	DrawText(TextFormat("T: throw pellet at the thin wall (CCD)  |  thrown %d  tunnelled %d", m_pelletsThrown, m_tunnelled),
		x + 10, y + 108, 10, m_tunnelled > 0 ? RED : DARKGRAY);
}

void ScaleGame::DrawAudioPanel() const
{
	const eng::AudioManager&	audio	= eng::Engine::GetInstance().GetAudio();
	const eng::MusicPlayer&		music	= audio.GetMusic();
	const char*					track	= music.GetCurrent() == m_themeMenu ? "menu"
										: music.GetCurrent() == m_themeGame ? "game" : "none";

	const int x = 10, y = 276;
	DrawRectangle(x, y, 330, 59, Fade(BEIGE, 0.6f));
	DrawRectangleLines(x, y, 330, 59, BROWN);
	DrawText(TextFormat("Audio  |  LMB / RMB sfx x%.2f (wheel)  |  M: %s theme", m_playerState.GetFactor(), track),
		x + 10, y + 10, 10, DARKGRAY);
	DrawText(TextFormat("ESC pause ducks the music  |  duck %.2f", audio.GetDuckFactor()), x + 10, y + 26, 10, DARKGRAY);
	DrawText(TextFormat("Game: %s  |  level %d of %zu", GameState::StatusName(m_gameState.GetStatus()),
		m_gameState.GetLevelIndex() + 1, m_gameplay.GetLevels().GetCount()), x + 10, y + 42, 10, DARKGRAY);
}

void ScaleGame::Destroy()
{
	// Actors first: their bodies leave the physics world before it shuts down
	m_gameUI.Shutdown();
	m_beams.Shutdown();
	m_gameplay.Shutdown();
	GameContext::SetGameState(nullptr);
	ClearSandbox();
	SoundFXBank::Instance().Clear();
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
