//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************

#define MaxLights 16
static const float PI = 3.14159265f;
static const float3 Fdielectric = 0.04;
static const float Epsilon = 0.00001;

#ifndef NUM_DIR_LIGHTS
#define NUM_DIR_LIGHTS 3
#endif

#ifndef NUM_POINT_LIGHTS
#define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
#define NUM_SPOT_LIGHTS 0
#endif

struct Light
{
    float3 Strength;
    float FalloffStart; // point/spot light only
    float3 Direction;   // directional/spot light only
    float FalloffEnd;   // point/spot light only
    float3 Position;    // point light only
    float SpotPower;    // spot light only
};

// probably not necessary as we want to store material data in textures
struct MaterialData
{
    float4   DiffuseAlbedo;
    float3   FresnelR0;
    float    Roughness;
    float4x4 MatTransform;
    uint     DiffuseMapIndex;
    uint     MatPad0;
    uint     MatPad1;
    uint     MatPad2;
};

StructuredBuffer<MaterialData> gMaterialData : register(t0, space1);

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorld;
	float4x4 gTexTransform;
	uint gMaterialIndex;
    int gTexturesUsed; // contains the flags that tells us what textures the model uses to allow us to index into the gTextures array
    uint gObjPad0;
	uint gObjPad1;
};

cbuffer cbPass : register(b1)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float3 gEyePosW;
    float cbPerObjectPad1;
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
    float4 gAmbientLight;

    // Indices [0, NUM_DIR_LIGHTS) are directional lights;
    // indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
    // indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
    // are spot lights for a maximum of MaxLights per object.
    Light gLights[MaxLights];
};

#define NUM_TEXTURES 4
Texture2D gTextures[NUM_TEXTURES] : register(t0);

SamplerState gsamPointWrap        : register(s0);
SamplerState gsamPointClamp       : register(s1);
SamplerState gsamLinearWrap       : register(s2);
SamplerState gsamLinearClamp      : register(s3);
SamplerState gsamAnisotropicWrap  : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);


struct VertexIn
{
	float3 position  : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float3 bitangent : BITANGENT;
    float2 tex_coord : TEXCOORD;
};

struct VertexOut
{
    float4 pixel_position    : SV_POSITION;
    float3 position    : POSITION;
    float2 tex_coord    : TEXCOORD;
    float3x3 tangent_basis : TBASIS;

};

VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut)0.0f;
    vout.position = mul(float4(vin.position, 1.0f), gWorld).xyz;
    vout.tex_coord = mul(float4(vin.tex_coord, 0.0f, 1.0f), gTexTransform); // can straight set it because we flip the v axis when inputting uv textures
    float3x3 tbn = float3x3(vin.tangent, vin.bitangent, vin.normal);
    vout.tangent_basis = mul(tbn, (float3x3)gWorld); //only use 3x3 to drop translation
    vout.pixel_position = mul(float4(vout.position, 1.0f), gViewProj); // post multiply because gViewProj is column matrix
    return vout;
}

float ndf_ggx(float cos_lh, float roughness) { // uses the GGX normal distribution function
    float alpha = roughness * roughness;
    float alpha_sq = alpha * alpha;
    float denom = (cos_lh * cos_lh) * (alpha_sq - 1.0f) + 1.0f;
    return alpha_sq / (PI * denom * denom);
}
float ga_schlick_g1(float cos_theta, float k) {
    return cos_theta / (cos_theta * (1.0f - k) + k);
}
float ga_schlick_ggx(float cos_li, float cos_lo, float roughness) { // schlick ggx approx of geometric attentuation function using smith's method
    float r = roughness + 1.0f;
    float k = (r * r) / 8.0f;
    return ga_schlick_g1(cos_li, k) * ga_schlick_g1(cos_lo, k);
}
float fresnel_schlick(float3 f0, float cos_theta) {
    return f0 + (1.0f - f0) * pow(1.0f - cos_theta, 5.0);
}

#define COLOR_INDEX 0
#define NORMAL_INDEX 1
#define ROUGHNESS_INDEX 2
#define METALLIC_INDEX 3
#define HEIGHT_INDEX 4

float4 PS(VertexOut pin) : SV_Target
{
    int color_used = (gTexturesUsed >> COLOR_INDEX) & 0x1;
    int normal_used = (gTexturesUsed >> NORMAL_INDEX) & 0x1;
    int roughness_used = (gTexturesUsed >> ROUGHNESS_INDEX) & 0x1;
    int metallic_used = (gTexturesUsed >> METALLIC_INDEX) & 0x1;
    int height_used = (gTexturesUsed >> HEIGHT_INDEX) & 0x1;
    
    // shading model parameters at the pixel
    
    float3 view = normalize(pin.position - gEyePosW);
    float2 tex = pin.tex_coord;
    float height_scale = 0.001f;
    if (height_used) {
        float height = gTextures[HEIGHT_INDEX].Sample(gsamAnisotropicWrap, tex).r;
        float2 p = view.xy * (height * height_scale);
        tex = tex - p;
    }
    float3 albedo = color_used * gTextures[COLOR_INDEX].Sample(gsamAnisotropicWrap, tex).rgb;
    float metalness = metallic_used * gTextures[METALLIC_INDEX].Sample(gsamAnisotropicWrap, tex).r;
    float roughness = roughness_used * gTextures[ROUGHNESS_INDEX].Sample(gsamAnisotropicWrap, tex).r;
    float3 normal_sample = normal_used * gTextures[NORMAL_INDEX].Sample(gsamAnisotropicWrap, tex).rgb;

    float3 lo = normalize(gEyePosW - pin.position); // outgoing light vector from surface position to eye

    float3 normal = normal_used * normalize(2.0f * normal_sample - 1.0f);
    normal = normalize(mul(normal, pin.tangent_basis)); // convert normal to be in tangent space

    // angle between the surface normal and outgoing light direction
    // dot alculated the cos for us because the magnitude of the vectors are both 1
    float cos_lo = max(0.0, dot(normal, lo));

    // specular reflection vector
    float lr = 2.0f * cos_lo * normal - lo;

    // fresnel reflectange at normal incidence (for metals use albedo color)
    float3 f0 = lerp(Fdielectric, albedo, metalness); 

    float3 direct_lighting = { 0.0f, 0.0f, 0.0f };

    for (uint i = 0; i < 1 /*NUM_DIR_LIGHTS + NUM_POINT_LIGHTS + NUM_SPOT_LIGHTS*/; i++) { // so far only have 3 directional lights
        float3 li = -gLights[i].Direction; // vector of incoming light from the surface
        float3 l_radiance = gLights[i].Strength;

        float3 lh = normalize(li + lo); // half angle betwen incoming and outcoming light

        float cos_li = max(0.0f, dot(normal, li));
        float cos_lh = max(0.0f, dot(normal, lh));

        float3 f = fresnel_schlick(f0, max(0.0f, dot(lh, lo)));
        float d = ndf_ggx(cos_lh, roughness);
        float g = ga_schlick_ggx(cos_li, cos_lo, roughness);
        float3 kd = lerp(float3(1, 1, 1) - f, float3(0, 0, 0), metalness);

        float3 diffuse_bdrf = kd * albedo;
        float3 specular_bdrf = (f * d * g) / max(Epsilon, 4.0 * cos_li * cos_lo);
        direct_lighting += (diffuse_bdrf + specular_bdrf) * l_radiance * cos_li;
    }
    // return float4(albedo, 1.0f);
    return float4(direct_lighting + gAmbientLight, 1.0f);
}

