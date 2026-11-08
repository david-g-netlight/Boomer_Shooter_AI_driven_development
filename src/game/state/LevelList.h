#pragma once
#include <string>
#include <vector>

/// <summary>
/// The levels of the game in play order, as .lvl paths relative to the executable (Unity SceneRef).
/// </summary>
class LevelList
{
public:
	LevelList() = default;
	explicit LevelList(std::vector<std::string> paths);

	/// <summary>
	/// level_01 .. level_03 in assets/levels (authored in Phase 12).
	/// </summary>
	static LevelList Default();

	void				Add(const std::string& path);
	size_t				GetCount() const;
	bool				IsValid(int index) const;
	bool				HasNext(int index) const;
	const std::string&	GetPath(int index) const;	// "" when out of range

private:
	std::vector<std::string>	m_paths;
};
