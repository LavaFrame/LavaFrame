/*
 * Read license.txt for license information.
 * This is based on the original GLSL-PathTracer by Asif Ali.
 */

#version 440

highp float;
highp int;
highp sampler2D;
highp samplerCube;
highp isampler2D;
highp sampler2DArray;

layout (location = 0) in vec2 position;
layout (location = 1) in vec2 texCoords;

out vec2 TexCoords;

void main()
{
    gl_Position = vec4(position.x,position.y,0.0,1.0);
    TexCoords = texCoords;
}