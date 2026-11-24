#pragma once
#include <eng.h>

class PlayerState;

/// <summary>
/// The gun the player holds, drawn in its own pass (Application::DrawViewModel): a separate camera at the origin
/// looking down -Z, with the depth buffer cleared first, so it never clips into walls and ignores the
/// world camera's pitch and yaw. The gun is the OBJ model assets/models/gun/gun.obj, fitted to GUN_LENGTH along -Z.
/// When the model is missing or `useObj` is off, it is the primitive gun: primitive meshes with lit materials.
/// Both are drawn through a RenderQueue of its own, with the world's sun turned into the view's space (the gun
/// camera never moves). It kicks back on every shot, bobs a little, its muzzle flashes, and its top light dims
/// while the gun cools down. Only drawn while playing or paused.
/// </summary>
class GunView
{
public:
	static constexpr const char*	MODEL_PATH	= "assets/models/gun/gun.obj";
	static constexpr float			GUN_LENGTH	= 0.6f;		// the model's length along -Z, in meters of view space

	/// <summary>
	/// Tuned live from the console in Debug (gun_obj, gun_ofs, gun_rot, gun_len, gun_muzzle). The defaults are the
	/// values the developer settled on in V3.
	/// </summary>
	struct Tuning
	{
		bool		useObj		= true;						// false: the primitive gun
		Vector3		offset		= { 0.0f, -0.02f, 0.4f };	// added to the gun's place in view space (meters)
		Vector3		rotation	= { 0.0f, 5.0f, 10.0f };		// pitch, yaw, roll of the model in degrees, before it is scaled
		float		length		= GUN_LENGTH;
		Vector3		muzzle		= { 0.0f, 0.05f, -0.1f };	// moves the tip flash from the barrel's end (meters)
	};

	~GunView();

	void	Init(const PlayerState& state);
	void	Shutdown();

	void	Update(float deltaTime);
	void	Draw() const;

	Tuning&			GetTuning()			{ return m_tuning; }
	const Tuning&	GetTuning() const	{ return m_tuning; }

	bool	HasModel() const	{ return m_model != nullptr; }

private:
	bool	LoadGunModel();
	void	SubmitPrimitiveGun(const Matrix& pose) const;
	void	SubmitObjGun(const Matrix& pose) const;
	void	SubmitBox(const eng::Material& material, const Matrix& pose, Vector3 center, Vector3 size) const;
	void	SubmitTube(const eng::Material& material, const Matrix& pose, float x, float y, float zStart, float zEnd, float radius) const;

	const PlayerState*		m_state		= nullptr;
	eng::Camera				m_camera;
	eng::EventBus::Token	m_token		= 0;
	float					m_kick		= 0.0f;		// 1 right after a shot, falls to 0
	float					m_flash		= 0.0f;		// muzzle tip glow

	// Draw() is const but fills the queue and sets the tints of the parts every frame
	struct Parts
	{
		eng::Material	barrel;
		eng::Material	tip;
		eng::Material	body;
		eng::Material	handle;
		eng::Material	light;
	};
	mutable Parts				m_parts;
	mutable eng::RenderQueue	m_queue;

	Tuning			m_tuning;
	const Model*	m_model			= nullptr;		// owned by the ResourceManager; nullptr = primitive gun
	BoundingBox		m_bounds		= {};			// the model's bounds, raw model units
	eng::Material	m_modelMaterial;				// lit, the model's albedo texture (only used when the model has a texture)
	bool			m_hasTexture	= false;		// false: the model's own materials are drawn (the .mtl's Kd colors)
};
