#pragma once
#include <string>
#include <vector>

namespace eng
{
	class GameActor;
	class Level;

	/// <summary>
	/// Text level files (.lvl). One block per actor, parents before children:
	///
	///		lvl 1
	///		spawn 0 0 10  yaw 0                          # where the player starts (feet), omitted when not set
	///		actor "Crate 01"
	///		  parent 0                                   # index of an earlier actor, omitted for roots
	///		  active 0                                   # omitted when active
	///		  transform 0 0.5 6   0 0 0   1 1 1          # position | euler degrees | scale (local)
	///		  component Collider shape=box half=0.5,0.5,0.5 layer=Prop
	///		  component RigidBody type=dynamic mass=2
	///		end
	///
	/// Components are created through the ComponentRegistry, in file order (a Collider goes before the RigidBody
	/// that uses it). An actor with a component that isn't registered is not saved (transient things such as
	/// beams), together with its children; the log says which. '#' starts a comment.
	/// </summary>
	namespace LevelSerializer
	{
		struct SaveResult
		{
			std::string	text;
			int			saved		= 0;
			int			skipped		= 0;
		};

		struct LoadResult
		{
			bool						ok			= false;
			std::vector<GameActor*>		created;			// in file order; the first is a root
			int							warnings	= 0;	// unknown components, malformed lines
		};

		SaveResult	ToText(const Level& level);

		/// <summary>
		/// Appends the actors of `text` to `level` (does not clear it).
		/// </summary>
		LoadResult	FromText(Level& level, const std::string& text);

		bool		Save(const Level& level, const std::string& path);

		/// <summary>
		/// Clears `level`, then loads the file. Call between frames, not inside a level update.
		/// </summary>
		LoadResult	Load(Level& level, const std::string& path);

		/// <summary>
		/// Copies an actor with its children and components next to the original (same parent; named "<name> Copy").
		/// </summary>
		GameActor*	Duplicate(Level& level, GameActor& actor);
	}
}
