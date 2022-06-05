// Generated by lf_txtgen on 05/06/2022 02:23:52
/*
* MIT License
* Check license.txt for more information on licensing
* Based on the original software by Alif Ali (knightcrawler25)
*/
#pragma once
#include <string>

std::string shader =
"#version 330\n"
"\n"
"out vec3 color;\n"
"in vec2 TexCoords;\n"
"\n"
"#include common/uniforms.glsl\n"
"#include common/globals.glsl\n"
"#include common/intersection.glsl\n"
"#include common/sampling.glsl\n"
"#include common/anyhit.glsl\n"
"#include common/closest_hit.glsl\n"
"#include common/disney.glsl\n"
"#include common/pathtrace.glsl\n"
"\n"
"uniform sampler2D imgTex;\n"
"\n"
"void main(void)\n"
"{\n"
"    InitRNG(gl_FragCoord.xy, 1);\n"
"\n"
"    float r1 = 2.0 * rand();\n"
"    float r2 = 2.0 * rand();\n"
"\n"
"    vec2 jitter;\n"
"    jitter.x = r1 < 1.0 ? sqrt(r1) - 1.0 : 1.0 - sqrt(2.0 - r1);\n"
"    jitter.y = r2 < 1.0 ? sqrt(r2) - 1.0 : 1.0 - sqrt(2.0 - r2);\n"
"\n"
"    jitter /= screenResolution;\n"
"    vec2 d = (2.0 * TexCoords - 1.0) + jitter;\n"
"\n"
"    float scale = tan(camera.fov * 0.5);\n"
"    d.y *= screenResolution.y / screenResolution.x * scale;\n"
"    d.x *= scale;\n"
"    vec3 rayDir = normalize(d.x * camera.right + d.y * camera.up + camera.forward);\n"
"\n"
"    vec3 focalPoint = camera.focalDist * rayDir;\n"
"    float cam_r1 = rand() * TWO_PI;\n"
"    float cam_r2 = rand() * camera.aperture;\n"
"#ifdef USE_DOF\n"
"    vec3 randomAperturePos = (cos(cam_r1) * camera.right + sin(cam_r1) * camera.up) * sqrt(cam_r2);\n"
"    vec3 finalRayDir = normalize(focalPoint - randomAperturePos);\n"
"#endif\n"
"#ifndef USE_DOF\n"
"\tvec3 finalRayDir = normalize(focalPoint);\n"
"#endif\n"
"\n"
"#ifdef USE_DOF\n"
"    Ray ray = Ray(camera.position + randomAperturePos, finalRayDir);\n"
"#endif\n"
"#ifndef USE_DOF\n"
"\tRay ray = Ray(camera.position, finalRayDir);\n"
"#endif\n"
"\n"
"    vec3 pixelColor = PathTrace(ray);\n"
"\n"
"    color = pixelColor;\n"
"}"
;