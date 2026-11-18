#include "GunView.h"
#include <algorithm>
#include <cmath>
#include <rlgl.h>
#include "fx/MuzzleFX.h"
#include "player/Gun.h"
#include "state/PlayerState.h"

namespace
{
	constexpr float KICK_DECAY	= 7.0f;		// per second
	constexpr float FLASH_DECAY	= 5.0f;
	constexpr Vector3 ORIGIN	= { 0.19f, -0.15f, -0.78f };	// the gun's center in view space (right, down, forward)
}

GunView::~GunView()
{
	Shutdown();
}

void GunView::Init(const PlayerState& state)
{
	Shutdown();
	m_state = &state;

	// A camera of its own: close clip planes, narrow field of view, always at the origin looking down -Z
	m_camera.SetPosition({ 0.0f, 0.0f, 0.0f });
	m_camera.SetYawPitch(0.0f, 0.0f);
	m_camera.SetPerspective({ 50.0f, 0.02f, 10.0f });

	m_token = eng::Engine::GetInstance().GetEvents().Subscribe<ShotFiredEvent>([this](const ShotFiredEvent& /*shot*/)
	{
		m_kick = 1.0f;
		m_flash = 1.0f;
	});
}

void GunView::Shutdown()
{
	if (m_token != 0)
		eng::Engine::GetInstance().GetEvents().Unsubscribe(m_token);
	m_token = 0;
	m_state = nullptr;
}

void GunView::Update(float deltaTime)
{
	m_kick	= std::max(0.0f, m_kick - KICK_DECAY * deltaTime);
	m_flash	= std::max(0.0f, m_flash - FLASH_DECAY * deltaTime);
}

void GunView::Draw() const
{
	const eng::EngineMode mode = eng::Engine::GetInstance().GetModes().GetMode();
	if (!m_state || (mode != eng::EngineMode::Play && mode != eng::EngineMode::Paused))
		return;

	const float time = static_cast<float>(eng::Engine::GetInstance().GetTime().GetTotalTime());
	const float bob	 = std::sin(time * 1.6f) * 0.004f;
	const float back = m_kick * 0.07f;			// kick: toward the player and a little up

	const bool isReady		= m_state->IsReadyToShoot();
	const Color light		= Fade(Color{ 80, 190, 255, 255 }, isReady ? 1.0f : 0.35f);
	const Color tipColor	= MuzzleFX::START_COLOR;

	m_camera.Begin3D();
	rlDrawRenderBatchActive();
	rlDisableDepthTest();		// parts are drawn far to near instead

	rlPushMatrix();
	rlTranslatef(ORIGIN.x, ORIGIN.y + bob + back * 0.25f, ORIGIN.z + back);
	rlRotatef(-m_kick * 6.0f, 1.0f, 0.0f, 0.0f);		// the muzzle lifts
	rlRotatef(-3.0f, 0.0f, 1.0f, 0.0f);					// pointing a little toward the center
	rlScalef(0.7f, 0.7f, 0.7f);

		// Barrel and tip (farthest)
		DrawCylinderEx({ 0.0f, 0.03f, -0.20f }, { 0.0f, 0.03f, -0.46f }, 0.035f, 0.035f, 12, { 150, 155, 165, 255 });
		DrawCylinderEx({ 0.0f, 0.03f, -0.46f }, { 0.0f, 0.03f, -0.50f }, 0.04f, 0.04f, 12, Fade(tipColor, 0.55f + 0.45f * m_flash));
		// Body, handle, top light (nearest)
		DrawCube({ 0.0f, 0.0f, 0.0f }, 0.11f, 0.13f, 0.38f, { 62, 68, 82, 255 });
		DrawCubeWires({ 0.0f, 0.0f, 0.0f }, 0.11f, 0.13f, 0.38f, { 30, 34, 44, 255 });
		DrawCube({ 0.0f, -0.12f, 0.12f }, 0.07f, 0.16f, 0.09f, { 48, 52, 64, 255 });
		DrawCube({ 0.0f, 0.075f, -0.02f }, 0.045f, 0.025f, 0.12f, light);

	rlPopMatrix();

	rlDrawRenderBatchActive();
	rlEnableDepthTest();
	m_camera.End3D();
}
