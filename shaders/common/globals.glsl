/*
 * Read license.txt for license information.
 * This is based on the original GLSL-PathTracer by Asif Ali.
 */

#define PI        3.14159265358979323
#define TWO_PI    6.28318530717958648
#define INFINITY  1000000.0
#define EPS 0.001

#define QUAD_LIGHT 0
#define SPHERE_LIGHT 1
#define DISTANT_LIGHT 2

mat4 transform;
vec3 tempTexCoords;

struct Ray
{
    vec3 origin;
    vec3 direction;
};

struct Material
{
    vec3 albedo;
    float specular;
    vec3 emission;
    float anisotropic;
    float metallic;
    float roughness;
    float subsurface;
    float specularTint;
    float sheen;
    float sheenTint;
    float clearcoat;
    float clearcoatRoughness;
    float specTrans;
    float ior;
    float atDistance;
    vec3 extinction;
    vec4 texIDs;
    // Roughness calculated from anisotropic parameter
    float ax;
    float ay;
};

struct Camera
{
    vec3 up;
    vec3 right;
    vec3 forward;
    vec3 position;
    float fov;
    float focalDist;
    float aperture;
    bool chromaticAberration;
};

struct Light
{
    vec3 position;
    vec3 emission;
    vec3 u;
    vec3 v;
    float radius;
    float area;
    float type;
};

struct State
{
    int depth;
    float eta;
    float hitDist;

    vec3 fhp;
    vec3 normal;
    vec3 ffnormal;
    vec3 tangent;
    vec3 bitangent;

    bool isEmitter;

    vec2 texCoord;
    vec3 bary;
    ivec3 triID;
    int matID;
    Material mat;
};

struct BsdfSampleRec
{
    vec3 L;
    vec3 f;
    float pdf;
};

struct LightSampleRec
{
    vec3 normal;
    vec3 emission;
    vec3 direction;
    float dist;
    float pdf;
};

uniform Camera camera;

//RNG from code by Moroz Mykhailo (https://www.shadertoy.com/view/wltcRS)

//internal RNG state 
uvec4 seed;
ivec2 pixel;

void InitRNG(vec2 p, int frame)
{
    pixel = ivec2(p);
    seed = uvec4(p, uint(frame), uint(p.x) + uint(p.y));
}

void pcg4d(inout uvec4 v)
{
    v = v * 1664525u + 1013904223u;
    v.x += v.y * v.w; v.y += v.z * v.x; v.z += v.x * v.y; v.w += v.y * v.z;
    v = v ^ (v >> 16u);
    v.x += v.y * v.w; v.y += v.z * v.x; v.z += v.x * v.y; v.w += v.y * v.z;
}

float rand()
{
    pcg4d(seed); return float(seed.x) / float(0xffffffffu);
}

vec3 FaceForward(vec3 a, vec3 b)
{
    return dot(a, b) < 0.0 ? -b : b;
}