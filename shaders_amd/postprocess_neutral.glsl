/*
 * Read license.txt for license information.
 * This is based on the original GLSL-PathTracer by Asif Ali.
 */

#version 330

highp float;
highp int;
highp sampler2D;
highp samplerCube;
highp isampler2D;
highp sampler2DArray;

out vec4 color;
in vec2 TexCoords;

uniform sampler2D pathTraceTexture;
uniform float invSampleCounter;

vec4 ToneMap(in vec4 c, float limit)
{
    float luminance = 0.3*c.x + 0.6*c.y + 0.1*c.z;

    return c * 1.0 / (1.0 + luminance / limit);
}

void main()
{
    color = texture(pathTraceTexture, TexCoords) * invSampleCounter;
    color = pow(ToneMap(color, 1.0), vec4(1.0 / 2.2));
}