#pragma once
#include "Time.h"
#include "WindowInfo.h"
#include "World.h"
#include "input/InputManager.h"

/////////////////////////////////////////////////////////
// Engine class
/**
 * Owns Application
 * In charge of deleting Application, ALWAYS
 * No other class should delete Application
 * Only one owner
 */
/////////////////////////////////////////////////////////
constexpr int DEFAULT_WINDOW_WIDTH	= 1280;
constexpr int DEFAULT_WINDOW_HEIGHT	= 720;

namespace eng
{
	/// Forward declaration: the full definition is only needed in Engine.cpp
	class Application;

	/// <summary>
	/// Singleton class
	/// </summary>
	class Engine
	{
	private:
		// Make all constructors private so the instance can only be created from inside the class.
		Engine() = default;

		// The following deletions enforce the singleton: no copies, no moves.
		Engine(const Engine&)	= delete;		// Copy constructor deleted
		Engine(Engine&&)		= delete;		// Move constructor deleted

		Engine& operator = (const Engine&)	= delete;	// Copy operator deleted
		Engine& operator = (Engine&&)		= delete;	// Move operator deleted

		~Engine();

	public:
		/// @brief Meyer's Singleton, creates an instance in static memory the first time it is requested
		static Engine& GetInstance();

		/// <summary>
		/// Creates the window, then calls Application::Init().
		/// Separates "object exists" from "object ready for use".
		/// </summary>
		bool Init(int width = DEFAULT_WINDOW_WIDTH, int height = DEFAULT_WINDOW_HEIGHT);

		/// <summary>
		///	Contains the Main Game Loop: Input, Game State/Logic, Sound and Rendering
		/// </summary>
		void Run();

		/// <summary>
		/// Logical shutdown: Application::Destroy(), then closes the window. Safe to call more than once.
		/// The Application instance itself is deleted in ~Engine.
		/// </summary>
		void Shutdown();

		/// <summary>
		/// Ownership transfer happens here. Gets the pointer to the Application created by main.cpp;
		/// the Engine is in charge of deleting it. A previous Application is deleted.
		/// </summary>
		void SetApplication(Application* app);

		Application*		GetApplication() const;	// non-owning access
		float				GetDeltaTime() const;

		//////////////////////////////////////////////////////////////
		// Getters to core objects
		//////////////////////////////////////////////////////////////
		InputManager&		GetInputManager();
		Time&				GetTime();
		const WindowInfo&	GetWindow() const;
		const World&		GetWorld() const;

	private:
		Application*	m_app				= nullptr;	// owning
		bool			m_isWindowOpen		= false;
		bool			m_isAppInitialized	= false;	// Destroy() is only called if Init() succeeded

		///////////////////////////////////////////////////////
		// Time, Window and World features
		///////////////////////////////////////////////////////
		// The Engine can call InputManager's private constructor because it is a friend class
		InputManager	m_inputManager;

		Time			m_time;
		WindowInfo		m_window;
		World			m_world;
	};
}
