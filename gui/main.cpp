#include <raylib.h>

#include "App.h"

void Run()
{
	App app(800, 600, 1);
	while (!WindowShouldClose())
	{
		app.Update();
		app.DrawFrame();
	}
}

int main(void)
{
	SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
	InitWindow(800, 600, "Kinematics Demo");

	Run();

	CloseWindow();
	return 0;
}
