#include "ResourceManager.h"
#include "core/Log.h"

namespace eng
{
	ResourceManager::~ResourceManager()
	{
		// GPU resources can't be released here: the static Engine is destroyed after CloseWindow()
		if (GetLoadedCount() > 0)
			Log::Warning("ResourceManager destroyed with %zu resources still loaded (UnloadAll() not called)", GetLoadedCount());
	}

	Model* ResourceManager::LoadModel(const std::string& path)
	{
		if (auto it = m_models.find(path); it != m_models.end())
			return &it->second;

		Model model = ::LoadModel(path.c_str());
		if (!IsModelValid(model))
		{
			Log::Warning("ResourceManager: failed to load model '%s'", path.c_str());
			UnloadModel(model);
			return nullptr;
		}

		return &m_models.emplace(path, model).first->second;
	}

	Texture2D* ResourceManager::LoadTexture(const std::string& path)
	{
		if (auto it = m_textures.find(path); it != m_textures.end())
			return &it->second;

		Texture2D texture = ::LoadTexture(path.c_str());
		if (!IsTextureValid(texture))
		{
			Log::Warning("ResourceManager: failed to load texture '%s'", path.c_str());
			return nullptr;
		}

		GenTextureMipmaps(&texture);
		SetTextureFilter(texture, TEXTURE_FILTER_TRILINEAR);
		return &m_textures.emplace(path, texture).first->second;
	}

	Shader* ResourceManager::LoadShader(const std::string& vertexPath, const std::string& fragmentPath)
	{
		const std::string key = vertexPath + "|" + fragmentPath;
		if (auto it = m_shaders.find(key); it != m_shaders.end())
			return &it->second;

		Shader shader = ::LoadShader(vertexPath.empty()   ? nullptr : vertexPath.c_str(),
									 fragmentPath.empty() ? nullptr : fragmentPath.c_str());
		if (!IsShaderValid(shader))
		{
			Log::Warning("ResourceManager: failed to load shader '%s' + '%s'", vertexPath.c_str(), fragmentPath.c_str());
			return nullptr;
		}

		return &m_shaders.emplace(key, shader).first->second;
	}

	Font* ResourceManager::LoadFont(const std::string& path, int baseSize)
	{
		const std::string key = path + "@" + std::to_string(baseSize);
		if (auto it = m_fonts.find(key); it != m_fonts.end())
			return &it->second;

		Font font = LoadFontEx(path.c_str(), baseSize, nullptr, 0);
		if (!IsFontValid(font))
		{
			Log::Warning("ResourceManager: failed to load font '%s'", path.c_str());
			return nullptr;
		}

		SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
		return &m_fonts.emplace(key, font).first->second;
	}

	const ::Mesh* ResourceManager::GetPrimitive(Primitive primitive)
	{
		const size_t index = static_cast<size_t>(primitive);
		if (index >= m_primitives.size())
			return nullptr;

		if (!m_isPrimitiveLoaded[index])
		{
			switch (primitive)
			{
				case Primitive::Cube:		m_primitives[index] = GenMeshCube(1.0f, 1.0f, 1.0f);	break;
				case Primitive::Plane:		m_primitives[index] = GenMeshPlane(1.0f, 1.0f, 1, 1);	break;
				case Primitive::Sphere:		m_primitives[index] = GenMeshSphere(0.5f, 16, 24);		break;
				case Primitive::Cylinder:	m_primitives[index] = GenMeshCylinder(0.5f, 1.0f, 24);	break;
				case Primitive::Count:		return nullptr;
			}
			m_isPrimitiveLoaded[index] = true;
		}

		return &m_primitives[index];
	}

	void ResourceManager::UnloadAll()
	{
		const size_t count = GetLoadedCount();

		for (auto& [path, model]   : m_models)		UnloadModel(model);
		for (auto& [path, texture] : m_textures)	UnloadTexture(texture);
		for (auto& [key, shader]   : m_shaders)		UnloadShader(shader);
		for (auto& [key, font]     : m_fonts)		UnloadFont(font);
		m_models.clear();
		m_textures.clear();
		m_shaders.clear();
		m_fonts.clear();

		for (size_t i = 0; i < m_primitives.size(); ++i)
		{
			if (m_isPrimitiveLoaded[i])
				UnloadMesh(m_primitives[i]);
			m_isPrimitiveLoaded[i] = false;
		}

		if (count > 0)
			Log::Info("ResourceManager: unloaded %zu resources", count);
	}

	size_t ResourceManager::GetLoadedCount() const
	{
		size_t primitives = 0;
		for (bool loaded : m_isPrimitiveLoaded)
			primitives += loaded ? 1 : 0;

		return m_models.size() + m_textures.size() + m_shaders.size() + m_fonts.size() + primitives;
	}
}
