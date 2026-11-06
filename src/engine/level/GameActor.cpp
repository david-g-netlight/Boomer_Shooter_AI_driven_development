#include "GameActor.h"
#include <algorithm>
#include <raymath.h>

namespace eng
{
	GameActor::~GameActor()
	{
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
			MatrixDecompose(local, &m_transform.translation, &m_transform.rotation, &m_transform.scale);
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
