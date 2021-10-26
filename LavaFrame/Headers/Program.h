/*
 * Read license.txt for license information.
 * This is based on the original GLSL-PathTracer by Asif Ali.
 */

#pragma once

#include "Shader.h"
#include <vector>

namespace LavaFrame
{
    class Program
    {
    private:
        GLuint object;

    public:
        Program(const std::vector<Shader> shaders);
        ~Program();
        void Use();
        void StopUsing();
        GLuint getObject();
    };
}
