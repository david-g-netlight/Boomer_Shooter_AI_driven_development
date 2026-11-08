#include "GameActor.h"
#include <algorithm>
#include <raymath.h>
#include "core/MathUtils.h"

namespace eng
{
	GameActor::~GameActor()
	{
		DestroyComponents();

		// The Level clears hierarchy links before deleting; this keeps direct deletes safe too
		DetachFromParent();
		for (GameActor* child : m_children)
			child->m_parent = nullptr;
	}

	//////////////////////////////////////////
	/// Identity and state
	//////////////////////////////////////////

	const std::string& GameActor::GetName() const			{ return m_name; }
	void GameActor::SetName(const std::string& name)		{ m_name = name; }
	bool GameActor::IsActive() const						{ return m_isActive; }
	void GameActor::SetActive(bool active)					{ m_isActive = active; }
	bool GameActor::IsMarkedForDestroy() const				{ return m_isMarkedForDestroy; }
	Level* GameActor::GetLevel() const						{ return m_level; }

	bool GameActor::IsActiveInHierarchy() const
	{
		for (const GameActor* actor = this; actor; actor = actor->m_parent)
			if (!actor->m_isActive)
				return false;
		return true;
	}

	void GameActor::MarkForDestroy()
	{
		if (m_isMarkedForDestroy)
			return;

		m_isMarkedForDestroy = true;
		for (GameActor* child : m_children)
			child->MarkForDestroy();
	}

	//////////////////////////////////////////
	/// Hierarchy
	//////////////////////////////////////////
	GameActor* GameActor::GetParent() const							{ return m_parent; }
	const std::vector<GameActor*>& GameActor::GetChildren() const	{ return m_children; }

	bool GameActor::IsDescendantOf(const GameActor* other) const
	{
		for (const GameActor* actor = m_parent; actor; actor = actor->m_parent)
			if (actor == other)
				return true;
		return false;
	}

	bool GameActor::SetParent(GameActor* parent, bool keepWorldTransform)
	{
		if (parent == m_parent)
			return true;
		if (parent == this || (parent && parent->IsDescendantOf(this)))
			return false;	// would create a cycle

		const Matrix world = GetWorldMatrix();

		DetachFromParent();
		m_parent = parent;
		if (parent)
			parent->m_children.push_back(this);

		if (keepWorldTransform)
		{
			// local = world * inverse(parent world), then split back into translation / rotation / scale
			const Matrix parentWorld	= parent ? parent->GetWorldMatrix() : MatrixIdentity();
			const Matrix local			= MatrixMultiply(world, MatrixInvert(parentWorld));
			DecomposeTransform(local, &m_transform.translation, &m_transform.rotation, &m_transform.scale);
		}
		return true;
	}

	void GameActor::DetachFromParent()
	{
		if (!m_parent)
			return;

		auto& siblings = m_parent->m_children;
		siblings.erase(std::remove(siblings.begin(), siblings.end(), this), siblings.end());
		m_parent = nullptr;
	}

	//////////////////////////////////////////
	/// Components
	//////////////////////////////////////////
	size_t GameActor::GetComponentCount() const
	{
		return m_components.size();
	}

	const std::vector<std::unique_ptr<Component>>& GameActor::GetComponents() const
	{
		return m_components;
	}

	void GameActor::DestroyComponents()
	{
		// Reverse creation order, like destructors: later components may depend on earlier ones
		for (auto it = m_components.rbegin(); it != m_components.rend(); ++it)
		{
			Component& component = **it;
			if (component.IsEnabled())
				component.OnDisable();
			component.OnDestroy();
		}
		m_components.clear();
	}

	void GameActor::Update(float deltaTime)
	{
		// By index over a snapshot of the count: components added during the loop start next frame
		const size_t count = m_components.size();
		for (size_t i = 0; i < count && !m_isMarkedForDestroy; ++i)
		{
			Component& component = *m_components[i];
			if (component.IsEnabled())
				component.Update(deltaTime);
		}

		if (!m_isMarkedForDestroy)
			OnUpdate(deltaTime);
	}

	void GameActor::FixedUpdate(float fixedDeltaTime)
	{
		const size_t count = m_components.size();
		for (size_t i = 0; i < count && !m_isMarkedForDestroy; ++i)
		{
			Component& component = *m_components[i];
			if (component.IsEnabled())
				component.FixedUpdate(fixedDeltaTime);
		}

		if (!m_isMarkedForDestroy)
			OnFixedUpdate(fixedDeltaTime);
	}

	void GameActor::SubmitRenderables(RenderQueue& queue) const
	{
		for (const auto& component : m_components)
			if (component->IsEnabled())
				component->SubmitRenderables(queue);
	}

	void GameActor::NotifyTriggerEnter(GameActor& other)
	{
		const size_t count = m_components.size();
		for (size_t i = 0; i < count && !m_isMarkedForDestroy; ++i)
			if (m_components[i]->IsEnabled())
				m_components[i]->OnTriggerEnter(other);
	}

	void GameActor::NotifyTriggerExit(GameActor& other)
	{
		const size_t count = m_components.size();
		for (size_t i = 0; i < count && !m_isMarkedForDestroy; ++i)
			if (m_components[i]->IsEnabled())
				m_components[i]->OnTriggerExit(other);
	}

	//////////////////////////////////////////
	/// Transform
	//////////////////////////////////////////
	::Transform& GameActor::GetTransform()				{ return m_transform; }
	const ::Transform& GameActor::GetTransform() const	{ return m_transform; }

	void GameActor::SetPosition(const Vector3& position)	{ m_transform.translation = position; }
	void GameActor::SetRotation(const Quaternion& rotation)	{ m_transform.rotation = QuaternionNormalize(rotation); }
	void GameActor::SetScale(const Vector3& scale)			{ m_transform.scale = scale; }
	void GameActor::SetScale(float uniformScale)			{ m_transform.scale = { uniformScale, uniformScale, uniformScale }; }

	void GameActor::SetRotationEuler(const Vector3& degrees)
	{
		m_transform.rotation = QuaternionFromEuler(degrees.x * DEG2RAD, degrees.y * DEG2RAD, degrees.z * DEG2RAD);
	}

	Matrix GameActor::GetLocalMatrix() const
	{
		const Matrix scale			= MatrixScale(m_transform.scale.x, m_transform.scale.y, m_transform.scale.z);
		const Matrix rotation		= QuaternionToMatrix(m_transform.rotation);
		const Matrix translation	= MatrixTranslate(m_transform.translation.x, m_transform.translation.y, m_transform.translation.z);
		return MatrixMultiply(MatrixMultiply(scale, rotation), translation);
	}

	Matrix GameActor::GetWorldMatrix() const
	{
		const Matrix local = GetLocalMatrix();
		return m_parent ? MatrixMultiply(local, m_parent->GetWorldMatrix()) : local;
	}

	Vector3 GameActor::GetWorldPosition() const
	{
		const Matrix world = GetWorldMatrix();
		return { world.m12, world.m13, world.m14 };
	}
}
