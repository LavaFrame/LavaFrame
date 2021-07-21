/*
 * Read license.txt for license information.
 * This is based on the original GLSL-PathTracer by Asif Ali.
 */

#pragma once

namespace LavaFrame
{
    struct Vec4
    {
    public:
        Vec4();
        Vec4(float x, float y, float z, float w);

        float operator[](int i) const;

        float x, y, z, w;
    };

    inline Vec4::Vec4()
    {
        x = y = z = w = 0;
    };

    inline Vec4::Vec4(float x, float y, float z, float w)
    {
        this->x = x;
        this->y = y;
        this->z = z;
        this->w = w;
    };

    inline float Vec4::operator[](int i) const
    {
        if (i == 0)
            return x;
        else if (i == 1)
            return y;
        else if (i == 2)
            return z;
        else
            return w;
    };
}