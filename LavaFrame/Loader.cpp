/*
    This is a modified version of the original code from the tinsel renderer.
    Link to original code: https://github.com/mmacklin/tinsel
*/

// Original license :

/*
Copyright (c) 2018 Miles Macklin

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgement in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/
#pragma warning( disable : 6031 )
#include "Loader.h"
#include "GlobalState.h"
#include "ImGuizmo.h"
#include <tiny_obj_loader.h>
#include <iostream>
#include <iterator>
#include <algorithm>
#include <stdio.h>

extern LavaFrameState GlobalState;

namespace LavaFrame
{
    static const int kMaxLineLength = 2048;
    int(*Log)(const char* szFormat, ...) = printf;
    int legacyAcesOverride = 2;

    bool LoadSceneFromFile(const std::string& filename, Scene* scene, RenderOptions& renderOptions)
    {
        FILE* file;
        file = fopen(filename.c_str(), "r");
        GlobalState.overrideTileSize = false;

        if (!file)
        {
#if defined(_WIN32)
            MessageBox(NULL, "Scene file could not be opened for reading !", "Loading failed", MB_ICONERROR);
#endif
            Log("Couldn't open scene file %s for reading\n", filename.c_str());
            return false;
        }
        Log(std::string("Loading scene " + filename + " ...\n").c_str());

        struct MaterialData
        {
            Material mat;
            int id;
        };

        std::map<std::string, MaterialData> materialMap;
        std::vector<std::string> albedoTex;
        std::vector<std::string> metallicRoughnessTex;
        std::vector<std::string> normalTex;
        std::string path = filename.substr(0, filename.find_last_of("/\\")) + "/";

        int materialCount = 0;
        char line[kMaxLineLength];

        //Defaults
        Material defaultMat;
        scene->AddMaterial(defaultMat);

        bool cameraAdded = false;

        while (fgets(line, kMaxLineLength, file))
        {
            // skip comments
            if (line[0] == '#')
                continue;

            // name used for materials and meshes
            char name[kMaxLineLength] = { 0 };

            //--------------------------------------------
            // Material

            if (sscanf(line, " material %s", name) == 1)
            {
                Material material;
                char albedoTexName[100] = "None";
                char metallicRoughnessTexName[100] = "None";
                char normalTexName[100] = "None";
                char emissionTexName[100] = "None";

                while (fgets(line, kMaxLineLength, file))
                {
                    // end group
                    if (strchr(line, '}'))
                        break;
                    if (GlobalState.useDebug) {
                        Log("Loading material data...\n");
                    }
                    sscanf(line, " name %s", name);
                    sscanf(line, " color %f %f %f", &material.albedo.x, &material.albedo.y, &material.albedo.z); //deprecated, only for legacy support
                    sscanf(line, " albedo %f %f %f", &material.albedo.x, &material.albedo.y, &material.albedo.z); //new
                    sscanf(line, " emission %f %f %f", &material.emission.x, &material.emission.y, &material.emission.z);
                    sscanf(line, " metallic %f", &material.metallic);
                    sscanf(line, " roughness %f", &material.roughness);
                    sscanf(line, " subsurface %f", &material.subsurface);
                    sscanf(line, " specular %f", &material.specular);
                    sscanf(line, " specularTint %f", &material.specularTint);
                    sscanf(line, " anisotropic %f", &material.anisotropic);
                    sscanf(line, " sheen %f", &material.sheen);
                    sscanf(line, " sheenTint %f", &material.sheenTint);
                    sscanf(line, " clearcoat %f", &material.clearcoat);
                    sscanf(line, " clearcoatRoughness %f", &material.clearcoatRoughness);
                    sscanf(line, " transmission %f", &material.transmission);
                    sscanf(line, " ior %f", &material.ior);
                    sscanf(line, " extinction %f %f %f", &material.extinction.x, &material.extinction.y, &material.extinction.z);
                    sscanf(line, " albedoTexture %s", albedoTexName);
                    sscanf(line, " metallicRoughnessTexture %s", metallicRoughnessTexName);
                    sscanf(line, " normalTexture %s", normalTexName);
                    sscanf(line, " emissionTexture %s", emissionTexName);
                }

                // Albedo Texture
                if (strcmp(albedoTexName, "None") != 0)
                    material.albedoTexID = scene->AddTexture(path + albedoTexName);

                // MetallicRoughness Texture
                if (strcmp(metallicRoughnessTexName, "None") != 0)
                    material.metallicRoughnessTexID = scene->AddTexture(path + metallicRoughnessTexName);

                // Normal Map Texture
                if (strcmp(normalTexName, "None") != 0)
                    material.normalmapTexID = scene->AddTexture(path + normalTexName);

                // Emission Map Texture
                if (strcmp(emissionTexName, "None") != 0)
                    material.emissionmapTexID = scene->AddTexture(path + emissionTexName);

                // add material to map
                if (materialMap.find(name) == materialMap.end()) // New material
                {
                    int id = scene->AddMaterial(material);
                    materialMap[name] = MaterialData{ material, id };
                }
            }

            //--------------------------------------------
            // Light

            if (strstr(line, "light"))
            {
                Light light;
                Vec3 v1, v2;
                char light_type[20] = "None";

                while (fgets(line, kMaxLineLength, file))
                {
                    if (GlobalState.useDebug) {
                        Log("Loading scene lighting...\n");
                    }
                    // end group
                    if (strchr(line, '}'))
                        break;

                    sscanf(line, " position %f %f %f", &light.position.x, &light.position.y, &light.position.z);
                    sscanf(line, " emission %f %f %f", &light.emission.x, &light.emission.y, &light.emission.z);

                    sscanf(line, " radius %f", &light.radius);
                    sscanf(line, " v1 %f %f %f", &v1.x, &v1.y, &v1.z);
                    sscanf(line, " v2 %f %f %f", &v2.x, &v2.y, &v2.z);
                    sscanf(line, " type %s", light_type);
                }

                if (strcmp(light_type, "Quad") == 0)
                {
                    light.type = LightType::RectLight;
                    light.u = v1 - light.position;
                    light.v = v2 - light.position;
                    light.area = Vec3::Length(Vec3::Cross(light.u, light.v));
                }
                else if (strcmp(light_type, "Sphere") == 0)
                {
                    light.type = LightType::SphereLight;
                    light.area = 4.0f * PI * light.radius * light.radius;
                }

                scene->AddLight(light);
            }

            //--------------------------------------------
            // Camera

            if (strstr(line, "Camera"))
            {
                Vec3 position;
                Vec3 lookAt;
                float fov;
                float aperture = 0, focalDist = 1;

                while (fgets(line, kMaxLineLength, file))
                {
                    // end group
                    if (GlobalState.useDebug) {
                        Log("Loading scene camera data...\n");
                    }
                    if (strchr(line, '}'))
                        break;

                    sscanf(line, " position %f %f %f", &position.x, &position.y, &position.z);
                    sscanf(line, " lookAt %f %f %f", &lookAt.x, &lookAt.y, &lookAt.z);
                    sscanf(line, " aperture %f ", &aperture);
                    sscanf(line, " focalDistance %f", &focalDist);
                    sscanf(line, " focaldist %f", &focalDist);
                    sscanf(line, " fov %f", &fov);
                }

                delete scene->camera;
                scene->AddCamera(position, lookAt, fov);
                scene->camera->aperture = aperture;
                scene->camera->focalDist = focalDist;
                cameraAdded = true;
            }

            //--------------------------------------------
            // Renderer

            if (strstr(line, "Renderer"))
            {
                char envMap[200] = "None";
                char enableRR[10] = "None";

                while (fgets(line, kMaxLineLength, file))
                {
                    // end group
                    if (strchr(line, '}'))
                        break;
                    if (GlobalState.useDebug) {
                        Log("Loading scene data...\n");
                    }
                    sscanf(line, " envMap %s", envMap); // Legacy
                    sscanf(line, " hdriMap %s", envMap);
                    sscanf(line, " resolution %d %d", &renderOptions.resolution.x, &renderOptions.resolution.y);
                    sscanf(line, " hdrMultiplier %f", &renderOptions.hdrMultiplier);
                    sscanf(line, " hdriMultiplier %f", &renderOptions.hdrMultiplier);
                    sscanf(line, " maxDepth %i", &renderOptions.maxDepth);
                    if (sscanf(line, " tileWidth %i", &renderOptions.tileWidth) == 1) {
                        GlobalState.overrideTileSize = true;
                    }
                    if (sscanf(line, " tileHeight %i", &renderOptions.tileHeight) == 1) {
                        GlobalState.overrideTileSize = true;
                    }
                    sscanf(line, " enableRR %s", enableRR);
                    sscanf(line, " RRDepth %i", &renderOptions.RRDepth);
                    sscanf(line, " tonemapIndex %i", &renderOptions.tonemapIndex);
                    sscanf(line, " useAces %i", &legacyAcesOverride);
                    if (legacyAcesOverride == 1) {
                        renderOptions.tonemapIndex = 1;
                    }
                    if (legacyAcesOverride == 0) {
                        renderOptions.tonemapIndex = 0;
                    }
                    // Chromatic Aberration controls
                    sscanf(line, " useChromaticAberration %i", &renderOptions.useCA);
                    sscanf(line, " CADistance %f", &renderOptions.caDistance);
                    sscanf(line, " useCAdistortion %i", &renderOptions.useCADistortion);
                    sscanf(line, " CAAngularity %f", &renderOptions.caP1);
                    sscanf(line, " CADirectionality %f", &renderOptions.caP2);
                    sscanf(line, " CACenter %f", &renderOptions.caP3);
                    // Vignette controls : useVignette, vignetteIntensity, vignettePower
                    sscanf(line, " useVignette %i", &renderOptions.useVignette);
                    sscanf(line, " vignetteIntensity %f", &renderOptions.vignetteIntensity);
                    sscanf(line, " vignettePower %f", &renderOptions.vignettePower);

                }

                if (strcmp(envMap, "None") != 0)
                {
                    scene->AddHDR(path + envMap);
                    renderOptions.useEnvMap = true;
                }

                if (strcmp(enableRR, "False") == 0)
                    renderOptions.enableRR = false;
                else if (strcmp(enableRR, "True") == 0)
                    renderOptions.enableRR = true;
            }


            //--------------------------------------------
            // Mesh

            if (strstr(line, "mesh"))
            {
                std::string filename;
                Vec3 pos;
                Vec3 scale;
                Mat4 xform;
                int material_id = 0; // Default Material ID
                char meshName[200] = "None";

                while (fgets(line, kMaxLineLength, file))
                {
                    // end group
                    if (strchr(line, '}'))
                        break;

                    char file[2048];
                    char matName[100];

                    sscanf(line, " name %[^\t\n]s", meshName);

                    if (sscanf(line, " file %s", file) == 1)
                    {
                        filename = path + file;
                    }

                    if (sscanf(line, " material %s", matName) == 1)
                    {
                        // look up material in dictionary
                        if (materialMap.find(matName) != materialMap.end())
                        {
                            material_id = materialMap[matName].id;
                        }
                        else
                        {
                            Log("Could not find material %s\n", matName);
                        }
                    }

                    sscanf(line, " position %f %f %f", &xform[3][0], &xform[3][1], &xform[3][2]);
                    sscanf(line, " scale %f %f %f", &xform[0][0], &xform[1][1], &xform[2][2]);
                    
                }
                if (!filename.empty())
                {
                    int mesh_id = scene->AddMesh(filename);
                    if (mesh_id != -1)
                    {
                        std::string instanceName;

                        if (strcmp(meshName, "None") != 0)
                        {
                            instanceName = std::string(meshName);
                        }
                        else
                        {
                            std::size_t pos = filename.find_last_of("/\\");
                            instanceName = filename.substr(pos + 1);
                        }

                        MeshInstance instance1(instanceName, mesh_id, xform, material_id);
                        scene->AddMeshInstance(instance1);
                    }
                }
            }
        }

        fclose(file);

        if (!GlobalState.overrideTileSize) {
            renderOptions.tileHeight = renderOptions.resolution.y;
            renderOptions.tileWidth = renderOptions.resolution.x;
        }

        GlobalState.initTileSizeX = renderOptions.tileWidth;
        GlobalState.initTileSizeY = renderOptions.tileHeight;

        if (!cameraAdded)
            scene->AddCamera(Vec3(0.0f, 0.0f, 10.0f), Vec3(0.0f, 0.0f, -10.0f), 35.0f);

        scene->CreateAccelerationStructures();

        Log("Scene loaded.\n");

        return true;
    }
}