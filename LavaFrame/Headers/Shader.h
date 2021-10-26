/*
 * Read license.txt for license information.
 * This is based on the original GLSL-PathTracer by Asif Ali.
 */

#pragma once

#include <string>
#include "ShaderIncludes.h"
#include "Config.h"

namespace LavaFrame
{
    class Shader
    {
    private:
        GLuint object;
    public:
        Shader(const ShaderInclude::ShaderSource& sourceObj, GLuint shaderType);
        GLuint getObject() const;
    };
}