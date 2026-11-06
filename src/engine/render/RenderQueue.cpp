#include "RenderQueue.h"
#include "Material.h"
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

	void RenderQueue::Draw() const
	{
		for (const RenderCommand& command : m_commands)
		{
			if (command.material)
			{
				command.material->Bind();
				DrawMesh(*command.mesh, command.material->Get(), command.transform);
			}
			else
			{
				DrawMesh(*command.mesh, *command.rawMaterial, command.transform);
			}
		}
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
