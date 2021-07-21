/*
 * Read license.txt for license information.
 * This is based on the original GLSL-PathTracer by Asif Ali.
 */

#include "Texture.h"
#include <iostream>
#include "stb_image.h"

namespace LavaFrame
{
    bool Texture::LoadTexture(const std::string& filename)
    {
        name = filename;

        texData = stbi_load(filename.c_str(), &width, &height, NULL, 3);

        if (texData != nullptr)
            return true;

        return false;
    }
}