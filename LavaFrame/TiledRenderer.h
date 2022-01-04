/*
 * Read license.txt for license information.
 * This is based on the original GLSL-PathTracer by Asif Ali.
 */

#pragma once

#include "Renderer.h"
#include "OpenImageDenoise/oidn.hpp"

namespace LavaFrame
{
    class Scene;
    class TiledRenderer : public Renderer
    {
    private:
        // FBOs
        GLuint pathTraceFBO;
        GLuint previewFBO;
        GLuint accumFBO;
        GLuint outputFBO;

        // Shaders
        Program* pathTraceShader;
        Program* previewEngineShader;
        Program* accumShader;
        Program* outputShader;
        Program* postShader;

        // Textures
        GLuint pathTraceTexture;
        GLuint pathTraceTextureLowRes;
        GLuint accumTexture;
        GLuint tileOutputTexture[2];
        GLuint denoisedTexture;

        int tileX;
        int tileY;
        int numTilesX;
        int numTilesY;
        int tileWidth;
        int tileHeight;

        int maxDepth;
        int currentBuffer;
        int frameCounter;
        int sampleCounter;
        float pixelRatio;

        Vec3* denoiserInputFramePtr;
        Vec3* frameOutputPtr;

        bool denoised;

    public:
        TiledRenderer(Scene* scene, const std::string& shadersDirectory);
        ~TiledRenderer();

        void Init();
        void Finish();

        void Render();
        void Present() const;
        void Update(float secondsElapsed);
        uint32_t SetViewport(int width, int height);
        uint32_t Denoise();
        float GetProgress() const;
        int GetSampleCount() const;
        void GetOutputBuffer(unsigned char**, int& w, int& h);
        void GetOutputBufferFloat(float** data, int& w, int& h);
    };
}