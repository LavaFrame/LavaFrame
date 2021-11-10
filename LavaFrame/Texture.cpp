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

        int img_channels = 3;

        texData = stbi_load(filename.c_str(), &width, &height, &img_channels, 4);

        if (texData != nullptr)
            return true;

        return false;
    }
}