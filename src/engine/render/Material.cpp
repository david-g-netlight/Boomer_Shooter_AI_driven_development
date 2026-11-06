#include "Material.h"
#include <utility>

namespace eng
{
	Material::Material()
		: m_material(LoadMaterialDefault())
	{
		m_defaultShader		= m_material.shader;
		m_defaultTexture	= m_material.maps[MATERIAL_MAP_ALBEDO].texture;
	}

	Material::Material(const Shader* shader)
		: Material()
	{
		SetShader(shader);
	}

	Material::~Material()
	{
		Release();
	}

	Material::Material(Material&& other) noexcept
		: m_material(other.m_material)
		, m_defaultShader(other.m_defaultShader)
		, m_defaultTexture(other.m_defaultTexture)
		, m_parameters(std::move(other.m_parameters))
		, m_locations(std::move(other.m_locations))
	{
		other.m_material.maps = nullptr;
	}

	Material& Material::operator = (Material&& other) noexcept
	{
		if (this != &other)
		{
			Release();
			m_material			= other.m_material;
			m_defaultShader		= other.m_defaultShader;
			m_defaultTexture	= other.m_defaultTexture;
			m_parameters		= std::move(other.m_parameters);
			m_locations			= std::move(other.m_locations);
			other.m_material.maps = nullptr;
		}
		return *this;
	}

	void Material::Release()
	{
		// Only the maps array belongs to us; shader and textures are shared (see header)
		MemFree(m_material.maps);
		m_material.maps = nullptr;
	}

	void Material::SetShader(const Shader* shader)
	{
		m_material.shader = shader ? *shader : m_defaultShader;
		m_locations.clear();
	}

	const Shader& Material::GetShader() const
	{
		return m_material.shader;
	}

	void Material::SetTint(Color tint)
	{
		m_material.maps[MATERIAL_MAP_ALBEDO].color = tint;
	}

	Color Material::GetTint() const
	{
		return m_material.maps[MATERIAL_MAP_ALBEDO].color;
	}

	void Material::SetTexture(const Texture2D* texture)
	{
		m_material.maps[MATERIAL_MAP_ALBEDO].texture = texture ? *texture : m_defaultTexture;
	}

	void Material::SetParameter(const std::string& name, float value)
	{
		m_parameters[name] = value;
	}

	float Material::GetParameter(const std::string& name, float fallback) const
	{
		const auto it = m_parameters.find(name);
		return it != m_parameters.end() ? it->second : fallback;
	}

	bool Material::HasParameter(const std::string& name) const
	{
		return m_parameters.count(name) > 0;
	}

	const std::unordered_map<std::string, float>& Material::GetParameters() const
	{
		return m_parameters;
	}

	void Material::Bind() const
	{
		for (const auto& [name, value] : m_parameters)
		{
			auto it = m_locations.find(name);
			if (it == m_locations.end())
				it = m_locations.emplace(name, GetShaderLocation(m_material.shader, name.c_str())).first;

			if (it->second >= 0)
				SetShaderValue(m_material.shader, it->second, &value, SHADER_UNIFORM_FLOAT);
		}
	}

	const ::Material& Material::Get() const
	{
		return m_material;
	}
}
