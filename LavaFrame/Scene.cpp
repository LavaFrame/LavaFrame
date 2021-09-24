/*
 * Read license.txt for license information.
 * This is based on the original GLSL-PathTracer by Asif Ali.
 */

#include <iostream>
#include "GlobalState.h"
#include "Scene.h"
#include "Camera.h"

extern LavaFrameState GlobalState;

namespace LavaFrame
{
    void Scene::AddCamera(Vec3 pos, Vec3 lookAt, float fov)
    {
        delete camera;
        camera = new Camera(pos, lookAt, fov);
    }

    int Scene::AddMesh(const std::string& filename)
    {
        int id = -1;
        // Check if mesh was already loaded
        for (int i = 0; i < meshes.size(); i++)
            if (meshes[i]->name == filename)
                return i;

        id = meshes.size();

        Mesh* mesh = new Mesh;

        if (mesh->LoadFromFile(filename))
        {
            meshes.push_back(mesh);
            if (GlobalState.useDebug) {
                printf("Geometry %s loaded sucessfully\n", filename.c_str());
            }
        }
        else
            id = -1;
        return id;
    }

    int Scene::AddTexture(const std::string& filename)
    {
        int id = -1;
        // Check if texture was already loaded
        for (int i = 0; i < textures.size(); i++)
            if (textures[i]->name == filename)
                return i;

        id = textures.size();

        Texture* texture = new Texture;

        if (texture->LoadTexture(filename))
        {
            textures.push_back(texture);
            if (GlobalState.useDebug) {
                printf("Material %s loaded sucessfully\n", filename.c_str());
            }
        }
        else
            id = -1;

        return id;
    }

    int Scene::AddMaterial(const Material& material)
    {
        int id = materials.size();
        materials.push_back(material);
        return id;
    }

    void Scene::AddHDR(const std::string& filename)
    {
        delete hdrData;
        hdrData = HDRLoader::load(filename.c_str());
        if (hdrData == nullptr)
            printf("Unable to load HDRI\n");
        else
        {
            if (GlobalState.useDebug) {
                printf("HDRI %s loaded\n", filename.c_str());
            }
            renderOptions.useEnvMap = true;
        }
    }

    int Scene::AddMeshInstance(const MeshInstance& meshInstance)
    {
        int id = meshInstances.size();
        meshInstances.push_back(meshInstance);
        return id;
    }

    int Scene::AddLight(const Light& light)
    {
        int id = lights.size();
        lights.push_back(light);
        return id;
    }

    void Scene::createTLAS()
    {
        // Loop through all the mesh Instances and build a Top Level BVH
        std::vector<RadeonRays::bbox> bounds;
        bounds.resize(meshInstances.size());

#pragma omp parallel for
        for (int i = 0; i < meshInstances.size(); i++)
        {
            RadeonRays::bbox bbox = meshes[meshInstances[i].meshID]->bvh->Bounds();
            Mat4 matrix = meshInstances[i].transform;

            Vec3 minBound = bbox.pmin;
            Vec3 maxBound = bbox.pmax;

            Vec3 right = Vec3(matrix[0][0], matrix[0][1], matrix[0][2]);
            Vec3 up = Vec3(matrix[1][0], matrix[1][1], matrix[1][2]);
            Vec3 forward = Vec3(matrix[2][0], matrix[2][1], matrix[2][2]);
            Vec3 translation = Vec3(matrix[3][0], matrix[3][1], matrix[3][2]);

            Vec3 xa = right * minBound.x;
            Vec3 xb = right * maxBound.x;

            Vec3 ya = up * minBound.y;
            Vec3 yb = up * maxBound.y;

            Vec3 za = forward * minBound.z;
            Vec3 zb = forward * maxBound.z;

            minBound = Vec3::Min(xa, xb) + Vec3::Min(ya, yb) + Vec3::Min(za, zb) + translation;
            maxBound = Vec3::Max(xa, xb) + Vec3::Max(ya, yb) + Vec3::Max(za, zb) + translation;

            RadeonRays::bbox bound;
            bound.pmin = minBound;
            bound.pmax = maxBound;

            bounds[i] = bound;
        }
        sceneBvh->Build(&bounds[0], bounds.size());
        sceneBounds = sceneBvh->Bounds();
    }

    void Scene::createBLAS()
    {
        // Loop through all meshes and build BVHs
#pragma omp parallel for
        for (int i = 0; i < meshes.size(); i++)
        {
            if (GlobalState.useDebug) {
                printf("Generating virtual geometry for %s\n", meshes[i]->name.c_str());
                printf("Constructing BVH for %s\n", meshes[i]->name.c_str());
            }
            meshes[i]->BuildBVH();
            if (GlobalState.useDebug) {
                printf("Loading material for %s\n", meshes[i]->name.c_str());
            }
        }
    }

    void Scene::RebuildInstances()
    {
        delete sceneBvh;
        sceneBvh = new RadeonRays::Bvh(10.0f, 64, false);

        createTLAS();
        bvhTranslator.UpdateTLAS(sceneBvh, meshInstances);

        //Copy transforms
        for (int i = 0; i < meshInstances.size(); i++)
            transforms[i] = meshInstances[i].transform;

        instancesModified = true;
    }

    void Scene::CreateAccelerationStructures()
    {
        createBLAS();

        if (GlobalState.useDebug) {
            printf("Building scene geometry...\n");
        }
        createTLAS();

        // Flatten BVH
        bvhTranslator.Process(sceneBvh, meshes, meshInstances);

        int verticesCnt = 0;

        //Copy mesh data
        for (int i = 0; i < meshes.size(); i++)
        {
            // Copy indices from BVH and not from Mesh
            int numIndices = meshes[i]->bvh->GetNumIndices();
            const int* triIndices = meshes[i]->bvh->GetIndices();

            for (int j = 0; j < numIndices; j++)
            {
                int index = triIndices[j];
                int v1 = (index * 3 + 0) + verticesCnt;
                int v2 = (index * 3 + 1) + verticesCnt;
                int v3 = (index * 3 + 2) + verticesCnt;

                vertIndices.push_back(Indices{ v1, v2, v3 });
            }

            verticesUVX.insert(verticesUVX.end(), meshes[i]->verticesUVX.begin(), meshes[i]->verticesUVX.end());
            normalsUVY.insert(normalsUVY.end(), meshes[i]->normalsUVY.begin(), meshes[i]->normalsUVY.end());

            verticesCnt += meshes[i]->verticesUVX.size();
        }

        //Copy transforms
        transforms.resize(meshInstances.size());
#pragma omp parallel for
        for (int i = 0; i < meshInstances.size(); i++)
            transforms[i] = meshInstances[i].transform;

        //Copy Textures
        for (int i = 0; i < textures.size(); i++)
        {
            texWidth = textures[i]->width;
            texHeight = textures[i]->height;
            int texSize = texWidth * texHeight;
            textureMapsArray.insert(textureMapsArray.end(), &textures[i]->texData[0], &textures[i]->texData[texSize * 3]);
        }
    }
}