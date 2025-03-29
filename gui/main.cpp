#include <raylib.h>

#include "App.h"

void Run(const size_t startingNumBodies)
{
	App app(800, 600, startingNumBodies);
	while (!WindowShouldClose())
	{
		app.Update();
		app.DrawFrame();
	}
}

int main(int argc, char **argv)
{
	SetConfigFlags(FLAG_WINDOW_RESIZABLE); //| FLAG_VSYNC_HINT);
	InitWindow(800, 600, "Kinematics Demo");

	const size_t startingNumBodies = argc > 1 ? static_cast<size_t>(std::atoi(argv[1])) : 1;
	Run(startingNumBodies);

	CloseWindow();
	return 0;
}
