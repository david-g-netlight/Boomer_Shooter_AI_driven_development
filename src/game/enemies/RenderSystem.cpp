#include "RenderSystem.h"
#include <raymath.h>
#include "EnemyComponents.h"

namespace enemies
{
	namespace
	{
		constexpr float	SPIKE_WIDTH			= 0.6f;		// x a body radius
		constexpr float	SPIKE_HEIGHT		= 0.9f;		// x a body radius
		constexpr float	FLASH_SQUASH_XZ		= 1.15f;
		constexpr float	FLASH_SQUASH_Y		= 0.85f;
	}

	void RenderSystem(eng::ecs::Registry& registry, const EnemyMaterials& materials, eng::RenderQueue& queue)
	{
		eng::ResourceManager& resources = eng::Engine::GetInstance().GetResources();
		const ::Mesh* sphere	= resources.GetPrimitive(eng::Primitive::Sphere);
		const ::Mesh* pyramid	= resources.GetPrimitive(eng::Primitive::Pyramid);
		if (!sphere || !pyramid)
			return;

		// The pyramid's tip is +Y; this turns it to -Z, the local forward
		const Matrix pointForward = MatrixRotateX(-90.0f * DEG2RAD);

		registry.Each<EnemyLook, Transform, EnemyKind, Life>([&](eng::ecs::Entity, EnemyLook& look, Transform& transform, EnemyKind& kind, Life& life)
		{
			const EnemyArchetype& archetype = GetArchetype(kind.type);
			const EnemyMaterials::Set& set = materials.byType[static_cast<size_t>(kind.type)];

			const float radius = archetype.bodyRadius * look.scale;
			const Matrix rotation = QuaternionToMatrix(look.renderRotation);
			const Vector3 forward = Vector3RotateByQuaternion({ 0.0f, 0.0f, -1.0f }, look.renderRotation);
			const Vector3 center = Vector3Add(transform.position, Vector3Scale(forward, look.lunge));

			const bool flashing = life.hitFlash > 0.0f;
			const eng::Material& bodyMaterial = flashing ? set.bodyFlash : set.body;

			if (set.model)
			{
				// The model: its fit (fitted to the diameter), then the look's scale and squash, rotation, translation
				const Vector3 squash = flashing ? Vector3{ FLASH_SQUASH_XZ, FLASH_SQUASH_Y, FLASH_SQUASH_XZ } : Vector3{ 1.0f, 1.0f, 1.0f };
				const Matrix scale = MatrixScale(look.scale * squash.x, look.scale * squash.y, look.scale * squash.z);
				const Matrix matrix = MatrixMultiply(MatrixMultiply(MatrixMultiply(set.fit, scale), rotation), MatrixTranslate(center.x, center.y, center.z));
				queue.SubmitModel(*set.model, &bodyMaterial, matrix);
				return;
			}

			// Body: scale, rotation, then translation (the GameActor order)
			const float diameter = 2.0f * radius;	// the sphere mesh has radius 0.5
			const Vector3 bodySize = flashing
				? Vector3{ diameter * FLASH_SQUASH_XZ, diameter * FLASH_SQUASH_Y, diameter * FLASH_SQUASH_XZ }
				: Vector3{ diameter, diameter, diameter };
			const Matrix body = MatrixMultiply(MatrixMultiply(MatrixScale(bodySize.x, bodySize.y, bodySize.z), rotation), MatrixTranslate(center.x, center.y, center.z));
			queue.Submit(*sphere, bodyMaterial, body);

			// Spike: its base sits on the body surface and its tip points along forward
			const Vector3 base = Vector3Add(center, Vector3Scale(forward, radius));
			const Matrix spike = MatrixMultiply(MatrixMultiply(MatrixMultiply(
				MatrixScale(SPIKE_WIDTH * radius, SPIKE_HEIGHT * radius, SPIKE_WIDTH * radius), pointForward), rotation), MatrixTranslate(base.x, base.y, base.z));
			queue.Submit(*pyramid, set.spike, spike);
		});
	}
}
