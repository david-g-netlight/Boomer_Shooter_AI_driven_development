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
	/// The arena (assets/levels/arena_01.lvl), the only level for now.
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
