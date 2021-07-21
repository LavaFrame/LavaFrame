#version 330

precision highp float;
precision highp int;
precision highp sampler2D;
precision highp samplerCube;
precision highp isampler2D;
precision highp sampler2DArray;

out vec4 color;
in vec2 TexCoords;

uniform sampler2D pathTraceTexture;
uniform float invSampleCounter;


vec4 ToneMap(in vec4 c, float limit)
{
    float a = 2.51f;
    float b = 0.03f;
    float y = 2.43f;
    float d = 0.59f;
    float e = 0.14f;

    float luminance = 0.3*c.x + 0.6*c.y + 0.1*c.z;

    return clamp((c * (a * c + b)) / (c * (y * c + d) + e), 0.0, 1.0);
}

void main()
{
    color = texture(pathTraceTexture, TexCoords) * invSampleCounter;
    color = pow(ToneMap(color, 1.5), vec4(1.0 / 2.2));
}