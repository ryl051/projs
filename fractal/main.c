#include "tree.h"
#include <raylib.h>

int main() {
    const int WIDTH = 800;
    const int HEIGHT = 600;

    InitWindow(WIDTH, HEIGHT, "Fractal tree yay!");
    SetTargetFPS(5);

    int maxDepth = 10;

    while (!WindowShouldClose()) {
        BeginDrawing();
            ClearBackground(RAYWHITE);
            DrawText("Fracal Tree", 10, 10, 20, LIGHTGRAY);
            DrawFractalTree(WIDTH / 2.0f, HEIGHT - 50, 150.0f, 0.0f, maxDepth);
        EndDrawing();
    }

    return 0;
}
