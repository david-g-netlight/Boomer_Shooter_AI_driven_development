#include "LevelSerializer.h"
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <raylib.h>
#include <raymath.h>
#include "core/Engine.h"
#include "core/Log.h"
#include "level/ComponentRegistry.h"
#include "level/GameActor.h"
#include "level/Level.h"
#include "level/PropertyBag.h"

namespace eng::LevelSerializer
{
	namespace
	{
		constexpr int FORMAT_VERSION = 1;

		std::string Number(float value)
		{
			char text[32];
			std::snprintf(text, sizeof(text), "%.6g", static_cast<double>(value));
			return text;
		}

		std::string Trim(const std::string& text)
		{
			const size_t first = text.find_first_not_of(" \t\r\n");
			if (first == std::string::npos)
				return "";
			return text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1);
		}

		std::vector<std::string> Split(const std::string& text)
		{
			std::vector<std::string> tokens;
			std::istringstream stream(text);
			for (std::string token; stream >> token;)
				tokens.push_back(token);
			return tokens;
		}

		//////////////////////////////////////////
		/// Writing
		//////////////////////////////////////////
		bool CanSave(const GameActor& actor, const ComponentRegistry& registry)
		{
			for (const auto& component : actor.GetComponents())
				if (!registry.IsRegistered(component->TypeName()))
				{
					Log::Warning("LevelSerializer: '%s' not saved, component '%s' is not registered",
						actor.GetName().c_str(), component->TypeName());
					return false;
				}
			return true;
		}

		int CountSubtree(const GameActor& actor)
		{
			int count = 1;
			for (const GameActor* child : actor.GetChildren())
				if (!child->IsMarkedForDestroy())
					count += CountSubtree(*child);
			return count;
		}

		void WriteActor(const GameActor& actor, int parentIndex, int& nextIndex, SaveResult& result, const ComponentRegistry& registry)
		{
			if (actor.IsMarkedForDestroy())
				return;
			if (!CanSave(actor, registry))
			{
				result.skipped += CountSubtree(actor);
				return;
			}

			const int index = nextIndex++;
			std::string name = actor.GetName();
			for (char& c : name)
				if (c == '"' || c == '\n' || c == '\r')
					c = '\'';

			std::ostringstream out;
			out << "actor \"" << name << "\"\n";
			if (parentIndex >= 0)	out << "  parent " << parentIndex << "\n";
			if (!actor.IsActive())	out << "  active 0\n";

			const ::Transform& t	= actor.GetTransform();
			const Vector3 euler		= QuaternionToEuler(t.rotation);
			out << "  transform " << Number(t.translation.x) << " " << Number(t.translation.y) << " " << Number(t.translation.z)
				<< "  " << Number(euler.x * RAD2DEG) << " " << Number(euler.y * RAD2DEG) << " " << Number(euler.z * RAD2DEG)
				<< "  " << Number(t.scale.x) << " " << Number(t.scale.y) << " " << Number(t.scale.z) << "\n";

			for (const auto& component : actor.GetComponents())
			{
				PropertyBag bag;
				component->Serialize(bag);
				out << "  component " << component->TypeName();
				for (const auto& [key, value] : bag.Items())
					out << " " << key << "=" << value;
				out << "\n";
			}
			out << "end\n";

			result.text += out.str();
			++result.saved;

			for (const GameActor* child : actor.GetChildren())
				WriteActor(*child, index, nextIndex, result, registry);
		}

		//////////////////////////////////////////
		/// Reading
		//////////////////////////////////////////
		struct ComponentBlock
		{
			std::string		type;
			PropertyBag		bag;
		};

		struct ActorBlock
		{
			std::string					name;
			int							parent		= -1;
			bool						isActive	= true;
			::Transform					transform	= { { 0, 0, 0 }, { 0, 0, 0, 1 }, { 1, 1, 1 } };
			Vector3						euler		= { 0, 0, 0 };
			std::vector<ComponentBlock>	components;
		};

		GameActor* Build(Level& level, const ActorBlock& block, const std::vector<GameActor*>& created, LoadResult& result)
		{
			GameActor* parent = nullptr;
			if (block.parent >= 0)
			{
				if (block.parent < static_cast<int>(created.size()))
					parent = created[static_cast<size_t>(block.parent)];
				else
				{
					Log::Warning("LevelSerializer: '%s' has parent %d that is not an earlier actor", block.name.c_str(), block.parent);
					++result.warnings;
				}
			}

			GameActor* actor = level.CreateGameActor(block.name, parent);
			actor->SetPosition(block.transform.translation);
			actor->SetRotationEuler(block.euler);
			actor->SetScale(block.transform.scale);

			const ComponentRegistry& registry = Engine::GetInstance().GetComponentRegistry();
			for (const ComponentBlock& component : block.components)
			{
				if (Component* made = registry.Create(component.type, *actor))
					made->Deserialize(component.bag);
				else
				{
					Log::Warning("LevelSerializer: '%s': unknown component '%s'", block.name.c_str(), component.type.c_str());
					++result.warnings;
				}
			}

			actor->SetActive(block.isActive);
			return actor;
		}
	}

	SaveResult ToText(const Level& level)
	{
		SaveResult result;
		result.text = "lvl " + std::to_string(FORMAT_VERSION) + "\n";
		if (const SpawnPoint& spawn = level.GetSpawn(); spawn.isSet)
			result.text += "spawn " + Number(spawn.position.x) + " " + Number(spawn.position.y) + " " + Number(spawn.position.z)
				+ "  yaw " + Number(spawn.yawDegrees) + "\n";

		const ComponentRegistry& registry = Engine::GetInstance().GetComponentRegistry();
		int nextIndex = 0;
		for (const auto& actor : level.GetActors())
			if (!actor->GetParent())
				WriteActor(*actor, -1, nextIndex, result, registry);
		return result;
	}

	LoadResult FromText(Level& level, const std::string& text)
	{
		LoadResult result;
		std::istringstream stream(text);
		std::string raw;
		ActorBlock block;
		bool isInBlock = false;
		int lineNumber = 0;

		auto warn = [&](const char* what)
		{
			Log::Warning("LevelSerializer: line %d: %s", lineNumber, what);
			++result.warnings;
		};

		while (std::getline(stream, raw))
		{
			++lineNumber;
			const std::string line = Trim(raw.substr(0, raw.find('#')));
			if (line.empty())
				continue;

			const std::vector<std::string> tokens = Split(line);
			const std::string& keyword = tokens[0];

			if (keyword == "lvl")
			{
				if (tokens.size() > 1 && std::atoi(tokens[1].c_str()) > FORMAT_VERSION)
					warn("file is newer than this engine understands");
			}
			else if (keyword == "actor")
			{
				if (isInBlock)
					warn("'actor' before the previous 'end'");
				block = ActorBlock{};
				const size_t open = line.find('"'), close = line.rfind('"');
				block.name = (open != std::string::npos && close > open) ? line.substr(open + 1, close - open - 1) : "Unnamed";
				isInBlock = true;
			}
			else if (keyword == "spawn")
			{
				if (tokens.size() < 4)
				{
					warn("spawn needs x y z");
					continue;
				}
				const float yaw = (tokens.size() >= 6 && tokens[4] == "yaw") ? std::strtof(tokens[5].c_str(), nullptr) : 0.0f;
				level.SetSpawn({ std::strtof(tokens[1].c_str(), nullptr), std::strtof(tokens[2].c_str(), nullptr), std::strtof(tokens[3].c_str(), nullptr) }, yaw);
			}
			else if (!isInBlock)
			{
				warn("line outside an actor block ignored");
			}
			else if (keyword == "parent" && tokens.size() > 1)
			{
				block.parent = std::atoi(tokens[1].c_str());
			}
			else if (keyword == "active" && tokens.size() > 1)
			{
				block.isActive = tokens[1] != "0";
			}
			else if (keyword == "transform")
			{
				if (tokens.size() < 10)
				{
					warn("transform needs 9 numbers");
					continue;
				}
				float v[9];
				for (int i = 0; i < 9; ++i)
					v[i] = std::strtof(tokens[static_cast<size_t>(i) + 1].c_str(), nullptr);
				block.transform.translation	= { v[0], v[1], v[2] };
				block.euler					= { v[3], v[4], v[5] };
				block.transform.scale		= { v[6], v[7], v[8] };
			}
			else if (keyword == "component" && tokens.size() > 1)
			{
				ComponentBlock component;
				component.type = tokens[1];
				for (size_t i = 2; i < tokens.size(); ++i)
				{
					const size_t equals = tokens[i].find('=');
					if (equals == std::string::npos)
						warn("component property without '='");
					else
						component.bag.SetString(tokens[i].substr(0, equals), tokens[i].substr(equals + 1));
				}
				block.components.push_back(std::move(component));
			}
			else if (keyword == "end")
			{
				result.created.push_back(Build(level, block, result.created, result));
				isInBlock = false;
			}
			else
			{
				warn("unknown line ignored");
			}
		}

		if (isInBlock)
			warn("file ends inside an actor block");

		result.ok = true;
		return result;
	}

	bool Save(const Level& level, const std::string& path)
	{
		const SaveResult saved = ToText(level);

		std::error_code error;
		const std::filesystem::path file(path);
		if (file.has_parent_path())
			std::filesystem::create_directories(file.parent_path(), error);

		std::ofstream out(file, std::ios::binary | std::ios::trunc);
		if (!out)
		{
			Log::Error("LevelSerializer: cannot write '%s'", path.c_str());
			return false;
		}
		out << saved.text;
		Log::Info("LevelSerializer: saved %d actors to '%s' (%d skipped)", saved.saved, path.c_str(), saved.skipped);
		return out.good();
	}

	LoadResult Load(Level& level, const std::string& path)
	{
		std::ifstream in(path, std::ios::binary);
		if (!in)
		{
			Log::Error("LevelSerializer: cannot open '%s'", path.c_str());
			return {};
		}

		std::stringstream buffer;
		buffer << in.rdbuf();

		level.Clear();
		const LoadResult result = FromText(level, buffer.str());
		Log::Info("LevelSerializer: loaded %zu actors from '%s' (%d warnings)", result.created.size(), path.c_str(), result.warnings);
		return result;
	}

	GameActor* Duplicate(Level& level, GameActor& actor)
	{
		const ComponentRegistry& registry = Engine::GetInstance().GetComponentRegistry();
		SaveResult subtree;
		int nextIndex = 0;
		WriteActor(actor, -1, nextIndex, subtree, registry);
		if (subtree.saved == 0 || subtree.skipped > 0)
		{
			Log::Warning("LevelSerializer: '%s' cannot be duplicated (not every component is registered)", actor.GetName().c_str());
			return nullptr;
		}

		const LoadResult copy = FromText(level, subtree.text);
		if (copy.created.empty())
			return nullptr;

		GameActor* root = copy.created.front();
		root->SetName(actor.GetName() + " Copy");
		if (actor.GetParent())
			level.SetParent(root, actor.GetParent());
		return root;
	}
}
