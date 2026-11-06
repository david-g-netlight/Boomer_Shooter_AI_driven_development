#pragma once
#include <cstddef>
#include <vector>
#include <raylib.h>

namespace eng
{
	class Material;

	/// <summary>
	/// One draw: mesh + material + world transform. Pointers are NON-owning and must stay valid until
	/// the queue is drawn (end of the frame).
	/// Use `material` for engine materials, or `rawMaterial` for a loaded model's own raylib material.
	/// </summary>
	struct RenderCommand
	{
		const ::Mesh*		mesh			= nullptr;
		const Material*		material		= nullptr;
		const ::Material*	rawMaterial		= nullptr;	// used when material == nullptr
		Matrix				transform		= { 1.0f, 0.0f, 0.0f, 0.0f,
												0.0f, 1.0f, 0.0f, 0.0f,
												0.0f, 0.0f, 1.0f, 0.0f,
												0.0f, 0.0f, 0.0f, 1.0f };
	};

	/// <summary>
	/// Collects RenderCommands during Application::SubmitRenderables() and draws them in the Engine's 3D pass.
	/// The Engine clears it at the end of every frame.
	/// </summary>
	class RenderQueue
	{
	public:
		RenderQueue();

		void	Submit(const RenderCommand& command);
		void	Submit(const ::Mesh& mesh, const Material& material, const Matrix& transform);

		/// <summary>
		/// One command per mesh of the model. overrideMaterial == nullptr uses the model's own materials.
		/// The model's root transform is applied before `transform`.
		/// </summary>
		void	SubmitModel(const Model& model, const Material* overrideMaterial, const Matrix& transform);

		/// <summary>
		/// Draws every command. Call between BeginMode3D / EndMode3D. Can be called more than once per frame
		/// (e.g. into a RenderTarget with another camera).
		/// </summary>
		void	Draw() const;
		void	Clear();

		size_t	GetCommandCount() const;

	private:
		static constexpr size_t INITIAL_CAPACITY = 256;

		std::vector<RenderCommand> m_commands;
	};
}
