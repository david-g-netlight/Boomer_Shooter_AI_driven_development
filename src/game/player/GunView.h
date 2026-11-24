#pragma once
#include <eng.h>

class PlayerState;

/// <summary>
/// The gun the player holds, drawn in its own pass (Application::DrawViewModel): a separate camera at the origin
/// looking down -Z, with the depth buffer cleared first, so it never clips into walls and ignores the
/// world camera's pitch and yaw. Its parts are primitive meshes with lit materials, drawn through a RenderQueue
/// of its own, with the world's sun turned into the view's space (the gun camera never moves).
/// The gun is the OBJ model assets/models/prototype/gun/gun.obj, recentered on its bounds and scaled to GUN_LENGTH;
/// when the file is missing or `useObj` is off, it is the primitive gun. It kicks back on every shot, bobs a little, its muzzle flashes, and its top
/// light dims while the gun cools down. Only drawn while playing or paused.
/// </summary>
class GunView
{
public:
	static constexpr const char*	MODEL_PATH	= "assets/models/prototype/gun/gun.obj";
	static constexpr float			GUN_LENGTH	= 0.6f;		// the model's length along -Z, in meters of view space (tuned in V3; the primitive gun is 0.48)

	/// <summary>
	/// Spike only: tuned live from the console (gun_obj, gun_ofs, gun_rot, gun_len, gun_muzzle). The defaults are the values the
	/// developer settled on in V3.
	/// </summary>
	struct Tuning
	{
		bool		useObj		= true;						// false: the primitive gun
		Vector3		offset		= { 0.05f, -0.05f, 0.2f };	// added to the gun's place in view space (meters)
		Vector3		rotation	= { 0.0f, 2.0f, 5.0f };		// pitch, yaw, roll of the model in degrees, before it is scaled
		float		length		= GUN_LENGTH;
		Vector3		muzzle		= { 0.0f, 0.05f, -0.1f };		// moves the tip flash from the barrel's end (meters)
	};

	~GunView();

	void	Init(const PlayerState& state);
	void	Shutdown();

	void	Update(float deltaTime);
	void	Draw() const;

	Tuning&			GetTuning()			{ return m_tuning; }
	const Tuning&	GetTuning() const	{ return m_tuning; }

	/// <summary>
	/// Loads the model again from disk (after a re-export). False = failed, the primitive gun is drawn.
	/// </summary>
	bool	ReloadModel();
	bool	HasModel() const	{ return m_model != nullptr; }

private:
	bool	LoadGunModel(bool isReload);
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
	eng::Material	m_modelMaterial;				// lit, the model's albedo texture (only used when the model has a texture)
	bool			m_hasTexture	= false;		// false: the model's own materials are drawn (the .mtl's Kd colors)
	Vector3			m_modelCenter	= { 0.0f, 0.0f, 0.0f };		// bounds center, raw model units
	Vector3			m_modelSize		= { 1.0f, 1.0f, 1.0f };		// bounds size, raw model units
};
