#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <raylib.h>

namespace eng
{
	class InputManager;

	/// <summary>
	/// One physical button bound to an action.
	/// </summary>
	struct InputBinding
	{
		enum class Device : uint8_t { Key, MouseButton };

		Device	device	= Device::Key;
		int		code	= KEY_NULL;		// raylib KeyboardKey or MouseButton

		static InputBinding Key(int key)			{ return { Device::Key, key }; }
		static InputBinding Mouse(int button)		{ return { Device::MouseButton, button }; }
	};

	/// <summary>
	/// One source of a 1D axis. Several sources on the same axis are summed.
	/// </summary>
	struct AxisBinding
	{
		enum class Source : uint8_t { Keys, MouseDeltaX, MouseDeltaY, MouseWheel };

		Source	source		= Source::Keys;
		int		negative	= KEY_NULL;		// Keys only: gives -1 while held
		int		positive	= KEY_NULL;		// Keys only: gives +1 while held
		float	scale		= 1.0f;

		static AxisBinding Keys(int negativeKey, int positiveKey, float scale = 1.0f)	{ return { Source::Keys, negativeKey, positiveKey, scale }; }
		static AxisBinding MouseX(float scale = 1.0f)									{ return { Source::MouseDeltaX, KEY_NULL, KEY_NULL, scale }; }
		static AxisBinding MouseY(float scale = 1.0f)									{ return { Source::MouseDeltaY, KEY_NULL, KEY_NULL, scale }; }
		static AxisBinding Wheel(float scale = 1.0f)									{ return { Source::MouseWheel, KEY_NULL, KEY_NULL, scale }; }
	};

	/// <summary>
	/// Named actions and axes on top of the InputManager snapshot, so game code asks for "Jump"
	/// instead of KEY_SPACE. Owned by the InputManager; bindings are registered by the game.
	///
	///   map.BindAction("Jump", InputBinding::Key(KEY_SPACE));
	///   map.BindAxis("MoveX", AxisBinding::Keys(KEY_A, KEY_D));
	///   map.BindAxis("MoveY", AxisBinding::Keys(KEY_S, KEY_W));
	///   map.BindAxis2D("Move", "MoveX", "MoveY", true);
	///   if (map.IsActionPressed("Jump")) ...;  Vector2 move = map.GetAxis2D("Move");
	///
	/// Querying a name that was never bound returns false / 0 and logs one warning.
	/// </summary>
	class InputMap
	{
	public:
		explicit InputMap(const InputManager& input);

		//////////////////////////////////////////
		/// Bindings
		//////////////////////////////////////////
		void	BindAction(const std::string& action, InputBinding binding);	// adds; an action can have many bindings
		void	BindAxis(const std::string& axis, AxisBinding binding);			// adds; sources are summed
		void	BindAxis2D(const std::string& axis, const std::string& xAxis, const std::string& yAxis,
						   bool clampToUnitLength);								// clamp: diagonals are not faster
		void	Unbind(const std::string& name);								// action, axis or 2D axis
		void	Clear();

		bool	HasAction(const std::string& action) const;
		bool	HasAxis(const std::string& axis) const;

		//////////////////////////////////////////
		/// Queries
		//////////////////////////////////////////
		bool	IsActionDown	(const std::string& action) const;	// any binding held
		bool	IsActionPressed	(const std::string& action) const;	// any binding went down this frame
		bool	IsActionReleased(const std::string& action) const;	// a binding went up and none is held

		float	GetAxis		(const std::string& axis) const;
		Vector2	GetAxis2D	(const std::string& axis) const;

	private:
		struct Axis2D
		{
			std::string	xAxis;
			std::string	yAxis;
			bool		clampToUnitLength = false;
		};

		const std::vector<InputBinding>* FindAction(const std::string& action) const;
		bool	IsBindingDown	(const InputBinding& binding) const;
		bool	IsBindingPressed(const InputBinding& binding) const;
		bool	IsBindingReleased(const InputBinding& binding) const;
		void	WarnUnbound(const std::string& name) const;

		const InputManager&											m_input;
		std::unordered_map<std::string, std::vector<InputBinding>>	m_actions;
		std::unordered_map<std::string, std::vector<AxisBinding>>	m_axes;
		std::unordered_map<std::string, Axis2D>						m_axes2D;
		mutable std::unordered_set<std::string>						m_warnedNames;	// warn once per typo
	};
}
