#pragma once
#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>
#include <raylib.h>

namespace eng
{
	class Camera;
	class Material;

	/// <summary>
	/// A transparent thick line (beam), drawn after the meshes: an open tapered tube (no end caps) with the
	/// color interpolated per vertex from start to end. Radius 0 at both ends = 1 pixel wide line.
	/// </summary>
	struct LineCommand
	{
		Vector3	start			= { 0.0f, 0.0f, 0.0f };
		Vector3	end				= { 0.0f, 0.0f, 0.0f };
		Color	startColor		= { 255, 255, 255, 255 };
		Color	endColor		= { 255, 255, 255, 255 };
		float	startRadius		= 0.02f;
		float	endRadius		= 0.02f;
	};

	/// <summary>
	/// Scene-wide uniforms for lit shaders (see assets/shaders/lit.fs).
	/// </summary>
	struct SceneLighting
	{
		Vector3	sunDirection	= { -0.4f, -1.0f, -0.3f };
		Vector3	sunColor		= { 1.0f, 1.0f, 1.0f };
		Vector3	ambientColor	= { 0.3f, 0.3f, 0.3f };
	};

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
		/// Transparent line, drawn after every mesh (alpha blended, no depth write).
		/// </summary>
		void	SubmitLine(const LineCommand& line);

		void	SetLighting(const SceneLighting& lighting);

		/// <summary>
		/// Draws every command with `camera` (inside its Begin3D / End3D). Can be called more than once per
		/// frame (e.g. into a RenderTarget with another camera). Before drawing, each shader that declares
		/// sunDirection / sunColor / ambientColor / viewPosition gets those uniforms once.
		/// </summary>
		void	Draw(const Camera& camera) const;
		void	Clear();

		size_t	GetCommandCount() const;
		size_t	GetLineCount() const;

	private:
		static constexpr size_t INITIAL_CAPACITY = 256;

		struct SceneUniformLocations
		{
			int sunDirection	= -1;
			int sunColor		= -1;
			int ambientColor	= -1;
			int viewPosition	= -1;
		};

		void	UploadSceneUniforms(const Shader& shader, const Vector3& viewPosition) const;
		void	BindMaterial(const Material& material) const;
		/// Sets the parameters ever uploaded to `shader` that `material` (nullptr: a raw material) doesn't set, to 0.
		void	ResetParameters(const Shader& shader, const Material* material) const;
		void	DrawLines() const;

		std::vector<RenderCommand>	m_commands;
		std::vector<LineCommand>	m_lines;
		SceneLighting				m_lighting;
		mutable std::unordered_map<unsigned int, SceneUniformLocations>	m_sceneLocations;	// by shader id

		// Material parameters ever uploaded to each shader (name -> location), so the ones a material doesn't
		// set can be reset to 0 instead of leaking from the previous material on the same shader
		mutable std::unordered_map<unsigned int, std::unordered_map<std::string, int>>	m_parameterLocations;
	};
}
