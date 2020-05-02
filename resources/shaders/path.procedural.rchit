#version 460
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require
#extension GL_NV_ray_tracing : require
#include "material.glsl"

layout(binding = 3) readonly buffer VertexArray { float Vertices[]; };
layout(binding = 4) readonly buffer IndexArray { uint Indices[]; };
layout(binding = 5) readonly buffer MaterialArray { Material[] Materials; };
layout(binding = 6) readonly buffer OffsetArray { uvec2[] Offsets; };
layout(binding = 7) uniform sampler2D[] TextureSamplers;
layout(binding = 8) readonly buffer SphereArray { vec4[] Spheres; };

#include "scatter.glsl"
#include "vertex.glsl"

hitAttributeNV vec4 Sphere;
rayPayloadInNV PerRayData prd_;

const float pi = 3.1415926535897932384626433832795;

vec2 GetSphereTexCoord(const vec3 point) {
    const float phi = atan(point.x, point.z);
    const float theta = asin(point.y);
    const float pi = 3.1415926535897932384626433832795;

    return vec2 ((phi + pi) / (2* pi),
				 1 - (theta + pi /2) / pi);
}

void main() {
    // Get the material.
    const uvec2 offsets = Offsets[gl_InstanceCustomIndexNV];
    const uint indexOffset = offsets.x;
    const uint vertexOffset = offsets.y;
    const Vertex v0 = UnpackVertex(vertexOffset + Indices[indexOffset]);
    const Material material = Materials[v0.MaterialIndex];

    // Compute the ray hit point properties.
    const vec4 sphere = Spheres[gl_InstanceCustomIndexNV];
    const vec3 center = sphere.xyz;
    const float radius = sphere.w;
    const vec3 point = gl_WorldRayOriginNV + gl_HitTNV * gl_WorldRayDirectionNV;
    const vec3 normal = (point - center) / radius;
    const vec2 texCoord = GetSphereTexCoord(normal);

    prd_.origin = gl_WorldRayOriginNV + gl_WorldRayDirectionNV * gl_HitTNV; // FIXME

    vec3 diffuse_color = material.diffuse.rgb;
    prd_.attenuation = prd_.attenuation * diffuse_color / pi;

    if (material.shading_model == MaterialDiffuseLight) {
        prd_.radiance = material.diffuse.rgb;
        prd_.done = true;
        return;
    }

    prd_.direction = normal + RandomInUnitSphere(prd_.seed);
    prd_.radiance = vec3(0.0);
}
