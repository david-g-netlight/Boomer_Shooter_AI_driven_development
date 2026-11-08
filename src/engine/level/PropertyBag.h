#pragma once
#include <string>
#include <utility>
#include <vector>
#include <raylib.h>

namespace eng
{
	/// <summary>
	/// Ordered key = value pairs of one component in a level file (`component RigidBody type=dynamic mass=2`).
	/// Components fill it in Serialize() and read it in Deserialize(); a missing key gives the default.
	/// Values never contain spaces. Vectors and colors are comma separated (`1,2,3`).
	/// </summary>
	class PropertyBag
	{
	public:
		void		SetString	(const std::string& key, const std::string& value);
		void		SetFloat	(const std::string& key, float value);
		void		SetInt		(const std::string& key, int value);
		void		SetBool		(const std::string& key, bool value);
		void		SetVec3		(const std::string& key, const Vector3& value);
		void		SetColor	(const std::string& key, const Color& value);

		bool		Has			(const std::string& key) const;
		std::string	GetString	(const std::string& key, const std::string& fallback = "") const;
		float		GetFloat	(const std::string& key, float fallback = 0.0f) const;
		int			GetInt		(const std::string& key, int fallback = 0) const;
		bool		GetBool		(const std::string& key, bool fallback = false) const;
		Vector3		GetVec3		(const std::string& key, const Vector3& fallback = { 0.0f, 0.0f, 0.0f }) const;
		Color		GetColor	(const std::string& key, const Color& fallback = WHITE) const;

		const std::vector<std::pair<std::string, std::string>>&	Items() const;

	private:
		const std::string*	Find(const std::string& key) const;

		std::vector<std::pair<std::string, std::string>>	m_items;
	};
}
