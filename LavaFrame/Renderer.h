/*
 * Read license.txt for license information.
 * This is based on the original GLSL-PathTracer by Asif Ali.
 */

#pragma once

#include "Quad.h"
#include "Program.h"
#include <Vec2.h>
#include <Vec3.h>
#include <vector>

namespace LavaFrame
{
    Program* LoadShaders(const ShaderInclude::ShaderSource& vertShaderObj, const ShaderInclude::ShaderSource& fragShaderObj);

    struct RenderOptions
    {
        RenderOptions()
        {
            maxDepth = 2;
            tileWidth = 512;
            tileHeight = 512;
            useEnvMap = false;
            resolution = iVec2(1280, 720);
            hdrMultiplier = 1.0f;
            enableRR = true;
            useConstantBg = false;
            RRDepth = 2;
            bgColor = Vec3(0.5f, 0.5f, 0.5f);
            denoiserFrameCnt = 50;
            enableAutomaticDenoise = false;
            useAces = true;
        }
        iVec2 resolution;
        int maxDepth;
        int tileWidth;
        int tileHeight;
        bool useEnvMap;
        bool enableRR;
        bool enableAutomaticDenoise;
        bool useConstantBg;
        bool useAces;
        int RRDepth;
        int denoiserFrameCnt;
        float hdrMultiplier;
        Vec3 bgColor;
    };

    class Scene;

    class Renderer
    {
    protected:
        Scene *scene;
        Quad* quad;

        iVec2 screenSize;
        std::string shadersDirectory;

        GLuint BVHBuffer;
        GLuint BVHTex;
        GLuint vertexIndicesBuffer;
        GLuint vertexIndicesTex;
        GLuint verticesBuffer;
        GLuint verticesTex;
        GLuint normalsBuffer;
        GLuint normalsTex;
        GLuint materialsTex;
        GLuint transformsTex;
        GLuint lightsTex;
        GLuint textureMapsArrayTex;
        GLuint hdrTex;
        GLuint hdrMarginalDistTex;
        GLuint hdrConditionalDistTex;

        int numOfLights;
        bool initialized;

    public:
        Renderer(Scene *scene, const std::string& shadersDirectory);
        virtual ~Renderer();

        const iVec2 GetScreenSize() const { return screenSize; }

        virtual void Init();
        virtual void Finish();

        virtual void Render() = 0;
        virtual void Present() const = 0;
        virtual void Update(float secondsElapsed);
        virtual float GetProgress() const = 0;
        virtual int GetSampleCount() const = 0;
        virtual void GetOutputBuffer(unsigned char**, int &w, int &h) = 0;
        virtual uint32_t SetViewport(int width, int height) = 0;
        virtual uint32_t Denoise() = 0;
    };
}