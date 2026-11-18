#include "PlayerActor.h"
#include "player/FirstPersonController.h"
#include "player/FirstPersonLook.h"
#include "player/Gun.h"

eng::GameActor* PlayerActor::Create(eng::Level& level, const PlayerContext& context, const Vector3& feetPosition, float yawDegrees,
									bool withBodyMesh)
{
	constexpr float HALF_HEIGHT = CAPSULE_RADIUS + 0.5f * CAPSULE_HEIGHT;

	eng::GameActor* player = level.CreateGameActor("Player");
	player->SetPosition({ feetPosition.x, feetPosition.y + HALF_HEIGHT, feetPosition.z });

	eng::Collider* collider = player->AddComponent<eng::Collider>();
	collider->SetCapsule(CAPSULE_RADIUS, CAPSULE_HEIGHT);
	collider->SetLayer(eng::Layer::Player);
	player->AddComponent<eng::CharacterMotor>();

	player->AddComponent<FirstPersonController>()->Bind(context.input, context.state);
	FirstPersonLook* look = player->AddComponent<FirstPersonLook>();
	look->Bind(context.input, context.camera);
	look->SetYawPitch(yawDegrees, 0.0f);
	player->AddComponent<Gun>()->Bind(context.input, context.state, context.camera);

	if (withBodyMesh)
	{
		// The unit cylinder sits on y = 0: offset it down to cover the capsule
		eng::ResourceManager& resources = eng::Engine::GetInstance().GetResources();
		eng::GameActor* body = level.CreateGameActor("Player Mesh", player);
		body->SetPosition({ 0.0f, -HALF_HEIGHT, 0.0f });
		body->SetScale({ 2.0f * CAPSULE_RADIUS, 2.0f * HALF_HEIGHT, 2.0f * CAPSULE_RADIUS });
		body->AddComponent<eng::MeshRenderer>(resources.GetPrimitive(eng::Primitive::Cylinder))
			->CreateOwnMaterial(resources.GetDefaultLitShader()).SetTint({ 70, 110, 220, 255 });
	}
	return player;
}
