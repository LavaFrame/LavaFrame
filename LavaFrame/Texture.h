/*
 * Read license.txt for license information.
 * This is based on the original GLSL-PathTracer by Asif Ali.
 */

#pragma once

#include <vector>
#include "split_bvh.h"

namespace LavaFrame
{
    class Texture
    {
    public:
        Texture() : texData(nullptr), width(0), height(0) {};
        ~Texture() { delete texData; }

        bool LoadTexture(const std::string& filename);

        int width;
        int height;
        unsigned char* texData;
        std::string name;
    };
}
