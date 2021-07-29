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

void main()
{
    color = texture(pathTraceTexture, TexCoords);
}