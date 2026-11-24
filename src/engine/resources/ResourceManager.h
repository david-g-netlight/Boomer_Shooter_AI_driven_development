#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <raylib.h>

namespace eng
{
	/// <summary>
	/// Unit primitives generated once and shared: cube 1x1x1, cube seen from inside (inverted winding and normals),
	/// plane 1x1 (XZ), sphere r=0.5, cylinder r=0.5 h=1.
	/// All are centered on the origin, except the cylinder and plane which sit on y=0 (raylib's convention).
	/// </summary>
	enum class Primitive : uint8_t
	{
		Cube,
		CubeInverted,
		Plane,
		Sphere,
		Cylinder,
		Pyramid,
		Count,
	};

	/// <summary>
	/// Owns every GPU resource loaded from disk. Loads each path once and returns the cached resource after that.
	/// Pointers stay valid until UnloadAll(). A failed load returns nullptr, logs a warning and is not cached.
	/// UnloadAll() is called by Engine::Shutdown() before the window (GL context) is closed.
	/// Sounds need the audio device (AudioManager::Init) and must be unloaded before it closes.
	/// </summary>
	class ResourceManager
	{
	public:
		ResourceManager() = default;
		~ResourceManager();

		ResourceManager(const ResourceManager&) = delete;
		ResourceManager& operator = (const ResourceManager&) = delete;

		//////////////////////////////////////////
		/// Loading (paths are relative to the executable directory, e.g. "assets/models/crate.glb")
		//////////////////////////////////////////
		Model*		LoadModel	(const std::string& path);
		/// Bounds computed once at load. nullptr when the model is not loaded (this does not load it).
		const BoundingBox*	GetModelBounds	(const std::string& path) const;
		Texture2D*	LoadTexture	(const std::string& path, TextureFilter filter = TEXTURE_FILTER_POINT); // mipmaps only for trilinear
		Shader*		LoadShader	(const std::string& vertexPath, const std::string& fragmentPath); // "" = raylib default stage
		Font*		LoadFont	(const std::string& path, int baseSize = 32);
		Sound*		LoadSound	(const std::string& path);	// whole file in memory, for short effects
		Music*		LoadMusic	(const std::string& path);	// streamed from disk; play one Music in one place at a time

		//////////////////////////////////////////
		/// Primitives
		//////////////////////////////////////////
		const ::Mesh*	GetPrimitive(Primitive primitive);

		/// <summary>
		/// The lit shader of the game (assets/shaders/lit.*). Level files mark materials with `lit=1`,
		/// and the editor gives it to new cubes. nullptr = raylib's default shader.
		/// </summary>
		void			SetDefaultLitShader(const Shader* shader);
		const Shader*	GetDefaultLitShader() const;

		void		UnloadAll();

		size_t		GetLoadedCount() const;

	private:
		static void UnloadModelAndTextures(Model& model);

		std::unordered_map<std::string, Model>		m_models;
		std::unordered_map<std::string, BoundingBox>	m_modelBounds;	// by path, set with m_models
		std::unordered_map<std::string, Texture2D>	m_textures;
		std::unordered_map<std::string, Shader>		m_shaders;
		std::unordered_map<std::string, Font>		m_fonts;
		std::unordered_map<std::string, Sound>		m_sounds;
		std::unordered_map<std::string, Music>		m_music;

		const Shader*	m_defaultLitShader	= nullptr;
		std::array<::Mesh, static_cast<size_t>(Primitive::Count)>	m_primitives			= {};
		std::array<bool,   static_cast<size_t>(Primitive::Count)>	m_isPrimitiveLoaded		= {};
	};
}
