#pragma once
#include <eng.h>

/// <summary>
/// Published by a NextLevelTrigger; the GameplayManager loads the next level.
/// </summary>
struct NextLevelRequestedEvent
{
	int fromLevel;
};

/// <summary>
/// Base of the level-flow volumes. Sits on an actor with a Collider (layer Trigger) and an engine TriggerVolume
/// (add them first); OnPlayerEnter() runs once when the player capsule enters. Anything else that enters
/// (crates, pellets) is ignored.
/// </summary>
class TriggerZone : public eng::Component
{
public:
	void			OnTriggerEnter(eng::GameActor& other) override;

protected:
	virtual void	OnPlayerEnter() = 0;
};

/// <summary>
/// Reaching it wins the level (GameState::WinGame).
/// </summary>
class WinTrigger : public TriggerZone
{
public:
	const char*		TypeName() const override { return "WinTrigger"; }

protected:
	void			OnPlayerEnter() override;
};

/// <summary>
/// A kill volume, e.g. below the map: entering it loses the level (GameState::LoseGame).
/// </summary>
class LoseTrigger : public TriggerZone
{
public:
	const char*		TypeName() const override { return "LoseTrigger"; }

protected:
	void			OnPlayerEnter() override;
};

/// <summary>
/// Asks the GameplayManager for the next level (a door in the middle of a level sequence).
/// </summary>
class NextLevelTrigger : public TriggerZone
{
public:
	const char*		TypeName() const override { return "NextLevelTrigger"; }

protected:
	void			OnPlayerEnter() override;
};
