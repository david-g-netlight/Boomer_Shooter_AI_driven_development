#include "ScaleGame.h"
#include <cmath>
#include <raymath.h>

//////////////////////////////////////////
/// Setup
//////////////////////////////////////////
bool ScaleGame::Init()
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
	BindInput();
	GameContext::SetGameState(&m_gameState);

	m_gameplay.Init(m_gameState, m_playerState, LevelList::Default(), [this](const std::string& path) { return LoadLevelFile(path); });
	m_gameplay.LoadNow(0);			// the menu's backdrop
	m_beams.Init(&m_level);			// every gun shot: muzzle burst, beam, impact, sound
	m_gameUI.Init(m_input, m_gameplay);
	m_gunView.Init(m_playerState);

	eng::Log::Info("ScaleGame initialized");
	return true;
}

void ScaleGame::RegisterComponents()
{
	// Level files and the editor's Add Component create these by name
	eng::ComponentRegistry& registry = eng::Engine::GetInstance().GetComponentRegistry();
	registry.Register<WinTrigger>("WinTrigger");
	registry.Register<LoseTrigger>("LoseTrigger");
	registry.Register<NextLevelTrigger>("NextLevelTrigger");
}

void ScaleGame::BindInput()
{
	eng::InputMap& map = eng::Engine::GetInstance().GetInputManager().GetMap();

	PlayerInput::BindDefaults(map);		// walk, look, jump, fire, wheel
	map.BindAction("ToggleDebug",		eng::InputBinding::Key(KEY_F2));	// editor builds
	map.BindAction("TogglePhysicsDebug", eng::InputBinding::Key(KEY_F3));
}

void ScaleGame::Destroy()
{
	// Actors first: their bodies leave the physics world before it shuts down
	m_gunView.Shutdown();
	m_gameUI.Shutdown();
	m_beams.Shutdown();
	m_gameplay.Shutdown();
	GameContext::SetGameState(nullptr);
	m_player.Reset();
	m_level.Clear();
	SoundFXBank::Instance().Clear();
	m_music.Shutdown();
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

//////////////////////////////////////////
/// Levels
//////////////////////////////////////////
bool ScaleGame::LoadLevelFile(const std::string& path)
{
	m_player.Reset();
	const bool isLoaded = eng::LevelSerializer::Load(m_level, path).ok;
	if (!isLoaded)
	{
		m_level.Clear();
		eng::Log::Error("ScaleGame: level '%s' could not be loaded, starting in an empty world", path.c_str());
	}

	SpawnPlayer();
	return isLoaded;
}

void ScaleGame::SpawnPlayer()
{
	const eng::SpawnPoint& spawn = m_level.GetSpawn();
	const Vector3 feet = spawn.isSet ? spawn.position : Vector3{ 0.0f, 1.0f, 0.0f };
	m_player = PlayerActor::Create(m_level, { &m_input, &m_playerState, &m_camera }, feet, spawn.isSet ? spawn.yawDegrees : 0.0f, false);
}

void ScaleGame::OnSettingsRequested()
{
	eng::Engine::GetInstance().GetUI().Push<SettingsScreen>();
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

void ScaleGame::OnLevelLoaded()
{
	// The editor replaced every actor (an old file may even contain a player): the game spawns its own
	for (const auto& owned : m_level.GetActors())
		if (owned->GetComponent<eng::CharacterMotor>())
			owned->MarkForDestroy();
	SpawnPlayer();
}

std::string ScaleGame::GetLevelPath() const
{
	return m_gameplay.GetCurrentPath();
}

//////////////////////////////////////////
/// Frame
//////////////////////////////////////////
void ScaleGame::Update(float deltaTime)
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
	UpdateMenuCamera(deltaTime);
}

void ScaleGame::UpdateMenuCamera(float deltaTime)
{
	// Play: the FirstPersonLook on the player owns the camera
	if (eng::Engine::GetInstance().GetModes().GetMode() != eng::EngineMode::MainMenu)
		return;

	m_orbitAngle += ORBIT_SPEED * deltaTime;
	const float angle = m_orbitAngle * DEG2RAD;
	m_camera.SetPosition({ std::sin(angle) * ORBIT_RADIUS, 10.0f, std::cos(angle) * ORBIT_RADIUS });
	m_camera.LookAt({ 0.0f, 1.0f, -1.0f });
}

void ScaleGame::DrawViewModel()
{
	m_gunView.Draw();
}

void ScaleGame::DrawHUD()
{
	const eng::EngineMode mode = eng::Engine::GetInstance().GetModes().GetMode();
	if (mode != eng::EngineMode::Play && mode != eng::EngineMode::Paused)
		return;

	m_gunHUD.Draw(m_player, m_playerState);

	DrawText(TextFormat("Level %d / %zu", m_gameplay.GetLevelIndex() + 1, m_gameplay.GetLevels().GetCount()), 16, 12, 20, Fade(RAYWHITE, 0.85f));
	if (eng::IsEditorBuild())
		DrawText(TextFormat("FPS %d  |  F2 grid  F3 physics  TAB editor", GetFPS()), 16, 36, 14, Fade(RAYWHITE, 0.7f));
}
