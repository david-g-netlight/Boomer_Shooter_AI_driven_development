#include <eng.h>
#include "ArenaGame.h"

int main()
{
	eng::Engine& engine = eng::Engine::GetInstance();
	engine.SetApplication(new ArenaGame()); // Ownership transfer: the Engine deletes the game

	if (engine.Init())
	{
		engine.Run();
	}

	engine.Shutdown();

	return 0;
}
