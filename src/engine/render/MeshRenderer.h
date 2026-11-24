#pragma once
#include <memory>
#include <string>
#include <raylib.h>
#include "level/Component.h"
#include "render/Material.h"

namespace eng
{
	/// <summary>
	/// Draws a mesh (or every mesh of a loaded model) at its actor's world transform.
	/// Mesh / model / shared material are NOT owned (ResourceManager or the game).
	/// CreateOwnMaterial() gives the actor its own material (e.g. a per-actor tint), owned by this component.
	/// Without any material: a mesh uses the default material; a model uses its own materials.
	/// Level files: `texture=<path>` ("none" = no texture; loaded point filtered through the ResourceManager) and
	/// `uv=<meters>` (the material's worldUV: meters per texture repeat, 0 = the mesh's own UVs) are saved with an own material.
	/// </summary>
	class MeshRenderer : public Component
	{
	public:
		MeshRenderer() = default;
		MeshRenderer(const ::Mesh* mesh, const Material* material = nullptr);
		explicit MeshRenderer(const Model* model, const Material* overrideMaterial = nullptr);

		void			SetMesh(const ::Mesh* mesh);
		void			SetModel(const Model* model);
		void			SetMaterial(const Material* material);	// shared; replaces an own material
		Material&		CreateOwnMaterial(const Shader* shader = nullptr);
		const Material*	GetMaterial() const;
		Material*		GetOwnMaterial();						// nullptr unless CreateOwnMaterial() was called

		void			SetVisible(bool visible);
		bool			IsVisible() const;

		/// <summary>
		/// Mesh / model bounds in local space, and transformed to world space (axis aligned).
		/// </summary>
		BoundingBox		GetLocalBounds() const;
		BoundingBox		GetWorldBounds() const;

		void			SubmitRenderables(RenderQueue& queue) const override;
		void			Serialize(PropertyBag& out) const override;
		void			Deserialize(const PropertyBag& in) override;
		const char*		TypeName() const override { return "MeshRenderer"; }

	private:
		void			RefreshBounds();
		const Material*	ResolveMaterial() const;

		const ::Mesh*				m_mesh			= nullptr;
		const Model*				m_model			= nullptr;
		const Material*				m_material		= nullptr;
		std::unique_ptr<Material>	m_ownMaterial;
		mutable std::unique_ptr<Material>	m_fallbackMaterial;	// default material for a mesh without one, created lazily
		BoundingBox					m_localBounds	= { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
		bool						m_isVisible		= true;
		std::string					m_texturePath;		// from a level file, "" = none (kept to save it again)
	};
}
