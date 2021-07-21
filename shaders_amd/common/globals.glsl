/*
 * Read license.txt for license information.
 * This is based on the original GLSL-PathTracer by Asif Ali.
 */

#define PI        3.14159265358979323
#define TWO_PI    6.28318530717958648
#define INFINITY  1000000.0
#define EPS 0.0001

#define REFL 0
#define REFR 1
#define SUBS 2

mat4 transform;

vec2 seed;
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
    vec3 extinction;
    vec3 texIDs;
    // Roughness calculated from anisotropic
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
    bool specularBounce;
    bool isSubsurface;

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
    vec3 surfacePos;
    vec3 normal;
    vec3 emission;
    float pdf;
};

uniform Camera camera;

float rand()
{
    seed -= randomVector.xy;
    return fract(sin(dot(seed.xy,vec2(12.9898,78.233)))*43758.5453123);
}

vec3 FaceForward(vec3 a, vec3 b)
{
    return dot(a, b) < 0.0 ? -b : b;
}