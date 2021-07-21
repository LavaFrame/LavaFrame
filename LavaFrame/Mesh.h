/*
 * Read license.txt for license information.
 * This is based on the original GLSL-PathTracer by Asif Ali.
 */

#pragma once

#include <vector>
#include "split_bvh.h"

namespace LavaFrame
{
    class Mesh
    {
    public:
        Mesh()
        {
            bvh = new RadeonRays::SplitBvh(2.0f, 64, 0, 0.001f, 0);
            //bvh = new RadeonRays::Bvh(2.0f, 64, false);
        }
        ~Mesh() { delete bvh; }

        void BuildBVH();
        bool LoadFromFile(const std::string& filename);

        std::vector<Vec4> verticesUVX; // Vertex Data + x coord of uv 
        std::vector<Vec4> normalsUVY;  // Normal Data + y coord of uv

        RadeonRays::Bvh* bvh;
        std::string name;
    };

    class MeshInstance
    {

    public:
        MeshInstance(std::string name, int meshId, Mat4 xform, int matId)
            : name(name)
            , meshID(meshId)
            , transform(xform)
            , materialID(matId)
        {
        }
        ~MeshInstance() {}

        Mat4 transform;
        std::string name;

        int materialID;
        int meshID;
    };
}
