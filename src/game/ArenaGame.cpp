#include "ArenaGame.h"
#include <cmath>
#include <raymath.h>

//////////////////////////////////////////
/// Setup
//////////////////////////////////////////
bool ArenaGame::Init()
{
	eng::Engine& engine = eng::Engine::GetInstance();

	m_camera.SetPerspective({ 60.0f, 0.05f, 300.0f });
	engine.GetMainCamera().SetActiveCamera(m_camera);

	eng::ResourceManager& resources = engine.GetResources();
	resources.SetDefaultLitShader(resources.LoadShader("assets/shaders/lit.vs", "assets/shaders/lit.fs"));	// level files (lit=1), editor cubes
	SoundFXBank::Instance().Load(resources);
	m_music.Init(resources);		// menu / gameplay themes follow the engine mode

	Settings::Instance().Load();
	Settings::Instance().Apply();

	RegisterComponents();
	m_enemies.Init();
#ifndef NDEBUG
	for (const std::string& failure : eng::ecs::RunRegistrySelfTest())
		eng::Log::Error("ECS self-check failed: %s", failure.c_str());
#endif
	BindInput();
	GameContext::SetGameState(&m_gameState);

	m_gameplay.Init(m_gameState, m_playerState, LevelList::Default(), [this](const std::string& path) { return LoadLevelFile(path); });
	m_gameplay.LoadNow(0);			// the menu's backdrop
	m_shotFX.Init(&m_level);		// every gun shot: muzzle burst, tracer, impact, sound
	m_gameUI.Init(m_input, m_gameplay);
	m_gunView.Init(m_playerState);

#if ENG_DEV_CONSOLE
	m_commands.Register({ &m_gameplay, &m_gameState, &m_playerState, &m_player, &m_level, &m_enemies });
#endif

	eng::Log::Info("ArenaGame initialized");
	return true;
}

void ArenaGame::RegisterComponents()
{
	// Level files and the editor's Add Component create these by name
	eng::ComponentRegistry& registry = eng::Engine::GetInstance().GetComponentRegistry();
	registry.Register<WinTrigger>("WinTrigger");
	registry.Register<LoseTrigger>("LoseTrigger");
	registry.Register<NextLevelTrigger>("NextLevelTrigger");
}

void ArenaGame::BindInput()
{
	eng::InputMap& map = eng::Engine::GetInstance().GetInputManager().GetMap();

	PlayerInput::BindDefaults(map);		// walk, look, jump, fire, wheel
	map.BindAction("ToggleDebug",		eng::InputBinding::Key(KEY_F2));	// editor builds
	map.BindAction("TogglePhysicsDebug", eng::InputBinding::Key(KEY_F3));
}

void ArenaGame::Destroy()
{
#if ENG_DEV_CONSOLE
	m_commands.Unregister();	// the handlers capture the game's members
#endif

	// Actors first: their bodies leave the physics world before it shuts down
	m_gunView.Shutdown();
	m_gameUI.Shutdown();
	m_shotFX.Shutdown();
	m_gameplay.Shutdown();
	GameContext::SetGameState(nullptr);
	m_player.Reset();
	m_level.Clear();
	SoundFXBank::Instance().Clear();
	m_music.Shutdown();
	eng::Log::Info("ArenaGame destroyed");
}

const char* ArenaGame::GetTitle() const
{
	return "Boomer Shooter";
}

eng::Level* ArenaGame::GetActiveLevel()
{
	return &m_level;
}

//////////////////////////////////////////
/// Levels
//////////////////////////////////////////
bool ArenaGame::LoadLevelFile(const std::string& path)
{
	m_player.Reset();
	m_enemies.Reset();		// the game's own loads (start, next level, restart, retry) do not call OnLevelLoaded
	const bool isLoaded = eng::LevelSerializer::Load(m_level, path).ok;
	if (!isLoaded)
	{
		m_level.Clear();
		eng::Log::Error("ArenaGame: level '%s' could not be loaded, starting in an empty world", path.c_str());
	}

	SpawnPlayer();
	return isLoaded;
}

void ArenaGame::SpawnPlayer()
{
	const eng::SpawnPoint& spawn = m_level.GetSpawn();
	const Vector3 feet = spawn.isSet ? spawn.position : Vector3{ 0.0f, 1.0f, 0.0f };
	m_player = PlayerActor::Create(m_level, { &m_input, &m_playerState, &m_camera }, feet, spawn.isSet ? spawn.yawDegrees : 0.0f, false);
}

void ArenaGame::OnSettingsRequested()
{
	eng::Engine::GetInstance().GetUI().Push<SettingsScreen>();
}

void ArenaGame::OnStartRequested()
{
	m_gameplay.LoadNow(0);		// between frames: safe to clear the level; the mode is Play, so the game starts
}

void ArenaGame::OnReturnToMainMenu()
{
	m_gameState.Reset();
}

void ArenaGame::OnRestartRequested()
{
	m_gameplay.LoadNow(m_gameplay.GetLevelIndex());
}

void ArenaGame::OnLevelLoaded()
{
	// The editor replaced every actor (an old file may even contain a player): the game spawns its own
	for (const auto& owned : m_level.GetActors())
		if (owned->GetComponent<eng::CharacterMotor>())
			owned->MarkForDestroy();
	m_enemies.Reset();
	SpawnPlayer();
}

std::string ArenaGame::GetLevelPath() const
{
	return m_gameplay.GetCurrentPath();
}

//////////////////////////////////////////
/// Frame
//////////////////////////////////////////
void ArenaGame::Update(float deltaTime)
{
	m_gameplay.Update();	// a requested level load (next level trigger, retry) happens here, between frames

	eng::Engine&			engine	= eng::Engine::GetInstance();
	const eng::InputMap&	map		= engine.GetInputManager().GetMap();

	if (eng::IsEditorBuild())
	{
		if (map.IsActionPressed("ToggleDebug"))
			engine.GetDebugDraw().SetEnabled(!engine.GetDebugDraw().IsEnabled());
		if (map.IsActionPressed("TogglePhysicsDebug"))
			engine.GetPhysics().SetDebugDrawEnabled(!engine.GetPhysics().IsDebugDrawEnabled());
	}

	m_input.Update(deltaTime, map);
	m_gunView.Update(deltaTime);
	if (engine.GetModes().IsSimulating())
		m_enemies.Animate(deltaTime);
	UpdateMenuCamera(deltaTime);
}

void ArenaGame::UpdateMenuCamera(float deltaTime)
{
	// Play: the FirstPersonLook on the player owns the camera
	if (eng::Engine::GetInstance().GetModes().GetMode() != eng::EngineMode::MainMenu)
		return;

	m_orbitAngle += ORBIT_SPEED * deltaTime;
	const float angle = m_orbitAngle * DEG2RAD;
	m_camera.SetPosition({ std::sin(angle) * ORBIT_RADIUS, ORBIT_HEIGHT, std::cos(angle) * ORBIT_RADIUS });
	m_camera.LookAt({ 0.0f, 1.0f, 0.0f });
}

void ArenaGame::FixedUpdate(float fixedDeltaTime)
{
	m_enemies.FixedUpdate(fixedDeltaTime);
}

void ArenaGame::SubmitRenderables(eng::RenderQueue& queue)
{
	m_enemies.Submit(queue);
}

void ArenaGame::DrawViewModel()
{
	m_gunView.Draw();
}

void ArenaGame::DrawHUD()
{
	const eng::EngineMode mode = eng::Engine::GetInstance().GetModes().GetMode();
	if (mode != eng::EngineMode::Play && mode != eng::EngineMode::Paused)
		return;

	m_gunHUD.Draw(m_player, m_playerState);

	DrawText(TextFormat("Level %d / %zu", m_gameplay.GetLevelIndex() + 1, m_gameplay.GetLevels().GetCount()), 16, 12, 20, Fade(RAYWHITE, 0.85f));
	if (eng::IsEditorBuild())
		DrawText(TextFormat("FPS %d  |  F2 grid  F3 physics  TAB editor", GetFPS()), 16, 36, 14, Fade(RAYWHITE, 0.7f));
}
