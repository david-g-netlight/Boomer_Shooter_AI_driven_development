#include "GunView.h"
#include <algorithm>
#include <cmath>
#include <raymath.h>
#include <rlgl.h>
#include "fx/MuzzleFX.h"
#include "player/Gun.h"
#include "state/PlayerState.h"

namespace
{
	constexpr float KICK_DECAY	= 7.0f;		// per second
	constexpr float FLASH_DECAY	= 5.0f;
	constexpr Vector3 ORIGIN	= { 0.19f, -0.15f, -0.78f };	// the gun's center in view space (right, down, forward)
	constexpr Color LIGHT_COLOR	= { 80, 190, 255, 255 };
	constexpr float PRIMITIVE_SCALE	= 0.7f;		// the primitive gun is modeled bigger and scaled down
	constexpr float MUZZLE_Y	= 0.021f;		// OBJ gun: the barrel's height above the model's center (meters, to tune)
	constexpr float TIP_LENGTH	= 0.028f;
	constexpr float TIP_RADIUS	= 0.028f;
	constexpr float ALPHA_CUTOFF	= 0.5f;		// texels more transparent than this are holes (only for textures with some)

	Color Brighten(Color color, float factor)
	{
		return { static_cast<unsigned char>(color.r * factor), static_cast<unsigned char>(color.g * factor), static_cast<unsigned char>(color.b * factor), 255 };
	}
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

	// Made here, not in the constructor: a Material takes raylib's default texture from the window
	const Shader* lit = eng::Engine::GetInstance().GetResources().GetDefaultLitShader();
	const auto make = [&](eng::Material& material, Color tint, float emissive)
	{
		material = eng::Material(lit);
		material.SetTint(tint);
		if (emissive > 0.0f)
			material.SetParameter("emissive", emissive);
	};
	make(m_parts.barrel, { 150, 155, 165, 255 }, 0.0f);
	make(m_parts.tip,    MuzzleFX::START_COLOR,  1.0f);
	make(m_parts.body,   { 62, 68, 82, 255 },    0.0f);
	make(m_parts.handle, { 48, 52, 64, 255 },    0.0f);
	make(m_parts.light,  LIGHT_COLOR,            1.0f);

	LoadGunModel();

	m_token = eng::Engine::GetInstance().GetEvents().Subscribe<ShotFiredEvent>([this](const ShotFiredEvent& /*shot*/)
	{
		m_kick = 1.0f;
		m_flash = 1.0f;
	});
}

bool GunView::LoadGunModel()
{
	eng::ResourceManager& resources = eng::Engine::GetInstance().GetResources();
	m_model = resources.LoadModel(MODEL_PATH);
	const BoundingBox* bounds = resources.GetModelBounds(MODEL_PATH);
	if (!m_model || !bounds)
	{
		m_model = nullptr;
		eng::Log::Warning("GunView: '%s' did not load, drawing the primitive gun", MODEL_PATH);
		return false;
	}
	m_bounds = *bounds;

	// A textured model is drawn with a lit override material (its texture; one with transparent texels is cut out).
	// A model without a texture keeps its own materials, which carry the .mtl's Kd colors (the lit shader is set at load)
	const Shader* lit = resources.GetDefaultLitShader();
	m_modelMaterial = eng::Material(lit);
	m_hasTexture = false;
	const Texture2D* texture = (m_model->materialCount > 0) ? &m_model->materials[0].maps[MATERIAL_MAP_DIFFUSE].texture : nullptr;
	if (texture && texture->id > 0 && texture->id != rlGetTextureIdDefault())
	{
		m_modelMaterial.SetTexture(texture);
		m_hasTexture = true;

		const bool hasTransparency = eng::HasTransparentTexels(*texture);
		if (hasTransparency)
			m_modelMaterial.SetParameter("alphaCutoff", ALPHA_CUTOFF);
		eng::Log::Info("GunView: gun texture %dx%d, %s", texture->width, texture->height, hasTransparency ? "with transparent texels (alphaCutoff on)" : "opaque");
	}
	return true;
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

	eng::ResourceManager& resources = eng::Engine::GetInstance().GetResources();
	if (!resources.GetPrimitive(eng::Primitive::Cube) || !resources.GetPrimitive(eng::Primitive::Cylinder))
		return;

	const float time = static_cast<float>(eng::Engine::GetInstance().GetTime().GetTotalTime());
	const float bob	 = std::sin(time * 1.6f) * 0.004f;
	const float back = m_kick * 0.07f;			// kick: toward the player and a little up

	const bool isReady = m_state->IsReadyToShoot();

	// Tip and light are unlit: their brightness is the tint
	m_parts.tip.SetTint(Brighten(MuzzleFX::START_COLOR, 0.55f + 0.45f * m_flash));
	m_parts.light.SetTint(Brighten(LIGHT_COLOR, isReady ? 1.0f : 0.35f));

	// Translate * RotateX(muzzle lifts) * RotateY(a little toward the center) * Scale; raymath applies the left matrix first
	const bool useObj = m_tuning.useObj && m_model;
	const Vector3 offset = useObj ? m_tuning.offset : Vector3{ 0.0f, 0.0f, 0.0f };
	const float scale = useObj ? 1.0f : PRIMITIVE_SCALE;		// the model is scaled to its length in meters instead
	const Matrix pose = MatrixMultiply(MatrixMultiply(MatrixMultiply(
		MatrixScale(scale, scale, scale),
		MatrixRotateY(-3.0f * DEG2RAD)),
		MatrixRotateX(-m_kick * 6.0f * DEG2RAD)),
		MatrixTranslate(ORIGIN.x + offset.x, ORIGIN.y + offset.y + bob + back * 0.25f, ORIGIN.z + offset.z + back));

	if (useObj)
		SubmitObjGun(pose);
	else
		SubmitPrimitiveGun(pose);

	// The sun in view space: the gun camera looks down -Z with +X right and +Y up, so the world's sun is turned by the
	// main camera's right / up / forward (forward becomes -Z)
	const eng::World& world = eng::Engine::GetInstance().GetWorld();
	Vector3 sun = world.sunDirection;
	if (const eng::Camera* view = eng::Engine::GetInstance().GetMainCamera().GetActiveCamera())
		sun = { Vector3DotProduct(sun, view->Right()), Vector3DotProduct(sun, view->Up()), -Vector3DotProduct(sun, view->Forward()) };
	m_queue.SetLighting({ sun, world.sunColor, world.ambientColor });

	m_camera.Begin3D();
	eng::ClearDepthBuffer();	// the world's depth is gone: the gun never clips into walls, and tests against itself
	m_queue.Draw(m_camera);
	rlDrawRenderBatchActive();
	m_camera.End3D();
	m_queue.Clear();
}

void GunView::SubmitBox(const eng::Material& material, const Matrix& pose, Vector3 center, Vector3 size) const
{
	const ::Mesh* cube = eng::Engine::GetInstance().GetResources().GetPrimitive(eng::Primitive::Cube);
	m_queue.Submit(*cube, material, MatrixMultiply(MatrixMultiply(MatrixScale(size.x, size.y, size.z), MatrixTranslate(center.x, center.y, center.z)), pose));
}

// A cylinder from z = zStart to z = zEnd (the mesh's +Y turned to -Z), centered on (x, y)
void GunView::SubmitTube(const eng::Material& material, const Matrix& pose, float x, float y, float zStart, float zEnd, float radius) const
{
	const ::Mesh* cylinder = eng::Engine::GetInstance().GetResources().GetPrimitive(eng::Primitive::Cylinder);
	const Matrix local = MatrixMultiply(MatrixMultiply(MatrixScale(radius * 2.0f, zStart - zEnd, radius * 2.0f), MatrixRotateX(-90.0f * DEG2RAD)), MatrixTranslate(x, y, zStart));
	m_queue.Submit(*cylinder, material, MatrixMultiply(local, pose));
}

void GunView::SubmitPrimitiveGun(const Matrix& pose) const
{
	SubmitTube(m_parts.barrel, pose, 0.0f, 0.03f, -0.20f, -0.46f, 0.035f);
	SubmitTube(m_parts.tip,    pose, 0.0f, 0.03f, -0.46f, -0.50f, 0.04f);
	SubmitBox(m_parts.body,   pose, { 0.0f,  0.0f,   0.0f   }, { 0.11f,  0.13f,  0.38f });
	SubmitBox(m_parts.handle, pose, { 0.0f, -0.12f,  0.12f  }, { 0.07f,  0.16f,  0.09f });
	SubmitBox(m_parts.light,  pose, { 0.0f,  0.075f, -0.02f }, { 0.045f, 0.025f, 0.12f });
}

void GunView::SubmitObjGun(const Matrix& pose) const
{
	// Fit to the length along -Z (recentered on the bounds), then turn (gun_rot)
	const Matrix fit = eng::FitModel(m_bounds, eng::FitAxis::Z, m_tuning.length);
	const Vector3 rotation = Vector3Scale(m_tuning.rotation, DEG2RAD);
	const Matrix turned = MatrixMultiply(MatrixRotateXYZ(rotation), pose);
	m_queue.SubmitModel(*m_model, m_hasTexture ? &m_modelMaterial : nullptr, MatrixMultiply(fit, turned));

	// Tip flash on the barrel's end and the ready light on top, in the fitted model's space (turned with the model)
	const Vector3 extent = Vector3Subtract(m_bounds.max, m_bounds.min);
	const float scale = eng::FitScale(m_bounds, eng::FitAxis::Z, m_tuning.length);
	const float halfLength = m_tuning.length * 0.5f;
	const float topY = extent.y * scale * 0.5f;
	SubmitTube(m_parts.tip, turned, m_tuning.muzzle.x, MUZZLE_Y + m_tuning.muzzle.y, -halfLength + m_tuning.muzzle.z + TIP_LENGTH, -halfLength + m_tuning.muzzle.z, TIP_RADIUS);
	SubmitBox(m_parts.light, turned, { 0.0f, topY, -0.014f }, { 0.0315f, 0.0175f, 0.084f });
}
