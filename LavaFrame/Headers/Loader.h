/*
 * Read license.txt for license information.
 * This is based on the original GLSL-PathTracer by Asif Ali.
 */

#pragma once

#include <cstring>
#include "Scene.h"

namespace LavaFrame
{
    class Scene;

    bool LoadSceneFromFile(const std::string& filename, Scene* scene, RenderOptions& renderOptions);
    // logger function. might be set at init time
    extern int(*Log)(const char* szFormat, ...);
}