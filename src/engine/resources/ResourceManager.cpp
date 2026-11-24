#include "ResourceManager.h"
#include <algorithm>
#include <vector>
#include <rlgl.h>
#include "core/Log.h"

namespace
{
	constexpr int MATERIAL_MAP_COUNT = 12;		// raylib's MAX_MATERIAL_MAPS (config.h, not in the public headers)

	/// UnloadModel() frees the materials' arrays but not the textures loaded from the MTL (raylib leaves that to
	/// the user, who may share them). Unloads each of them once (skipping raylib's default texture), then the model.
	void UnloadModelAndTextures(Model& model)
	{
		std::vector<Texture2D> textures;
		for (int i = 0; i < model.materialCount && model.materials; ++i)
		{
			if (!model.materials[i].maps)
				continue;
			for (int map = 0; map < MATERIAL_MAP_COUNT; ++map)
			{
				const Texture2D& texture = model.materials[i].maps[map].texture;
				const bool isKnown = std::any_of(textures.begin(), textures.end(), [&](const Texture2D& other) { return other.id == texture.id; });
				if (texture.id > 0 && texture.id != rlGetTextureIdDefault() && !isKnown)
					textures.push_back(texture);
			}
		}

		UnloadModel(model);
		for (const Texture2D& texture : textures)
			UnloadTexture(texture);
	}

	/// Swaps vertices a and b of an attribute array with `stride` floats per vertex.
	void SwapVertices(float* data, int stride, int a, int b)
	{
		if (!data)
			return;

		for (int k = 0; k < stride; ++k)
			std::swap(data[a * stride + k], data[b * stride + k]);
	}

	/// The cube seen from inside: every triangle's winding is reversed and every normal negated.
	/// GenMeshCube has already uploaded the outward faces, so the GPU buffers are overwritten with the flipped data.
	::Mesh MakeInvertedCube()
	{
		::Mesh mesh = GenMeshCube(1.0f, 1.0f, 1.0f);

		for (int t = 0; t < mesh.triangleCount; ++t)
		{
			if (mesh.indices)
			{
				std::swap(mesh.indices[t * 3 + 1], mesh.indices[t * 3 + 2]);
			}
			else
			{
				SwapVertices(mesh.vertices, 3, t * 3 + 1, t * 3 + 2);
				SwapVertices(mesh.texcoords, 2, t * 3 + 1, t * 3 + 2);
				SwapVertices(mesh.normals, 3, t * 3 + 1, t * 3 + 2);
			}
		}

		for (int i = 0; i < mesh.vertexCount * 3; ++i)
			mesh.normals[i] = -mesh.normals[i];

		rlUpdateVertexBuffer(mesh.vboId[0], mesh.vertices, mesh.vertexCount * 3 * sizeof(float), 0);
		rlUpdateVertexBuffer(mesh.vboId[1], mesh.texcoords, mesh.vertexCount * 2 * sizeof(float), 0);
		rlUpdateVertexBuffer(mesh.vboId[2], mesh.normals, mesh.vertexCount * 3 * sizeof(float), 0);
		if (mesh.indices)
			rlUpdateVertexBufferElements(mesh.vboId[6], mesh.indices, mesh.triangleCount * 3 * sizeof(unsigned short), 0);

		return mesh;
	}
}

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
			UnloadModelAndTextures(model);
			return nullptr;
		}

		// The lit shader of the game on every material (it keeps the material's texture and color), and the point
		// filter on its textures: raylib's loader leaves them bilinear
		for (int i = 0; i < model.materialCount; ++i)
		{
			Material& material = model.materials[i];
			if (m_defaultLitShader)
				material.shader = *m_defaultLitShader;
			for (int map = 0; map < MATERIAL_MAP_COUNT; ++map)
				if (material.maps[map].texture.id > 0 && material.maps[map].texture.id != rlGetTextureIdDefault())
					SetTextureFilter(material.maps[map].texture, TEXTURE_FILTER_POINT);
		}

		const BoundingBox bounds = GetModelBoundingBox(model);
		Log::Info("ResourceManager: model '%s': %d meshes, %d materials, bounds (%.4f %.4f %.4f) to (%.4f %.4f %.4f)",
			path.c_str(), model.meshCount, model.materialCount,
			bounds.min.x, bounds.min.y, bounds.min.z, bounds.max.x, bounds.max.y, bounds.max.z);

		return &m_models.emplace(path, model).first->second;
	}

	Texture2D* ResourceManager::LoadTexture(const std::string& path, int filter)
	{
		if (auto it = m_textures.find(path); it != m_textures.end())
			return &it->second;

		Texture2D texture = ::LoadTexture(path.c_str());
		if (!IsTextureValid(texture))
		{
			Log::Warning("ResourceManager: failed to load texture '%s'", path.c_str());
			return nullptr;
		}

		if (filter == TEXTURE_FILTER_TRILINEAR)
			GenTextureMipmaps(&texture);
		SetTextureFilter(texture, filter);
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

	Sound* ResourceManager::LoadSound(const std::string& path)
	{
		if (auto it = m_sounds.find(path); it != m_sounds.end())
			return &it->second;

		Sound sound = ::LoadSound(path.c_str());
		if (!IsSoundValid(sound))
		{
			Log::Warning("ResourceManager: failed to load sound '%s'", path.c_str());
			return nullptr;
		}

		return &m_sounds.emplace(path, sound).first->second;
	}

	Music* ResourceManager::LoadMusic(const std::string& path)
	{
		if (auto it = m_music.find(path); it != m_music.end())
			return &it->second;

		Music music = LoadMusicStream(path.c_str());
		if (!IsMusicValid(music))
		{
			Log::Warning("ResourceManager: failed to load music '%s'", path.c_str());
			return nullptr;
		}

		return &m_music.emplace(path, music).first->second;
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
				case Primitive::CubeInverted:	m_primitives[index] = MakeInvertedCube();			break;
				case Primitive::Plane:		m_primitives[index] = GenMeshPlane(1.0f, 1.0f, 1, 1);	break;
				case Primitive::Sphere:		m_primitives[index] = GenMeshSphere(0.5f, 16, 24);		break;
				case Primitive::Cylinder:	m_primitives[index] = GenMeshCylinder(0.5f, 1.0f, 24);	break;
				case Primitive::Pyramid:	m_primitives[index] = GenMeshCone(0.5f, 1.0f, 4);		break;
				case Primitive::Count:		return nullptr;
			}
			m_isPrimitiveLoaded[index] = true;
		}

		return &m_primitives[index];
	}

	void ResourceManager::SetDefaultLitShader(const Shader* shader)	{ m_defaultLitShader = shader; }
	const Shader* ResourceManager::GetDefaultLitShader() const		{ return m_defaultLitShader; }

	void ResourceManager::UnloadAll()
	{
		const size_t count = GetLoadedCount();
		m_defaultLitShader = nullptr;

		for (auto& [path, model]   : m_models)		UnloadModelAndTextures(model);
		for (auto& [path, texture] : m_textures)	UnloadTexture(texture);
		for (auto& [key, shader]   : m_shaders)		UnloadShader(shader);
		for (auto& [key, font]     : m_fonts)		UnloadFont(font);
		for (auto& [path, sound]   : m_sounds)		UnloadSound(sound);
		for (auto& [path, music]   : m_music)		UnloadMusicStream(music);
		m_models.clear();
		m_textures.clear();
		m_shaders.clear();
		m_fonts.clear();
		m_sounds.clear();
		m_music.clear();

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

		return m_models.size() + m_textures.size() + m_shaders.size() + m_fonts.size() + m_sounds.size() + m_music.size() + primitives;
	}
}
