#pragma once
#include <eng.h>
#include <string>

/// <summary>
/// Demo component for a TriggerVolume: counts Enter / Exit events and tints the zone while occupied.
/// </summary>
class TriggerCounter : public eng::Component
{
public:
	void OnTriggerEnter(eng::GameActor& other) override
	{
		++m_enters;
		m_last = "entered: " + other.GetName();
		Refresh();
	}

	void OnTriggerExit(eng::GameActor& other) override
	{
		++m_exits;
		m_last = "left: " + other.GetName();
		Refresh();
	}

	int					GetEnters() const	{ return m_enters; }
	int					GetExits() const	{ return m_exits; }
	const std::string&	GetLast() const		{ return m_last; }
	size_t				GetInside() const
	{
		const eng::TriggerVolume* volume = GetOwner()->GetComponent<eng::TriggerVolume>();
		return volume ? volume->GetOverlapCount() : 0;
	}

	const char* TypeName() const override { return "TriggerCounter"; }

private:
	void Refresh()
	{
		// Exit is reported after the overlap set is updated, so GetInside() is already current here
		if (eng::MeshRenderer* renderer = GetOwner()->GetComponent<eng::MeshRenderer>())
			if (eng::Material* material = renderer->GetOwnMaterial())
				material->SetTint(GetInside() > 0 ? Color{ 60, 220, 90, 90 } : Color{ 60, 140, 255, 70 });
	}

	int			m_enters	= 0;
	int			m_exits		= 0;
	std::string	m_last		= "nothing yet";
};
