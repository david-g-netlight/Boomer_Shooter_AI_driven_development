#include "RenderQueue.h"
#include "Material.h"
#include "Camera.h"
#include <algorithm>
#include <cmath>
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

	void RenderQueue::SubmitLine(const LineCommand& line)
	{
		m_lines.push_back(line);
	}

	namespace
	{
		constexpr int TUBE_SIDES = 8;

		// Open tube from a to b: sides only (caps would show as stacked discs through the transparency)
		void DrawTube(const LineCommand& line)
		{
			const Vector3 axis = Vector3Subtract(line.end, line.start);
			if (Vector3LengthSqr(axis) < 1e-10f)
				return;

			const Vector3 direction	= Vector3Normalize(axis);
			const Vector3 helper	= std::fabs(direction.y) < 0.99f ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
			const Vector3 u			= Vector3Normalize(Vector3CrossProduct(helper, direction));
			const Vector3 v			= Vector3CrossProduct(direction, u);

			auto ring = [&](const Vector3& center, float radius, int i)
			{
				const float angle = 2.0f * PI * static_cast<float>(i) / TUBE_SIDES;
				return Vector3Add(center, Vector3Add(Vector3Scale(u, std::cos(angle) * radius), Vector3Scale(v, std::sin(angle) * radius)));
			};

			auto vertex = [](const Vector3& p, Color c)
			{
				rlColor4ub(c.r, c.g, c.b, c.a);
				rlVertex3f(p.x, p.y, p.z);
			};

			// Two triangles per side; culling is off, so one winding is visible from inside and outside
			rlBegin(RL_TRIANGLES);
			for (int i = 0; i < TUBE_SIDES; ++i)
			{
				const Vector3 a0 = ring(line.start, line.startRadius, i);
				const Vector3 a1 = ring(line.start, line.startRadius, i + 1);
				const Vector3 b0 = ring(line.end, line.endRadius, i);
				const Vector3 b1 = ring(line.end, line.endRadius, i + 1);

				vertex(a0, line.startColor); vertex(b0, line.endColor); vertex(a1, line.startColor);
				vertex(a1, line.startColor); vertex(b0, line.endColor); vertex(b1, line.endColor);
			}
			rlEnd();
		}
	}

	void RenderQueue::DrawLines() const
	{
		if (m_lines.empty())
			return;

		rlDrawRenderBatchActive();
		rlDisableDepthMask();		// transparent: test depth, don't write it
		rlDisableBackfaceCulling();	// the tube is seen from inside and outside
		BeginBlendMode(BLEND_ALPHA);
		for (const LineCommand& line : m_lines)
		{
			if (line.startRadius <= 0.0f && line.endRadius <= 0.0f)
				DrawLine3D(line.start, line.end, ColorLerp(line.startColor, line.endColor, 0.5f));
			else
				DrawTube(line);
		}
		EndBlendMode();
		rlDrawRenderBatchActive();
		rlEnableBackfaceCulling();
		rlEnableDepthMask();
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
				ResetParameters(command.rawMaterial->shader, nullptr);
				DrawMesh(*command.mesh, *command.rawMaterial, command.transform);
			}
		}

		DrawLines();
	}

	void RenderQueue::BindMaterial(const Material& material) const
	{
		const Shader&	shader		= material.GetShader();
		ResetParameters(shader, &material);

		material.Bind();

		auto& known = m_parameterLocations[shader.id];
		for (const auto& [name, value] : material.GetParameters())
			if (known.find(name) == known.end())
				known.emplace(name, GetShaderLocation(shader, name.c_str()));
	}

	void RenderQueue::ResetParameters(const Shader& shader, const Material* material) const
	{
		const float zero = 0.0f;
		for (const auto& [name, location] : m_parameterLocations[shader.id])
			if (location >= 0 && !(material && material->HasParameter(name)))
				SetShaderValue(shader, location, &zero, SHADER_UNIFORM_FLOAT);
	}

	void RenderQueue::Clear()
	{
		m_commands.clear();
		m_lines.clear();
	}

	size_t RenderQueue::GetLineCount() const
	{
		return m_lines.size();
	}

	size_t RenderQueue::GetCommandCount() const
	{
		return m_commands.size();
	}
}
