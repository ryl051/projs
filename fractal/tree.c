#include <math.h>
#include <raylib.h>

#define WIDTH 900
#define HEIGHT 600

void DrawFractalTree(float startX, float startY, float length, float angle, int depth) {
    if (depth == 0 || length < 2.0f) {
        return;
    }
    float endX = startX + length * sinf(angle);
    float endY = startY - length * cosf(angle);

    // thin out the thickness
    float thickness = depth * 0.8f;
    if (thickness < 1.0f) {
        thickness = 1.0f;
    }

    DrawLineEx((Vector2){startX, startY}, (Vector2){endX, endY}, thickness, DARKGREEN);

    float branchAngle = 0.45f;
    float shrinkFactor = 0.75f;

    DrawFractalTree(endX, endY, length * shrinkFactor, angle - branchAngle, depth - 1);
    DrawFractalTree(endX, endY, length * shrinkFactor, angle + branchAngle, depth - 1);
}
