/*
 * Read license.txt for license information.
 * This is based on the original GLSL-PathTracer by Asif Ali.
 */

#version 330

out vec3 color;
in vec2 TexCoords;

#include common/uniforms.glsl
#include common/globals.glsl
#include common/intersection.glsl
#include common/sampling.glsl
#include common/anyhit.glsl
#include common/closest_hit.glsl
#include common/disney.glsl
#include common/pathtrace.glsl

float map(float value, float low1, float high1, float low2, float high2)
{
    return low2 + ((value - low1) * (high2 - low2)) / (high1 - low1);
}

void main(void)
{
    vec2 coordsTile = TexCoords;
    vec2 coordsFS;

    float xoffset = -1.0 + 2.0 * invNumTilesX * float(tileX);
    float yoffset = -1.0 + 2.0 * invNumTilesY * float(tileY);

    coordsTile.x = map(coordsTile.x, 0.0, 1.0, xoffset, xoffset + 2.0 * invNumTilesX);
    coordsTile.y = map(coordsTile.y, 0.0, 1.0, yoffset, yoffset + 2.0 * invNumTilesY);

    coordsFS.x = map(TexCoords.x, 0.0, 1.0, invNumTilesX * float(tileX), invNumTilesX * float(tileX) + invNumTilesX);
    coordsFS.y = map(TexCoords.y, 0.0, 1.0, invNumTilesY * float(tileY), invNumTilesY * float(tileY) + invNumTilesY);

    InitRNG(coordsFS * screenResolution, frame);

    float r1 = 2.0 * rand();
    float r2 = 2.0 * rand();

    vec2 jitter;
    jitter.x = r1 < 1.0 ? sqrt(r1) - 1.0 : 1.0 - sqrt(2.0 - r1);
    jitter.y = r2 < 1.0 ? sqrt(r2) - 1.0 : 1.0 - sqrt(2.0 - r2);

    jitter /= (screenResolution * 0.5);
    vec2 d = coordsTile + jitter;

    float scale = tan(camera.fov * 0.5);
    d.y *= screenResolution.y / screenResolution.x * scale;
    d.x *= scale;
    vec3 rayDir = normalize(d.x * camera.right + d.y * camera.up + camera.forward);

    vec3 focalPoint = camera.focalDist * rayDir;
    float cam_r1 = rand() * TWO_PI;
    float cam_r2 = rand() * camera.aperture;
    vec3 randomAperturePos = (cos(cam_r1) * camera.right + sin(cam_r1) * camera.up) * sqrt(cam_r2);
    vec3 finalRayDir = normalize(focalPoint - randomAperturePos);

    Ray ray = Ray(camera.position + randomAperturePos, finalRayDir);

    vec3 accumColor = texture(accumTexture, coordsFS).xyz;

    vec3 pixelColor = PathTrace(ray);

    color = pixelColor + accumColor;
}