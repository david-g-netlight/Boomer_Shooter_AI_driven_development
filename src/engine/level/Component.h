#pragma once

namespace eng
{
	class GameActor;
	class RenderQueue;

	/// <summary>
	/// A piece of behaviour or data attached to a GameActor (like a Unity MonoBehaviour).
	/// Created with GameActor::AddComponent<T>(), owned by the actor, destroyed with it.
	///
	/// Lifecycle:  OnCreate -> OnEnable -> (Update / FixedUpdate / SubmitRenderables)* -> OnDisable -> OnDestroy
	/// Update hooks only run while the component is enabled and its actor is active in the hierarchy.
	/// </summary>
	class Component
	{
	public:
		virtual ~Component() = default;

		Component(const Component&) = delete;
		Component& operator = (const Component&) = delete;

		virtual void		OnCreate() {}		// owner is set
		virtual void		OnEnable() {}
		virtual void		OnDisable() {}
		virtual void		OnDestroy() {}

		virtual void		Update(float /*deltaTime*/) {}
		virtual void		FixedUpdate(float /*fixedDeltaTime*/) {}
		virtual void		SubmitRenderables(RenderQueue& /*queue*/) const {}

		/// <summary>
		/// A TriggerVolume involving this actor started / stopped overlapping `other` (called during the fixed tick).
		/// Both sides are notified: the trigger's actor and the actor that entered it.
		/// </summary>
		virtual void		OnTriggerEnter(GameActor& /*other*/) {}
		virtual void		OnTriggerExit(GameActor& /*other*/) {}

		virtual const char*	TypeName() const = 0;

		/// <summary>
		/// nullptr only for components used standalone (not attached to an actor).
		/// </summary>
		GameActor*			GetOwner() const;

		bool				IsEnabled() const;
		void				SetEnabled(bool enabled);	// calls OnEnable / OnDisable on change

	protected:
		Component() = default;

	private:
		GameActor*			m_owner		= nullptr;
		bool				m_isEnabled	= true;

		friend class GameActor;
	};
}
