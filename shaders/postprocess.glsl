/*
 * Read license.txt for license information.
 * This is based on the original GLSL-PathTracer by Asif Ali.
 */

#version 330

out vec4 color;
in vec2 TexCoords;

uniform sampler2D pathTraceTexture;
uniform bool isInPreview;
uniform float invSampleCounter;
uniform float caDistance;
uniform float caP1;
uniform float caP2;
uniform float caP3;
uniform int tonemapIndex;
uniform bool useCA;
uniform bool useCADistortion;
uniform bool useVignette;
uniform float vignetteIntensity;
uniform float vignettePower;

// Linear tonemapping
vec4 tonemap(in vec4 c, float limit)
{
    float luminance = 0.3*c.x + 0.6*c.y + 0.1*c.z;

    return c * 1.0 / (1.0 + luminance / limit);
}

// ACES Tonemapping
vec4 tonemapACES(in vec4 c, float limit)
{
    float a = 2.51f;
    float b = 0.03f;
    float y = 2.43f;
    float d = 0.59f;
    float e = 0.14f;

    return clamp((c * (a * c + b)) / (c * (y * c + d) + e), 0.0, 1.0);
}

// Reinhard tonemapping
vec4 tonemapReinhard(in vec4 c, float limit)
{
    return clamp(c / (c + limit), 0.0, 1.0);
}

// Filmic tonemapping
vec4 tonemapFilmic(in vec4 c, float limit)
{
    float a = 2.51f;
    float b = 0.03f;
    float y = 2.43f;
    float d = 0.59f;
    float e = 0.14f;

    return clamp((c * (a * c + b)) / (c * (y * c + d) + e), 0.0, 1.0);
}

// Reinhard tonemapping
vec4 tonemapReinhardFilmic(in vec4 c, float limit)
{
    float a = 2.51f;
    float b = 0.03f;
    float y = 2.43f;
    float d = 0.59f;
    float e = 0.14f;

    return clamp(c / (c + limit) * (c * (a * c + b) + y * d) / (c * (y * c + d) + e), 0.0, 1.0);
}

float gamma(float c, float gamma)
{
    return pow(c, 1.0 / gamma);
}

float linearToSRGB(float c)
{
    return c <= 0.0031308f ? 12.92f * c : 1.055f * pow(c, 1.0 / 2.4) - 0.055f;
}

vec4 linearToSRGB(vec4 c)
{
    return vec4(linearToSRGB(c.r), linearToSRGB(c.g), linearToSRGB(c.b), c.a);
}

// Kanjero tonemapping
vec4 tonemapKanjero(in vec4 c)
{	
	vec3 colorBuffer = c.rgb;
    float a = 1.1295;
	float b = 0.0135;
	float y = 1.0935;
	float d = 0.2655;
	float e = 0.063;

	c = pow((c * (c * (a * c + b) + y * d) / (c * (y * c + d) + e)), vec4(1.6, 1.6, 1.6, 1.0)) * 1.5;
	c.rgb = pow(c.rgb, vec3(1.0/0.9));
	c = clamp(c, 0.0, 1.0);
	
	return c;
}

vec4 chromaticAberration() {
    float offset = caDistance;

	vec4 colorBuffer = vec4(0.0, 0.0, 0.0, 0.0);
		
	float dist = 0 + (pow(distance(TexCoords, vec2(caP3)), caP1) * caP2);

    if (useCADistortion) {
	    colorBuffer.r = texture(pathTraceTexture, TexCoords + vec2(offset * dist)).r * invSampleCounter;
	    colorBuffer.g = texture(pathTraceTexture, TexCoords).g * invSampleCounter;
	    colorBuffer.b = texture(pathTraceTexture, TexCoords - vec2(offset * dist)).b * invSampleCounter;
    }
    else {
        offset = offset * 0.025;
        colorBuffer.r = texture(pathTraceTexture, TexCoords + (offset * caP2)).r * invSampleCounter;
	    colorBuffer.g = texture(pathTraceTexture, TexCoords).g * invSampleCounter;
	    colorBuffer.b = texture(pathTraceTexture, TexCoords - (offset * caP2)).b * invSampleCounter;
    }
    
	return colorBuffer;
}

// Vignette, intensity and sharpness are adjustable. Sharpness adjusts the shape/falloff of the vignette.
vec4 vignette(vec4 c, float intensity, float sharpness) {
	float dist = 1.0 - pow(distance(TexCoords, vec2(0.5)), sharpness) * intensity;
	return c * dist;
}

void main()
{
    if (!isInPreview) {
        if (!useCA) color = texture(pathTraceTexture, TexCoords) * invSampleCounter;
        if (useCA) color = chromaticAberration();
    }
    else color = texture(pathTraceTexture, TexCoords) * invSampleCounter;
    
    if (tonemapIndex == 0) {
        color = pow(tonemap(color, 2), vec4(1.0 / 2.2));
    }

    if (tonemapIndex == 1) {
        color = pow(tonemapACES(color, 1.5), vec4(1.0 / 2.2));
    }

    if (tonemapIndex == 2) {
        color = pow(tonemapReinhard(color, 2), vec4(1.0 / 2.2));
    }

    if (tonemapIndex == 3) {
        color = pow(tonemapKanjero(color), vec4(1.0 / 2.2));
    }

    // Apply vignette
    if (useVignette) {
        color = vignette(color, vignetteIntensity, vignettePower);
    }
}