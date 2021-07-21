/*
 * Read license.txt for license information.
 * This is based on the original GLSL-PathTracer by Asif Ali.
 */

#pragma once

namespace LavaFrame
{
    struct iVec2
    {
    public:
        iVec2() { x = 0, y = 0; };
        iVec2(int x, int y) { this->x = x; this->y = y; };

        int x, y;
    };

    struct Vec2
    {
    public:
        Vec2() { x = 0, y = 0; };
        Vec2(float x, float y) { this->x = x; this->y = y; };

        float x, y;
    };
}