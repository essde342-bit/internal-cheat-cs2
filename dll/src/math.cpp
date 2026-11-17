#include "math.h"

bool Math::world_to_screen(const Vec3& world, Vec2& screen, const ViewMatrix& view, int width, int height) {
    float w = view.m[3][0] * world.x + view.m[3][1] * world.y + view.m[3][2] * world.z + view.m[3][3];
    if (w < 0.001f) return false;
    
    float x = view.m[0][0] * world.x + view.m[0][1] * world.y + view.m[0][2] * world.z + view.m[0][3];
    float y = view.m[1][0] * world.x + view.m[1][1] * world.y + view.m[1][2] * world.z + view.m[1][3];
    
    float inv_w = 1.0f / w;
    x *= inv_w;
    y *= inv_w;
    
    screen.x = (width * 0.5f) + (x * width * 0.5f);
    screen.y = (height * 0.5f) - (y * height * 0.5f);
    
    return true;
}
