/*
 * Read license.txt for license information.
 * This is based on the original GLSL-PathTracer by Asif Ali.
 */

#include "Shader.h"
#include <iostream>
#include <fstream>
#include <sstream>

extern bool useDebug;

namespace LavaFrame
{
    Shader::Shader(const ShaderInclude::ShaderSource& sourceObj, GLenum shaderType)
    {
        object = glCreateShader(shaderType);
        if (useDebug) {
            printf("Compiling shader %s -> %d\n", sourceObj.path.c_str(), int(object));
        }
        const GLchar* src = (const GLchar*)sourceObj.src.c_str();
        glShaderSource(object, 1, &src, 0);
        glCompileShader(object);
        GLint success = 0;
        glGetShaderiv(object, GL_COMPILE_STATUS, &success);
        if (success == GL_FALSE)
        {
            std::string msg;
            GLint logSize = 0;
            glGetShaderiv(object, GL_INFO_LOG_LENGTH, &logSize);
            char* info = new char[logSize + 1];
            glGetShaderInfoLog(object, logSize, NULL, info);
            msg += sourceObj.path;
            msg += "\n";
            msg += info;
            delete[] info;
            glDeleteShader(object);
            object = 0;
            printf("Shader compile error : %s\n", msg.c_str());
            throw std::runtime_error(msg.c_str());
        }
        else
        {
            if (useDebug) {
                printf("Shader compile successful !\n");
            }
        }
    }

    GLuint Shader::getObject() const
    {
        return object;
    }
}