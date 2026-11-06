#pragma once
#include <string>
#include <unordered_map>
#include <raylib.h>

namespace eng
{
	/// <summary>
	/// Surface description used by RenderCommands: shader + tint + albedo texture + named float parameters.
	/// Shaders and textures are NOT owned (they live in the ResourceManager); the Material only owns
	/// raylib's map array. That's why it never calls UnloadMaterial(), which would also unload the shared
	/// shader and textures.
	/// Must be created after the window exists (needs raylib's default shader and texture).
	/// </summary>
	class Material
	{
	public:
		Material();
		explicit Material(const Shader* shader);
		~Material();

		// Owns the maps array: no copies, moves transfer it
		Material(const Material&) = delete;
		Material& operator = (const Material&) = delete;
		Material(Material&& other) noexcept;
		Material& operator = (Material&& other) noexcept;

		/// <summary>
		/// nullptr = raylib's default shader (unlit, vertex color * tint * texture).
		/// </summary>
		void			SetShader(const Shader* shader);
		const Shader&	GetShader() const;

		void			SetTint(Color tint);
		Color			GetTint() const;

		/// <summary>
		/// Albedo (diffuse) texture. nullptr = raylib's 1x1 white texture.
		/// </summary>
		void			SetTexture(const Texture2D* texture);

		/// <summary>
		/// Float uniform uploaded on Bind(). Uniforms the shader doesn't have are ignored.
		/// </summary>
		void			SetParameter(const std::string& name, float value);
		float			GetParameter(const std::string& name, float fallback = 0.0f) const;
		bool			HasParameter(const std::string& name) const;
		const std::unordered_map<std::string, float>&	GetParameters() const;

		/// <summary>
		/// Uploads the parameters to the shader. Called by the RenderQueue right before drawing.
		/// Uniforms keep their value per shader program, so the RenderQueue first resets parameters set by
		/// other materials on the same shader to 0: a parameter a material doesn't set reads as 0.
		/// </summary>
		void			Bind() const;

		const ::Material&	Get() const;

	private:
		void			Release();

		::Material								m_material			= {};
		Shader									m_defaultShader		= {};
		Texture2D								m_defaultTexture	= {};
		std::unordered_map<std::string, float>	m_parameters;
		mutable std::unordered_map<std::string, int>	m_locations;	// uniform locations, per shader
	};
}
