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

// Fitted ACES Tonemapping
vec4 tonemapACES(in vec4 c)
{
    float a = 2.51f;
    float b = 0.03f;
    float y = 2.43f;
    float d = 0.59f;
    float e = 0.14f;

    return clamp((c * (a * c + b)) / (c * (y * c + d) + e), 0.0, 1.0);
}

// Reinhard tonemapping
vec4 tonemapReinhard(in vec4 c)
{
    return clamp(c / (c + 1), 0.0, 1.0);
}

// Hejl Richard tonemapping
vec4 toneMapHejlRichard(vec4 color)
{
    color = max(vec4(0.0), color - vec4(0.004));
    return (color*(6.2*color+.5))/(color*(6.2*color+1.7)+0.06);
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
    float a = 1.2295;
	float b = 0.3135;
	float y = 1.1935;
	float d = 0.4655;
	float e = 0.073;

	c = pow((c * (c * (a * c + b) + y * d) / (c * (y * c + d) + e)), vec4(1.7));
	c.rgb = pow(c.rgb, vec3(1.0/0.8));
	c *= 0.8;
	c = clamp(c, 0.0, 1.0);
	
	return c;
}

// Uncharted 2 Tonemapping
vec4 toneMapUncharted2(vec4 color)
{
    const float A = 0.15;
    const float B = 0.50;
    const float C = 0.10;
    const float D = 0.20;
    const float E = 0.02;
    const float F = 0.30;
    return ((color*(A*color+C*B)+D*E)/(color*(A*color+B)+D*F))-E/F;
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
        color = color;
    }

	// Case for linear tonemapping
    if (tonemapIndex == 1) {
        color = pow(tonemap(color, 2), vec4(1.0 / 2.2));
    }

    // Case for ACES tonemapping
    if (tonemapIndex == 2) {
        color = pow(tonemapACES(color), vec4(1.0 / 2.2));
    }

	// Case for Reinhard tonemapping
    if (tonemapIndex == 3) {
        color = pow(tonemapReinhard(color), vec4(1.0 / 2.2));
    }

    // Case for Kanjero tonemapping
    if (tonemapIndex == 4) {
        color = pow(tonemapKanjero(color), vec4(1.0 / 2.2));
    }

    // Case for linear tonemapping
    if (tonemapIndex == 5) {
        color = toneMapHejlRichard(color);
    }

    if (tonemapIndex == 6) {
        color = pow(toneMapUncharted2(color), vec4(1.0 / 2.2));
        color = color * 1.75;
    }

    // Apply vignette
    if (useVignette) {
        color = vignette(color, vignetteIntensity, vignettePower);
    }
}