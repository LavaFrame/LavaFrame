/*
 * Read license.txt for license information.
 * This is based on the original GLSL-PathTracer by Asif Ali.
 */

#include "Program.h"
#include "GlobalState.h"
#include <stdexcept>

extern LavaFrameState GlobalState;

namespace LavaFrame
{
    Program::Program(const std::vector<Shader> shaders)
    {
        object = glCreateProgram();
        for (unsigned i = 0; i < shaders.size(); i++)
            glAttachShader(object, shaders[i].getObject());

        glLinkProgram(object);
        if (GlobalState.useDebug) {
            printf("Linking program %d\n", int(object));
        }
        for (unsigned i = 0; i < shaders.size(); i++)
            glDetachShader(object, shaders[i].getObject());
        GLint success = 0;
        glGetProgramiv(object, GL_LINK_STATUS, &success);
        if (success == GL_FALSE)
        {
            std::string msg("Error while linking program\n");
            GLint logSize = 0;
            glGetProgramiv(object, GL_INFO_LOG_LENGTH, &logSize);
            char* info = new char[logSize + 1];
            glGetShaderInfoLog(object, logSize, NULL, info);
            msg += info;
            delete[] info;
            glDeleteProgram(object);
            object = 0;
            if (GlobalState.useDebug) {
                printf("Error %s\n", msg.c_str());
            }
            throw std::runtime_error(msg.c_str());
        }
    }

    Program::~Program()
    {
        glDeleteProgram(object);
    }

    void Program::Use()
    {
        glUseProgram(object);
    }

    void Program::StopUsing()
    {
        glUseProgram(0);
    }

    GLuint Program::getObject()
    {
        return object;
    }
}