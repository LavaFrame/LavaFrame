/*
 * Read license.txt for license information.
 * This is based on the original GLSL-PathTracer by Asif Ali.
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include "hdrloader.h"
#include "bvh.h"
#include "Renderer.h"
#include "Mesh.h"
#include "Camera.h"
#include "bvh_translator.h"
#include "Texture.h"
#include "Material.h"

namespace LavaFrame
{
    class Camera;

    enum LightType // TODO: Working on Directional light here
    {
        RectLight,
        SphereLight,
        DistantLight
    };

    struct Light
    {
        Vec3 position;
        Vec3 emission;
        Vec3 u;
        Vec3 v;
        float radius;
        float area;
        float type;
    };

    struct Indices
    {
        int x, y, z;
    };

    class Scene
    {
    public:
        Scene() : camera(nullptr), hdrData(nullptr) {
            sceneBvh = new RadeonRays::Bvh(10.0f, 64, false);
        }
        ~Scene() { delete camera; delete sceneBvh; delete hdrData; };

        int AddMesh(const std::string& filename);
        int AddTexture(const std::string& filename);
        int AddMaterial(const Material& material);
        int AddMeshInstance(const MeshInstance& meshInstance);
        int AddLight(const Light& light);

        void AddCamera(Vec3 eye, Vec3 lookat, float fov);
        void AddHDR(const std::string& filename);

        void CreateAccelerationStructures();
        void RebuildInstances();

        //Options
        RenderOptions renderOptions;

        //Meshs
        std::vector<Mesh*> meshes;

        // Scene Mesh Data 
        std::vector<Indices> vertIndices;
        std::vector<Vec4> verticesUVX; // Vertex Data + x coord of uv 
        std::vector<Vec4> normalsUVY;  // Normal Data + y coord of uv
        std::vector<Mat4> transforms;

        //Instances
        std::vector<Material> materials;
        std::vector<MeshInstance> meshInstances;
        bool instancesModified = false;

        //Lights
        std::vector<Light> lights;

        //HDR
        HDRData* hdrData;

        //Camera
        Camera* camera;

        //Bvh
        RadeonRays::BvhTranslator bvhTranslator; // Produces a flat bvh array for GPU consumption
        RadeonRays::bbox sceneBounds;

        //Texture Data
        std::vector<Texture*> textures;
        std::vector<unsigned char> textureMapsArray;
        int texWidth;
        int texHeight; // TODO: allow textures of different sizes

    private:
        RadeonRays::Bvh* sceneBvh;
        void createBLAS();
        void createTLAS();
    };
}