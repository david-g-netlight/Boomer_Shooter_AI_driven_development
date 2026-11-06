#pragma once
#include <string>
#include <vector>
#include <raylib.h>

namespace eng
{
	class Level;

	/// <summary>
	/// An object in a Level: name, local transform, parent/children, active flag.
	/// The transform is raylib's ::Transform (translation, quaternion rotation, scale) - the same fields as
	/// the reference engine's Transform.
	///
	/// Created and owned by a Level (CreateGameActor); parent/children pointers are NON-owning.
	/// MarkForDestroy() never deletes immediately: the Level deletes marked actors at the end of the frame,
	/// so it is safe to call from inside any Update.
	/// </summary>
	class GameActor
	{
	public:
		virtual ~GameActor();

		GameActor(const GameActor&) = delete;
		GameActor& operator = (const GameActor&) = delete;

		//////////////////////////////////////////
		/// Identity and state
		//////////////////////////////////////////
		const std::string&	GetName() const;
		void				SetName(const std::string& name);

		bool				IsActive() const;				// own flag
		bool				IsActiveInHierarchy() const;	// false if this or any parent is inactive
		void				SetActive(bool active);

		void				MarkForDestroy();				// also marks every child
		bool				IsMarkedForDestroy() const;

		Level*				GetLevel() const;

		//////////////////////////////////////////
		/// Hierarchy
		//////////////////////////////////////////
		GameActor*						GetParent() const;
		const std::vector<GameActor*>&	GetChildren() const;

		/// <summary>
		/// nullptr = root. Refuses to parent to itself or to one of its own descendants.
		/// keepWorldTransform: keep the world pose by recomputing the local transform.
		/// </summary>
		bool				SetParent(GameActor* parent, bool keepWorldTransform = false);
		bool				IsDescendantOf(const GameActor* other) const;

		//////////////////////////////////////////
		/// Transform (local, relative to the parent)
		//////////////////////////////////////////
		[[nodiscard]] ::Transform&			GetTransform();
		[[nodiscard]] const ::Transform&	GetTransform() const;

		void				SetPosition(const Vector3& position);
		void				SetRotation(const Quaternion& rotation);
		void				SetRotationEuler(const Vector3& degrees);	// x = pitch, y = yaw, z = roll
		void				SetScale(const Vector3& scale);
		void				SetScale(float uniformScale);

		[[nodiscard]] Matrix	GetLocalMatrix() const;		// scale, then rotation, then translation
		[[nodiscard]] Matrix	GetWorldMatrix() const;		// local * parent world
		[[nodiscard]] Vector3	GetWorldPosition() const;

		//////////////////////////////////////////
		/// Per-frame hooks for derived actors (called by the Level)
		//////////////////////////////////////////
		virtual void		OnUpdate(float /*deltaTime*/) {}
		virtual void		OnFixedUpdate(float /*fixedDeltaTime*/) {}

	protected:
		// Preventing direct instantiation: actors are created by a Level
		GameActor() = default;

	private:
		void				DetachFromParent();

		std::string				m_name				= "Unnamed";
		GameActor*				m_parent			= nullptr;
		std::vector<GameActor*>	m_children;
		Level*					m_level				= nullptr;
		bool					m_isActive			= true;
		bool					m_isMarkedForDestroy= false;

		::Transform				m_transform			= { { 0.0f, 0.0f, 0.0f },			// translation
														{ 0.0f, 0.0f, 0.0f, 1.0f },		// rotation (identity)
														{ 1.0f, 1.0f, 1.0f } };			// scale

		friend class Level;
	};
}
