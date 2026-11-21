#include "MeshRenderer.h"
#include "render/RenderQueue.h"
#include "level/GameActor.h"
#include <algorithm>
#include <raymath.h>
#include "core/Engine.h"
#include "core/Log.h"
#include "level/PropertyBag.h"

namespace eng
{
	MeshRenderer::MeshRenderer(const ::Mesh* mesh, const Material* material)
		: m_mesh(mesh)
		, m_material(material)
	{
		RefreshBounds();
	}

	MeshRenderer::MeshRenderer(const Model* model, const Material* overrideMaterial)
		: m_model(model)
		, m_material(overrideMaterial)
	{
		RefreshBounds();
	}

	void MeshRenderer::SetMesh(const ::Mesh* mesh)
	{
		m_mesh	= mesh;
		m_model	= nullptr;
		RefreshBounds();
	}

	void MeshRenderer::SetModel(const Model* model)
	{
		m_model	= model;
		m_mesh	= nullptr;
		RefreshBounds();
	}

	void MeshRenderer::SetMaterial(const Material* material)
	{
		m_material = material;
		m_ownMaterial.reset();
	}

	Material& MeshRenderer::CreateOwnMaterial(const Shader* shader)
	{
		m_ownMaterial	= std::make_unique<Material>(shader);
		m_material		= m_ownMaterial.get();
		return *m_ownMaterial;
	}

	const Material* MeshRenderer::GetMaterial() const	{ return m_material; }
	Material* MeshRenderer::GetOwnMaterial()			{ return m_ownMaterial.get(); }
	void MeshRenderer::SetVisible(bool visible)			{ m_isVisible = visible; }
	bool MeshRenderer::IsVisible() const				{ return m_isVisible; }
	BoundingBox MeshRenderer::GetLocalBounds() const	{ return m_localBounds; }

	BoundingBox MeshRenderer::GetWorldBounds() const
	{
		const GameActor* owner = GetOwner();
		if (!owner)
			return m_localBounds;

		// Transform the 8 corners and take their axis-aligned bounds
		const Matrix	world	= owner->GetWorldMatrix();
		const Vector3&	lo		= m_localBounds.min;
		const Vector3&	hi		= m_localBounds.max;
		BoundingBox		result	= { { 1e30f, 1e30f, 1e30f }, { -1e30f, -1e30f, -1e30f } };

		for (int corner = 0; corner < 8; ++corner)
		{
			const Vector3 local = { (corner & 1) ? hi.x : lo.x, (corner & 2) ? hi.y : lo.y, (corner & 4) ? hi.z : lo.z };
			const Vector3 point = Vector3Transform(local, world);
			result.min = Vector3Min(result.min, point);
			result.max = Vector3Max(result.max, point);
		}
		return result;
	}

	void MeshRenderer::SubmitRenderables(RenderQueue& queue) const
	{
		const GameActor* owner = GetOwner();
		if (!m_isVisible || !owner)
			return;

		const Matrix world = owner->GetWorldMatrix();
		if (m_model)
			queue.SubmitModel(*m_model, m_material, world);
		else if (m_mesh)
			queue.Submit(*m_mesh, *ResolveMaterial(), world);
	}

	const Material* MeshRenderer::ResolveMaterial() const
	{
		if (m_material)
			return m_material;

		// Created lazily (needs the window), only for meshes that were given no material
		if (!m_fallbackMaterial)
			m_fallbackMaterial = std::make_unique<Material>();
		return m_fallbackMaterial.get();
	}

	void MeshRenderer::RefreshBounds()
	{
		if (m_model)
			m_localBounds = GetModelBoundingBox(*m_model);
		else if (m_mesh)
			m_localBounds = GetMeshBoundingBox(*m_mesh);
		else
			m_localBounds = { { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
	}

	namespace
	{
		constexpr const char* PRIMITIVE_NAMES[] = { "cube", "cube_inverted", "plane", "sphere", "cylinder", "pyramid" };
	}

	void MeshRenderer::Serialize(PropertyBag& out) const
	{
		ResourceManager& resources = Engine::GetInstance().GetResources();
		bool isPrimitive = false;
		for (int i = 0; i < static_cast<int>(Primitive::Count) && m_mesh; ++i)
			if (m_mesh == resources.GetPrimitive(static_cast<Primitive>(i)))
			{
				out.SetString("mesh", PRIMITIVE_NAMES[i]);
				isPrimitive = true;
			}

		if (!isPrimitive)
			Log::Warning("MeshRenderer: only primitive meshes are saved in level files ('%s')",
				GetOwner() ? GetOwner()->GetName().c_str() : "?");

		out.SetBool("visible", m_isVisible);	// always written: the console can only set keys that exist

		if (m_ownMaterial)
		{
			out.SetColor("tint", m_ownMaterial->GetTint());
			const Shader* lit = resources.GetDefaultLitShader();
			if (lit && m_ownMaterial->GetShader().id == lit->id)
				out.SetBool("lit", true);
			if (m_ownMaterial->HasParameter("specularStrength"))
				out.SetFloat("specular", m_ownMaterial->GetParameter("specularStrength"));
		}
	}

	void MeshRenderer::Deserialize(const PropertyBag& in)
	{
		ResourceManager& resources = Engine::GetInstance().GetResources();
		const std::string mesh = in.GetString("mesh", "cube");
		for (int i = 0; i < static_cast<int>(Primitive::Count); ++i)
			if (mesh == PRIMITIVE_NAMES[i])
				SetMesh(resources.GetPrimitive(static_cast<Primitive>(i)));

		SetVisible(in.GetBool("visible", true));

		if (in.Has("tint") || in.GetBool("lit"))
		{
			Material& material = CreateOwnMaterial(in.GetBool("lit") ? resources.GetDefaultLitShader() : nullptr);
			material.SetTint(in.GetColor("tint", WHITE));
			if (in.Has("specular"))
				material.SetParameter("specularStrength", in.GetFloat("specular"));
		}
	}
}
