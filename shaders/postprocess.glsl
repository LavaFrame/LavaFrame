/*
 * Read license.txt for license information.
 * This is based on the original GLSL-PathTracer by Asif Ali.
 */

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
uniform bool isInPreview;
uniform float invSampleCounter;
uniform float caDistance;
uniform float caP1;
uniform float caP2;
uniform float caP3;
uniform bool useAces;
uniform bool useCA;
uniform bool useCADistortion;

vec4 tonemapACES(in vec4 c, float limit)
{
    float a = 2.51f;
    float b = 0.03f;
    float y = 2.43f;
    float d = 0.59f;
    float e = 0.14f;

    float luminance = 0.3*c.x + 0.6*c.y + 0.1*c.z;

    return clamp((c * (a * c + b)) / (c * (y * c + d) + e), 0.0, 1.0);
}

vec4 tonemap(in vec4 c, float limit)
{
    float luminance = 0.3*c.x + 0.6*c.y + 0.1*c.z;

    return c * 1.0 / (1.0 + luminance / limit);
}

vec4 chromaticAberration() {

    float offset = caDistance;

	vec4 chromColor = vec4(0.0, 0.0, 0.0, 0.0);
		
	float dist = 0 + (pow(distance(TexCoords, vec2(caP3)), caP1) * caP2);

    if (useCADistortion) {
	    chromColor.r = texture(pathTraceTexture, TexCoords + vec2(offset * dist)).r * invSampleCounter;
	    chromColor.g = texture(pathTraceTexture, TexCoords).g * invSampleCounter;
	    chromColor.b = texture(pathTraceTexture, TexCoords - vec2(offset * dist)).b * invSampleCounter;
    }
    else {
        offset = offset * 0.025;
        chromColor.r = texture(pathTraceTexture, TexCoords + (offset * caP2)).r * invSampleCounter;
	    chromColor.g = texture(pathTraceTexture, TexCoords).g * invSampleCounter;
	    chromColor.b = texture(pathTraceTexture, TexCoords - (offset * caP2)).b * invSampleCounter;
    }
    
	return chromColor;
}

void main()
{
    if (!isInPreview) {
        if (!useCA) color = texture(pathTraceTexture, TexCoords) * invSampleCounter;
        if (useCA) color = chromaticAberration();
    }
    else color = texture(pathTraceTexture, TexCoords) * invSampleCounter;
    
    if (useAces) {
        color = pow(tonemapACES(color, 1.5), vec4(1.0 / 2.2));
    }
    
    if (!useAces) {
        color = pow(tonemap(color, 1.5), vec4(1.0 / 2.2));
    }
}