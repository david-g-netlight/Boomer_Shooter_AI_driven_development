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
	m_sfxStretch	= resources.LoadSound("assets/audio/sfx_stretch.wav");
	m_sfxShrink		= resources.LoadSound("assets/audio/sfx_shrink.wav");

	m_themeMenu = resources.LoadMusic("assets/audio/music_menu.wav");
	m_themeGame = resources.LoadMusic("assets/audio/music_game.wav");
	if (m_themeMenu)
	{
		eng::MusicPlayer& music = eng::Engine::GetInstance().GetAudio().GetMusic();
		music.SetVolume(0.7f);
		music.Play(*m_themeMenu, MUSIC_FADE_IN);
	}

	BuildSandbox();

	// EventBus demo: the player walking into the trigger zone opens a dialog (preview of the Phase 12 win screen)
	m_triggerToken = eng::Engine::GetInstance().GetEvents().Subscribe<TriggerEnteredEvent>([](const TriggerEnteredEvent& e)
	{
		if (e.other != "Player")
			return;
		eng::Engine::GetInstance().GetUI().Push<eng::MessageScreen>("Zone reached", "The player entered the " + e.zone + ".",
			std::vector<eng::MessageScreen::Button>{
				{ "Continue", {} },
				{ "Main Menu", [] { eng::Engine::GetInstance().GetModes().Request(eng::EngineMode::MainMenu); } } });
	});

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

	map.BindAction("ToggleDebug", InputBinding::Key(KEY_F2));
	map.BindAction("SwitchTheme", InputBinding::Key(KEY_M));

	// Physics sandbox
	map.BindAction("DeleteAimed", InputBinding::Key(KEY_X));
	map.BindAction("TogglePlayer", InputBinding::Key(KEY_V));
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
				BodyType::Dynamic, HEAVY_MASS, HEAVY }));

	for (int i = 0; i < 5; ++i)
		m_crates.push_back(SpawnBox({ TextFormat("Light %d", i), { 5.0f, 0.45f + 0.9f * i, 2.0f }, { 0.9f, 0.9f, 0.9f }, {},
			BodyType::Dynamic, LIGHT_MASS, LIGHT }));

	for (int i = 0; i < 4; ++i)
		m_crates.push_back(SpawnBox({ TextFormat("Dropped %d", i), { -4.0f + 0.4f * i, 6.0f + 2.0f * i, 2.0f + 0.3f * i },
			{ 0.8f, 0.8f, 0.8f }, { 15.0f * i, 30.0f * i, 10.0f }, BodyType::Dynamic, 5.0f, DROPPED }));

	m_crates.push_back(SpawnBox({ "Jump crate", { -3.0f, 0.4f, 7.0f }, { 1.6f, 0.8f, 1.6f }, {},
		BodyType::Dynamic, 60.0f, RED }));

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

	SpawnPlayer();
}

void ScaleGame::ThrowPellet()
{
	// Oldest out when the cap is reached
	if (m_pellets.size() >= MAX_PELLETS)
	{
		m_pellets.front()->MarkForDestroy();
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
	for (eng::GameActor*& pellet : m_pellets)
	{
		const Vector3 p = pellet->GetWorldPosition();
		if (p.z < WALL_Z - 0.3f && std::fabs(p.x) < 3.0f && p.y > 0.0f && p.y < 3.0f)
		{
			++m_tunnelled;
			pellet->MarkForDestroy();
		}
	}
	m_pellets.erase(std::remove_if(m_pellets.begin(), m_pellets.end(),
		[](const eng::GameActor* pellet) { return pellet->IsMarkedForDestroy(); }), m_pellets.end());
}

void ScaleGame::SpawnPlayer()
{
	eng::ResourceManager& resources = eng::Engine::GetInstance().GetResources();

	// Capsule radius 0.5, 1 m between the sphere centers: 2 m tall, actor position = capsule center
	m_player = m_level.CreateGameActor("Player");
	m_player->SetPosition(PLAYER_SPAWN);
	eng::Collider* collider = m_player->AddComponent<eng::Collider>();
	collider->SetCapsule(0.5f, 1.0f);
	collider->SetLayer(eng::Layer::Player);
	m_player->AddComponent<eng::CharacterMotor>();

	// Visual: the unit cylinder sits on y = 0, so offset it down to cover the capsule (-1 .. +1)
	eng::GameActor* body = m_level.CreateGameActor("Player Mesh", m_player);
	body->SetPosition({ 0.0f, -1.0f, 0.0f });
	body->SetScale({ 1.0f, 2.0f, 1.0f });
	body->AddComponent<eng::MeshRenderer>(resources.GetPrimitive(eng::Primitive::Cylinder))
		->CreateOwnMaterial(m_litShader).SetTint({ 70, 110, 220, 255 });
}

void ScaleGame::EmitImpact(float sign)
{
	if (!m_aimHit)
		return;

	// Unity BeamImpactParticle / RayBeamController gradients: stretch blue, shrink yellow, alpha 1 -> 0
	eng::ParticleEmitterSettings impact;
	impact.burstCount		= 40;
	impact.speed			= 3.5f;
	impact.spreadDegrees	= 70.0f;
	impact.gravity			= { 0.0f, -3.0f, 0.0f };
	impact.additive			= true;
	impact.startColor		= sign > 0.0f ? Color{ 40, 120, 255, 255 } : Color{ 255, 220, 40, 255 };
	impact.endColor			= sign > 0.0f ? Color{ 40, 255, 120, 0 }   : Color{ 255, 60, 20, 0 };
	eng::Engine::GetInstance().GetParticles().Emit(impact, m_aimHit->point, m_aimHit->normal);
}

void ScaleGame::FireBeam(float sign)
{
	// Muzzle offset from Unity RayBeamProvider: a gun held low and to the right
	const Vector3 muzzle = Vector3Add(m_camera.GetPosition(),
		Vector3Add(Vector3Scale(m_camera.Right(), 0.3f), Vector3Add(Vector3Scale(m_camera.Up(), -0.25f), Vector3Scale(m_camera.Forward(), 0.6f))));
	const Vector3 target = m_aimHit ? m_aimHit->point : Vector3Add(m_camera.GetPosition(), Vector3Scale(m_camera.Forward(), AIM_REACH));

	constexpr float FADE_SECONDS = 0.35f;
	eng::GameActor* beam = m_level.CreateGameActor("Beam");
	eng::LineRenderer* line = beam->AddComponent<eng::LineRenderer>();
	line->SetPoints(muzzle, target);
	// Unity RayBeamController: stretch blue -> green, shrink yellow -> red
	line->SetColors(sign > 0.0f ? Color{ 40, 120, 255, 230 } : Color{ 255, 220, 40, 230 },
					sign > 0.0f ? Color{ 40, 255, 120, 200 } : Color{ 255, 60, 20, 200 });
	line->SetWidth(0.06f, 0.02f);
	line->FadeOut(FADE_SECONDS);
	beam->AddComponent<SelfDestructTimer>(FADE_SECONDS);
}

bool ScaleGame::IsCrate(const eng::GameActor* actor) const
{
	return std::find(m_crates.begin(), m_crates.end(), actor) != m_crates.end();
}

void ScaleGame::UpdateAim()
{
	const eng::InputMap& map = eng::Engine::GetInstance().GetInputManager().GetMap();

	// Center-screen ray, like the scaling gun (Unity ScalingProvider): Ground | Prop, ignores player and triggers
	m_aimHit = eng::Engine::GetInstance().GetPhysics().Raycast(m_camera.GetPosition(), m_camera.Forward(), AIM_REACH,
		eng::Layer::Ground | eng::Layer::Prop);

	// Beam + impact burst wherever the ray hits (Phase 7)
	if (map.IsActionPressed("Fire"))	{ FireBeam(+1.0f); EmitImpact(+1.0f); }
	if (map.IsActionPressed("AltFire"))	{ FireBeam(-1.0f); EmitImpact(-1.0f); }

	if (!m_aimHit || !IsCrate(m_aimHit->actor))
		return;

	// LMB: kick the crate at the hit point (off-center hits also spin it)
	if (map.IsActionPressed("Fire"))
	{
		eng::RigidBody* body = m_aimHit->actor->GetComponent<eng::RigidBody>();
		const Vector3 offset = Vector3Subtract(m_aimHit->point, m_aimHit->actor->GetWorldPosition());
		body->AddImpulse(Vector3Scale(m_camera.Forward(), body->GetMass() * KICK_SPEED), offset);
	}

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
	m_isPlayerMode		= false;
	m_pellets.clear();
	m_crates.clear();
	m_level.Clear();
}

void ScaleGame::OnRestartRequested()
{
	// Called by the ModeMachine between frames (not inside a level update): safe to clear the level
	ClearSandbox();
	BuildSandbox();
	m_deletedCrates = m_pelletsThrown = m_tunnelled = 0;
	eng::Log::Info("ScaleGame: sandbox rebuilt");
}

void ScaleGame::PruneDestroyedCrates()
{
	m_crates.erase(std::remove_if(m_crates.begin(), m_crates.end(),
		[](const eng::GameActor* crate) { return crate->IsMarkedForDestroy(); }), m_crates.end());
}

//////////////////////////////////////////
/// Frame
//////////////////////////////////////////
void ScaleGame::Update(float deltaTime)
{
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

	if (map.IsActionPressed("TogglePlayer"))
	{
		m_isPlayerMode = !m_isPlayerMode;
		if (m_isPlayerMode)
			m_player->GetComponent<eng::CharacterMotor>()->Warp(PLAYER_SPAWN);
	}

	// Menus, pause and the editor only move the camera: clicks there belong to the UI
	const bool isPlaying = eng::Engine::GetInstance().GetModes().IsSimulating();
	if (isPlaying && map.IsActionPressed("ThrowPellet"))
		ThrowPellet();

	UpdatePlayer(deltaTime);
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

void ScaleGame::UpdatePlayer(float /*deltaTime*/)
{
	const eng::InputManager&	input	= eng::Engine::GetInstance().GetInputManager();
	const eng::InputMap&		map		= input.GetMap();
	eng::CharacterMotor*		motor	= m_player->GetComponent<eng::CharacterMotor>();

	if (!m_isPlayerMode)
	{
		motor->SetWalkVelocity({ 0.0f, 0.0f, 0.0f });
		return;
	}

	// TODO(Phase 10): replaced by PlayerInput + FirstPersonController + FirstPersonLook
	if (input.IsCursorLocked())
	{
		const Vector2 look = map.GetAxis2D("Look");
		m_camera.Rotate(look.x * LOOK_SENSITIVITY, -look.y * LOOK_SENSITIVITY);	// screen y grows downwards
	}

	// Walk relative to where the camera faces, on the ground plane
	const Vector2 move		= map.GetAxis2D("Move");
	Vector3 forward			= m_camera.Forward();
	forward.y				= 0.0f;
	forward					= Vector3Normalize(forward);
	const Vector3 right		= m_camera.Right();
	motor->SetWalkVelocity(Vector3Scale(Vector3Add(Vector3Scale(forward, move.y), Vector3Scale(right, move.x)), WALK_SPEED));

	if (map.IsActionPressed("Jump"))
		motor->Jump(JUMP_SPEED);
}

void ScaleGame::UpdateCamera(float deltaTime)
{
	const eng::InputManager&	input	= eng::Engine::GetInstance().GetInputManager();
	const eng::InputMap&		map		= input.GetMap();

	if (m_isPlayerMode)
	{
		// Eyes on the capsule (the actor holds the physics position after the last fixed step)
		m_camera.SetPosition(Vector3Add(m_player->GetWorldPosition(), { 0.0f, EYE_HEIGHT, 0.0f }));
		return;
	}

	const eng::EngineMode mode = eng::Engine::GetInstance().GetModes().GetMode();
	if (mode == eng::EngineMode::Play && input.IsCursorLocked())
	{
		// TODO(Phase 10): replaced by FirstPersonLook / FirstPersonController
		const Vector2 look = map.GetAxis2D("Look");
		const Vector2 move = map.GetAxis2D("Move");
		m_camera.Rotate(look.x * LOOK_SENSITIVITY, -look.y * LOOK_SENSITIVITY);	// screen y grows downwards
		m_camera.MoveLocal(move.y * FLY_SPEED * deltaTime, move.x * FLY_SPEED * deltaTime, 0.0f);
	}
	else if (mode == eng::EngineMode::MainMenu)
	{
		// Menu backdrop: slow orbit around the sandbox
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

	// Wheel picks the scale factor (Unity PlayerState::ChangeFactor)
	const float scroll = map.GetAxis("Scroll");
	if (scroll > 0.0f && m_scaleFactorIndex < 2)	++m_scaleFactorIndex;
	if (scroll < 0.0f && m_scaleFactorIndex > 0)	--m_scaleFactorIndex;
	const float factor = SCALE_FACTORS[m_scaleFactorIndex];

	// Unity ScalingAudioFX: stretch pitch = (1 + factor) + 0.02, shrink pitch = (1 - factor) - 0.02
	if (map.IsActionPressed("Fire") && m_sfxStretch)
		audio.Play(*m_sfxStretch, 0.8f, 1.0f + factor + 0.02f);
	if (map.IsActionPressed("AltFire") && m_sfxShrink)
		audio.Play(*m_sfxShrink, 0.8f, 1.0f - factor - 0.02f);

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
	DrawText(TextFormat("V: %s  |  ESC: pause  |  F2 / F3 debug  |  %s", m_isPlayerMode ? "fly camera" : "play (WASD, Space)",
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
	for (const eng::GameActor* crate : m_crates)
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
			IsCrate(m_aimHit->actor) ? "  (LMB kick, X delete)" : ""), x + 10, y + 60, 10, BLACK);
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
	DrawRectangle(x, y, 330, 45, Fade(BEIGE, 0.6f));
	DrawRectangleLines(x, y, 330, 45, BROWN);
	DrawText(TextFormat("Audio  |  LMB / RMB sfx x%.2f (wheel)  |  M: %s theme", SCALE_FACTORS[m_scaleFactorIndex], track),
		x + 10, y + 10, 10, DARKGRAY);
	DrawText(TextFormat("ESC pause ducks the music  |  duck %.2f", audio.GetDuckFactor()), x + 10, y + 26, 10, DARKGRAY);
}

void ScaleGame::Destroy()
{
	// Actors first: their bodies leave the physics world before it shuts down
	eng::Engine::GetInstance().GetEvents().Unsubscribe(m_triggerToken);
	ClearSandbox();
	m_sfxStretch	= nullptr;
	m_sfxShrink		= nullptr;
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
