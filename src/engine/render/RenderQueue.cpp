#include "RenderQueue.h"
#include "Material.h"
#include "Camera.h"
#include <unordered_set>
#include <rlgl.h>
#include "core/Log.h"
#include <raymath.h>

namespace eng
{
	RenderQueue::RenderQueue()
	{
		m_commands.reserve(INITIAL_CAPACITY);
	}

	void RenderQueue::Submit(const RenderCommand& command)
	{
		if (!command.mesh || (!command.material && !command.rawMaterial))
		{
			Log::Warning("RenderQueue: command without mesh or material ignored");
			return;
		}
		m_commands.push_back(command);
	}

	void RenderQueue::Submit(const ::Mesh& mesh, const Material& material, const Matrix& transform)
	{
		RenderCommand command;
		command.mesh		= &mesh;
		command.material	= &material;
		command.transform	= transform;
		m_commands.push_back(command);
	}

	void RenderQueue::SubmitModel(const Model& model, const Material* overrideMaterial, const Matrix& transform)
	{
		const Matrix world = MatrixMultiply(model.transform, transform);

		for (int i = 0; i < model.meshCount; ++i)
		{
			RenderCommand command;
			command.mesh		= &model.meshes[i];
			command.material	= overrideMaterial;
			command.rawMaterial	= overrideMaterial ? nullptr : &model.materials[model.meshMaterial[i]];
			command.transform	= world;
			m_commands.push_back(command);
		}
	}

	void RenderQueue::SetLighting(const SceneLighting& lighting)
	{
		m_lighting = lighting;
	}

	void RenderQueue::UploadSceneUniforms(const Shader& shader, const Vector3& viewPosition) const
	{
		auto it = m_sceneLocations.find(shader.id);
		if (it == m_sceneLocations.end())
		{
			SceneUniformLocations locations;
			locations.sunDirection	= GetShaderLocation(shader, "sunDirection");
			locations.sunColor		= GetShaderLocation(shader, "sunColor");
			locations.ambientColor	= GetShaderLocation(shader, "ambientColor");
			locations.viewPosition	= GetShaderLocation(shader, "viewPosition");
			it = m_sceneLocations.emplace(shader.id, locations).first;
		}

		const SceneUniformLocations& locations = it->second;
		if (locations.sunDirection >= 0)	SetShaderValue(shader, locations.sunDirection, &m_lighting.sunDirection, SHADER_UNIFORM_VEC3);
		if (locations.sunColor >= 0)		SetShaderValue(shader, locations.sunColor, &m_lighting.sunColor, SHADER_UNIFORM_VEC3);
		if (locations.ambientColor >= 0)	SetShaderValue(shader, locations.ambientColor, &m_lighting.ambientColor, SHADER_UNIFORM_VEC3);
		if (locations.viewPosition >= 0)	SetShaderValue(shader, locations.viewPosition, &viewPosition, SHADER_UNIFORM_VEC3);
	}

	void RenderQueue::Draw(const Camera& camera) const
	{
		// Scene uniforms once per distinct custom shader (raylib's default shader has none)
		std::unordered_set<unsigned int> uploaded;
		for (const RenderCommand& command : m_commands)
		{
			const Shader& shader = command.material ? command.material->GetShader() : command.rawMaterial->shader;
			if (shader.id != rlGetShaderIdDefault() && uploaded.insert(shader.id).second)
				UploadSceneUniforms(shader, camera.GetPosition());
		}

		for (const RenderCommand& command : m_commands)
		{
			if (command.material)
			{
				BindMaterial(*command.material);
				DrawMesh(*command.mesh, command.material->Get(), command.transform);
			}
			else
			{
				DrawMesh(*command.mesh, *command.rawMaterial, command.transform);
			}
		}
	}

	void RenderQueue::BindMaterial(const Material& material) const
	{
		const Shader&	shader		= material.GetShader();
		auto&			known		= m_parameterLocations[shader.id];
		const float		zero		= 0.0f;

		for (const auto& [name, location] : known)
			if (location >= 0 && !material.HasParameter(name))
				SetShaderValue(shader, location, &zero, SHADER_UNIFORM_FLOAT);

		material.Bind();

		for (const auto& [name, value] : material.GetParameters())
			if (known.find(name) == known.end())
				known.emplace(name, GetShaderLocation(shader, name.c_str()));
	}

	void RenderQueue::Clear()
	{
		m_commands.clear();
	}

	size_t RenderQueue::GetCommandCount() const
	{
		return m_commands.size();
	}
}
