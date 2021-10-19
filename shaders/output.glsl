/*
 * Read license.txt for license information.
 * This is based on the original GLSL-PathTracer by Asif Ali.
 */

#version 330

out vec4 color;
in vec2 TexCoords;

uniform sampler2D imgTex;

void main()
{
    color = texture(imgTex, TexCoords);
}