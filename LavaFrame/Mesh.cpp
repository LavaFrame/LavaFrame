/*
 * Read license.txt for license information.
 * This is based on the original GLSL-PathTracer by Asif Ali.
 */

#define TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_USE_MAPBOX_EARCUT
#include <tiny_obj_loader.h>
#include <tinyobj_loader_opt.h>
#include "Mesh.h"
#include <iostream>
#include "GlobalState.h"

extern LavaFrameState GlobalState;

namespace LavaFrame
{
    float sphericalTheta(const Vec3& v)
    {
        return acosf(Math::Clamp(v.y, -1.f, 1.f));
    }

    float sphericalPhi(const Vec3& v)
    {
        float p = atan2f(v.z, v.x);
        return (p < 0.f) ? p + 2.f * PI : p;
    }

    bool Mesh::LoadFromFile(const std::string& filename)
    {
        name = filename;
        tinyobj::attrib_t attrib;
        std::vector<tinyobj::shape_t> shapes;
        std::vector<tinyobj::material_t> materials;
        std::string warn;
        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str(), 0, true);

        if (!ret)
        {
            printf("Unable to load geometry.\n");
            printf(err.c_str());
#if defined(_WIN32)
            MessageBox(NULL, "Failed to load geometry.", "Loading failed", MB_ICONERROR);
#endif
            return false;
        }

        // Loop over shapes
        for (size_t s = 0; s < shapes.size(); s++)
        {
            // Loop over faces(polygon)
            size_t index_offset = 0;

            for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
            {
                int fv = shapes[s].mesh.num_face_vertices[f];
                // Loop over vertices in the face.
                for (size_t v = 0; v < fv; v++)
                {
                    // access to vertex
                    tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                    tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
                    tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
                    tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
                    tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
                    tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
                    tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];

                    tinyobj::real_t tx, ty;

                    // temporary fix
                    if (!attrib.texcoords.empty())
                    {
                        tx = attrib.texcoords[2 * idx.texcoord_index + 0];
                        ty = attrib.texcoords[2 * idx.texcoord_index + 1];
                    }
                    else
                    {
                        tx = ty = 0;
                    }

                    verticesUVX.push_back(Vec4(vx, vy, vz, tx));
                    normalsUVY.push_back(Vec4(nx, ny, nz, ty));
                }

                index_offset += fv;
            }
        }

        return true;
    }

    void Mesh::BuildBVH()
    {
        const int numTris = verticesUVX.size() / 3;
        std::vector<RadeonRays::bbox> bounds(numTris);

#pragma omp parallel for
        for (int i = 0; i < numTris; ++i)
        {
            const Vec3 v1 = Vec3(verticesUVX[i * 3 + 0]);
            const Vec3 v2 = Vec3(verticesUVX[i * 3 + 1]);
            const Vec3 v3 = Vec3(verticesUVX[i * 3 + 2]);

            bounds[i].grow(v1);
            bounds[i].grow(v2);
            bounds[i].grow(v3);
        }

        bvh->Build(&bounds[0], numTris);
    }
}