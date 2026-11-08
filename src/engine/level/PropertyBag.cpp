#include "PropertyBag.h"
#include <cstdio>
#include <cstdlib>

namespace eng
{
	namespace
	{
		std::string Number(float value)
		{
			char text[32];
			std::snprintf(text, sizeof(text), "%.6g", static_cast<double>(value));
			return text;
		}

		/// <summary>
		/// Reads up to `count` comma separated numbers; false if there are fewer.
		/// </summary>
		bool ParseList(const std::string& text, float* out, int count)
		{
			const char* cursor = text.c_str();
			for (int i = 0; i < count; ++i)
			{
				char* end = nullptr;
				out[i] = std::strtof(cursor, &end);
				if (end == cursor)
					return false;
				cursor = end;
				if (*cursor == ',')
					++cursor;
			}
			return true;
		}
	}

	void PropertyBag::SetString(const std::string& key, const std::string& value)
	{
		for (auto& item : m_items)
			if (item.first == key) { item.second = value; return; }
		m_items.emplace_back(key, value);
	}

	void PropertyBag::SetFloat(const std::string& key, float value)	{ SetString(key, Number(value)); }
	void PropertyBag::SetInt(const std::string& key, int value)		{ SetString(key, std::to_string(value)); }
	void PropertyBag::SetBool(const std::string& key, bool value)	{ SetString(key, value ? "1" : "0"); }

	void PropertyBag::SetVec3(const std::string& key, const Vector3& value)
	{
		SetString(key, Number(value.x) + "," + Number(value.y) + "," + Number(value.z));
	}

	void PropertyBag::SetColor(const std::string& key, const Color& value)
	{
		SetString(key, std::to_string(value.r) + "," + std::to_string(value.g) + "," + std::to_string(value.b) + "," + std::to_string(value.a));
	}

	const std::string* PropertyBag::Find(const std::string& key) const
	{
		for (const auto& item : m_items)
			if (item.first == key)
				return &item.second;
		return nullptr;
	}

	bool PropertyBag::Has(const std::string& key) const { return Find(key) != nullptr; }

	std::string PropertyBag::GetString(const std::string& key, const std::string& fallback) const
	{
		const std::string* value = Find(key);
		return value ? *value : fallback;
	}

	float PropertyBag::GetFloat(const std::string& key, float fallback) const
	{
		const std::string* value = Find(key);
		return value ? std::strtof(value->c_str(), nullptr) : fallback;
	}

	int PropertyBag::GetInt(const std::string& key, int fallback) const
	{
		const std::string* value = Find(key);
		return value ? std::atoi(value->c_str()) : fallback;
	}

	bool PropertyBag::GetBool(const std::string& key, bool fallback) const
	{
		const std::string* value = Find(key);
		return value ? (*value == "1" || *value == "true") : fallback;
	}

	Vector3 PropertyBag::GetVec3(const std::string& key, const Vector3& fallback) const
	{
		const std::string* value = Find(key);
		float v[3];
		return value && ParseList(*value, v, 3) ? Vector3{ v[0], v[1], v[2] } : fallback;
	}

	Color PropertyBag::GetColor(const std::string& key, const Color& fallback) const
	{
		const std::string* value = Find(key);
		float v[4];
		if (!value || !ParseList(*value, v, 4))
			return fallback;

		auto channel = [](float x) { return static_cast<unsigned char>(x < 0.0f ? 0.0f : (x > 255.0f ? 255.0f : x)); };
		return { channel(v[0]), channel(v[1]), channel(v[2]), channel(v[3]) };
	}

	const std::vector<std::pair<std::string, std::string>>& PropertyBag::Items() const { return m_items; }
}
