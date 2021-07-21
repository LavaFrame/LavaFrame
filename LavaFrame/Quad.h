/*
 * Read license.txt for license information.
 * This is based on the original GLSL-PathTracer by Asif Ali.
 */

#pragma once
#include "Config.h"

namespace LavaFrame
{
    class Program;

    class Quad
    {
    public:
        Quad();
        void Draw(Program*);

    private:
        GLuint vao;
        GLuint vbo;
    };
}