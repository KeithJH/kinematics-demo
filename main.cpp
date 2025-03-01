#include <raylib.h>

int main(void)
{
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(800, 600, "Kinematics Demo");
	SetTargetFPS(60);

	while (!WindowShouldClose())
	{
		BeginDrawing();

		ClearBackground(BEIGE);
		DrawFPS(10, 10);

		EndDrawing();
	}

	CloseWindow();
	return 0;
}
