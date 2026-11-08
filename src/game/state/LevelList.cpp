#include "LevelList.h"

LevelList::LevelList(std::vector<std::string> paths) : m_paths(std::move(paths)) {}

LevelList LevelList::Default()
{
	return LevelList({ "assets/levels/level_01.lvl", "assets/levels/level_02.lvl", "assets/levels/level_03.lvl" });
}

void LevelList::Add(const std::string& path)	{ m_paths.push_back(path); }
size_t LevelList::GetCount() const				{ return m_paths.size(); }
bool LevelList::IsValid(int index) const		{ return index >= 0 && static_cast<size_t>(index) < m_paths.size(); }
bool LevelList::HasNext(int index) const		{ return IsValid(index + 1); }

const std::string& LevelList::GetPath(int index) const
{
	static const std::string empty;
	return IsValid(index) ? m_paths[static_cast<size_t>(index)] : empty;
}
