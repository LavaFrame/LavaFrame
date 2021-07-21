/*
 * Read license.txt for license information.
 * This is based on the original GLSL-PathTracer by Asif Ali.
 */

#pragma once

#include <vector>
#include <Vec3.h>

namespace LavaFrame
{
    class Material
    {
    public:
        Material()
        {
            albedo = Vec3(1.0f, 1.0f, 1.0f);
            specular = 0.5f;

            emission = Vec3(0.0f, 0.0f, 0.0f);
            anisotropic = 0.0f;

            metallic = 0.0f;
            roughness = 0.5f;
            subsurface = 0.0f;
            specularTint = 0.0f;

            sheen = 0.0f;
            sheenTint = 0.0f;
            clearcoat = 0.0f;
            clearcoatRoughness = 0.0f;

            transmission = 0.0f;
            ior = 1.45f;
            extinction = Vec3(1.0f, 1.0f, 1.0f);

            albedoTexID = -1.0f;
            metallicRoughnessTexID = -1.0f;
            normalmapTexID = -1.0f;
        };

        Vec3 albedo;
        float specular;

        Vec3 emission;
        float anisotropic;

        float metallic;
        float roughness;
        float subsurface;
        float specularTint;

        float sheen;
        float sheenTint;
        float clearcoat;
        float clearcoatRoughness;

        float transmission;
        float ior;
        Vec3 extinction;

        float albedoTexID;
        float metallicRoughnessTexID;
        float normalmapTexID;
    };
}