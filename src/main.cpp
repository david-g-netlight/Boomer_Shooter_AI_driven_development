#include <eng.h>
#include "ScaleGame.h"

int main()
{
	eng::Engine& engine = eng::Engine::GetInstance();
	engine.SetApplication(new ScaleGame()); // Ownership transfer: the Engine deletes the game

	if (engine.Init())
	{
		engine.Run();
	}

	engine.Shutdown();

	return 0;
}
