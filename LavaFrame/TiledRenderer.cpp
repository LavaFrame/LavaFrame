/*
 * Read license.txt for license information.
 * This is based on the original GLSL-PathTracer by Asif Ali.
 */

#include "Config.h"
#include "TiledRenderer.h"
#include "ShaderIncludes.h"
#include "GlobalState.h"
#include "Camera.h"
#include "Scene.h"
#include <string>

extern LavaFrameState GlobalState;

namespace LavaFrame
{
    TiledRenderer::TiledRenderer(Scene* scene, const std::string& shadersDirectory) : Renderer(scene, shadersDirectory)
        , tileWidth(scene->renderOptions.tileWidth)
        , tileHeight(scene->renderOptions.tileHeight)
        , maxDepth(scene->renderOptions.maxDepth)
        , pathTraceFBO(0)
        , previewFBO(0)
        , accumFBO(0)
        , outputFBO(0)
        , pathTraceShader(nullptr)
        , previewEngineShader(nullptr)
        , accumShader(nullptr)
        , outputShader(nullptr)
        , postShader(nullptr)
        , pathTraceTexture(0)
        , pathTraceTextureLowRes(0)
        , accumTexture(0)
        , tileOutputTexture()
        , tileX(-1)
        , tileY(-1)
        , numTilesX(-1)
        , numTilesY(-1)
        , currentBuffer(0)
        , sampleCounter(0)
    {
    }

    TiledRenderer::~TiledRenderer()
    {
    }

    void TiledRenderer::Init()
    {
        if (initialized)
            return;

        Renderer::Init();

        sampleCounter = 1;
        currentBuffer = 0;
        frameCounter = 1;

        numTilesX = ceil((float)screenSize.x / tileWidth);
        numTilesY = ceil((float)screenSize.y / tileHeight);
        pixelRatio = GlobalState.previewScale;

        tileX = -1;
        tileY = numTilesY - 1;

        //----------------------------------------------------------
        // Shaders
        //----------------------------------------------------------

        ShaderInclude::ShaderSource vertexShaderSrcObj = ShaderInclude::load(shadersDirectory + "common/vertex.glsl");
        ShaderInclude::ShaderSource pathTraceShaderSrcObj = ShaderInclude::load(shadersDirectory + "renderer.glsl");
        ShaderInclude::ShaderSource previewEngineSrcObj = ShaderInclude::load(shadersDirectory + "preview_flareon.glsl");
        ShaderInclude::ShaderSource outputShaderSrcObj = ShaderInclude::load(shadersDirectory + "output.glsl");
        ShaderInclude::ShaderSource tonemapShaderSrcObj = ShaderInclude::load(shadersDirectory + "postprocess.glsl");


        // Add preprocessor defines for conditional compilation
        std::string defines = "";
        if (scene->renderOptions.useEnvMap && scene->hdrData != nullptr)
            defines += "#define ENVMAP\n";
        if (!scene->lights.empty())
            defines += "#define LIGHTS\n";
        if (scene->renderOptions.enableRR)
        {
            defines += "#define RR\n";
            defines += "#define RR_DEPTH " + std::to_string(scene->renderOptions.RRDepth) + "\n";
        }
        if (scene->renderOptions.useConstantBg)
            defines += "#define CONSTANT_BG\n";
        if (GlobalState.useDofInPreview)
            defines += "#define USE_DOF\n";

        if (defines.size() > 0)
        {
            size_t idx = pathTraceShaderSrcObj.src.find("#version");
            if (idx != -1)
                idx = pathTraceShaderSrcObj.src.find("\n", idx);
            else
                idx = 0;
            pathTraceShaderSrcObj.src.insert(idx + 1, defines);

            idx = previewEngineSrcObj.src.find("#version");
            if (idx != -1)
                idx = previewEngineSrcObj.src.find("\n", idx);
            else
                idx = 0;
            previewEngineSrcObj.src.insert(idx + 1, defines);
        }

        pathTraceShader = LoadShaders(vertexShaderSrcObj, pathTraceShaderSrcObj);
        previewEngineShader = LoadShaders(vertexShaderSrcObj, previewEngineSrcObj);
        outputShader = LoadShaders(vertexShaderSrcObj, outputShaderSrcObj);
        postShader = LoadShaders(vertexShaderSrcObj, tonemapShaderSrcObj);

        if (GlobalState.useDebug) {
            printf("Debug sizes : %d %d - %d %d\n", tileWidth, tileHeight, screenSize.x, screenSize.y);
        }
        //----------------------------------------------------------
        // FBO Setup
        //----------------------------------------------------------
        // Create FBOs for path trace shader (Tiled)
        if (GlobalState.useDebug) {
            printf("Buffer pathTraceFBO\n");
        }
        glGenFramebuffers(1, &pathTraceFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, pathTraceFBO);

        // Create Texture for FBO
        glGenTextures(1, &pathTraceTexture);
        glBindTexture(GL_TEXTURE_2D, pathTraceTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, tileWidth, tileHeight, 0, GL_RGB, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        glBindTexture(GL_TEXTURE_2D, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pathTraceTexture, 0);

        // Create FBOs for preview shader
        if (GlobalState.useDebug) {
            printf("Buffer previewFBO\n");
        }
        glGenFramebuffers(1, &previewFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, previewFBO);

        // Create Texture for FBO
        glGenTextures(1, &pathTraceTextureLowRes);
        glBindTexture(GL_TEXTURE_2D, pathTraceTextureLowRes);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, screenSize.x * pixelRatio, screenSize.y * pixelRatio, 0, GL_RGB, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        glBindTexture(GL_TEXTURE_2D, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pathTraceTextureLowRes, 0);

        // Create FBOs for accum buffer
        if (GlobalState.useDebug) {
            printf("Buffer accumFBO\n");
        }
        glGenFramebuffers(1, &accumFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, accumFBO);

        // Create Texture for FBO
        glGenTextures(1, &accumTexture);
        glBindTexture(GL_TEXTURE_2D, accumTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, GLsizei(screenSize.x), GLsizei(screenSize.y), 0, GL_RGB, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        glBindTexture(GL_TEXTURE_2D, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, accumTexture, 0);

        // Create FBOs for tile output shader
        if (GlobalState.useDebug) {
            printf("Buffer outputFBO\n");
        }
        glGenFramebuffers(1, &outputFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);

        // Create Texture for FBO
        glGenTextures(1, &tileOutputTexture[0]);
        glBindTexture(GL_TEXTURE_2D, tileOutputTexture[0]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, screenSize.x, screenSize.y, 0, GL_RGB, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        glBindTexture(GL_TEXTURE_2D, 0);

        glGenTextures(1, &tileOutputTexture[1]);
        glBindTexture(GL_TEXTURE_2D, tileOutputTexture[1]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, screenSize.x, screenSize.y, 0, GL_RGB, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        glBindTexture(GL_TEXTURE_2D, 0);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tileOutputTexture[currentBuffer], 0);

        // For Denoiser
        denoiserInputFramePtr = new Vec3[screenSize.x * screenSize.y];
        frameOutputPtr = new Vec3[screenSize.x * screenSize.y];
        denoised = false;

        glGenTextures(1, &denoisedTexture);
        glBindTexture(GL_TEXTURE_2D, denoisedTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, screenSize.x, screenSize.y, 0, GL_RGB, GL_FLOAT, 0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
        glBindTexture(GL_TEXTURE_2D, 0);

        GLuint shaderObject;

        pathTraceShader->Use();
        shaderObject = pathTraceShader->getObject();

        glUniform1f(glGetUniformLocation(shaderObject, "hdrResolution"), scene->hdrData == nullptr ? 0 : float(scene->hdrData->width * scene->hdrData->height));
        glUniform1i(glGetUniformLocation(shaderObject, "topBVHIndex"), scene->bvhTranslator.topLevelIndex);
        glUniform2f(glGetUniformLocation(shaderObject, "screenResolution"), float(screenSize.x), float(screenSize.y));
        glUniform1i(glGetUniformLocation(shaderObject, "numOfLights"), numOfLights);
        glUniform1f(glGetUniformLocation(shaderObject, "invNumTilesX"), 1.0f / ((float)screenSize.x / tileWidth));
        glUniform1f(glGetUniformLocation(shaderObject, "invNumTilesY"), 1.0f / ((float)screenSize.y / tileHeight));
        glUniform1i(glGetUniformLocation(shaderObject, "accumTexture"), 0);
        glUniform1i(glGetUniformLocation(shaderObject, "BVH"), 1);
        glUniform1i(glGetUniformLocation(shaderObject, "vertexIndicesTex"), 2);
        glUniform1i(glGetUniformLocation(shaderObject, "verticesTex"), 3);
        glUniform1i(glGetUniformLocation(shaderObject, "normalsTex"), 4);
        glUniform1i(glGetUniformLocation(shaderObject, "materialsTex"), 5);
        glUniform1i(glGetUniformLocation(shaderObject, "transformsTex"), 6);
        glUniform1i(glGetUniformLocation(shaderObject, "lightsTex"), 7);
        glUniform1i(glGetUniformLocation(shaderObject, "textureMapsArrayTex"), 8);
        glUniform1i(glGetUniformLocation(shaderObject, "hdrTex"), 9);
        glUniform1i(glGetUniformLocation(shaderObject, "hdrMarginalDistTex"), 10);
        glUniform1i(glGetUniformLocation(shaderObject, "hdrCondDistTex"), 11);

        pathTraceShader->StopUsing();

        previewEngineShader->Use();
        shaderObject = previewEngineShader->getObject();

        glUniform1f(glGetUniformLocation(shaderObject, "hdrResolution"), scene->hdrData == nullptr ? 0 : float(scene->hdrData->width * scene->hdrData->height));
        glUniform1i(glGetUniformLocation(shaderObject, "topBVHIndex"), scene->bvhTranslator.topLevelIndex);
        glUniform2f(glGetUniformLocation(shaderObject, "screenResolution"), float(screenSize.x), float(screenSize.y));
        glUniform1i(glGetUniformLocation(shaderObject, "numOfLights"), numOfLights);
        glUniform1i(glGetUniformLocation(shaderObject, "accumTexture"), 0);
        glUniform1i(glGetUniformLocation(shaderObject, "BVH"), 1);
        glUniform1i(glGetUniformLocation(shaderObject, "vertexIndicesTex"), 2);
        glUniform1i(glGetUniformLocation(shaderObject, "verticesTex"), 3);
        glUniform1i(glGetUniformLocation(shaderObject, "normalsTex"), 4);
        glUniform1i(glGetUniformLocation(shaderObject, "materialsTex"), 5);
        glUniform1i(glGetUniformLocation(shaderObject, "transformsTex"), 6);
        glUniform1i(glGetUniformLocation(shaderObject, "lightsTex"), 7);
        glUniform1i(glGetUniformLocation(shaderObject, "textureMapsArrayTex"), 8);
        glUniform1i(glGetUniformLocation(shaderObject, "hdrTex"), 9);
        glUniform1i(glGetUniformLocation(shaderObject, "hdrMarginalDistTex"), 10);
        glUniform1i(glGetUniformLocation(shaderObject, "hdrCondDistTex"), 11);

        previewEngineShader->StopUsing();

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_BUFFER, BVHTex);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_BUFFER, vertexIndicesTex);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_BUFFER, verticesTex);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_BUFFER, normalsTex);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, materialsTex);
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, transformsTex);
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, lightsTex);
        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_2D_ARRAY, textureMapsArrayTex);
        glActiveTexture(GL_TEXTURE9);
        glBindTexture(GL_TEXTURE_2D, hdrTex);
        glActiveTexture(GL_TEXTURE10);
        glBindTexture(GL_TEXTURE_2D, hdrMarginalDistTex);
        glActiveTexture(GL_TEXTURE11);
        glBindTexture(GL_TEXTURE_2D, hdrConditionalDistTex);
    }

    void TiledRenderer::Finish()
    {
        if (!initialized)
            return;

        glDeleteTextures(1, &pathTraceTexture);
        glDeleteTextures(1, &pathTraceTextureLowRes);
        glDeleteTextures(1, &accumTexture);
        glDeleteTextures(1, &tileOutputTexture[0]);
        glDeleteTextures(1, &tileOutputTexture[1]);
        glDeleteTextures(1, &denoisedTexture);

        glDeleteFramebuffers(1, &pathTraceFBO);
        glDeleteFramebuffers(1, &previewFBO);
        glDeleteFramebuffers(1, &outputFBO);

        delete pathTraceShader;
        delete previewEngineShader;
        delete accumShader;
        delete outputShader;
        delete postShader;

        delete denoiserInputFramePtr;
        delete frameOutputPtr;

        Renderer::Finish();
    }

    void TiledRenderer::Render()
    {
        if (!initialized)
        {
            printf("Renderer is not initialized.\n");
            return;
        }

        glActiveTexture(GL_TEXTURE0);

        if (scene->camera->isMoving || scene->instancesModified)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, previewFBO);
            glViewport(0, 0, screenSize.x * pixelRatio, screenSize.y * pixelRatio);
            quad->Draw(previewEngineShader);
            scene->instancesModified = false;
        }
        else
        {
            glBindFramebuffer(GL_FRAMEBUFFER, pathTraceFBO);
            glViewport(0, 0, tileWidth, tileHeight);
            glBindTexture(GL_TEXTURE_2D, accumTexture);
            quad->Draw(pathTraceShader);

            glBindFramebuffer(GL_FRAMEBUFFER, accumFBO);
            glViewport(tileWidth * tileX, tileHeight * tileY, tileWidth, tileHeight);
            glBindTexture(GL_TEXTURE_2D, pathTraceTexture);
            quad->Draw(outputShader);

            glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tileOutputTexture[currentBuffer], 0);
            glViewport(0, 0, screenSize.x, screenSize.y);
            glBindTexture(GL_TEXTURE_2D, accumTexture);
            quad->Draw(postShader);
        }
    }

    void TiledRenderer::Present() const
    {
        if (!initialized)
            return;

        glActiveTexture(GL_TEXTURE0);

        if (scene->camera->isMoving || sampleCounter == 1)
        {
            glBindTexture(GL_TEXTURE_2D, pathTraceTextureLowRes);
            quad->Draw(postShader);
        }
        else
        {
            if (scene->renderOptions.enableAutomaticDenoise && denoised)
                glBindTexture(GL_TEXTURE_2D, denoisedTexture);
            else
                glBindTexture(GL_TEXTURE_2D, tileOutputTexture[1 - currentBuffer]);

            quad->Draw(outputShader);
        }
    }

    float TiledRenderer::GetProgress() const
    {
        return float((numTilesY - tileY - 1) * numTilesX + tileX) / float(numTilesX * numTilesY);
    }

    void TiledRenderer::GetOutputBuffer(unsigned char** data, int& w, int& h)
    {
        w = scene->renderOptions.resolution.x;
        h = scene->renderOptions.resolution.y;

        *data = new unsigned char[w * h * 3];

        glActiveTexture(GL_TEXTURE0);

        if (denoised)
            glBindTexture(GL_TEXTURE_2D, denoisedTexture);
        else
            glBindTexture(GL_TEXTURE_2D, tileOutputTexture[1 - currentBuffer]);

        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, *data);
    }

    void TiledRenderer::GetOutputBufferHDR(float** data, int& w, int& h)
    {
        w = scene->renderOptions.resolution.x;
        h = scene->renderOptions.resolution.y;

        *data = new float[w * h * 3];

        glActiveTexture(GL_TEXTURE0);

        if (denoised)
            glBindTexture(GL_TEXTURE_2D, denoisedTexture);
        else
            glBindTexture(GL_TEXTURE_2D, tileOutputTexture[1 - currentBuffer]);

        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, *data);
    }

    int TiledRenderer::GetSampleCount() const
    {
        return sampleCounter;
    }

    void TiledRenderer::Update(float secondsElapsed)
    {
        Renderer::Update(secondsElapsed);

        // Denoise Image automatically if requested by user
        if (scene->renderOptions.enableAutomaticDenoise && frameCounter % (scene->renderOptions.denoiserFrameCnt * (numTilesX * numTilesY)) == 0)
        {
            glBindTexture(GL_TEXTURE_2D, tileOutputTexture[1 - currentBuffer]);
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, denoiserInputFramePtr);

            // Create an Intel Open Image Denoise device
            oidn::DeviceRef device = oidn::newDevice();
            device.commit();

            // Create a denoising filter
            oidn::FilterRef filter = device.newFilter("RT"); // generic ray tracing filter
            filter.setImage("color", denoiserInputFramePtr, oidn::Format::Float3, screenSize.x, screenSize.y);
            filter.setImage("output", frameOutputPtr, oidn::Format::Float3, screenSize.x, screenSize.y);
            if (GlobalState.scene->renderOptions.tonemapIndex == 4) {
                filter.set("hdr", true);
                if (GlobalState.useDebug) {
                    std::cout << GlobalState.scene->renderOptions.tonemapIndex << std::endl;
                    printf("Denoise HDR\n");
                }
            }
            else {
                filter.set("hdr", false);
                if (GlobalState.useDebug) {
                    std::cout << GlobalState.scene->renderOptions.tonemapIndex << std::endl;
                    printf("Denoise LDR\n");
                }
            }
            filter.commit();

            // Filter the image
            filter.execute();
            
            // Check for errors
            const char* errorMessage;
            if (device.getError(errorMessage) != oidn::Error::None)
                std::cout << "Error: " << errorMessage << std::endl;

            glBindTexture(GL_TEXTURE_2D, denoisedTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, screenSize.x, screenSize.y, 0, GL_RGB, GL_FLOAT, frameOutputPtr);

            GlobalState.denoiseTexture = denoisedTexture;

            denoised = true;
        }

        if (scene->camera->isMoving || scene->instancesModified)
        {
            tileX = -1;
            tileY = numTilesY - 1;
            sampleCounter = 1;
            denoised = false;
            frameCounter = 1;

            glBindFramebuffer(GL_FRAMEBUFFER, accumFBO);
            //glViewport(0, 0, screenSize.x, screenSize.y);
            glClear(GL_COLOR_BUFFER_BIT);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
        else
        {
            frameCounter++;
            tileX++;
            if (tileX >= numTilesX)
            {
                tileX = 0;
                tileY--;
                if (tileY < 0)
                {
                    tileX = 0;
                    tileY = numTilesY - 1;
                    sampleCounter++;
                    currentBuffer = 1 - currentBuffer;
                }
            }
        }

        GLuint shaderObject;

        pathTraceShader->Use();
        shaderObject = pathTraceShader->getObject();
        glUniform3f(glGetUniformLocation(shaderObject, "camera.position"), scene->camera->position.x, scene->camera->position.y, scene->camera->position.z);
        glUniform3f(glGetUniformLocation(shaderObject, "camera.right"), scene->camera->right.x, scene->camera->right.y, scene->camera->right.z);
        glUniform3f(glGetUniformLocation(shaderObject, "camera.up"), scene->camera->up.x, scene->camera->up.y, scene->camera->up.z);
        glUniform3f(glGetUniformLocation(shaderObject, "camera.forward"), scene->camera->forward.x, scene->camera->forward.y, scene->camera->forward.z);
        glUniform1f(glGetUniformLocation(shaderObject, "camera.fov"), scene->camera->fov);
        glUniform1f(glGetUniformLocation(shaderObject, "camera.focalDist"), scene->camera->focalDist);
        glUniform1f(glGetUniformLocation(shaderObject, "camera.aperture"), scene->camera->aperture);
        glUniform1i(glGetUniformLocation(shaderObject, "useEnvMap"), scene->hdrData == nullptr ? false : scene->renderOptions.useEnvMap);
        glUniform1f(glGetUniformLocation(shaderObject, "hdrMultiplier"), scene->renderOptions.hdrMultiplier);
        glUniform1i(glGetUniformLocation(shaderObject, "maxDepth"), scene->renderOptions.maxDepth);
        glUniform1i(glGetUniformLocation(shaderObject, "tileX"), tileX);
        glUniform1i(glGetUniformLocation(shaderObject, "tileY"), tileY);
        glUniform3f(glGetUniformLocation(shaderObject, "bgColor"), scene->renderOptions.bgColor.x, scene->renderOptions.bgColor.y, scene->renderOptions.bgColor.z);
        glUniform1i(glGetUniformLocation(shaderObject, "frame"), frameCounter);
        pathTraceShader->StopUsing();

        previewEngineShader->Use();
        shaderObject = previewEngineShader->getObject();
        glUniform3f(glGetUniformLocation(shaderObject, "camera.position"), scene->camera->position.x, scene->camera->position.y, scene->camera->position.z);
        glUniform3f(glGetUniformLocation(shaderObject, "camera.right"), scene->camera->right.x, scene->camera->right.y, scene->camera->right.z);
        glUniform3f(glGetUniformLocation(shaderObject, "camera.up"), scene->camera->up.x, scene->camera->up.y, scene->camera->up.z);
        glUniform3f(glGetUniformLocation(shaderObject, "camera.forward"), scene->camera->forward.x, scene->camera->forward.y, scene->camera->forward.z);
        glUniform1f(glGetUniformLocation(shaderObject, "camera.fov"), scene->camera->fov);
        glUniform1f(glGetUniformLocation(shaderObject, "camera.focalDist"), scene->camera->focalDist);
        glUniform1f(glGetUniformLocation(shaderObject, "camera.aperture"), scene->camera->aperture);
        glUniform1i(glGetUniformLocation(shaderObject, "useEnvMap"), scene->hdrData == nullptr ? false : scene->renderOptions.useEnvMap);
        glUniform1f(glGetUniformLocation(shaderObject, "hdrMultiplier"), scene->renderOptions.hdrMultiplier);
        glUniform1i(glGetUniformLocation(shaderObject, "maxDepth"), scene->camera->isMoving || scene->instancesModified ? 2 : scene->renderOptions.maxDepth);
        glUniform3f(glGetUniformLocation(shaderObject, "camera.position"), scene->camera->position.x, scene->camera->position.y, scene->camera->position.z);
        glUniform3f(glGetUniformLocation(shaderObject, "bgColor"), scene->renderOptions.bgColor.x, scene->renderOptions.bgColor.y, scene->renderOptions.bgColor.z);
        previewEngineShader->StopUsing();

        postShader->Use();
        shaderObject = postShader->getObject();
        glUniform1i(glGetUniformLocation(shaderObject, "isInPreview"), scene->camera->isMoving);
        glUniform1f(glGetUniformLocation(shaderObject, "invSampleCounter"), 1.0f / (sampleCounter));
        glUniform1i(glGetUniformLocation(shaderObject, "tonemapIndex"), scene->renderOptions.tonemapIndex);
        glUniform1i(glGetUniformLocation(shaderObject, "useCA"), scene->renderOptions.useCA);
        glUniform1i(glGetUniformLocation(shaderObject, "useCADistortion"), scene->renderOptions.useCADistortion);
        glUniform1f(glGetUniformLocation(shaderObject, "caDistance"), scene->renderOptions.caDistance);
        glUniform1f(glGetUniformLocation(shaderObject, "caP1"), scene->renderOptions.caP1);
        glUniform1f(glGetUniformLocation(shaderObject, "caP2"), scene->renderOptions.caP2);
        glUniform1f(glGetUniformLocation(shaderObject, "caP3"), scene->renderOptions.caP3);
        glUniform1i(glGetUniformLocation(shaderObject, "useVignette"), scene->renderOptions.useVignette);
        glUniform1f(glGetUniformLocation(shaderObject, "vignetteIntensity"), scene->renderOptions.vignetteIntensity);
        glUniform1f(glGetUniformLocation(shaderObject, "vignettePower"), scene->renderOptions.vignettePower);
        postShader->StopUsing();
    }

    uint32_t TiledRenderer::SetViewport(int width, int height)
    {
        if (GlobalState.scene->camera->isMoving || sampleCounter == 1)
        {
            glBindTexture(GL_TEXTURE_2D, pathTraceTextureLowRes);
            quad->Draw(postShader);
            return pathTraceTextureLowRes;
        }
        else
        {
            return tileOutputTexture[1 - currentBuffer];
        }
    }

    uint32_t TiledRenderer::Denoise() {
        ////For Denoiser
        auto denoiserInputFramePtr = new Vec3[screenSize.x * screenSize.y];
        auto frameOutputPtr = new Vec3[screenSize.x * screenSize.y];

        glBindTexture(GL_TEXTURE_2D, tileOutputTexture[1 - currentBuffer]);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_FLOAT, denoiserInputFramePtr);

        // Create an Intel Open Image Denoise device
        oidn::DeviceRef device = oidn::newDevice();
        device.commit();

        // Create a denoising filter
        oidn::FilterRef filter = device.newFilter("RT"); // generic ray tracing filter
        filter.setImage("color", denoiserInputFramePtr, oidn::Format::Float3, screenSize.x, screenSize.y);
        filter.setImage("output", frameOutputPtr, oidn::Format::Float3, screenSize.x, screenSize.y);
        if (GlobalState.scene->renderOptions.tonemapIndex == 4) { 
            filter.set("hdr", true);
            if (GlobalState.useDebug) {
                std::cout << GlobalState.scene->renderOptions.tonemapIndex << std::endl;
                printf("Denoise HDR\n");
            }
        }
        else {
            filter.set("hdr", false);
            if (GlobalState.useDebug) {
                std::cout << GlobalState.scene->renderOptions.tonemapIndex << std::endl;
                printf("Denoise LDR\n");
            }
        }
        filter.commit();

        // Filter the image
        filter.execute();

        // Check for errors
        const char* errorMessage;
        if (device.getError(errorMessage) != oidn::Error::None)
            std::cout << "Denoise Error: " << errorMessage << std::endl;

        // Copy the denoised data to denoisedTexture
        glBindTexture(GL_TEXTURE_2D, denoisedTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, screenSize.x, screenSize.y, 0, GL_RGB, GL_FLOAT, frameOutputPtr);

        denoised = true;

        return denoisedTexture;
    }
}