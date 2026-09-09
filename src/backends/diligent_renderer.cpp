#include <gloom/backends/diligent_renderer.hpp>
#include <gloom/render/render_graph.hpp>
#include <gloom/render/lighting.hpp>
#include <gloom/render/temporal.hpp>
#include <gloom/render/ui.hpp>

#include <BasicMath.hpp>
#include <BasicPlatformDebug.hpp>
#include <EngineFactoryVk.h>
#include <GraphicsTypes.h>
#include <MapHelper.hpp>
#include <Query.h>
#include <RefCntAutoPtr.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <deque>
#include <fstream>
#include <cstring>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace gloom::backends {
namespace {

constexpr char vertex_shader_source[] = R"(
cbuffer Constants
{
    float4x4 WorldViewProjection;
    float4x4 PreviousWorldViewProjection;
    float4x4 World;
    float4x4 NormalWorld;
    float4 TemporalJitter;
    float4 InstanceColor;
    float4 BaseColor;
    float4 MaterialParameters;
    float4 Emissive;
    float4 CameraExposure;
    float4 CameraForward;
    float4 DirectionalDirectionIntensity;
    float4 DirectionalColor;
    float4 EnvironmentSky;
    float4 EnvironmentGround;
    uint4 ClusterDimensions;
    float4 ClusterDepthViewport;
    float4x4 ShadowMatrices[4];
    float4 ShadowSplits;
    float4 SurfaceParameters;
    float4 SpecularColorRotation;
    float4 SurfaceAnimation;
    float4 SurfaceFlags;
    float4 TextureMappings[20];
    float4 SkinFlags;
    float4 ParticleDepth;
};
cbuffer SkinConstants {
    float4x4 Bones[256];
    float4x4 PreviousBones[256];
    float4x4 BoneNormals[256];
};
struct VSInput { float3 position : ATTRIB0; float3 normal : ATTRIB1; float2 uv : ATTRIB2; float4 tangent : ATTRIB3; float2 uv1 : ATTRIB4; uint4 joints0 : ATTRIB5; uint4 joints1 : ATTRIB6; float4 weights0 : ATTRIB7; float4 weights1 : ATTRIB8; };
struct PSInput { float4 position : SV_POSITION; float3 normal : NORMAL; float4 tangent : TANGENT; float4 color : COLOR; float2 uv : TEX_COORD; float2 uv1 : TEXCOORD3; float3 world_position : WORLD_POSITION; float4 current_clip : TEXCOORD1; float4 previous_clip : TEXCOORD2; };
void main(in VSInput input, out PSInput output)
{
    float4 position=float4(input.position,1), previous=position;
    float3 normal=input.normal, tangent=input.tangent.xyz;
    if (SkinFlags.x>0.5) {
        position=0;previous=0;normal=0;tangent=0;
        [unroll] for (uint i=0;i<8;++i) {
            uint j=i<4?input.joints0[i]:input.joints1[i-4];
            float w=i<4?input.weights0[i]:input.weights1[i-4];
            if (w>0 && j<(uint)SkinFlags.x) {
                position+=mul(float4(input.position,1),Bones[j])*w;
                previous+=mul(float4(input.position,1),PreviousBones[j])*w;
                normal+=mul(float4(input.normal,0),BoneNormals[j]).xyz*w;
                tangent+=mul(float4(input.tangent.xyz,0),Bones[j]).xyz*w;
            }
        }
    }
    output.position = mul(position, WorldViewProjection);
    output.position.xy += TemporalJitter.xy * output.position.w;
    output.current_clip = output.position;
    output.previous_clip = mul(previous, PreviousWorldViewProjection);
    output.previous_clip.xy += TemporalJitter.zw * output.previous_clip.w;
    output.normal = normalize(mul(float4(normal, 0.0), NormalWorld).xyz);
    output.tangent = float4(normalize(mul(float4(tangent, 0.0), World).xyz), input.tangent.w * MaterialParameters.z);
    output.color = InstanceColor * BaseColor;
    output.uv = input.uv;
    output.uv1 = input.uv1;
    output.world_position = mul(position, World).xyz;
}
)";

constexpr char pixel_shader_source[] = R"(
struct PSInput { float4 position : SV_POSITION; float3 normal : NORMAL; float4 tangent : TANGENT; float4 color : COLOR; float2 uv : TEX_COORD; float2 uv1 : TEXCOORD3; float3 world_position : WORLD_POSITION; float4 current_clip : TEXCOORD1; float4 previous_clip : TEXCOORD2; };
cbuffer Constants
{
    float4x4 WorldViewProjection;
    float4x4 PreviousWorldViewProjection;
    float4x4 World;
    float4x4 NormalWorld;
    float4 TemporalJitter;
    float4 InstanceColor;
    float4 BaseColor;
    float4 MaterialParameters;
    float4 Emissive;
    float4 CameraExposure;
    float4 CameraForward;
    float4 DirectionalDirectionIntensity;
    float4 DirectionalColor;
    float4 EnvironmentSky;
    float4 EnvironmentGround;
    uint4 ClusterDimensions;
    float4 ClusterDepthViewport;
    float4x4 ShadowMatrices[4];
    float4 ShadowSplits;
    float4 SurfaceParameters;
    float4 SpecularColorRotation;
    float4 SurfaceAnimation;
    float4 SurfaceFlags;
    float4 TextureMappings[20];
    float4 SkinFlags;
    float4 ParticleDepth;
};
struct PointLightData { float4 PositionRange; float4 ColorIntensity; };
StructuredBuffer<PointLightData> PointLights;
StructuredBuffer<uint2> LightClusters;
StructuredBuffer<uint> LightIndices;
TextureCube EnvironmentTexture;
SamplerState EnvironmentTexture_sampler;
Texture2DArray ShadowMap;

Texture2D SceneColorCopy;
Texture2D SceneDepthCopy;
SamplerState SceneColorCopy_sampler;
Texture2D BaseColorTexture;
SamplerState BaseColorTexture_sampler;
Texture2D MetallicRoughnessTexture;
SamplerState MetallicRoughnessTexture_sampler;
Texture2D NormalTexture;
SamplerState NormalTexture_sampler;
Texture2D EmissiveTexture;
Texture2D OcclusionTexture;
Texture2D SpecularTexture;
Texture2D SpecularColorTexture;
Texture2D AnisotropyTexture;
Texture2D DetailTexture;
Texture2D LightmapTexture;
float2 mapped_uv(PSInput input, uint slot) {
    float4 transform=TextureMappings[slot*2];
    float4 rotation=TextureMappings[slot*2+1];
    float2 uv=rotation.z>0.5 ? input.uv1 : input.uv;
    uv*=transform.xy;
    uv=float2(rotation.x*uv.x-rotation.y*uv.y,rotation.y*uv.x+rotation.x*uv.y);
    uv+=transform.zw+SurfaceAnimation.xy*SurfaceFlags.z;
    if (SurfaceAnimation.z>0.0) uv+=sin(uv.yx*6.2831853+SurfaceFlags.z)*SurfaceAnimation.z;
    return uv;
}
float3 brdf(float3 N,float3 T,float3 B,float3 V,float3 L,float3 albedo,float metallic,
            float roughness,float3 f0,float anisotropy) {
    float3 H=normalize(L+V);
    float nl=saturate(dot(N,L)), nv=max(saturate(dot(N,V)),0.001);
    float nh=saturate(dot(N,H)), vh=saturate(dot(V,H));
    float alpha=roughness*roughness, a2=alpha*alpha;
    float denominator=nh*nh*(a2-1.0)+1.0;
    float D=a2/max(3.14159265*denominator*denominator,0.000001);
    float k=(roughness+1.0)*(roughness+1.0)/8.0;
    float visibility=rcp(max(4.0*(nv*(1-k)+k)*(nl*(1-k)+k),0.000001));
    if (anisotropy>0.0) {
        float at=lerp(alpha,1.0,anisotropy*anisotropy), ab=alpha;
        float3 f=float3(ab*dot(T,H),at*dot(B,H),at*ab*nh);
        float w2=at*ab/max(dot(f,f),0.000001);
        D=at*ab*w2*w2/3.14159265;
        float gv=nl*length(float3(at*dot(T,V),ab*dot(B,V),nv));
        float gl=nv*length(float3(at*dot(T,L),ab*dot(B,L),nl));
        visibility=0.5/max(gv+gl,0.000001);
    }
    float3 F=f0+(1-f0)*pow(1-vh,5);
    return ((1-F)*(1-metallic)*albedo/3.14159265+D*visibility*F)*nl;
}
float cascade_shadow(float3 world_position, uint cascade, float ndotl) {
    const float4 clip = mul(float4(world_position, 1.0), ShadowMatrices[cascade]);
    const float3 ndc = clip.xyz / max(clip.w, 0.0001);
    const float2 uv = float2(ndc.x * 0.5 + 0.5, 0.5 - ndc.y * 0.5);
    if (any(uv < 0.0) || any(uv > 1.0) || ndc.z < 0.0 || ndc.z > 1.0) return 1.0;
    uint width, height, layers;
    ShadowMap.GetDimensions(width, height, layers);
    const int2 center = int2(uv * float2(width, height));
    const float bias = max(0.00012, 0.0005 * (1.0 - ndotl));
    float visibility = 0.0;
    [unroll] for (int y = -1; y <= 1; ++y) {
        [unroll] for (int x = -1; x <= 1; ++x) {
            const int2 texel = clamp(center + int2(x, y), int2(0, 0), int2(width - 1, height - 1));
            const float depth = ShadowMap.Load(int4(texel, cascade, 0)).r;
            visibility += ndc.z - bias <= depth ? 1.0 : 0.0;
        }
    }
    return visibility / 9.0;
}
struct PSOutput { float4 color : SV_TARGET0; float2 motion : SV_TARGET1; float depth : SV_TARGET2; };
PSOutput main(in PSInput input, bool front_face : SV_IsFrontFace)
{
    if (SkinFlags.y>0.5) {
        PSOutput p;
        float2 uv=saturate(input.uv);
        float4 tex=BaseColorTexture.Sample(BaseColorTexture_sampler,uv);
        // Original additive flare textures already encode intensity in RGB.
        // Applying their matching alpha again would square and erase the halo.
        float alpha=(SurfaceFlags.x==3.0?1.0:tex.a)*InstanceColor.a;
        float2 screen=input.position.xy*ParticleDepth.zw;
        float z=SceneDepthCopy.SampleLevel(SceneColorCopy_sampler,screen,0).r;
        float linearScene=ParticleDepth.x*ParticleDepth.y/max(ParticleDepth.y-z*(ParticleDepth.y-ParticleDepth.x),0.0001);
        float linearParticle=ParticleDepth.x*ParticleDepth.y/max(ParticleDepth.y-input.position.z*(ParticleDepth.y-ParticleDepth.x),0.0001);
        if (SkinFlags.w>0) alpha*=saturate((linearScene-linearParticle)/SkinFlags.w);
        float3 rgb=tex.rgb*InstanceColor.rgb;
        if (SkinFlags.z>0) {
            float edge=saturate(1-length(uv*2-1));edge*=edge;
            float2 flow=NormalTexture.Sample(NormalTexture_sampler,uv).rg;
            float2 offset=(flow*2-1)*SkinFlags.z*edge;
            float2 warped=clamp(screen+offset,ParticleDepth.zw*.5,1-ParticleDepth.zw*.5);
            float targetDepth=SceneDepthCopy.SampleLevel(SceneColorCopy_sampler,warped,0).r;
            if (targetDepth<input.position.z) warped=screen;
            rgb=SceneColorCopy.SampleLevel(SceneColorCopy_sampler,warped,0).rgb;
            alpha*=edge;
        }
        p.color=float4(rgb,alpha);p.motion=0;p.depth=input.position.z;
        return p;
    }
    float4 surface_color = input.color * BaseColorTexture.Sample(BaseColorTexture_sampler,mapped_uv(input,0));
    if (SurfaceFlags.x==1.0) clip(surface_color.a-SurfaceAnimation.w);
    surface_color.rgb *= DetailTexture.Sample(BaseColorTexture_sampler,mapped_uv(input,8)).rgb;
    float3 geometric_normal = normalize(input.normal);
    if (SurfaceFlags.y>0.5 && !front_face) geometric_normal=-geometric_normal;
    float3 tangent = input.tangent.xyz - geometric_normal * dot(input.tangent.xyz, geometric_normal);
    if (dot(tangent,tangent)<0.000001) tangent=cross(abs(geometric_normal.y)<0.95?float3(0,1,0):float3(1,0,0),geometric_normal);
    tangent=normalize(tangent);
    float3 bitangent=normalize(cross(geometric_normal,tangent))*input.tangent.w;
    float2 normal_xy=(NormalTexture.Sample(NormalTexture_sampler,mapped_uv(input,2)).xy*2-1)*SurfaceParameters.x;
    float3 sampled_normal=float3(normal_xy,sqrt(saturate(1-dot(normal_xy,normal_xy))));
    float3 N=normalize(tangent*sampled_normal.x+bitangent*sampled_normal.y+geometric_normal*sampled_normal.z);
    float3 L=normalize(-DirectionalDirectionIntensity.xyz);
    float3 V=normalize(CameraExposure.xyz-input.world_position);
    float4 packed_material=MetallicRoughnessTexture.Sample(MetallicRoughnessTexture_sampler,mapped_uv(input,1));
    float metallic=saturate(MaterialParameters.x*packed_material.b);
    float roughness=clamp(MaterialParameters.y*packed_material.g,0.04,1.0);
    float weight=SurfaceParameters.z*SpecularTexture.Sample(BaseColorTexture_sampler,mapped_uv(input,5)).a;
    float3 specular_color=SpecularColorRotation.rgb*SpecularColorTexture.Sample(BaseColorTexture_sampler,mapped_uv(input,6)).rgb;
    float3 F0=lerp(min(0.04*specular_color,1.0)*weight,surface_color.rgb,metallic);
    float3 anisotropy_map=AnisotropyTexture.Sample(BaseColorTexture_sampler,mapped_uv(input,7)).rgb;
    float strength=SurfaceParameters.w*anisotropy_map.b;
    float2 direction=SurfaceFlags.w>0.5 ? anisotropy_map.rg*2-1 : float2(1,0);
    direction=dot(direction,direction)>0.000001 ? normalize(direction) : float2(1,0);
    float c=cos(SpecularColorRotation.w), ss=sin(SpecularColorRotation.w);
    direction=float2(c*direction.x-ss*direction.y,ss*direction.x+c*direction.y);
    float3 T=normalize(tangent*direction.x+bitangent*direction.y);
    T=T-N*dot(T,N);
    if (dot(T,T)<0.000001) T=cross(abs(N.y)<0.95?float3(0,1,0):float3(1,0,0),N);
    T=normalize(T);
    float3 B=normalize(cross(N,T));
    float NdotL=saturate(dot(N,L));
    const float camera_distance = dot(input.world_position - CameraExposure.xyz, CameraForward.xyz);
    const uint cascade = camera_distance < ShadowSplits.x ? 0 :
                         camera_distance < ShadowSplits.y ? 1 :
                         camera_distance < ShadowSplits.z ? 2 : 3;
    float shadow = 1.0;
    if (MaterialParameters.w < 0.5 && camera_distance <= ShadowSplits.w) {
        shadow = cascade_shadow(input.world_position, cascade, NdotL);
        // Blend only the outer tenth of each cascade to avoid a visible seam.
        if (cascade < 3) {
            const float previous_split = cascade == 0 ? 0.0 : ShadowSplits[cascade - 1];
            const float blend_width = (ShadowSplits[cascade] - previous_split) * 0.1;
            const float blend = saturate((camera_distance - ShadowSplits[cascade] + blend_width) / blend_width);
            if (blend > 0.0) shadow = lerp(shadow, cascade_shadow(input.world_position, cascade + 1, NdotL), blend);
        }
    }
    float3 direct=brdf(N,T,B,V,L,surface_color.rgb,metallic,roughness,F0,strength)*
                  DirectionalDirectionIntensity.w*DirectionalColor.rgb*shadow;
    if (ClusterDimensions.w > 0)
    {
        const uint2 tile = min(uint2(input.position.xy / ClusterDepthViewport.zw *
                                     ClusterDimensions.xy), ClusterDimensions.xy - 1);
        const float distance_to_camera = max(length(input.world_position - CameraExposure.xyz),
                                             ClusterDepthViewport.x);
        const uint slice = min((uint)(log(distance_to_camera / ClusterDepthViewport.x) /
                                      log(ClusterDepthViewport.y / ClusterDepthViewport.x) *
                                      ClusterDimensions.z), ClusterDimensions.z - 1);
        const uint cluster_index = tile.x + tile.y * ClusterDimensions.x +
                                   slice * ClusterDimensions.x * ClusterDimensions.y;
        const uint2 cluster = LightClusters[cluster_index];
        for (uint light_offset = 0; light_offset < cluster.y; ++light_offset)
        {
            const PointLightData light = PointLights[LightIndices[cluster.x + light_offset]];
            const float3 light_vector = light.PositionRange.xyz - input.world_position;
            const float light_distance = length(light_vector);
            const float3 point_direction = light_vector / max(light_distance, 0.001);
            const float attenuation = pow(saturate(1.0 - light_distance / light.PositionRange.w), 2.0);
            direct += brdf(N,T,B,V,point_direction,surface_color.rgb,metallic,roughness,F0,strength)*
                      attenuation*light.ColorIntensity.w*light.ColorIntensity.rgb;
        }
    }
    const float sky_amount = saturate(N.y * 0.5 + 0.5);
    const float3 environment_tint = lerp(EnvironmentGround.rgb, EnvironmentSky.rgb, sky_amount);
    const float3 irradiance = EnvironmentTexture.SampleLevel(EnvironmentTexture_sampler, N, EnvironmentGround.w>0 ? EnvironmentGround.w : 0).rgb *
                              (EnvironmentGround.w>0 ? float3(1,1,1) : environment_tint) * EnvironmentSky.w;
    const float3 reflected = EnvironmentTexture.SampleLevel(EnvironmentTexture_sampler,
                                                        reflect(-V, N), roughness * EnvironmentGround.w).rgb *
                             (EnvironmentGround.w>0 ? float3(1,1,1) : EnvironmentSky.rgb) * EnvironmentSky.w;
    const float3 ambient = irradiance * surface_color.rgb * (1.0 - metallic) +
                           reflected * F0 * (1.0 - roughness);
    PSOutput output;
    float ao=lerp(1.0,OcclusionTexture.Sample(BaseColorTexture_sampler,mapped_uv(input,4)).r,SurfaceParameters.y);
    float3 lightmap=LightmapTexture.Sample(BaseColorTexture_sampler,mapped_uv(input,9)).rgb;
    float3 emission=Emissive.rgb*EmissiveTexture.Sample(BaseColorTexture_sampler,mapped_uv(input,3)).rgb;
    const float3 fill = environment_tint * DirectionalColor.w * surface_color.rgb;
    output.color = float4(ambient*ao*lightmap + fill*ao + direct + emission, surface_color.a);
    const float2 current_ndc = input.current_clip.xy / max(input.current_clip.w, 0.0001);
    const float2 previous_ndc = input.previous_clip.xy / max(input.previous_clip.w, 0.0001);
    output.motion = (current_ndc - previous_ndc) * float2(0.5, -0.5);
    output.depth = input.position.z;
    return output;
}
)";

constexpr char shadow_vertex_shader_source[] = R"(
cbuffer SkinConstants {
    float4x4 Bones[256];
    float4x4 PreviousBones[256];
    float4x4 BoneNormals[256];
};
cbuffer ShadowConstants { float4x4 ShadowWorldViewProjection; float4 ShadowMapping; float4 ShadowRotation; float4 ShadowAlpha; float4 SkinFlags; };
struct VSInput { float3 position : ATTRIB0; float2 uv : ATTRIB2; float2 uv1 : ATTRIB4; uint4 joints0 : ATTRIB5; uint4 joints1 : ATTRIB6; float4 weights0 : ATTRIB7; float4 weights1 : ATTRIB8; };
struct PSInput { float4 position : SV_POSITION; float2 uv : TEXCOORD0; };
PSInput main(in VSInput input) {
    PSInput output;
    float4 position=float4(input.position,1);
    if (SkinFlags.x>0.5) {
        position=0;
        [unroll] for (uint i=0;i<8;++i) {
            uint j=i<4?input.joints0[i]:input.joints1[i-4];
            float w=i<4?input.weights0[i]:input.weights1[i-4];
            if (w>0 && j<(uint)SkinFlags.x) position+=mul(float4(input.position,1),Bones[j])*w;
        }
    }
    output.position=mul(position,ShadowWorldViewProjection);
    float2 uv=(ShadowRotation.z>0.5?input.uv1:input.uv)*ShadowMapping.xy;
    output.uv=float2(ShadowRotation.x*uv.x-ShadowRotation.y*uv.y,ShadowRotation.y*uv.x+ShadowRotation.x*uv.y)+ShadowMapping.zw;
    return output;
}
)";

constexpr char shadow_pixel_shader_source[] = R"(
cbuffer ShadowConstants { float4x4 ShadowWorldViewProjection; float4 ShadowMapping; float4 ShadowRotation; float4 ShadowAlpha; float4 SkinFlags; };
Texture2D ShadowBaseColor; SamplerState ShadowBaseColor_sampler;
void main(float4 position : SV_POSITION, float2 uv : TEXCOORD0) {
    if (ShadowAlpha.x>0.5) clip(ShadowBaseColor.Sample(ShadowBaseColor_sampler,uv).a*ShadowAlpha.z-ShadowAlpha.y);
}
)";

constexpr char tone_map_vertex_shader_source[] = R"(
struct PSInput { float4 position : SV_POSITION; float2 uv : TEX_COORD; };
void main(uint vertex_id : SV_VertexID, out PSInput output)
{
    const float2 position = float2((vertex_id << 1) & 2, vertex_id & 2);
    output.uv = float2(position.x, 1.0 - position.y);
    output.position = float4(position * 2.0 - 1.0, 0.0, 1.0);
}
)";

constexpr char tone_map_pixel_shader_source[] = R"(
Texture2D HdrTexture;
SamplerState HdrTexture_sampler;
cbuffer ToneConstants { float4 ToneParameters; };
float4 main(float4 position : SV_POSITION, float2 uv : TEX_COORD) : SV_TARGET
{
    const float2 texel = ToneParameters.yz;
    const float3 center = HdrTexture.Sample(HdrTexture_sampler, uv).rgb;
    const float3 neighborhood =
        (HdrTexture.Sample(HdrTexture_sampler, saturate(uv + float2(texel.x, 0.0))).rgb +
         HdrTexture.Sample(HdrTexture_sampler, saturate(uv - float2(texel.x, 0.0))).rgb +
         HdrTexture.Sample(HdrTexture_sampler, saturate(uv + float2(0.0, texel.y))).rgb +
         HdrTexture.Sample(HdrTexture_sampler, saturate(uv - float2(0.0, texel.y))).rgb) * 0.25;
    const float luminance = dot(center, float3(0.2126, 0.7152, 0.0722));
    const float contrast_limiter = rcp(1.0 + luminance);
    float3 color = max(center + (center - neighborhood) *
                                ToneParameters.w * contrast_limiter,
                       0.0) * ToneParameters.x;
    float3 bloom=0;
    [unroll] for (int radius=2;radius<=8;radius+=3) {
        [unroll] for (int direction=0;direction<4;++direction) {
            float2 offset=direction==0?float2(radius,0):direction==1?float2(-radius,0):direction==2?float2(0,radius):float2(0,-radius);
            bloom+=max(HdrTexture.Sample(HdrTexture_sampler,saturate(uv+offset*texel)).rgb-1.0,0.0);
        }
    }
    color+=bloom*(0.12/12.0)*ToneParameters.x;
    const float3 a = color * (2.51 * color + 0.03);
    const float3 b = color * (2.43 * color + 0.59) + 0.14;
    color = saturate(a / b);
    return float4(color, 1.0);
}
)";

constexpr char temporal_pixel_shader_source[] = R"(
Texture2D CurrentColor;
SamplerState CurrentColor_sampler;
Texture2D MotionVectors;
SamplerState MotionVectors_sampler;
Texture2D HistoryColor;
SamplerState HistoryColor_sampler;
Texture2D CurrentDepth;
SamplerState CurrentDepth_sampler;
Texture2D HistoryDepth;
SamplerState HistoryDepth_sampler;
cbuffer TemporalConstants { float4 TemporalParameters; };
struct TemporalOutput { float4 color : SV_TARGET0; float depth : SV_TARGET1; };
TemporalOutput main(float4 position : SV_POSITION, float2 uv : TEX_COORD)
{
    const float2 texel = TemporalParameters.xy;
    const float3 current = CurrentColor.Sample(CurrentColor_sampler, uv).rgb;
    const float3 north = CurrentColor.Sample(CurrentColor_sampler,
                                              saturate(uv + float2(0.0, texel.y))).rgb;
    const float3 south = CurrentColor.Sample(CurrentColor_sampler,
                                              saturate(uv - float2(0.0, texel.y))).rgb;
    const float3 east = CurrentColor.Sample(CurrentColor_sampler,
                                             saturate(uv + float2(texel.x, 0.0))).rgb;
    const float3 west = CurrentColor.Sample(CurrentColor_sampler,
                                             saturate(uv - float2(texel.x, 0.0))).rgb;
    const float3 neighborhood_min = min(current, min(min(north, south), min(east, west)));
    const float3 neighborhood_max = max(current, max(max(north, south), max(east, west)));
    const float2 motion = MotionVectors.Sample(MotionVectors_sampler, uv).xy;
    const float2 history_uv = uv - motion;
    const bool valid_uv = all(history_uv >= 0.0) && all(history_uv <= 1.0);
    const float3 history = clamp(HistoryColor.Sample(HistoryColor_sampler, history_uv).rgb,
                                 neighborhood_min, neighborhood_max);
    const float current_depth = CurrentDepth.Sample(CurrentDepth_sampler, uv).r;
    const float history_depth = HistoryDepth.Sample(HistoryDepth_sampler, history_uv).r;
    const float depth_threshold = max(0.0015, current_depth * 0.015);
    const bool depth_valid = abs(current_depth - history_depth) <= depth_threshold;
    const float current_luminance = dot(current, float3(0.2126, 0.7152, 0.0722));
    const float history_luminance = dot(history, float3(0.2126, 0.7152, 0.0722));
    const float reactive = saturate(abs(current_luminance - history_luminance) /
                                    max(current_luminance, 0.1));
    const float weight = valid_uv && depth_valid
                             ? TemporalParameters.z * (1.0 - reactive)
                             : 0.0;
    TemporalOutput output;
    output.color = float4(lerp(current, history, weight), 1.0);
    output.depth = current_depth;
    return output;
}
)";

struct Vertex {
    Diligent::float3 position;
    Diligent::float3 normal;
};

constexpr std::array cube_vertices{
    Vertex{{-1, -1, -1}, {0, 0, -1}}, Vertex{{-1, +1, -1}, {0, 0, -1}},
    Vertex{{+1, +1, -1}, {0, 0, -1}}, Vertex{{+1, -1, -1}, {0, 0, -1}},
    Vertex{{-1, -1, +1}, {0, 0, +1}}, Vertex{{+1, -1, +1}, {0, 0, +1}},
    Vertex{{+1, +1, +1}, {0, 0, +1}}, Vertex{{-1, +1, +1}, {0, 0, +1}},
    Vertex{{-1, -1, -1}, {-1, 0, 0}}, Vertex{{-1, -1, +1}, {-1, 0, 0}},
    Vertex{{-1, +1, +1}, {-1, 0, 0}}, Vertex{{-1, +1, -1}, {-1, 0, 0}},
    Vertex{{+1, -1, -1}, {+1, 0, 0}}, Vertex{{+1, +1, -1}, {+1, 0, 0}},
    Vertex{{+1, +1, +1}, {+1, 0, 0}}, Vertex{{+1, -1, +1}, {+1, 0, 0}},
    Vertex{{-1, -1, -1}, {0, -1, 0}}, Vertex{{+1, -1, -1}, {0, -1, 0}},
    Vertex{{+1, -1, +1}, {0, -1, 0}}, Vertex{{-1, -1, +1}, {0, -1, 0}},
    Vertex{{-1, +1, -1}, {0, +1, 0}}, Vertex{{-1, +1, +1}, {0, +1, 0}},
    Vertex{{+1, +1, +1}, {0, +1, 0}}, Vertex{{+1, +1, -1}, {0, +1, 0}},
};

constexpr std::array<Diligent::Uint32, 36> cube_indices{
    0, 1, 2, 0, 2, 3, 4, 5, 6, 4, 6, 7, 8, 9, 10, 8, 10, 11,
    12, 13, 14, 12, 14, 15, 16, 17, 18, 16, 18, 19, 20, 21, 22, 20, 22, 23,
};

constexpr std::array horizontal_quad_vertices{
    Vertex{{-1, 0, -1}, {0, +1, 0}}, Vertex{{-1, 0, +1}, {0, +1, 0}},
    Vertex{{+1, 0, +1}, {0, +1, 0}}, Vertex{{+1, 0, -1}, {0, +1, 0}},
};
constexpr std::array<Diligent::Uint32, 6> horizontal_quad_indices{0, 1, 2, 0, 2, 3};

struct alignas(16) DrawConstants {
    Diligent::float4x4 world_view_projection;
    Diligent::float4x4 previous_world_view_projection;
    Diligent::float4x4 world;
    Diligent::float4x4 normal_world;
    Diligent::float4 temporal_jitter;
    Diligent::float4 color;
    Diligent::float4 base_color;
    Diligent::float4 material_parameters;
    Diligent::float4 emissive;
    Diligent::float4 camera_exposure;
    Diligent::float4 camera_forward;
    Diligent::float4 directional_direction_intensity;
    Diligent::float4 directional_color;
    Diligent::float4 environment_sky;
    Diligent::float4 environment_ground;
    std::array<std::uint32_t, 4> cluster_dimensions{};
    Diligent::float4 cluster_depth_viewport;
    std::array<Diligent::float4x4, 4> shadow_matrices;
    Diligent::float4 shadow_splits;
    Diligent::float4 surface_parameters;
    Diligent::float4 specular_color_rotation;
    Diligent::float4 surface_animation;
    Diligent::float4 surface_flags;
    std::array<Diligent::float4,20> texture_mappings;
    Diligent::float4 skin_flags,particle_depth;
};

struct alignas(16) SkinConstants {
    std::array<Diligent::float4x4,256> bones,previous_bones,bone_normals;
};

struct ShadowConstants {
    Diligent::float4x4 world_view_projection;
    Diligent::float4 mapping, rotation, alpha, skin_flags;
};

struct PointLightGpu {
    Diligent::float4 position_range;
    Diligent::float4 color_intensity;
};

struct ToneConstants {
    float exposure{1.0F};
    float inverse_output_width{1.0F};
    float inverse_output_height{1.0F};
    float sharpness{0.0F};
};

struct TemporalConstants {
    float inverse_render_width{1.0F};
    float inverse_render_height{1.0F};
    float history_weight{0.0F};
    float padding{0.0F};
};

[[nodiscard]] render::MeshUpload builtin_cube_upload() {
    render::MeshUpload upload{.id = render::builtin_cube_mesh};
    upload.vertices.reserve(cube_vertices.size());
    for (const auto& vertex : cube_vertices) {
        upload.vertices.push_back({
            .position = {vertex.position.x, vertex.position.y, vertex.position.z},
            .normal = {vertex.normal.x, vertex.normal.y, vertex.normal.z},
        });
    }
    upload.indices.assign(cube_indices.begin(), cube_indices.end());
    return upload;
}

[[nodiscard]] render::MeshUpload builtin_horizontal_quad_upload() {
    render::MeshUpload upload{.id = render::builtin_horizontal_quad_mesh};
    for (const auto& vertex : horizontal_quad_vertices) {
        upload.vertices.push_back({
            .position = {vertex.position.x, vertex.position.y, vertex.position.z},
            .normal = {vertex.normal.x, vertex.normal.y, vertex.normal.z},
        });
    }
    upload.indices.assign(horizontal_quad_indices.begin(), horizontal_quad_indices.end());
    return upload;
}

[[nodiscard]] std::uint64_t upload_size(const render::MeshUpload& upload) noexcept {
    return upload.vertices.size() * sizeof(render::GpuVertex) +
           upload.indices.size() * sizeof(std::uint32_t);
}

[[nodiscard]] std::uint64_t upload_size(const render::TextureUpload& upload) noexcept {
    std::uint64_t result = 0;
    for (const auto& level : upload.mip_levels) {
        result += level.data.size();
    }
    return result;
}

[[nodiscard]] Diligent::float3 to_diligent(const render::Vec3 value) {
    return {value.x, value.y, value.z};
}

[[nodiscard]] Diligent::float4x4 camera_view(const render::Camera& camera) {
    const Diligent::float3 position = to_diligent(camera.position);
    const Diligent::float3 forward = Diligent::normalize(to_diligent(camera.target) - position);
    const Diligent::float3 right = Diligent::normalize(Diligent::cross(to_diligent(camera.up), forward));
    const Diligent::float3 up = Diligent::cross(forward, right);
    return Diligent::float4x4::Translation(-position.x, -position.y, -position.z) *
           Diligent::float4x4::ViewFromBasis(right, up, forward);
}

[[nodiscard]] Diligent::float4x4 world_matrix(const render::Transform& transform) {
    const Diligent::QuaternionF rotation{transform.rotation.x,
                                         transform.rotation.y,
                                         transform.rotation.z,
                                         transform.rotation.w};
    return Diligent::float4x4::Scale(to_diligent(transform.scale)) * rotation.ToMatrix() *
           Diligent::float4x4::Translation(to_diligent(transform.position));
}

struct ShadowFrame {
    std::array<Diligent::float4x4, 4> matrices;
    Diligent::float4 splits;
};

[[nodiscard]] ShadowFrame shadow_frame(const render::Camera& camera,
                                       const render::DirectionalLight& light, float aspect) {
    ShadowFrame result;
    const float near_plane = std::max(camera.near_plane, 0.05F);
    const float far_plane = std::max(std::min(camera.far_plane, 80.0F), near_plane + 1.0F);
    std::array<float, 4> splits{};
    constexpr float logarithmic_weight = 0.80F;
    for (std::size_t cascade = 0; cascade < splits.size(); ++cascade) {
        const float fraction = static_cast<float>(cascade + 1) /
                               static_cast<float>(splits.size());
        const float logarithmic = near_plane * std::pow(far_plane / near_plane, fraction);
        const float linear = near_plane + (far_plane - near_plane) * fraction;
        splits[cascade] = std::lerp(linear, logarithmic, logarithmic_weight);
    }
    result.splits = {splits[0], splits[1], splits[2], splits[3]};
    const auto camera_position = to_diligent(camera.position);
    const auto camera_forward = Diligent::normalize(to_diligent(camera.target) - camera_position);
    auto light_forward = to_diligent(light.direction);
    if (Diligent::length(light_forward) < 0.001F) {
        light_forward = {0.0F, -1.0F, 0.0F};
    }
    light_forward = Diligent::normalize(light_forward);
    const Diligent::float3 up_hint = std::abs(light_forward.y) > 0.95F
                                         ? Diligent::float3{0.0F, 0.0F, 1.0F}
                                         : Diligent::float3{0.0F, 1.0F, 0.0F};
    const auto right = Diligent::normalize(Diligent::cross(up_hint, light_forward));
    const auto up = Diligent::cross(light_forward, right);
    float previous_split = near_plane;
    for (std::size_t cascade = 0; cascade < splits.size(); ++cascade) {
        const float half_height = splits[cascade] * std::tan(camera.vertical_field_of_view_radians * 0.5F);
        const float half_width = half_height * aspect;
        const float half_depth = (splits[cascade] - previous_split) * 0.5F;
        const float radius = std::ceil(std::sqrt(half_width*half_width + half_height*half_height + half_depth*half_depth) * 16.0F) / 16.0F;
        const float midpoint = (previous_split + splits[cascade]) * 0.5F;
        auto center = camera_position + camera_forward * midpoint;
        const float texel = 2.0F * radius / 2048.0F;
        const float horizontal = Diligent::dot(center, right), vertical = Diligent::dot(center, up);
        center += right * (std::round(horizontal / texel) * texel - horizontal);
        center += up * (std::round(vertical / texel) * texel - vertical);
        const float depth_radius = radius + 30.0F;
        const auto light_position = center - light_forward * depth_radius;
        const auto view = Diligent::float4x4::Translation(-light_position.x,
                                                          -light_position.y,
                                                          -light_position.z) *
                          Diligent::float4x4::ViewFromBasis(right, up, light_forward);
        const auto projection = Diligent::float4x4::Ortho(radius * 2.0F,
                                                          radius * 2.0F,
                                                          0.1F,
                                                          depth_radius * 2.0F,
                                                          false);
        result.matrices[cascade] = view * projection;
        previous_split = splits[cascade];
    }
    return result;
}

} // namespace

struct DiligentRenderer::Impl {
    render::UiDrawData ui;
    Diligent::RefCntAutoPtr<Diligent::IPipelineState> ui_pipeline;
    Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> ui_resources;
    Diligent::RefCntAutoPtr<Diligent::IBuffer> ui_vertices;
    Diligent::ITextureView* ui_bound_view{nullptr};
    std::filesystem::path capture_path;
    static constexpr std::size_t timing_frame_count = 4;
    struct TimingFrame {
        Diligent::RefCntAutoPtr<Diligent::IQuery> shadow;
        Diligent::RefCntAutoPtr<Diligent::IQuery> opaque;
        Diligent::RefCntAutoPtr<Diligent::IQuery> tone_map;
        Diligent::RefCntAutoPtr<Diligent::IQuery> temporal;
        bool shadow_pending{false};
        bool opaque_pending{false};
        bool tone_map_pending{false};
        bool temporal_pending{false};
    };
    struct MeshResource {
        Diligent::RefCntAutoPtr<Diligent::IBuffer> vertices;
        Diligent::RefCntAutoPtr<Diligent::IBuffer> indices;
        Diligent::Uint32 index_count{0};
        std::uint64_t byte_size{0};
        std::uint64_t last_used_frame{0};
    };
    struct TextureResource {
        Diligent::RefCntAutoPtr<Diligent::ITexture> texture;
        Diligent::RefCntAutoPtr<Diligent::ITextureView> view;
        std::uint64_t byte_size{0};
        std::uint64_t last_used_frame{0};
    };
    struct DeferredMesh {
        MeshResource resource;
        std::uint64_t fence_value{0};
    };
    struct DeferredTexture {
        TextureResource resource;
        std::uint64_t fence_value{0};
    };

    Diligent::RefCntAutoPtr<Diligent::IRenderDevice> device;
    Diligent::RefCntAutoPtr<Diligent::IDeviceContext> immediate_context;
    Diligent::RefCntAutoPtr<Diligent::ISwapChain> swap_chain;
    Diligent::RefCntAutoPtr<Diligent::IPipelineState> scene_pipeline;
    std::array<Diligent::RefCntAutoPtr<Diligent::IPipelineState>,8> surface_pipelines;
    Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> scene_resources;
    Diligent::RefCntAutoPtr<Diligent::IBuffer> draw_constants,skin_constants;
    Diligent::RefCntAutoPtr<Diligent::ITexture> hdr_texture;
    Diligent::RefCntAutoPtr<Diligent::ITexture> particle_color_copy,particle_depth_copy;
    Diligent::RefCntAutoPtr<Diligent::ITextureView> hdr_render_target;
    Diligent::RefCntAutoPtr<Diligent::ITextureView> hdr_shader_resource;
    Diligent::RefCntAutoPtr<Diligent::ITexture> motion_texture;
    Diligent::RefCntAutoPtr<Diligent::ITextureView> motion_render_target;
    Diligent::RefCntAutoPtr<Diligent::ITextureView> motion_shader_resource;
    Diligent::RefCntAutoPtr<Diligent::ITexture> linear_depth_texture;
    Diligent::RefCntAutoPtr<Diligent::ITextureView> linear_depth_render_target;
    Diligent::RefCntAutoPtr<Diligent::ITextureView> linear_depth_shader_resource;
    Diligent::RefCntAutoPtr<Diligent::ITexture> scene_depth_texture;
    Diligent::RefCntAutoPtr<Diligent::ITextureView> scene_depth_target;
    std::array<Diligent::RefCntAutoPtr<Diligent::ITexture>, 2> history_textures;
    std::array<Diligent::RefCntAutoPtr<Diligent::ITextureView>, 2> history_render_targets;
    std::array<Diligent::RefCntAutoPtr<Diligent::ITextureView>, 2> history_shader_resources;
    std::array<Diligent::RefCntAutoPtr<Diligent::ITexture>, 2> history_depth_textures;
    std::array<Diligent::RefCntAutoPtr<Diligent::ITextureView>, 2>
        history_depth_render_targets;
    std::array<Diligent::RefCntAutoPtr<Diligent::ITextureView>, 2>
        history_depth_shader_resources;
    Diligent::RefCntAutoPtr<Diligent::IPipelineState> temporal_pipeline;
    std::array<Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding>, 2>
        temporal_resources;
    Diligent::RefCntAutoPtr<Diligent::IBuffer> temporal_constants;
    Diligent::RefCntAutoPtr<Diligent::IPipelineState> tone_map_pipeline;
    std::array<Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding>, 3>
        tone_map_resources;
    Diligent::RefCntAutoPtr<Diligent::IBuffer> tone_constants;
    Diligent::RefCntAutoPtr<Diligent::IBuffer> point_light_buffer;
    Diligent::RefCntAutoPtr<Diligent::IBuffer> cluster_buffer;
    Diligent::RefCntAutoPtr<Diligent::IBuffer> light_index_buffer;
    Diligent::RefCntAutoPtr<Diligent::ITexture> environment_texture, probe_texture;
    std::shared_ptr<const render::EnvironmentProbe> active_probe;
    Diligent::RefCntAutoPtr<Diligent::ITextureView> environment_shader_resource;
    Diligent::RefCntAutoPtr<Diligent::ITexture> shadow_texture;
    Diligent::RefCntAutoPtr<Diligent::ITextureView> shadow_shader_resource;
    std::array<Diligent::RefCntAutoPtr<Diligent::ITextureView>, 4> shadow_depth_targets;
    Diligent::RefCntAutoPtr<Diligent::IPipelineState> shadow_pipeline, double_sided_shadow_pipeline;
    Diligent::RefCntAutoPtr<Diligent::IShaderResourceBinding> shadow_resources;
    Diligent::RefCntAutoPtr<Diligent::IBuffer> shadow_constants;
    Diligent::RefCntAutoPtr<Diligent::IFence> residency_fence;
    std::unordered_map<render::RenderAssetId, MeshResource, render::RenderAssetIdHash> meshes;
    std::unordered_map<render::RenderAssetId, TextureResource, render::RenderAssetIdHash> textures;
    std::unordered_map<render::RenderAssetId, render::MaterialUpload, render::RenderAssetIdHash>
        materials;
    mutable std::mutex upload_mutex;
    std::deque<render::MeshUpload> mesh_uploads;
    std::deque<render::TextureUpload> texture_uploads;
    std::deque<render::MaterialUpload> material_uploads;
    std::deque<render::RenderAssetId> release_requests;
    std::unordered_map<render::RenderAssetId, render::GpuAssetState, render::RenderAssetIdHash>
        asset_states;
    render::RenderGraph frame_graph;
    std::vector<DeferredMesh> deferred_meshes;
    std::vector<DeferredTexture> deferred_textures;
    render::GpuResidencyMetrics residency_metrics;
    render::FrameRenderMetrics frame_metrics;
    std::array<TimingFrame, timing_frame_count> timing_frames;
    bool opaque_timing_active{false};
    bool shadow_rendered{false};
    bool history_valid{false};
    bool drew_scene{false};
    render::TemporalSelection temporal_selection;
    render::DynamicResolutionController dynamic_resolution;
    render::RenderExtent render_extent{};
    render::RenderExtent output_extent{};
    render::CameraJitter jitter{};
    render::CameraJitter previous_jitter{};
    Diligent::float4x4 previous_view_projection;
    render::Camera previous_camera;
    bool previous_camera_valid{false};
    float exposure{1.0F};
    std::uint64_t frame_index{0};
    std::uint64_t submitted_fence_value{0};
};

DiligentRenderer::DiligentRenderer(platform::Window& window, const render::RendererSettings settings)
    : window_{window}, settings_{settings}, impl_{std::make_unique<Impl>()} {
    const render::TemporalCapabilities temporal{.motion_vectors = true,
                                                 .jittered_camera = true,
                                                 .history_resources = true,
                                                 .resolution_scaling = true,
                                                 .taa = true,
                                                 .depth_disocclusion = true,
                                                 .reactive_history = true,
                                                 .contrast_adaptive_sharpening = true,
                                                 .dynamic_resolution = true};
    impl_->temporal_selection = render::negotiate_temporal_feature(settings_.temporal, temporal);
    impl_->dynamic_resolution = render::DynamicResolutionController{
        settings_.temporal.dynamic_resolution, impl_->temporal_selection.render_scale};
}

DiligentRenderer::~DiligentRenderer() { stop(); }

std::string_view DiligentRenderer::name() const noexcept { return "render.diligent.vulkan"; }

core::SubsystemState DiligentRenderer::state() const noexcept { return state_; }

render::RenderCapabilities DiligentRenderer::capabilities() const noexcept {
    const bool bc = impl_->device &&
                    impl_->device->GetDeviceInfo().Features.TextureCompressionBC !=
                        Diligent::DEVICE_FEATURE_STATE_DISABLED;
    const bool timestamps = impl_->device &&
                            impl_->device->GetDeviceInfo().Features.DurationQueries !=
                                Diligent::DEVICE_FEATURE_STATE_DISABLED;
    return {.api = render::GraphicsApi::vulkan,
            .temporal_upscaling = true,
            .texture_compression_bc = bc,
            .gpu_timestamps = timestamps,
            .temporal = {.motion_vectors = true,
                         .jittered_camera = true,
                         .history_resources = true,
                         .resolution_scaling = true,
                         .taa = true,
                         .depth_disocclusion = true,
                         .reactive_history = true,
                         .contrast_adaptive_sharpening = true,
                         .dynamic_resolution = true}};
}

void DiligentRenderer::start() {
    if (state_ == core::SubsystemState::running) {
        throw std::logic_error{"Diligent renderer is already running"};
    }
    if (window_.state() != core::SubsystemState::running) {
        throw std::logic_error{"The platform window must start before the renderer"};
    }

    auto* const factory = Diligent::GetEngineFactoryVk();
    if (factory == nullptr) {
        throw std::runtime_error{"Diligent Vulkan factory is unavailable"};
    }
    Diligent::EngineVkCreateInfo engine_create_info;
    engine_create_info.Features.IndependentBlend = Diligent::DEVICE_FEATURE_STATE_ENABLED;
#if defined(_DEBUG)
    Diligent::BasicPlatformDebug::SetBreakOnError(false);
    engine_create_info.SetValidationLevel(Diligent::VALIDATION_LEVEL_1);
#endif
    Diligent::IDeviceContext* contexts[1]{};
    factory->CreateDeviceAndContextsVk(engine_create_info, &impl_->device, contexts);
    impl_->immediate_context.Attach(contexts[0]);
    if (!impl_->device || !impl_->immediate_context) {
        throw std::runtime_error{"Diligent failed to create the Vulkan device or immediate context"};
    }
    Diligent::FenceDesc fence_description;
    fence_description.Name = "Gloom GPU residency fence";
    impl_->device->CreateFence(fence_description, &impl_->residency_fence);
    if (!impl_->residency_fence) {
        throw std::runtime_error{"Diligent failed to create the GPU residency fence"};
    }

    const auto [width, height] = window_.drawable_size();
    Diligent::SwapChainDesc swap_chain_description;
    swap_chain_description.Width = width;
    swap_chain_description.Height = height;
    swap_chain_description.Usage |= Diligent::SWAP_CHAIN_USAGE_COPY_SOURCE;
    swap_chain_description.ColorBufferFormat = Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB;
    swap_chain_description.DepthBufferFormat = Diligent::TEX_FORMAT_D32_FLOAT;
    const Diligent::NativeWindow native_window{window_.native_handle()};
    if (native_window.hWnd == nullptr) {
        throw std::runtime_error{"SDL did not provide a native Win32 window handle"};
    }
    factory->CreateSwapChainVk(impl_->device,
                               impl_->immediate_context,
                               swap_chain_description,
                               native_window,
                               &impl_->swap_chain);
    if (!impl_->swap_chain) {
        throw std::runtime_error{"Diligent failed to create the Vulkan swap chain"};
    }
    create_scene_resources();
    create_frame_resources(width, height);
    if (capabilities().gpu_timestamps) {
        for (auto& timing : impl_->timing_frames) {
            Diligent::QueryDesc query_description{Diligent::QUERY_TYPE_DURATION};
            query_description.Name = "Gloom shadow GPU duration";
            impl_->device->CreateQuery(query_description, &timing.shadow);
            query_description.Name = "Gloom opaque GPU duration";
            impl_->device->CreateQuery(query_description, &timing.opaque);
            query_description.Name = "Gloom tone-map GPU duration";
            impl_->device->CreateQuery(query_description, &timing.tone_map);
            query_description.Name = "Gloom temporal resolve GPU duration";
            impl_->device->CreateQuery(query_description, &timing.temporal);
            if (!timing.shadow || !timing.opaque || !timing.tone_map || !timing.temporal) {
                throw std::runtime_error{"Diligent failed to create GPU duration queries"};
            }
        }
    }
    enqueue(builtin_cube_upload());
    enqueue(builtin_horizontal_quad_upload());
    enqueue(render::TextureUpload{
        .id = render::builtin_white_texture,
        .mip_levels = {{.data = {std::byte{0xff},
                                 std::byte{0xff},
                                 std::byte{0xff},
                                 std::byte{0xff}}}},
    });
    enqueue(render::TextureUpload{
        .id = render::builtin_flat_normal_texture,
        .srgb = false,
        .mip_levels = {{.data = {std::byte{0x80},
                                 std::byte{0x80},
                                 std::byte{0xff},
                                 std::byte{0xff}}}},
    });
    enqueue(render::MaterialUpload{.id = render::builtin_default_material});
    state_ = core::SubsystemState::running;
}

void DiligentRenderer::create_scene_resources() {
    Diligent::BufferDesc constants_description;
    constants_description.Name = "Gloom per-draw constants";
    constants_description.Usage = Diligent::USAGE_DYNAMIC;
    constants_description.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
    constants_description.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
    constants_description.Size = sizeof(DrawConstants);
    impl_->device->CreateBuffer(constants_description, nullptr, &impl_->draw_constants);
    constants_description.Name="Gloom animated skin constants";
    constants_description.Size=sizeof(SkinConstants);
    impl_->device->CreateBuffer(constants_description,nullptr,&impl_->skin_constants);

    Diligent::ShaderCreateInfo shader_create_info;
    shader_create_info.SourceLanguage = Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
    // CPU transforms and every shader mul() use row-vector order. Without this
    // flag HLSL defaults to column-major packing, placing translation in the
    // wrong coefficients and making geometry converge towards a point at w=0.
    shader_create_info.CompileFlags = Diligent::SHADER_COMPILE_FLAG_PACK_MATRIX_ROW_MAJOR;
    shader_create_info.EntryPoint = "main";
    Diligent::RefCntAutoPtr<Diligent::IShader> vertex_shader;
    shader_create_info.Desc.ShaderType = Diligent::SHADER_TYPE_VERTEX;
    shader_create_info.Desc.Name = "Gloom scene vertex shader";
    shader_create_info.Source = vertex_shader_source;
    impl_->device->CreateShader(shader_create_info, &vertex_shader);
    Diligent::RefCntAutoPtr<Diligent::IShader> pixel_shader;
    shader_create_info.Desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
    shader_create_info.Desc.Name = "Gloom scene pixel shader";
    shader_create_info.Source = pixel_shader_source;
    impl_->device->CreateShader(shader_create_info, &pixel_shader);

    if (!impl_->draw_constants || !impl_->skin_constants || !vertex_shader || !pixel_shader) {
        throw std::runtime_error{"Diligent failed to create scene GPU resources"};
    }
    constexpr std::array input_layout{
        Diligent::LayoutElement{0, 0, 3, Diligent::VT_FLOAT32, false},
        Diligent::LayoutElement{1, 0, 3, Diligent::VT_FLOAT32, false},
        Diligent::LayoutElement{2, 0, 2, Diligent::VT_FLOAT32, false},
        Diligent::LayoutElement{3, 0, 4, Diligent::VT_FLOAT32, false},
        Diligent::LayoutElement{4, 0, 2, Diligent::VT_FLOAT32, false},
        Diligent::LayoutElement{5, 0, 4, Diligent::VT_UINT16, false},
        Diligent::LayoutElement{6, 0, 4, Diligent::VT_UINT16, false},
        Diligent::LayoutElement{7, 0, 4, Diligent::VT_FLOAT32, false},
        Diligent::LayoutElement{8, 0, 4, Diligent::VT_FLOAT32, false},
    };
    Diligent::GraphicsPipelineStateCreateInfo pipeline_create_info;
    pipeline_create_info.PSODesc.Name = "Gloom opaque scene pipeline";
    pipeline_create_info.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_GRAPHICS;
    pipeline_create_info.PSODesc.ResourceLayout.DefaultVariableType =
        Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
    constexpr Diligent::ShaderResourceVariableDesc resource_variables[]{
        {Diligent::SHADER_TYPE_PIXEL,
         "BaseColorTexture",
         Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {Diligent::SHADER_TYPE_PIXEL,
         "MetallicRoughnessTexture",
         Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {Diligent::SHADER_TYPE_PIXEL,
         "NormalTexture",
         Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {Diligent::SHADER_TYPE_PIXEL,"EmissiveTexture",Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {Diligent::SHADER_TYPE_PIXEL,"OcclusionTexture",Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {Diligent::SHADER_TYPE_PIXEL,"SpecularTexture",Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {Diligent::SHADER_TYPE_PIXEL,"SpecularColorTexture",Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {Diligent::SHADER_TYPE_PIXEL,"AnisotropyTexture",Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {Diligent::SHADER_TYPE_PIXEL,"DetailTexture",Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {Diligent::SHADER_TYPE_PIXEL,"LightmapTexture",Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {Diligent::SHADER_TYPE_PIXEL,"SceneColorCopy",Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {Diligent::SHADER_TYPE_PIXEL,"SceneDepthCopy",Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC},
        {Diligent::SHADER_TYPE_PIXEL,
         "PointLights",
         Diligent::SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {Diligent::SHADER_TYPE_PIXEL,
         "LightClusters",
         Diligent::SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {Diligent::SHADER_TYPE_PIXEL,
         "LightIndices",
         Diligent::SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {Diligent::SHADER_TYPE_PIXEL,
         "EnvironmentTexture",
         Diligent::SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {Diligent::SHADER_TYPE_PIXEL,
         "ShadowMap",
         Diligent::SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
    };
    Diligent::SamplerDesc surface_sampler;
    surface_sampler.AddressU=surface_sampler.AddressV=Diligent::TEXTURE_ADDRESS_WRAP;
    Diligent::SamplerDesc copy_sampler;
    copy_sampler.AddressU=copy_sampler.AddressV=Diligent::TEXTURE_ADDRESS_CLAMP;
    const Diligent::ImmutableSamplerDesc immutable_samplers[]{
        {Diligent::SHADER_TYPE_PIXEL,"SceneColorCopy_sampler",copy_sampler},
        {Diligent::SHADER_TYPE_PIXEL, "BaseColorTexture_sampler", surface_sampler},
        {Diligent::SHADER_TYPE_PIXEL,
         "MetallicRoughnessTexture_sampler",
         surface_sampler},
        {Diligent::SHADER_TYPE_PIXEL, "NormalTexture_sampler", surface_sampler},
        {Diligent::SHADER_TYPE_PIXEL, "EnvironmentTexture_sampler", Diligent::SamplerDesc{}},
    };
    pipeline_create_info.PSODesc.ResourceLayout.Variables = resource_variables;
    pipeline_create_info.PSODesc.ResourceLayout.NumVariables =
        static_cast<Diligent::Uint32>(std::size(resource_variables));
    pipeline_create_info.PSODesc.ResourceLayout.ImmutableSamplers = immutable_samplers;
    pipeline_create_info.PSODesc.ResourceLayout.NumImmutableSamplers =
        static_cast<Diligent::Uint32>(std::size(immutable_samplers));
    pipeline_create_info.GraphicsPipeline.NumRenderTargets = 3;
    pipeline_create_info.GraphicsPipeline.RTVFormats[0] = Diligent::TEX_FORMAT_RGBA16_FLOAT;
    pipeline_create_info.GraphicsPipeline.RTVFormats[1] = Diligent::TEX_FORMAT_RG16_FLOAT;
    pipeline_create_info.GraphicsPipeline.RTVFormats[2] = Diligent::TEX_FORMAT_R32_FLOAT;
    pipeline_create_info.GraphicsPipeline.DSVFormat = Diligent::TEX_FORMAT_D32_FLOAT;
    pipeline_create_info.GraphicsPipeline.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipeline_create_info.GraphicsPipeline.RasterizerDesc.CullMode = Diligent::CULL_MODE_BACK;
    pipeline_create_info.GraphicsPipeline.RasterizerDesc.FrontCounterClockwise = false;
    pipeline_create_info.GraphicsPipeline.DepthStencilDesc.DepthEnable = true;
    pipeline_create_info.GraphicsPipeline.InputLayout.LayoutElements = input_layout.data();
    pipeline_create_info.GraphicsPipeline.InputLayout.NumElements =
        static_cast<Diligent::Uint32>(input_layout.size());
    pipeline_create_info.pVS = vertex_shader;
    pipeline_create_info.pPS = pixel_shader;
    impl_->device->CreateGraphicsPipelineState(pipeline_create_info, &impl_->scene_pipeline);
    if (!impl_->scene_pipeline) {
        throw std::runtime_error{"Diligent failed to create the scene pipeline"};
    }
    auto* constants =
        impl_->scene_pipeline->GetStaticVariableByName(Diligent::SHADER_TYPE_VERTEX, "Constants");
    if (constants == nullptr) {
        throw std::runtime_error{"Diligent could not bind scene constants"};
    }
    constants->Set(impl_->draw_constants);
    constants =
        impl_->scene_pipeline->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL, "Constants");
    if (constants == nullptr) {
        throw std::runtime_error{"Diligent could not bind pixel material constants"};
    }
    constants->Set(impl_->draw_constants);
    impl_->scene_pipeline->GetStaticVariableByName(Diligent::SHADER_TYPE_VERTEX,"SkinConstants")->Set(impl_->skin_constants);
    impl_->surface_pipelines[0]=impl_->scene_pipeline;
    for (std::uint32_t variant=1;variant<8;++variant) {
        auto& graphics=pipeline_create_info.GraphicsPipeline;
        graphics.RasterizerDesc.CullMode=(variant%2)!=0 ? Diligent::CULL_MODE_NONE : Diligent::CULL_MODE_BACK;
        const auto mode=variant/2;
        graphics.DepthStencilDesc.DepthWriteEnable=mode<2;
        graphics.BlendDesc.RenderTargets[1].RenderTargetWriteMask=mode>=2?Diligent::COLOR_MASK_NONE:Diligent::COLOR_MASK_ALL;
        graphics.BlendDesc.RenderTargets[2].RenderTargetWriteMask=mode>=2?Diligent::COLOR_MASK_NONE:Diligent::COLOR_MASK_ALL;
        auto& blend=graphics.BlendDesc.RenderTargets[0];
        graphics.BlendDesc.IndependentBlendEnable=true;
        blend.BlendEnable=mode>=2;
        blend.SrcBlend=Diligent::BLEND_FACTOR_SRC_ALPHA;
        blend.DestBlend=mode==3 ? Diligent::BLEND_FACTOR_ONE : Diligent::BLEND_FACTOR_INV_SRC_ALPHA;
        blend.SrcBlendAlpha=Diligent::BLEND_FACTOR_ONE;
        blend.DestBlendAlpha=Diligent::BLEND_FACTOR_INV_SRC_ALPHA;
        impl_->device->CreateGraphicsPipelineState(pipeline_create_info,&impl_->surface_pipelines[variant]);
        auto* pipeline=impl_->surface_pipelines[variant].RawPtr();
        if (!pipeline) throw std::runtime_error{"Failed to create surface pipeline"};
        pipeline->GetStaticVariableByName(Diligent::SHADER_TYPE_VERTEX,"SkinConstants")->Set(impl_->skin_constants);
        for (auto stage : {Diligent::SHADER_TYPE_VERTEX,Diligent::SHADER_TYPE_PIXEL})
            pipeline->GetStaticVariableByName(stage,"Constants")->Set(impl_->draw_constants);
    }
    impl_->scene_pipeline->CreateShaderResourceBinding(&impl_->scene_resources, true);
    if (!impl_->scene_resources) {
        throw std::runtime_error{"Diligent failed to create scene shader resources"};
    }

    const auto create_structured_buffer = [&](const char* name,
                                              const Diligent::Uint64 size,
                                              const Diligent::Uint32 stride,
                                              Diligent::RefCntAutoPtr<Diligent::IBuffer>& buffer) {
        Diligent::BufferDesc description;
        description.Name = name;
        description.Size = size;
        description.Usage = Diligent::USAGE_DYNAMIC;
        description.BindFlags = Diligent::BIND_SHADER_RESOURCE;
        description.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
        description.Mode = Diligent::BUFFER_MODE_STRUCTURED;
        description.ElementByteStride = stride;
        impl_->device->CreateBuffer(description, nullptr, &buffer);
    };
    create_structured_buffer("Gloom clustered point lights",
                             256U * sizeof(PointLightGpu),
                             sizeof(PointLightGpu),
                             impl_->point_light_buffer);
    create_structured_buffer("Gloom light cluster ranges",
                             16U * 9U * 24U * sizeof(render::LightClusterRange),
                             sizeof(render::LightClusterRange),
                             impl_->cluster_buffer);
    create_structured_buffer("Gloom clustered light indices",
                             16U * 9U * 24U * 64U * sizeof(std::uint32_t),
                             sizeof(std::uint32_t),
                             impl_->light_index_buffer);
    for (const auto [name, buffer] :
         {std::pair{"PointLights", impl_->point_light_buffer.RawPtr()},
          std::pair{"LightClusters", impl_->cluster_buffer.RawPtr()},
          std::pair{"LightIndices", impl_->light_index_buffer.RawPtr()}}) {
        auto* variable = impl_->scene_resources->GetVariableByName(Diligent::SHADER_TYPE_PIXEL,
                                                                    name);
        if (variable == nullptr || buffer == nullptr) {
            throw std::runtime_error{"Diligent failed to create clustered-light resources"};
        }
        variable->Set(buffer->GetDefaultView(Diligent::BUFFER_VIEW_SHADER_RESOURCE));
    }
    constexpr std::array<std::array<float, 4>, 6> environment_faces{{
        {0.78F, 0.86F, 1.0F, 1.0F},
        {0.62F, 0.72F, 0.94F, 1.0F},
        {1.0F, 1.0F, 1.0F, 1.0F},
        {0.16F, 0.13F, 0.11F, 1.0F},
        {0.70F, 0.80F, 1.0F, 1.0F},
        {0.52F, 0.62F, 0.82F, 1.0F},
    }};
    std::array<Diligent::TextureSubResData, environment_faces.size()> environment_subresources{};
    for (std::size_t face = 0; face < environment_faces.size(); ++face) {
        environment_subresources[face].pData = environment_faces[face].data();
        environment_subresources[face].Stride = sizeof(environment_faces[face]);
    }
    Diligent::TextureData environment_data{
        environment_subresources.data(),
        static_cast<Diligent::Uint32>(environment_subresources.size())};
    Diligent::TextureDesc environment_description;
    environment_description.Name = "Gloom fallback image-based environment";
    environment_description.Type = Diligent::RESOURCE_DIM_TEX_CUBE;
    environment_description.Width = 1;
    environment_description.Height = 1;
    environment_description.ArraySize = 6;
    environment_description.Format = Diligent::TEX_FORMAT_RGBA32_FLOAT;
    environment_description.Usage = Diligent::USAGE_IMMUTABLE;
    environment_description.BindFlags = Diligent::BIND_SHADER_RESOURCE;
    impl_->device->CreateTexture(environment_description,
                                 &environment_data,
                                 &impl_->environment_texture);
    if (impl_->environment_texture) {
        impl_->environment_shader_resource = impl_->environment_texture->GetDefaultView(
            Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
    }
    auto* environment_variable = impl_->scene_resources->GetVariableByName(
        Diligent::SHADER_TYPE_PIXEL, "EnvironmentTexture");
    if (environment_variable == nullptr || !impl_->environment_shader_resource) {
        throw std::runtime_error{"Diligent failed to create the fallback IBL environment"};
    }
    environment_variable->Set(impl_->environment_shader_resource);

    Diligent::TextureDesc shadow_description;
    shadow_description.Name = "Gloom cascaded directional shadow map";
    shadow_description.Type = Diligent::RESOURCE_DIM_TEX_2D_ARRAY;
    shadow_description.Width = 2048;
    shadow_description.Height = 2048;
    shadow_description.ArraySize = 4;
    shadow_description.Format = Diligent::TEX_FORMAT_D32_FLOAT;
    shadow_description.BindFlags = Diligent::BIND_DEPTH_STENCIL |
                                   Diligent::BIND_SHADER_RESOURCE;
    impl_->device->CreateTexture(shadow_description, nullptr, &impl_->shadow_texture);
    if (impl_->shadow_texture) {
        impl_->shadow_shader_resource = impl_->shadow_texture->GetDefaultView(
            Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
        for (Diligent::Uint32 cascade = 0; cascade < impl_->shadow_depth_targets.size();
             ++cascade) {
            Diligent::TextureViewDesc view_description;
            view_description.Name = "Gloom shadow cascade DSV";
            view_description.ViewType = Diligent::TEXTURE_VIEW_DEPTH_STENCIL;
            view_description.TextureDim = Diligent::RESOURCE_DIM_TEX_2D_ARRAY;
            view_description.Format = Diligent::TEX_FORMAT_D32_FLOAT;
            view_description.FirstArraySlice = cascade;
            view_description.NumArraySlices = 1;
            impl_->shadow_texture->CreateView(view_description,
                                              &impl_->shadow_depth_targets[cascade]);
        }
    }
    auto* shadow_variable = impl_->scene_resources->GetVariableByName(
        Diligent::SHADER_TYPE_PIXEL, "ShadowMap");
    if (shadow_variable == nullptr || !impl_->shadow_shader_resource ||
        std::ranges::any_of(impl_->shadow_depth_targets,
                            [](const auto& view) { return !view; })) {
        throw std::runtime_error{"Diligent failed to create cascaded shadow resources"};
    }
    shadow_variable->Set(impl_->shadow_shader_resource);

    Diligent::BufferDesc shadow_constants_description;
    shadow_constants_description.Name = "Gloom shadow draw constants";
    shadow_constants_description.Size = sizeof(ShadowConstants);
    shadow_constants_description.Usage = Diligent::USAGE_DYNAMIC;
    shadow_constants_description.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
    shadow_constants_description.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
    impl_->device->CreateBuffer(shadow_constants_description, nullptr, &impl_->shadow_constants);
    Diligent::RefCntAutoPtr<Diligent::IShader> shadow_vertex_shader;
    shader_create_info.Desc.ShaderType = Diligent::SHADER_TYPE_VERTEX;
    shader_create_info.Desc.Name = "Gloom shadow vertex shader";
    shader_create_info.Source = shadow_vertex_shader_source;
    impl_->device->CreateShader(shader_create_info, &shadow_vertex_shader);
    Diligent::RefCntAutoPtr<Diligent::IShader> shadow_pixel_shader;
    shader_create_info.Desc.ShaderType=Diligent::SHADER_TYPE_PIXEL;
    shader_create_info.Desc.Name="Gloom alpha-aware shadow pixel shader";
    shader_create_info.Source=shadow_pixel_shader_source;
    impl_->device->CreateShader(shader_create_info,&shadow_pixel_shader);
    Diligent::GraphicsPipelineStateCreateInfo shadow_pipeline_description;
    shadow_pipeline_description.PSODesc.Name = "Gloom cascaded shadow pipeline";
    shadow_pipeline_description.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_GRAPHICS;
    shadow_pipeline_description.PSODesc.ResourceLayout.DefaultVariableType =
        Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
    const std::array shadow_layout{
        Diligent::LayoutElement{0,0,3,Diligent::VT_FLOAT32,false,0,sizeof(render::GpuVertex)},
        Diligent::LayoutElement{2,0,2,Diligent::VT_FLOAT32,false,offsetof(render::GpuVertex,texture_coordinate),sizeof(render::GpuVertex)},
        Diligent::LayoutElement{4,0,2,Diligent::VT_FLOAT32,false,offsetof(render::GpuVertex,texture_coordinate_1),sizeof(render::GpuVertex)},
        Diligent::LayoutElement{5,0,4,Diligent::VT_UINT16,false,offsetof(render::GpuVertex,joints),sizeof(render::GpuVertex)},
        Diligent::LayoutElement{6,0,4,Diligent::VT_UINT16,false,offsetof(render::GpuVertex,joints)+8,sizeof(render::GpuVertex)},
        Diligent::LayoutElement{7,0,4,Diligent::VT_FLOAT32,false,offsetof(render::GpuVertex,weights),sizeof(render::GpuVertex)},
        Diligent::LayoutElement{8,0,4,Diligent::VT_FLOAT32,false,offsetof(render::GpuVertex,weights)+16,sizeof(render::GpuVertex)}};
    const Diligent::ShaderResourceVariableDesc shadow_texture{Diligent::SHADER_TYPE_PIXEL,"ShadowBaseColor",Diligent::SHADER_RESOURCE_VARIABLE_TYPE_DYNAMIC};
    const Diligent::ImmutableSamplerDesc shadow_sampler{Diligent::SHADER_TYPE_PIXEL,"ShadowBaseColor_sampler",surface_sampler};
    shadow_pipeline_description.PSODesc.ResourceLayout.Variables=&shadow_texture;
    shadow_pipeline_description.PSODesc.ResourceLayout.NumVariables=1;
    shadow_pipeline_description.PSODesc.ResourceLayout.ImmutableSamplers=&shadow_sampler;
    shadow_pipeline_description.PSODesc.ResourceLayout.NumImmutableSamplers=1;
    shadow_pipeline_description.GraphicsPipeline.NumRenderTargets = 0;
    shadow_pipeline_description.GraphicsPipeline.DSVFormat = Diligent::TEX_FORMAT_D32_FLOAT;
    shadow_pipeline_description.GraphicsPipeline.PrimitiveTopology =
        Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    shadow_pipeline_description.GraphicsPipeline.RasterizerDesc.CullMode =
        Diligent::CULL_MODE_BACK;
    shadow_pipeline_description.GraphicsPipeline.RasterizerDesc.DepthBias = 2;
    shadow_pipeline_description.GraphicsPipeline.RasterizerDesc.SlopeScaledDepthBias = 1.5F;
    shadow_pipeline_description.GraphicsPipeline.DepthStencilDesc.DepthEnable = true;
    shadow_pipeline_description.GraphicsPipeline.InputLayout.LayoutElements = shadow_layout.data();
    shadow_pipeline_description.GraphicsPipeline.InputLayout.NumElements = static_cast<Diligent::Uint32>(shadow_layout.size());
    shadow_pipeline_description.pVS = shadow_vertex_shader;
    shadow_pipeline_description.pPS = shadow_pixel_shader;
    impl_->device->CreateGraphicsPipelineState(shadow_pipeline_description,
                                                &impl_->shadow_pipeline);
    if (!impl_->shadow_pipeline || !impl_->shadow_constants || !shadow_vertex_shader) {
        throw std::runtime_error{"Diligent failed to create the cascaded shadow pipeline"};
    }
    auto* shadow_constants = impl_->shadow_pipeline->GetStaticVariableByName(
        Diligent::SHADER_TYPE_VERTEX, "ShadowConstants");
    if (shadow_constants == nullptr) {
        throw std::runtime_error{"Diligent could not bind shadow constants"};
    }
    shadow_constants->Set(impl_->shadow_constants);
    impl_->shadow_pipeline->GetStaticVariableByName(Diligent::SHADER_TYPE_VERTEX,"SkinConstants")->Set(impl_->skin_constants);
    impl_->shadow_pipeline->GetStaticVariableByName(Diligent::SHADER_TYPE_PIXEL,"ShadowConstants")->Set(impl_->shadow_constants);
    shadow_pipeline_description.GraphicsPipeline.RasterizerDesc.CullMode=Diligent::CULL_MODE_NONE;
    shadow_pipeline_description.PSODesc.Name="Gloom double-sided shadow pipeline";
    impl_->device->CreateGraphicsPipelineState(shadow_pipeline_description,&impl_->double_sided_shadow_pipeline);
    if (!impl_->double_sided_shadow_pipeline) throw std::runtime_error{"Double sided shadow pipeline failed"};
    impl_->double_sided_shadow_pipeline->GetStaticVariableByName(Diligent::SHADER_TYPE_VERTEX,"SkinConstants")->Set(impl_->skin_constants);
    for (const auto stage:{Diligent::SHADER_TYPE_VERTEX,Diligent::SHADER_TYPE_PIXEL})
        impl_->double_sided_shadow_pipeline->GetStaticVariableByName(stage,"ShadowConstants")->Set(impl_->shadow_constants);
    impl_->shadow_pipeline->CreateShaderResourceBinding(&impl_->shadow_resources, true);
    if (!impl_->shadow_resources) {
        throw std::runtime_error{"Diligent failed to create shadow shader resources"};
    }

    Diligent::BufferDesc tone_constants_description;
    tone_constants_description.Name = "Gloom tone-map constants";
    tone_constants_description.Size = sizeof(ToneConstants);
    tone_constants_description.Usage = Diligent::USAGE_DYNAMIC;
    tone_constants_description.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
    tone_constants_description.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
    impl_->device->CreateBuffer(tone_constants_description, nullptr, &impl_->tone_constants);

    Diligent::RefCntAutoPtr<Diligent::IShader> tone_vertex_shader;
    shader_create_info.Desc.ShaderType = Diligent::SHADER_TYPE_VERTEX;
    shader_create_info.Desc.Name = "Gloom tone-map vertex shader";
    shader_create_info.Source = tone_map_vertex_shader_source;
    impl_->device->CreateShader(shader_create_info, &tone_vertex_shader);
    Diligent::RefCntAutoPtr<Diligent::IShader> tone_pixel_shader;
    shader_create_info.Desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
    shader_create_info.Desc.Name = "Gloom tone-map pixel shader";
    shader_create_info.Source = tone_map_pixel_shader_source;
    impl_->device->CreateShader(shader_create_info, &tone_pixel_shader);
    Diligent::GraphicsPipelineStateCreateInfo tone_pipeline;
    tone_pipeline.PSODesc.Name = "Gloom ACES tone-map pipeline";
    tone_pipeline.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_GRAPHICS;
    tone_pipeline.PSODesc.ResourceLayout.DefaultVariableType =
        Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
    const Diligent::ShaderResourceVariableDesc tone_variable{
        Diligent::SHADER_TYPE_PIXEL,
        "HdrTexture",
        Diligent::SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE};
    const Diligent::ImmutableSamplerDesc tone_sampler{
        Diligent::SHADER_TYPE_PIXEL, "HdrTexture_sampler", Diligent::SamplerDesc{}};
    tone_pipeline.PSODesc.ResourceLayout.Variables = &tone_variable;
    tone_pipeline.PSODesc.ResourceLayout.NumVariables = 1;
    tone_pipeline.PSODesc.ResourceLayout.ImmutableSamplers = &tone_sampler;
    tone_pipeline.PSODesc.ResourceLayout.NumImmutableSamplers = 1;
    tone_pipeline.GraphicsPipeline.NumRenderTargets = 1;
    tone_pipeline.GraphicsPipeline.RTVFormats[0] = impl_->swap_chain->GetDesc().ColorBufferFormat;
    tone_pipeline.GraphicsPipeline.PrimitiveTopology = Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    tone_pipeline.GraphicsPipeline.RasterizerDesc.CullMode = Diligent::CULL_MODE_NONE;
    tone_pipeline.GraphicsPipeline.DepthStencilDesc.DepthEnable = false;
    tone_pipeline.pVS = tone_vertex_shader;
    tone_pipeline.pPS = tone_pixel_shader;
    impl_->device->CreateGraphicsPipelineState(tone_pipeline, &impl_->tone_map_pipeline);
    if (!impl_->tone_map_pipeline || !impl_->tone_constants || !tone_vertex_shader ||
        !tone_pixel_shader) {
        throw std::runtime_error{"Diligent failed to create HDR tone-map resources"};
    }
    auto* tone_constants = impl_->tone_map_pipeline->GetStaticVariableByName(
        Diligent::SHADER_TYPE_PIXEL, "ToneConstants");
    if (tone_constants == nullptr) {
        throw std::runtime_error{"Diligent could not bind tone-map constants"};
    }
    tone_constants->Set(impl_->tone_constants);
    for (auto& resources : impl_->tone_map_resources) {
        impl_->tone_map_pipeline->CreateShaderResourceBinding(&resources, true);
        if (!resources) {
            throw std::runtime_error{"Diligent could not create tone-map bindings"};
        }
    }

    Diligent::BufferDesc temporal_constants_description;
    temporal_constants_description.Name = "Gloom temporal resolve constants";
    temporal_constants_description.Size = sizeof(TemporalConstants);
    temporal_constants_description.Usage = Diligent::USAGE_DYNAMIC;
    temporal_constants_description.BindFlags = Diligent::BIND_UNIFORM_BUFFER;
    temporal_constants_description.CPUAccessFlags = Diligent::CPU_ACCESS_WRITE;
    impl_->device->CreateBuffer(
        temporal_constants_description, nullptr, &impl_->temporal_constants);
    Diligent::RefCntAutoPtr<Diligent::IShader> temporal_pixel_shader;
    shader_create_info.Desc.ShaderType = Diligent::SHADER_TYPE_PIXEL;
    shader_create_info.Desc.Name = "Gloom native TAA pixel shader";
    shader_create_info.Source = temporal_pixel_shader_source;
    impl_->device->CreateShader(shader_create_info, &temporal_pixel_shader);
    Diligent::GraphicsPipelineStateCreateInfo temporal_pipeline;
    temporal_pipeline.PSODesc.Name = "Gloom native TAA pipeline";
    temporal_pipeline.PSODesc.PipelineType = Diligent::PIPELINE_TYPE_GRAPHICS;
    temporal_pipeline.PSODesc.ResourceLayout.DefaultVariableType =
        Diligent::SHADER_RESOURCE_VARIABLE_TYPE_STATIC;
    constexpr Diligent::ShaderResourceVariableDesc temporal_variables[]{
        {Diligent::SHADER_TYPE_PIXEL,
         "CurrentColor",
         Diligent::SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {Diligent::SHADER_TYPE_PIXEL,
         "MotionVectors",
         Diligent::SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {Diligent::SHADER_TYPE_PIXEL,
         "HistoryColor",
         Diligent::SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {Diligent::SHADER_TYPE_PIXEL,
         "CurrentDepth",
         Diligent::SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
        {Diligent::SHADER_TYPE_PIXEL,
         "HistoryDepth",
         Diligent::SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE},
    };
    constexpr Diligent::ImmutableSamplerDesc temporal_samplers[]{
        {Diligent::SHADER_TYPE_PIXEL, "CurrentColor_sampler", Diligent::SamplerDesc{}},
        {Diligent::SHADER_TYPE_PIXEL, "MotionVectors_sampler", Diligent::SamplerDesc{}},
        {Diligent::SHADER_TYPE_PIXEL, "HistoryColor_sampler", Diligent::SamplerDesc{}},
        {Diligent::SHADER_TYPE_PIXEL, "CurrentDepth_sampler", Diligent::SamplerDesc{}},
        {Diligent::SHADER_TYPE_PIXEL, "HistoryDepth_sampler", Diligent::SamplerDesc{}},
    };
    temporal_pipeline.PSODesc.ResourceLayout.Variables = temporal_variables;
    temporal_pipeline.PSODesc.ResourceLayout.NumVariables =
        static_cast<Diligent::Uint32>(std::size(temporal_variables));
    temporal_pipeline.PSODesc.ResourceLayout.ImmutableSamplers = temporal_samplers;
    temporal_pipeline.PSODesc.ResourceLayout.NumImmutableSamplers =
        static_cast<Diligent::Uint32>(std::size(temporal_samplers));
    temporal_pipeline.GraphicsPipeline.NumRenderTargets = 2;
    temporal_pipeline.GraphicsPipeline.RTVFormats[0] = Diligent::TEX_FORMAT_RGBA16_FLOAT;
    temporal_pipeline.GraphicsPipeline.RTVFormats[1] = Diligent::TEX_FORMAT_R32_FLOAT;
    temporal_pipeline.GraphicsPipeline.PrimitiveTopology =
        Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    temporal_pipeline.GraphicsPipeline.RasterizerDesc.CullMode = Diligent::CULL_MODE_NONE;
    temporal_pipeline.GraphicsPipeline.DepthStencilDesc.DepthEnable = false;
    temporal_pipeline.pVS = tone_vertex_shader;
    temporal_pipeline.pPS = temporal_pixel_shader;
    impl_->device->CreateGraphicsPipelineState(temporal_pipeline, &impl_->temporal_pipeline);
    if (!impl_->temporal_pipeline || !impl_->temporal_constants || !temporal_pixel_shader) {
        throw std::runtime_error{"Diligent failed to create native TAA resources"};
    }
    auto* temporal_constants = impl_->temporal_pipeline->GetStaticVariableByName(
        Diligent::SHADER_TYPE_PIXEL, "TemporalConstants");
    if (temporal_constants == nullptr) {
        throw std::runtime_error{"Diligent could not bind native TAA constants"};
    }
    temporal_constants->Set(impl_->temporal_constants);
    for (auto& resources : impl_->temporal_resources) {
        impl_->temporal_pipeline->CreateShaderResourceBinding(&resources, true);
        if (!resources) {
            throw std::runtime_error{"Diligent failed to create native TAA bindings"};
        }
    }
}

void DiligentRenderer::create_frame_resources(const std::uint32_t width,
                                              const std::uint32_t height) {
    impl_->scene_depth_target.Release();
    impl_->scene_depth_texture.Release();
    impl_->motion_shader_resource.Release();
    impl_->motion_render_target.Release();
    impl_->motion_texture.Release();
    impl_->linear_depth_shader_resource.Release();
    impl_->linear_depth_render_target.Release();
    impl_->linear_depth_texture.Release();
    impl_->hdr_shader_resource.Release();
    impl_->hdr_render_target.Release();
    impl_->hdr_texture.Release();
    impl_->particle_color_copy.Release();impl_->particle_depth_copy.Release();
    for (std::size_t index = 0; index < impl_->history_textures.size(); ++index) {
        impl_->history_shader_resources[index].Release();
        impl_->history_render_targets[index].Release();
        impl_->history_textures[index].Release();
        impl_->history_depth_shader_resources[index].Release();
        impl_->history_depth_render_targets[index].Release();
        impl_->history_depth_textures[index].Release();
    }
    impl_->output_extent = {width, height};
    impl_->render_extent = render::scaled_render_extent(
        impl_->output_extent, impl_->temporal_selection.render_scale);
    impl_->history_valid = false;
    impl_->previous_camera_valid = false;
    if (width == 0 || height == 0 || !impl_->device) {
        return;
    }
    Diligent::TextureDesc description;
    description.Name = "Gloom HDR scene color";
    description.Type = Diligent::RESOURCE_DIM_TEX_2D;
    description.Width = impl_->render_extent.width;
    description.Height = impl_->render_extent.height;
    description.Format = Diligent::TEX_FORMAT_RGBA16_FLOAT;
    description.BindFlags = Diligent::BIND_RENDER_TARGET | Diligent::BIND_SHADER_RESOURCE;
    impl_->device->CreateTexture(description, nullptr, &impl_->hdr_texture);
    if (impl_->hdr_texture) {
        impl_->hdr_render_target =
            impl_->hdr_texture->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET);
        impl_->hdr_shader_resource =
            impl_->hdr_texture->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
    }
    if (!impl_->hdr_texture || !impl_->hdr_render_target || !impl_->hdr_shader_resource) {
        throw std::runtime_error{"Diligent failed to create the HDR scene target"};
    }

    description.Name="Gloom particle immutable color snapshot";
    description.BindFlags=Diligent::BIND_SHADER_RESOURCE;
    impl_->device->CreateTexture(description,nullptr,&impl_->particle_color_copy);
    description.Format=Diligent::TEX_FORMAT_R32_FLOAT;
    description.Name="Gloom particle immutable depth snapshot";
    impl_->device->CreateTexture(description,nullptr,&impl_->particle_depth_copy);
    if (!impl_->particle_color_copy || !impl_->particle_depth_copy) throw std::runtime_error{"Particle snapshot allocation failed"};
    description.BindFlags=Diligent::BIND_RENDER_TARGET|Diligent::BIND_SHADER_RESOURCE;
    description.Name = "Gloom motion vectors";
    description.Format = Diligent::TEX_FORMAT_RG16_FLOAT;
    impl_->device->CreateTexture(description, nullptr, &impl_->motion_texture);
    if (impl_->motion_texture) {
        impl_->motion_render_target =
            impl_->motion_texture->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET);
        impl_->motion_shader_resource =
            impl_->motion_texture->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
    }
    description.Name = "Gloom linear depth";
    description.Format = Diligent::TEX_FORMAT_R32_FLOAT;
    impl_->device->CreateTexture(description, nullptr, &impl_->linear_depth_texture);
    if (impl_->linear_depth_texture) {
        impl_->linear_depth_render_target =
            impl_->linear_depth_texture->GetDefaultView(Diligent::TEXTURE_VIEW_RENDER_TARGET);
        impl_->linear_depth_shader_resource =
            impl_->linear_depth_texture->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
    }
    description.Name = "Gloom scene depth";
    description.Format = Diligent::TEX_FORMAT_D32_FLOAT;
    description.BindFlags = Diligent::BIND_DEPTH_STENCIL;
    impl_->device->CreateTexture(description, nullptr, &impl_->scene_depth_texture);
    if (impl_->scene_depth_texture) {
        impl_->scene_depth_target =
            impl_->scene_depth_texture->GetDefaultView(Diligent::TEXTURE_VIEW_DEPTH_STENCIL);
    }
    if (!impl_->motion_render_target || !impl_->motion_shader_resource ||
        !impl_->linear_depth_render_target || !impl_->linear_depth_shader_resource ||
        !impl_->scene_depth_target) {
        throw std::runtime_error{
            "Diligent failed to create motion-vector or scene-depth targets"};
    }

    description.Width = width;
    description.Height = height;
    description.Format = Diligent::TEX_FORMAT_RGBA16_FLOAT;
    description.BindFlags = Diligent::BIND_RENDER_TARGET | Diligent::BIND_SHADER_RESOURCE;
    for (std::size_t index = 0; index < impl_->history_textures.size(); ++index) {
        description.Name = index == 0 ? "Gloom temporal history A" : "Gloom temporal history B";
        impl_->device->CreateTexture(description, nullptr, &impl_->history_textures[index]);
        if (impl_->history_textures[index]) {
            impl_->history_render_targets[index] = impl_->history_textures[index]->GetDefaultView(
                Diligent::TEXTURE_VIEW_RENDER_TARGET);
            impl_->history_shader_resources[index] = impl_->history_textures[index]->GetDefaultView(
                Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
        }
        if (!impl_->history_render_targets[index] || !impl_->history_shader_resources[index]) {
            throw std::runtime_error{"Diligent failed to create temporal history resources"};
        }
    }
    description.Format = Diligent::TEX_FORMAT_R32_FLOAT;
    for (std::size_t index = 0; index < impl_->history_depth_textures.size(); ++index) {
        description.Name = index == 0 ? "Gloom depth history A" : "Gloom depth history B";
        impl_->device->CreateTexture(description, nullptr, &impl_->history_depth_textures[index]);
        if (impl_->history_depth_textures[index]) {
            impl_->history_depth_render_targets[index] =
                impl_->history_depth_textures[index]->GetDefaultView(
                    Diligent::TEXTURE_VIEW_RENDER_TARGET);
            impl_->history_depth_shader_resources[index] =
                impl_->history_depth_textures[index]->GetDefaultView(
                    Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
        }
        if (!impl_->history_depth_render_targets[index] ||
            !impl_->history_depth_shader_resources[index]) {
            throw std::runtime_error{"Diligent failed to create temporal depth history"};
        }
    }
    const std::array<Diligent::ITextureView*, 3> tone_inputs{
        impl_->hdr_shader_resource,
        impl_->history_shader_resources[0],
        impl_->history_shader_resources[1],
    };
    for (std::size_t index = 0; index < impl_->tone_map_resources.size(); ++index) {
        if (impl_->tone_map_resources[index] == nullptr) {
            continue;
        }
        if (auto* variable = impl_->tone_map_resources[index]->GetVariableByName(
                Diligent::SHADER_TYPE_PIXEL, "HdrTexture")) {
            variable->Set(tone_inputs[index], Diligent::SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
        }
    }
    for (std::size_t index = 0; index < impl_->temporal_resources.size(); ++index) {
        if (impl_->temporal_resources[index] == nullptr) {
            continue;
        }
        const auto bind = [&](const char* name, Diligent::ITextureView* view) {
            auto* variable = impl_->temporal_resources[index]->GetVariableByName(
                Diligent::SHADER_TYPE_PIXEL, name);
            if (variable == nullptr) {
                throw std::runtime_error{std::string{"Diligent lost temporal binding: "} + name};
            }
            variable->Set(view, Diligent::SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
        };
        bind("CurrentColor", impl_->hdr_shader_resource);
        bind("MotionVectors", impl_->motion_shader_resource);
        bind("HistoryColor", impl_->history_shader_resources[1U - index]);
        bind("CurrentDepth", impl_->linear_depth_shader_resource);
        bind("HistoryDepth", impl_->history_depth_shader_resources[1U - index]);
    }
}

void DiligentRenderer::tick([[maybe_unused]] const double delta_seconds) {}

void DiligentRenderer::enqueue(render::MeshUpload upload) {
    std::scoped_lock lock{impl_->upload_mutex};
    impl_->asset_states[upload.id] = render::GpuAssetState::queued;
    impl_->residency_metrics.queued_bytes += upload_size(upload);
    impl_->mesh_uploads.push_back(std::move(upload));
}

void DiligentRenderer::enqueue(render::TextureUpload upload) {
    std::scoped_lock lock{impl_->upload_mutex};
    impl_->asset_states[upload.id] = render::GpuAssetState::queued;
    impl_->residency_metrics.queued_bytes += upload_size(upload);
    impl_->texture_uploads.push_back(std::move(upload));
}

void DiligentRenderer::enqueue(render::MaterialUpload upload) {
    if (!render::valid_material_surface(upload.surface)) throw std::invalid_argument{"Invalid extended material surface"};
    std::scoped_lock lock{impl_->upload_mutex};
    impl_->asset_states[upload.id] = render::GpuAssetState::queued;
    impl_->material_uploads.push_back(std::move(upload));
}

void DiligentRenderer::release(const render::RenderAssetId id) {
    std::scoped_lock lock{impl_->upload_mutex};
    impl_->release_requests.push_back(id);
}

render::GpuAssetState DiligentRenderer::asset_state(const render::RenderAssetId id) const noexcept {
    std::scoped_lock lock{impl_->upload_mutex};
    const auto found = impl_->asset_states.find(id);
    return found == impl_->asset_states.end() ? render::GpuAssetState::missing : found->second;
}

render::GpuResidencyMetrics DiligentRenderer::residency_metrics() const noexcept {
    std::scoped_lock lock{impl_->upload_mutex};
    return impl_->residency_metrics;
}

render::FrameRenderMetrics DiligentRenderer::frame_metrics() const noexcept {
    return impl_->frame_metrics;
}

void DiligentRenderer::process_uploads() {
    std::vector<render::MeshUpload> meshes;
    std::vector<render::TextureUpload> textures;
    std::vector<render::MaterialUpload> materials;
    std::deque<render::RenderAssetId> releases;
    {
        std::scoped_lock lock{impl_->upload_mutex};
        std::uint64_t remaining = settings_.upload_budget_bytes_per_frame;
        const auto select_mesh = [&] {
            const auto bytes = upload_size(impl_->mesh_uploads.front());
            if (bytes > remaining && (!meshes.empty() || !textures.empty())) {
                return false;
            }
            remaining = bytes > remaining ? 0 : remaining - bytes;
            impl_->residency_metrics.queued_bytes -=
                std::min(impl_->residency_metrics.queued_bytes, bytes);
            meshes.push_back(std::move(impl_->mesh_uploads.front()));
            impl_->mesh_uploads.pop_front();
            return true;
        };
        const auto select_texture = [&] {
            const auto bytes = upload_size(impl_->texture_uploads.front());
            if (bytes > remaining && (!meshes.empty() || !textures.empty())) {
                return false;
            }
            remaining = bytes > remaining ? 0 : remaining - bytes;
            impl_->residency_metrics.queued_bytes -=
                std::min(impl_->residency_metrics.queued_bytes, bytes);
            textures.push_back(std::move(impl_->texture_uploads.front()));
            impl_->texture_uploads.pop_front();
            return true;
        };
        while (!impl_->mesh_uploads.empty() && select_mesh()) {
        }
        while (!impl_->texture_uploads.empty() && select_texture()) {
        }
        while (!impl_->material_uploads.empty()) {
            materials.push_back(std::move(impl_->material_uploads.front()));
            impl_->material_uploads.pop_front();
        }
        if (!impl_->mesh_uploads.empty() || !impl_->texture_uploads.empty()) {
            ++impl_->residency_metrics.budget_limited_frames;
        }
        releases.swap(impl_->release_requests);
    }
    const auto set_state = [&](const render::RenderAssetId id, const render::GpuAssetState state) {
        std::scoped_lock lock{impl_->upload_mutex};
        impl_->asset_states[id] = state;
    };

    const auto completed_fence = impl_->residency_fence->GetCompletedValue();
    const auto released_meshes = std::erase_if(impl_->deferred_meshes, [&](const auto& entry) {
        return entry.fence_value <= completed_fence;
    });
    const auto released_textures =
        std::erase_if(impl_->deferred_textures, [&](const auto& entry) {
            return entry.fence_value <= completed_fence;
        });
    if (released_meshes + released_textures > 0) {
        std::scoped_lock lock{impl_->upload_mutex};
        impl_->residency_metrics.completed_releases += released_meshes + released_textures;
    }

    const auto defer_asset = [&](const render::RenderAssetId id,
                                 const bool mark_missing,
                                 const bool eviction) {
        std::uint64_t removed_bytes = 0;
        std::uint64_t deferred = 0;
        if (auto mesh = impl_->meshes.find(id); mesh != impl_->meshes.end()) {
            removed_bytes += mesh->second.byte_size;
            impl_->deferred_meshes.push_back(
                {.resource = std::move(mesh->second),
                 .fence_value = impl_->submitted_fence_value});
            impl_->meshes.erase(mesh);
            ++deferred;
        }
        if (auto texture = impl_->textures.find(id); texture != impl_->textures.end()) {
            removed_bytes += texture->second.byte_size;
            impl_->deferred_textures.push_back(
                {.resource = std::move(texture->second),
                 .fence_value = impl_->submitted_fence_value});
            impl_->textures.erase(texture);
            ++deferred;
        }
        impl_->materials.erase(id);
        {
            std::scoped_lock lock{impl_->upload_mutex};
            impl_->residency_metrics.resident_bytes -=
                std::min(impl_->residency_metrics.resident_bytes, removed_bytes);
            impl_->residency_metrics.deferred_releases += deferred;
            if (eviction && removed_bytes > 0) {
                ++impl_->residency_metrics.evictions;
            }
            if (mark_missing) {
                impl_->asset_states.erase(id);
            }
        }
    };

    for (const auto id : releases) {
        if (id != render::builtin_cube_mesh && id != render::builtin_horizontal_quad_mesh &&
            id != render::builtin_default_material &&
            id != render::builtin_white_texture && id != render::builtin_flat_normal_texture) {
            defer_asset(id, true, false);
        }
    }

    for (auto& upload : meshes) {
        if (upload.id.value == 0 || upload.vertices.empty() || upload.indices.empty()) {
            set_state(upload.id, render::GpuAssetState::failed);
            continue;
        }
        Impl::MeshResource resource;
        Diligent::BufferDesc vertex_description;
        vertex_description.Name = "Gloom resident mesh vertices";
        vertex_description.Usage = Diligent::USAGE_IMMUTABLE;
        vertex_description.BindFlags = Diligent::BIND_VERTEX_BUFFER;
        vertex_description.Size = upload.vertices.size() * sizeof(render::GpuVertex);
        const Diligent::BufferData vertex_data{upload.vertices.data(), vertex_description.Size};
        impl_->device->CreateBuffer(vertex_description, &vertex_data, &resource.vertices);

        Diligent::BufferDesc index_description;
        index_description.Name = "Gloom resident mesh indices";
        index_description.Usage = Diligent::USAGE_IMMUTABLE;
        index_description.BindFlags = Diligent::BIND_INDEX_BUFFER;
        index_description.Size = upload.indices.size() * sizeof(std::uint32_t);
        const Diligent::BufferData index_data{upload.indices.data(), index_description.Size};
        impl_->device->CreateBuffer(index_description, &index_data, &resource.indices);
        resource.index_count = static_cast<Diligent::Uint32>(upload.indices.size());
        resource.byte_size = upload_size(upload);
        resource.last_used_frame = impl_->frame_index;
        if (!resource.vertices || !resource.indices) {
            set_state(upload.id, render::GpuAssetState::failed);
            continue;
        }
        defer_asset(upload.id, false, false);
        impl_->meshes.insert_or_assign(upload.id, std::move(resource));
        {
            std::scoped_lock lock{impl_->upload_mutex};
            impl_->residency_metrics.uploaded_bytes += upload_size(upload);
            impl_->residency_metrics.resident_bytes += upload_size(upload);
        }
        set_state(upload.id, render::GpuAssetState::resident);
    }

    for (auto& upload : textures) {
        bool valid = upload.id.value != 0 && !upload.mip_levels.empty();
        std::uint32_t expected_width = valid ? upload.mip_levels.front().width : 0;
        std::uint32_t expected_height = valid ? upload.mip_levels.front().height : 0;
        for (const auto& level : upload.mip_levels) {
            const bool compressed = upload.format != render::TextureFormat::rgba8;
            const std::size_t required_bytes = compressed
                                                   ? static_cast<std::size_t>((level.width + 3U) / 4U) *
                                                         ((level.height + 3U) / 4U) * 16U
                                                   : static_cast<std::size_t>(level.width) *
                                                         level.height * 4U;
            valid = valid && level.width == expected_width && level.height == expected_height &&
                    level.data.size() == required_bytes;
            expected_width = std::max(expected_width / 2U, 1U);
            expected_height = std::max(expected_height / 2U, 1U);
        }
        if (!valid) {
            set_state(upload.id, render::GpuAssetState::failed);
            continue;
        }
        Diligent::TextureDesc description;
        description.Name = "Gloom resident texture";
        description.Type = Diligent::RESOURCE_DIM_TEX_2D;
        description.Width = upload.mip_levels.front().width;
        description.Height = upload.mip_levels.front().height;
        description.MipLevels = static_cast<Diligent::Uint32>(upload.mip_levels.size());
        switch (upload.format) {
        case render::TextureFormat::bc5:
            description.Format = Diligent::TEX_FORMAT_BC5_UNORM;
            break;
        case render::TextureFormat::bc7:
            description.Format = upload.srgb ? Diligent::TEX_FORMAT_BC7_UNORM_SRGB
                                             : Diligent::TEX_FORMAT_BC7_UNORM;
            break;
        case render::TextureFormat::rgba8:
            description.Format = upload.srgb ? Diligent::TEX_FORMAT_RGBA8_UNORM_SRGB
                                             : Diligent::TEX_FORMAT_RGBA8_UNORM;
            break;
        }
        description.Usage = Diligent::USAGE_IMMUTABLE;
        description.BindFlags = Diligent::BIND_SHADER_RESOURCE;
        std::vector<Diligent::TextureSubResData> subresources;
        subresources.reserve(upload.mip_levels.size());
        for (const auto& level : upload.mip_levels) {
            const auto stride = upload.format == render::TextureFormat::rgba8
                                    ? static_cast<Diligent::Uint64>(level.width) * 4U
                                    : static_cast<Diligent::Uint64>((level.width + 3U) / 4U) * 16U;
            subresources.emplace_back(level.data.data(), stride);
        }
        Diligent::TextureData data{subresources.data(),
                                   static_cast<Diligent::Uint32>(subresources.size())};
        Impl::TextureResource resource;
        resource.byte_size = upload_size(upload);
        resource.last_used_frame = impl_->frame_index;
        impl_->device->CreateTexture(description, &data, &resource.texture);
        if (resource.texture) {
            resource.view = resource.texture->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE);
        }
        if (!resource.texture || !resource.view) {
            set_state(upload.id, render::GpuAssetState::failed);
            continue;
        }
        defer_asset(upload.id, false, false);
        impl_->textures.insert_or_assign(upload.id, std::move(resource));
        {
            std::scoped_lock lock{impl_->upload_mutex};
            impl_->residency_metrics.uploaded_bytes += upload_size(upload);
            impl_->residency_metrics.resident_bytes += upload_size(upload);
        }
        set_state(upload.id, render::GpuAssetState::resident);
    }

    for (auto& upload : materials) {
        if (upload.id.value == 0) {
            set_state(upload.id, render::GpuAssetState::failed);
            continue;
        }
        impl_->materials.insert_or_assign(upload.id, std::move(upload));
        set_state(upload.id, render::GpuAssetState::resident);
    }

    for (;;) {
        std::uint64_t resident_bytes = 0;
        {
            std::scoped_lock lock{impl_->upload_mutex};
            resident_bytes = impl_->residency_metrics.resident_bytes;
        }
        if (resident_bytes <= settings_.resident_budget_bytes) {
            break;
        }
        render::RenderAssetId oldest{};
        std::uint64_t oldest_frame = std::numeric_limits<std::uint64_t>::max();
        for (const auto& [id, resource] : impl_->meshes) {
            if (id != render::builtin_cube_mesh &&
                id != render::builtin_horizontal_quad_mesh &&
                resource.last_used_frame < oldest_frame) {
                oldest = id;
                oldest_frame = resource.last_used_frame;
            }
        }
        for (const auto& [id, resource] : impl_->textures) {
            if (id != render::builtin_white_texture &&
                id != render::builtin_flat_normal_texture &&
                resource.last_used_frame < oldest_frame) {
                oldest = id;
                oldest_frame = resource.last_used_frame;
            }
        }
        if (oldest.value == 0) {
            break;
        }
        defer_asset(oldest, true, true);
    }
}

void DiligentRenderer::stop() noexcept {
    if (impl_->immediate_context) {
        impl_->immediate_context->Flush();
        impl_->immediate_context->WaitForIdle();
    }
    impl_->scene_resources.Release();
    for (auto& pipeline : impl_->surface_pipelines) pipeline.Release();
    impl_->scene_pipeline.Release();
    impl_->draw_constants.Release();
    impl_->skin_constants.Release();
    for (auto& resources : impl_->tone_map_resources) {
        resources.Release();
    }
    impl_->tone_map_pipeline.Release();
    impl_->ui_resources.Release();impl_->ui_pipeline.Release();impl_->ui_vertices.Release();impl_->ui_bound_view=nullptr;impl_->ui.vertices.clear();
    impl_->tone_constants.Release();
    for (auto& resources : impl_->temporal_resources) {
        resources.Release();
    }
    impl_->temporal_pipeline.Release();
    impl_->temporal_constants.Release();
    impl_->point_light_buffer.Release();
    impl_->cluster_buffer.Release();
    impl_->light_index_buffer.Release();
    impl_->environment_shader_resource.Release();
    impl_->active_probe.reset();
    impl_->probe_texture.Release();
    impl_->environment_texture.Release();
    impl_->double_sided_shadow_pipeline.Release();
    impl_->shadow_pipeline.Release();
    impl_->shadow_resources.Release();
    impl_->shadow_constants.Release();
    for (auto& view : impl_->shadow_depth_targets) {
        view.Release();
    }
    impl_->shadow_shader_resource.Release();
    impl_->shadow_texture.Release();
    for (auto& timing : impl_->timing_frames) {
        timing.shadow.Release();
        timing.opaque.Release();
        timing.tone_map.Release();
        timing.temporal.Release();
        timing = {};
    }
    impl_->opaque_timing_active = false;
    for (std::size_t index = 0; index < impl_->history_textures.size(); ++index) {
        impl_->history_shader_resources[index].Release();
        impl_->history_render_targets[index].Release();
        impl_->history_textures[index].Release();
        impl_->history_depth_shader_resources[index].Release();
        impl_->history_depth_render_targets[index].Release();
        impl_->history_depth_textures[index].Release();
    }
    impl_->scene_depth_target.Release();
    impl_->scene_depth_texture.Release();
    impl_->motion_shader_resource.Release();
    impl_->motion_render_target.Release();
    impl_->motion_texture.Release();
    impl_->linear_depth_shader_resource.Release();
    impl_->linear_depth_render_target.Release();
    impl_->linear_depth_texture.Release();
    impl_->hdr_shader_resource.Release();
    impl_->hdr_render_target.Release();
    impl_->hdr_texture.Release();
    impl_->particle_color_copy.Release();impl_->particle_depth_copy.Release();
    impl_->materials.clear();
    impl_->textures.clear();
    impl_->meshes.clear();
    impl_->deferred_textures.clear();
    impl_->deferred_meshes.clear();
    {
        std::scoped_lock lock{impl_->upload_mutex};
        impl_->mesh_uploads.clear();
        impl_->texture_uploads.clear();
        impl_->material_uploads.clear();
        impl_->release_requests.clear();
        impl_->asset_states.clear();
        impl_->residency_metrics = {};
    }
    impl_->residency_fence.Release();
    impl_->swap_chain.Release();
    impl_->immediate_context.Release();
    impl_->device.Release();
    state_ = core::SubsystemState::stopped;
}

void DiligentRenderer::resize(const std::uint32_t width, const std::uint32_t height) {
    if (impl_->swap_chain) {
        if (impl_->output_extent.width == width && impl_->output_extent.height == height) return;
        // Frame targets also back mutable tone-map/TAA descriptors. Drain their
        // users before replacing them, including SDL's initial resize events.
        impl_->immediate_context->Flush();
        impl_->immediate_context->WaitForIdle();
        impl_->swap_chain->Resize(width, height);
        create_frame_resources(width, height);
    }
}

void DiligentRenderer::begin_frame() {
    impl_->ui.vertices.clear();
    if (state_ != core::SubsystemState::running) {
        throw std::logic_error{"Diligent renderer must be running before begin_frame"};
    }
    process_uploads();
    impl_->shadow_rendered = false;
    impl_->drew_scene = false;
    impl_->jitter = impl_->temporal_selection.technique == render::TemporalTechnique::taa
                        ? render::temporal_jitter(impl_->frame_index, impl_->render_extent)
                        : render::CameraJitter{};
    impl_->frame_metrics.frame_index = impl_->frame_index;
    impl_->frame_metrics.gpu_timing_supported = capabilities().gpu_timestamps;
    if (impl_->frame_metrics.gpu_timing_supported) {
        auto& timing = impl_->timing_frames[impl_->frame_index % Impl::timing_frame_count];
        const auto collect = [](Diligent::IQuery* query,
                                bool& pending,
                                std::uint64_t& nanoseconds) {
            if (!pending) {
                return;
            }
            Diligent::QueryDataDuration result;
            if (query->GetData(&result, sizeof(result), true)) {
                nanoseconds = result.Frequency == 0
                                  ? 0
                                  : static_cast<std::uint64_t>(
                                        static_cast<long double>(result.Duration) *
                                        1'000'000'000.0L /
                                        static_cast<long double>(result.Frequency));
                pending = false;
            }
        };
        collect(timing.opaque, timing.opaque_pending, impl_->frame_metrics.opaque_nanoseconds);
        collect(timing.shadow, timing.shadow_pending, impl_->frame_metrics.shadow_nanoseconds);
        collect(timing.tone_map,
                timing.tone_map_pending,
                impl_->frame_metrics.tone_map_nanoseconds);
        collect(timing.temporal,
                timing.temporal_pending,
                impl_->frame_metrics.temporal_resolve_nanoseconds);
    }
    const auto gpu_nanoseconds = impl_->frame_metrics.shadow_nanoseconds +
                                 impl_->frame_metrics.opaque_nanoseconds +
                                 impl_->frame_metrics.temporal_resolve_nanoseconds +
                                 impl_->frame_metrics.tone_map_nanoseconds;
    const float requested_scale = impl_->dynamic_resolution.update(
        static_cast<float>(gpu_nanoseconds) / 1'000'000.0F);
    const auto requested_extent = render::scaled_render_extent(impl_->output_extent,
                                                               requested_scale);
    if (requested_extent != impl_->render_extent && impl_->output_extent.width != 0 &&
        impl_->output_extent.height != 0) {
        impl_->immediate_context->Flush();
        impl_->immediate_context->WaitForIdle();
        impl_->temporal_selection.render_scale = requested_scale;
        create_frame_resources(impl_->output_extent.width, impl_->output_extent.height);
    }
    const auto& dynamic_metrics = impl_->dynamic_resolution.metrics();
    impl_->frame_metrics.render_width = impl_->render_extent.width;
    impl_->frame_metrics.render_height = impl_->render_extent.height;
    impl_->frame_metrics.output_width = impl_->output_extent.width;
    impl_->frame_metrics.output_height = impl_->output_extent.height;
    impl_->frame_metrics.render_scale = dynamic_metrics.scale;
    impl_->frame_metrics.filtered_gpu_frame_milliseconds =
        dynamic_metrics.filtered_frame_milliseconds;
    impl_->frame_metrics.dynamic_resolution_changes = dynamic_metrics.scale_changes;
    impl_->frame_metrics.temporal_history_valid = impl_->history_valid;
    impl_->frame_graph.clear();
    const auto back_buffer = impl_->frame_graph.add_resource(
        "swapchain color",
        {.external = true, .initial_state = render::ResourceState::present});
    const auto hdr_color = impl_->frame_graph.add_resource(
        "HDR scene color",
        {.format = Diligent::TEX_FORMAT_RGBA16_FLOAT,
         .width = impl_->render_extent.width,
         .height = impl_->render_extent.height,
         .external = true,
         .initial_state = render::ResourceState::shader_resource});
    const auto motion = impl_->frame_graph.add_resource(
        "motion vectors",
        {.format = Diligent::TEX_FORMAT_RG16_FLOAT,
         .width = impl_->render_extent.width,
         .height = impl_->render_extent.height,
         .external = true,
         .initial_state = render::ResourceState::shader_resource});
    const auto linear_depth = impl_->frame_graph.add_resource(
        "linear depth",
        {.format = Diligent::TEX_FORMAT_R32_FLOAT,
         .width = impl_->render_extent.width,
         .height = impl_->render_extent.height,
         .external = true,
         .initial_state = render::ResourceState::shader_resource});
    const auto history = impl_->frame_graph.add_resource(
        "temporal history",
        {.format = Diligent::TEX_FORMAT_RGBA16_FLOAT,
         .width = impl_->output_extent.width,
         .height = impl_->output_extent.height,
         .external = true,
         .initial_state = render::ResourceState::shader_resource});
    const auto history_depth = impl_->frame_graph.add_resource(
        "temporal depth history",
        {.format = Diligent::TEX_FORMAT_R32_FLOAT,
         .width = impl_->output_extent.width,
         .height = impl_->output_extent.height,
         .external = true,
         .initial_state = render::ResourceState::shader_resource});
    const auto depth = impl_->frame_graph.add_resource(
        "swapchain depth",
        {.external = true, .initial_state = render::ResourceState::depth_write});
    const auto shadow_depth = impl_->frame_graph.add_resource(
        "directional shadow cascades",
        {.format = Diligent::TEX_FORMAT_D32_FLOAT,
         .width = 1024,
         .height = 1024,
         .external = true,
         .initial_state = render::ResourceState::shader_resource});
    const render::ResourceUse shadow_uses[]{
        {shadow_depth, render::AccessMode::write, render::ResourceState::depth_write},
    };
    const auto shadow_pass = impl_->frame_graph.add_pass("directional shadow cascades",
                                                         shadow_uses);
    const render::ResourceUse clear_uses[]{
        {hdr_color, render::AccessMode::write, render::ResourceState::render_target},
        {motion, render::AccessMode::write, render::ResourceState::render_target},
        {linear_depth, render::AccessMode::write, render::ResourceState::render_target},
        {depth, render::AccessMode::write, render::ResourceState::depth_write},
    };
    const auto clear_pass = impl_->frame_graph.add_pass("clear", clear_uses);
    impl_->frame_graph.add_dependency(shadow_pass, clear_pass);
    const render::ResourceUse opaque_uses[]{
        {hdr_color, render::AccessMode::read_write, render::ResourceState::render_target},
        {motion, render::AccessMode::read_write, render::ResourceState::render_target},
        {linear_depth, render::AccessMode::read_write, render::ResourceState::render_target},
        {depth, render::AccessMode::read_write, render::ResourceState::depth_write},
    };
    const auto opaque_pass = impl_->frame_graph.add_pass("opaque PBR", opaque_uses);
    impl_->frame_graph.add_dependency(clear_pass, opaque_pass);
    const auto particle_color=impl_->frame_graph.add_resource("particle color snapshot",{.format=Diligent::TEX_FORMAT_RGBA16_FLOAT,.external=true});
    const auto particle_depth=impl_->frame_graph.add_resource("particle depth snapshot",{.format=Diligent::TEX_FORMAT_R32_FLOAT,.external=true});
    const render::ResourceUse particle_copy_uses[]{
        {hdr_color,render::AccessMode::read,render::ResourceState::copy_source},
        {linear_depth,render::AccessMode::read,render::ResourceState::copy_source},
        {particle_color,render::AccessMode::write,render::ResourceState::copy_destination},
        {particle_depth,render::AccessMode::write,render::ResourceState::copy_destination}};
    const auto particle_copy_pass=impl_->frame_graph.add_pass("particle scene snapshots",particle_copy_uses);
    impl_->frame_graph.add_dependency(opaque_pass,particle_copy_pass);
    const render::ResourceUse particle_uses[]{
        {particle_color,render::AccessMode::read,render::ResourceState::shader_resource},
        {particle_depth,render::AccessMode::read,render::ResourceState::shader_resource},
        {hdr_color,render::AccessMode::read_write,render::ResourceState::render_target}};
    const auto particle_pass=impl_->frame_graph.add_pass("sorted transparent particles and refraction",particle_uses);
    impl_->frame_graph.add_dependency(particle_copy_pass,particle_pass);
    auto tone_input = hdr_color;
    auto tone_dependency = particle_pass;
    if (impl_->temporal_selection.technique == render::TemporalTechnique::taa) {
        const render::ResourceUse temporal_uses[]{
            {hdr_color, render::AccessMode::read, render::ResourceState::shader_resource},
            {motion, render::AccessMode::read, render::ResourceState::shader_resource},
            {linear_depth, render::AccessMode::read, render::ResourceState::shader_resource},
            {history, render::AccessMode::read_write, render::ResourceState::render_target},
            {history_depth,
             render::AccessMode::read_write,
             render::ResourceState::render_target},
        };
        tone_dependency = impl_->frame_graph.add_pass("native TAA resolve", temporal_uses);
        impl_->frame_graph.add_dependency(particle_pass, tone_dependency);
        tone_input = history;
    }
    const render::ResourceUse tone_map_uses[]{
        {tone_input, render::AccessMode::read, render::ResourceState::shader_resource},
        {back_buffer, render::AccessMode::write, render::ResourceState::render_target},
    };
    const auto tone_map_pass = impl_->frame_graph.add_pass("ACES tone map", tone_map_uses);
    impl_->frame_graph.add_dependency(tone_dependency, tone_map_pass);
    const render::ResourceUse present_uses[]{
        {back_buffer, render::AccessMode::read, render::ResourceState::present},
    };
    const auto present_pass = impl_->frame_graph.add_pass("present", present_uses);
    impl_->frame_graph.add_dependency(tone_map_pass, present_pass);
    if (const auto graph = impl_->frame_graph.compile(); !graph) {
        throw std::runtime_error{"Could not compile frame render graph: " + graph.error()};
    }

    if (!impl_->hdr_render_target || !impl_->motion_render_target ||
        !impl_->linear_depth_render_target || !impl_->scene_depth_target) {
        return;
    }
    Diligent::ITextureView* render_targets[]{impl_->hdr_render_target,
                                             impl_->motion_render_target,
                                             impl_->linear_depth_render_target};
    auto* const depth_target = impl_->scene_depth_target.RawPtr();
    impl_->immediate_context->SetRenderTargets(3,
                                               render_targets,
                                               depth_target,
                                               Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    constexpr std::array clear_color{0.025F, 0.035F, 0.065F, 1.0F};
    impl_->immediate_context->ClearRenderTarget(render_targets[0],
                                                clear_color.data(),
                                                Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    constexpr std::array clear_motion{0.0F, 0.0F, 0.0F, 0.0F};
    impl_->immediate_context->ClearRenderTarget(render_targets[1],
                                                clear_motion.data(),
                                                Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    constexpr std::array clear_linear_depth{1.0F, 1.0F, 1.0F, 1.0F};
    impl_->immediate_context->ClearRenderTarget(render_targets[2],
                                                clear_linear_depth.data(),
                                                Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
    impl_->immediate_context->ClearDepthStencil(depth_target,
                                                Diligent::CLEAR_DEPTH_FLAG,
                                                1.0F,
                                                0,
                                                Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
}

void DiligentRenderer::draw(const render::RenderSnapshot& snapshot) {
    if(snapshot.ui){
        if(snapshot.ui->vertices.size()>65536)throw std::invalid_argument{"UI draw list exceeds capacity"};
        impl_->ui=*snapshot.ui;
    }
    if (state_ != core::SubsystemState::running) {
        throw std::logic_error{"Diligent renderer must be running before drawing a snapshot"};
    }
    const auto& swap_chain_description = impl_->swap_chain->GetDesc();
    if (swap_chain_description.Width == 0 || swap_chain_description.Height == 0 ||
        !impl_->hdr_render_target || snapshot.instances.empty()) {
        return;
    }
    // A frame-local allocation is required even for static draws sharing the shader.
    {
        Diligent::MapHelper<SkinConstants> constants{impl_->immediate_context,impl_->skin_constants,Diligent::MAP_WRITE,Diligent::MAP_FLAG_DISCARD};
        if (static_cast<SkinConstants*>(constants)==nullptr) throw std::runtime_error{"Skin constant mapping failed"};
    }
    const auto upload_skin=[&](const render::RenderInstance& instance) {
        if (instance.pose) {
            Diligent::MapHelper<SkinConstants> constants{impl_->immediate_context,impl_->skin_constants,Diligent::MAP_WRITE,Diligent::MAP_FLAG_DISCARD};
            if (static_cast<SkinConstants*>(constants)==nullptr) throw std::runtime_error{"Skin constant mapping failed"};
                const auto& pose=*instance.pose;
                if (pose.matrices.size()>256 || pose.normal_matrices.size()!=pose.matrices.size())
                    throw std::runtime_error{"Invalid GPU skin palette"};
                const auto& previous=instance.previous_pose && instance.has_previous_transform &&
                    instance.previous_pose->matrices.size()==pose.matrices.size()?*instance.previous_pose:pose;
                for (std::size_t j=0;j<pose.matrices.size();++j) {
                    std::memcpy(&constants->bones[j],pose.matrices[j].data(),64);
                    std::memcpy(&constants->previous_bones[j],previous.matrices[j].data(),64);
                    std::memcpy(&constants->bone_normals[j],pose.normal_matrices[j].data(),64);
                }
            }
    };
    const float aspect = static_cast<float>(impl_->output_extent.width) /
                         static_cast<float>(impl_->output_extent.height);
    const auto view = camera_view(snapshot.camera);
    const auto projection = Diligent::float4x4::Projection(snapshot.camera.vertical_field_of_view_radians,
                                                           aspect,
                                                           snapshot.camera.near_plane,
                                                           snapshot.camera.far_plane,
                                                           false);
    const auto current_view_projection = view * projection;
    if (impl_->previous_camera_valid) {
        const auto distance_squared = [](const render::Vec3 a, const render::Vec3 b) {
            const float x = a.x - b.x;
            const float y = a.y - b.y;
            const float z = a.z - b.z;
            return x * x + y * y + z * z;
        };
        const bool camera_cut = distance_squared(snapshot.camera.position,
                                                  impl_->previous_camera.position) > 25.0F ||
                                distance_squared(snapshot.camera.target,
                                                 impl_->previous_camera.target) > 25.0F ||
                                std::abs(snapshot.camera.vertical_field_of_view_radians -
                                         impl_->previous_camera.vertical_field_of_view_radians) >
                                    0.1F;
        if (camera_cut) {
            impl_->history_valid = false;
        }
    }
    const auto previous_view_projection = impl_->previous_camera_valid && impl_->history_valid
                                              ? impl_->previous_view_projection
                                              : current_view_projection;

    render::DirectionalLight directional;
    render::EnvironmentLighting environment;
    render::LightClusterGrid cluster_grid;
    std::uint32_t point_light_count = 0;
    const auto upload_dynamic_buffer = [&](Diligent::IBuffer* buffer,
                                           const void* data,
                                           const std::size_t bytes) {
        void* mapped = nullptr;
        impl_->immediate_context->MapBuffer(buffer,
                                            Diligent::MAP_WRITE,
                                            Diligent::MAP_FLAG_DISCARD,
                                            mapped);
        if (mapped == nullptr) {
            throw std::runtime_error{"Could not map clustered-light buffer"};
        }
        if (bytes != 0) {
            std::memcpy(mapped, data, bytes);
        }
        impl_->immediate_context->UnmapBuffer(buffer, Diligent::MAP_WRITE);
    };
    if (snapshot.lighting != nullptr) {
        directional = snapshot.lighting->directional;
        environment = snapshot.lighting->environment;
        cluster_grid = snapshot.lighting->grid;
        point_light_count = static_cast<std::uint32_t>(snapshot.lighting->point_lights.size());
        if (point_light_count > 256U || snapshot.lighting->clusters.size() > 16U * 9U * 24U ||
            snapshot.lighting->light_indices.size() > 16U * 9U * 24U * 64U) {
            throw std::length_error{"Clustered lighting exceeds renderer buffer capacity"};
        }
        std::array<PointLightGpu, 256> gpu_lights{};
        for (std::size_t light_index = 0; light_index < snapshot.lighting->point_lights.size(); ++light_index) {
            const auto& light = snapshot.lighting->point_lights[light_index];
            gpu_lights[light_index] = {
                .position_range = {light.position.x,
                                   light.position.y,
                                   light.position.z,
                                   light.range},
                .color_intensity = {light.color.x,
                                    light.color.y,
                                    light.color.z,
                                    light.intensity},
            };
        }
        upload_dynamic_buffer(impl_->point_light_buffer,
                              gpu_lights.data(),
                              point_light_count * sizeof(PointLightGpu));
        upload_dynamic_buffer(
            impl_->cluster_buffer,
            snapshot.lighting->clusters.data(),
            snapshot.lighting->clusters.size() * sizeof(render::LightClusterRange));
        upload_dynamic_buffer(
            impl_->light_index_buffer,
            snapshot.lighting->light_indices.data(),
            snapshot.lighting->light_indices.size() * sizeof(std::uint32_t));
        impl_->frame_metrics.point_lights = point_light_count;
        impl_->frame_metrics.light_references = snapshot.lighting->light_indices.size();
    } else {
        // Diligent discards every dynamic allocation at frame end. The buffers
        // remain bound to the scene SRB, so they must receive a fresh allocation
        // even when clustered lighting is disabled for this snapshot.
        upload_dynamic_buffer(impl_->point_light_buffer, nullptr, 0);
        upload_dynamic_buffer(impl_->cluster_buffer, nullptr, 0);
        upload_dynamic_buffer(impl_->light_index_buffer, nullptr, 0);
        environment = {};
        impl_->frame_metrics.point_lights = 0;
        impl_->frame_metrics.light_references = 0;
    }
    if (environment.probe != impl_->active_probe) {
        // Probe replacement is an explicit scene change, never a per-frame upload.
        impl_->immediate_context->WaitForIdle();
        impl_->probe_texture.Release();
        if (environment.probe) {
            const auto& probe=*environment.probe;
            if (probe.size==0 || probe.size>1024 || probe.mip_levels==0 || probe.mip_levels>11)
                throw std::invalid_argument{"Invalid environment probe dimensions"};
            std::vector<Diligent::TextureSubResData> subresources;
            std::size_t offset=0;
            for (unsigned face=0;face<6;++face) for (unsigned level=0;level<probe.mip_levels;++level) {
                const auto size=std::max(1U,probe.size>>level);
                if (offset+size*size>probe.radiance.size()) throw std::invalid_argument{"Truncated environment probe"};
                Diligent::TextureSubResData sub;
                sub.pData=probe.radiance.data()+offset;sub.Stride=size*sizeof(probe.radiance.front());
                subresources.push_back(sub);offset+=size*size;
            }
            if (offset!=probe.radiance.size()) throw std::invalid_argument{"Unexpected environment probe texels"};
            Diligent::TextureDesc description;
            description.Name="Gloom static GGX environment probe";
            description.Type=Diligent::RESOURCE_DIM_TEX_CUBE;description.Width=description.Height=probe.size;
            description.ArraySize=6;description.MipLevels=probe.mip_levels;
            description.Format=Diligent::TEX_FORMAT_RGBA32_FLOAT;description.Usage=Diligent::USAGE_IMMUTABLE;
            description.BindFlags=Diligent::BIND_SHADER_RESOURCE;
            Diligent::TextureData data{subresources.data(),static_cast<Diligent::Uint32>(subresources.size())};
            impl_->device->CreateTexture(description,&data,&impl_->probe_texture);
            if (!impl_->probe_texture) throw std::runtime_error{"Environment probe upload failed"};
        }
        impl_->active_probe=environment.probe;
        impl_->scene_resources->GetVariableByName(Diligent::SHADER_TYPE_PIXEL,"EnvironmentTexture")->Set(
            impl_->probe_texture ? impl_->probe_texture->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE) : impl_->environment_shader_resource.RawPtr(),
            Diligent::SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
    }
    impl_->exposure = environment.exposure;
    const auto shadows = shadow_frame(snapshot.camera, directional,
        static_cast<float>(impl_->render_extent.width) / static_cast<float>(impl_->render_extent.height));

    if (!impl_->shadow_rendered && impl_->shadow_pipeline) {
        auto* timing = impl_->frame_metrics.gpu_timing_supported
                           ? &impl_->timing_frames[impl_->frame_index % Impl::timing_frame_count]
                           : nullptr;
        const bool measure_shadows = timing != nullptr && !timing->shadow_pending;
        if (measure_shadows) {
            impl_->immediate_context->BeginQuery(timing->shadow);
        }
        impl_->immediate_context->SetPipelineState(impl_->shadow_pipeline);

        for (std::size_t cascade = 0; cascade < impl_->shadow_depth_targets.size(); ++cascade) {
            auto* depth_target = impl_->shadow_depth_targets[cascade].RawPtr();
            impl_->immediate_context->SetRenderTargets(
                0,
                nullptr,
                depth_target,
                Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            impl_->immediate_context->SetViewports(1, nullptr, 0, 0);
            impl_->immediate_context->ClearDepthStencil(
                depth_target,
                Diligent::CLEAR_DEPTH_FLAG,
                1.0F,
                0,
                Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            if (!directional.casts_shadows) {
                continue;
            }
            for (const auto& instance : snapshot.shadow_instances.empty() ? snapshot.instances : snapshot.shadow_instances) {
                if (instance.view_model || !instance.casts_shadow ||
                    std::abs(instance.transform.scale.x * instance.transform.scale.y * instance.transform.scale.z) < 1.0e-12F) continue;
                auto material=impl_->materials.find(instance.material);
                if (material==impl_->materials.end()) material=impl_->materials.find(render::builtin_default_material);
                const auto& shadow_material=material->second;
                if (shadow_material.surface.alpha_mode>=2) continue;
                impl_->immediate_context->SetPipelineState(shadow_material.surface.double_sided ? impl_->double_sided_shadow_pipeline : impl_->shadow_pipeline);
                auto texture=impl_->textures.find(shadow_material.base_color_texture);
                if (texture==impl_->textures.end()) texture=impl_->textures.find(render::builtin_white_texture);
                impl_->shadow_resources->GetVariableByName(Diligent::SHADER_TYPE_PIXEL,"ShadowBaseColor")->Set(texture->second.view,Diligent::SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
                impl_->immediate_context->CommitShaderResources(impl_->shadow_resources,Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
                const auto mesh = impl_->meshes.find(instance.mesh);
                if (mesh == impl_->meshes.end()) {
                    continue;
                }
                Diligent::IBuffer* vertex_buffers[] = {mesh->second.vertices};
                constexpr Diligent::Uint64 offsets[] = {0};
                impl_->immediate_context->SetVertexBuffers(
                    0,
                    1,
                    vertex_buffers,
                    offsets,
                    Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
                    Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);
                impl_->immediate_context->SetIndexBuffer(
                    mesh->second.indices,
                    0,
                    Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
                upload_skin(instance);
                {
                    Diligent::MapHelper<ShadowConstants> constants{
                        impl_->immediate_context,
                        impl_->shadow_constants,
                        Diligent::MAP_WRITE,
                        Diligent::MAP_FLAG_DISCARD};
                    constants->skin_flags={instance.pose?static_cast<float>(instance.pose->matrices.size()):0,0,0,0};
                    constants->world_view_projection = world_matrix(instance.transform) *
                                                       shadows.matrices[cascade];
                    const auto& uv=shadow_material.surface.mapping[0];
                    constants->mapping={uv.scale[0],uv.scale[1],uv.offset[0]+shadow_material.surface.uv_scroll[0]*snapshot.presentation_seconds,uv.offset[1]+shadow_material.surface.uv_scroll[1]*snapshot.presentation_seconds};
                    constants->rotation={std::cos(uv.rotation),std::sin(uv.rotation),static_cast<float>(uv.uv_set),0};
                    constants->alpha={shadow_material.surface.alpha_mode==1?1.0F:0.0F,shadow_material.surface.alpha_cutoff,shadow_material.base_color[3]*instance.color.alpha,0};
                }
                Diligent::DrawIndexedAttribs draw_attributes;
                draw_attributes.NumIndices = mesh->second.index_count;
                draw_attributes.IndexType = Diligent::VT_UINT32;
                draw_attributes.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
                impl_->immediate_context->DrawIndexed(draw_attributes);
            }
        }
        Diligent::ITextureView* render_targets[]{impl_->hdr_render_target,
                                                 impl_->motion_render_target,
                                                 impl_->linear_depth_render_target};
        auto* depth_target = impl_->scene_depth_target.RawPtr();
        impl_->immediate_context->SetRenderTargets(
            3,
            render_targets,
            depth_target,
            Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        impl_->immediate_context->SetViewports(1, nullptr, 0, 0);
        if (measure_shadows) {
            impl_->immediate_context->EndQuery(timing->shadow);
            timing->shadow_pending = true;
        }
        impl_->shadow_rendered = true;
    }

    if (impl_->frame_metrics.gpu_timing_supported && !impl_->opaque_timing_active) {
        auto& timing = impl_->timing_frames[impl_->frame_index % Impl::timing_frame_count];
        if (!timing.opaque_pending) {
            impl_->immediate_context->BeginQuery(timing.opaque);
            impl_->opaque_timing_active = true;
        }
    }

    impl_->immediate_context->SetPipelineState(impl_->scene_pipeline);
    const auto draw_batch = [&](const render::RenderAssetId mesh_id,
                                const render::RenderAssetId material_id,
                                const std::span<const render::RenderInstance> instances, bool view_models) {
        const auto mesh = impl_->meshes.find(mesh_id);
        if (mesh == impl_->meshes.end()) {
            return;
        }
        const auto material = impl_->materials.find(material_id);
        const render::MaterialUpload default_material{.id = render::builtin_default_material};
        const auto& material_data =
            material == impl_->materials.end() ? default_material : material->second;
        const auto variant=std::min(material_data.surface.alpha_mode,3U)*2U+(material_data.surface.double_sided?1U:0U);
        impl_->immediate_context->SetPipelineState(impl_->surface_pipelines[variant]);
        const auto find_texture = [&](const render::RenderAssetId requested,
                                      const render::RenderAssetId fallback) {
            auto found = impl_->textures.find(requested);
            return found == impl_->textures.end() ? impl_->textures.find(fallback) : found;
        };
        auto base_color_texture =
            find_texture(material_data.base_color_texture, render::builtin_white_texture);
        auto metallic_roughness_texture = find_texture(
            material_data.metallic_roughness_texture, render::builtin_white_texture);
        auto normal_texture =
            find_texture(material_data.normal_texture, render::builtin_flat_normal_texture);
        if (base_color_texture == impl_->textures.end() ||
            metallic_roughness_texture == impl_->textures.end() ||
            normal_texture == impl_->textures.end()) {
            return;
        }
        mesh->second.last_used_frame = impl_->frame_index;
        base_color_texture->second.last_used_frame = impl_->frame_index;
        metallic_roughness_texture->second.last_used_frame = impl_->frame_index;
        normal_texture->second.last_used_frame = impl_->frame_index;
        const auto bind_texture = [&](const char* name, Diligent::ITextureView* view) {
            auto* const variable = impl_->scene_resources->GetVariableByName(
                Diligent::SHADER_TYPE_PIXEL, name);
            if (variable == nullptr) {
                throw std::runtime_error{std::string{"Diligent lost texture binding: "} + name};
            }
            variable->Set(view, Diligent::SET_SHADER_RESOURCE_FLAG_ALLOW_OVERWRITE);
        };
        const bool particle=!instances.empty() && instances.front().particle;
        bind_texture("SceneColorCopy",particle?impl_->particle_color_copy->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE):base_color_texture->second.view.RawPtr());
        bind_texture("SceneDepthCopy",particle?impl_->particle_depth_copy->GetDefaultView(Diligent::TEXTURE_VIEW_SHADER_RESOURCE):base_color_texture->second.view.RawPtr());
        bind_texture("BaseColorTexture", base_color_texture->second.view);
        bind_texture("MetallicRoughnessTexture", metallic_roughness_texture->second.view);
        bind_texture("NormalTexture", normal_texture->second.view);
        constexpr std::array extra_names{"EmissiveTexture","OcclusionTexture","SpecularTexture",
                                         "SpecularColorTexture","AnisotropyTexture","DetailTexture","LightmapTexture"};
        for (std::size_t slot=0;slot<extra_names.size();++slot) {
            auto texture=find_texture(material_data.extra_textures[slot],render::builtin_white_texture);
            texture->second.last_used_frame=impl_->frame_index;
            bind_texture(extra_names[slot],texture->second.view);
        }
        impl_->immediate_context->CommitShaderResources(
            impl_->scene_resources, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);

        Diligent::IBuffer* vertex_buffers[] = {mesh->second.vertices};
        constexpr Diligent::Uint64 offsets[] = {0};
        impl_->immediate_context->SetVertexBuffers(
            0,
            1,
            vertex_buffers,
            offsets,
            Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
            Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);
        impl_->immediate_context->SetIndexBuffer(
            mesh->second.indices, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        for (const auto& instance : instances) {
            if (instance.view_model != view_models || std::abs(instance.transform.scale.x * instance.transform.scale.y * instance.transform.scale.z) < 1.0e-12F) continue;
            const auto world = world_matrix(instance.transform);
            const auto previous_world = world_matrix(instance.has_previous_transform
                                                          ? instance.previous_transform
                                                          : instance.transform);
            upload_skin(instance);
            Diligent::MapHelper<DrawConstants> constants{impl_->immediate_context,
                                                         impl_->draw_constants,
                                                         Diligent::MAP_WRITE,
                                                         Diligent::MAP_FLAG_DISCARD};
            if (static_cast<DrawConstants*>(constants)==nullptr) throw std::runtime_error{"Draw constant mapping failed"};
            constants->skin_flags={instance.pose?static_cast<float>(instance.pose->matrices.size()):0,instance.particle?1.0F:0.0F,instance.distortion,instance.soft_distance};
            constants->particle_depth={snapshot.camera.near_plane,snapshot.camera.far_plane,1.0F/static_cast<float>(impl_->render_extent.width),1.0F/static_cast<float>(impl_->render_extent.height)};
            constants->world = world;
            constants->normal_world = world_matrix(render::normal_transform(instance.transform));
            const auto camera_forward = Diligent::normalize(to_diligent(snapshot.camera.target) - to_diligent(snapshot.camera.position));
            constants->camera_forward = {camera_forward.x, camera_forward.y, camera_forward.z, 0.0F};
            constants->world_view_projection = world * current_view_projection;
            constants->previous_world_view_projection =
                previous_world * previous_view_projection;
            constants->temporal_jitter = {impl_->jitter.x,
                                          impl_->jitter.y,
                                          impl_->previous_jitter.x,
                                          impl_->previous_jitter.y};
            constants->color = {instance.color.red,
                                instance.color.green,
                                instance.color.blue,
                                instance.color.alpha};
            constants->base_color = {material_data.base_color[0],
                                     material_data.base_color[1],
                                     material_data.base_color[2],
                                     material_data.base_color[3]};
            constants->material_parameters = {
                material_data.metallic, material_data.roughness,
                instance.transform.scale.x * instance.transform.scale.y * instance.transform.scale.z < 0.0F ? -1.0F : 1.0F,
                instance.view_model ? 1.0F : 0.0F};
            constants->emissive = {material_data.emissive[0],
                                   material_data.emissive[1],
                                   material_data.emissive[2],
                                   0.0F};
            const auto& surface=material_data.surface;
            constants->surface_parameters={surface.normal_scale,surface.occlusion_strength,surface.specular_factor,surface.anisotropy_strength};
            constants->specular_color_rotation={surface.specular_color[0],surface.specular_color[1],surface.specular_color[2],surface.anisotropy_rotation};
            constants->surface_animation={surface.uv_scroll[0],surface.uv_scroll[1],surface.lava_wave,surface.alpha_cutoff};
            constants->surface_flags={static_cast<float>(surface.alpha_mode),surface.double_sided?1.0F:0.0F,
                snapshot.presentation_seconds,material_data.extra_textures[4]!=render::builtin_white_texture?1.0F:0.0F};
            for (std::size_t slot=0;slot<surface.mapping.size();++slot) {
                const auto& map=surface.mapping[slot];
                constants->texture_mappings[slot*2]={map.scale[0],map.scale[1],map.offset[0],map.offset[1]};
                constants->texture_mappings[slot*2+1]={std::cos(map.rotation),std::sin(map.rotation),static_cast<float>(map.uv_set),0.0F};
            }
            constants->camera_exposure = {snapshot.camera.position.x,
                                          snapshot.camera.position.y,
                                          snapshot.camera.position.z,
                                          environment.exposure};
            constants->directional_direction_intensity = {
                directional.direction.x,
                directional.direction.y,
                directional.direction.z,
                directional.intensity};
            constants->directional_color = {
                directional.color.x, directional.color.y, directional.color.z, environment.ambient_fill};
            constants->environment_sky = {environment.sky_radiance.x,
                                          environment.sky_radiance.y,
                                          environment.sky_radiance.z,
                                          environment.intensity};
            constants->environment_ground = {environment.ground_radiance.x,
                                             environment.ground_radiance.y,
                                             environment.ground_radiance.z,
                                             environment.probe ? static_cast<float>(environment.probe->mip_levels-1) : 0.0F};
            constants->cluster_dimensions = {cluster_grid.width,
                                             cluster_grid.height,
                                             cluster_grid.depth,
                                             point_light_count};
            constants->cluster_depth_viewport = {
                cluster_grid.near_plane,
                cluster_grid.far_plane,
                static_cast<float>(impl_->render_extent.width),
                static_cast<float>(impl_->render_extent.height)};
            constants->shadow_matrices = shadows.matrices;
            constants->shadow_splits = shadows.splits;
            Diligent::DrawIndexedAttribs draw_attributes;
            draw_attributes.NumIndices = mesh->second.index_count;
            draw_attributes.IndexType = Diligent::VT_UINT32;
            draw_attributes.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
            impl_->immediate_context->DrawIndexed(draw_attributes);
        }
    };
    for (const bool view_models : {false, true}) {
        if (view_models) impl_->immediate_context->ClearDepthStencil(impl_->scene_depth_target,
            Diligent::CLEAR_DEPTH_FLAG, 1.0F, 0, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        std::vector<const render::RenderInstance*> transparent;
        if (snapshot.batches.empty()) {
            for (const auto& instance : snapshot.instances) {
                if (instance.view_model != view_models) continue;
                const auto material=impl_->materials.find(instance.material);
                if (material!=impl_->materials.end() && material->second.surface.alpha_mode>=2) transparent.push_back(&instance);
                else draw_batch(instance.mesh,instance.material,{&instance,1},view_models);
            }
        } else {
            for (const auto& batch : snapshot.batches) {
                if (batch.first_instance >= snapshot.instances.size()) continue;
                const std::size_t count = std::min<std::size_t>(batch.instance_count,
                                                                 snapshot.instances.size() - batch.first_instance);
                const auto instances = snapshot.instances.subspan(batch.first_instance, count);
                bool has_opaque = false;
                for (const auto& instance : instances) {
                    if (instance.view_model != view_models) continue;
                    const auto material=impl_->materials.find(instance.material);
                    if (material!=impl_->materials.end() && material->second.surface.alpha_mode>=2) transparent.push_back(&instance);
                    else has_opaque = true;
                }
                if (has_opaque) draw_batch(batch.mesh, batch.material, instances, view_models);
            }
        }
        if (std::ranges::any_of(transparent,[](const auto* instance){return instance->particle;})) {
            impl_->immediate_context->SetRenderTargets(0,nullptr,nullptr,Diligent::RESOURCE_STATE_TRANSITION_MODE_NONE);
            impl_->immediate_context->CopyTexture({impl_->hdr_texture,Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
                impl_->particle_color_copy,Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION});
            impl_->immediate_context->CopyTexture({impl_->linear_depth_texture,Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
                impl_->particle_depth_copy,Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION});
            Diligent::ITextureView* targets[]{impl_->hdr_render_target,impl_->motion_render_target,impl_->linear_depth_render_target};
            impl_->immediate_context->SetRenderTargets(3,targets,impl_->scene_depth_target,Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        }
        const auto distance=[&](const auto* instance) {
            const auto delta=to_diligent(instance->transform.position)-to_diligent(snapshot.camera.position);
            return Diligent::dot(delta,delta);
        };
        std::stable_sort(transparent.begin(),transparent.end(),[&](const auto* a,const auto* b){return distance(a)>distance(b);});
        for (const auto* instance : transparent) draw_batch(instance->mesh,instance->material,{instance,1},view_models);
    }
    impl_->previous_view_projection = current_view_projection;
    impl_->previous_camera = snapshot.camera;
    impl_->previous_camera_valid = true;
    impl_->drew_scene = true;
}

void DiligentRenderer::capture_next_frame(std::filesystem::path path) {
    if (path.empty() || !impl_->capture_path.empty()) throw std::invalid_argument{"Invalid or pending frame capture"};
    impl_->capture_path = std::move(path);
}

void DiligentRenderer::end_frame() {
    if (state_ != core::SubsystemState::running) {
        throw std::logic_error{"Diligent renderer must be running before end_frame"};
    }
    const auto& swap_chain_description = impl_->swap_chain->GetDesc();
    auto* timing = impl_->frame_metrics.gpu_timing_supported
                       ? &impl_->timing_frames[impl_->frame_index % Impl::timing_frame_count]
                       : nullptr;
    if (impl_->opaque_timing_active && timing != nullptr) {
        impl_->immediate_context->EndQuery(timing->opaque);
        timing->opaque_pending = true;
        impl_->opaque_timing_active = false;
    }
    std::size_t tone_resource_index = 0;
    if (swap_chain_description.Width != 0 && swap_chain_description.Height != 0 &&
        impl_->temporal_selection.technique == render::TemporalTechnique::taa &&
        impl_->temporal_pipeline && impl_->hdr_shader_resource) {
        const std::size_t history_index = impl_->frame_index % 2U;
        const bool measure_temporal = timing != nullptr && !timing->temporal_pending;
        if (measure_temporal) {
            impl_->immediate_context->BeginQuery(timing->temporal);
        }
        Diligent::ITextureView* history_targets[]{
            impl_->history_render_targets[history_index],
            impl_->history_depth_render_targets[history_index],
        };
        impl_->immediate_context->SetRenderTargets(
            2,
            history_targets,
            nullptr,
            Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        impl_->immediate_context->SetPipelineState(impl_->temporal_pipeline);
        {
            Diligent::MapHelper<TemporalConstants> constants{impl_->immediate_context,
                                                             impl_->temporal_constants,
                                                             Diligent::MAP_WRITE,
                                                             Diligent::MAP_FLAG_DISCARD};
            constants->inverse_render_width = 1.0F / static_cast<float>(impl_->render_extent.width);
            constants->inverse_render_height =
                1.0F / static_cast<float>(impl_->render_extent.height);
            constants->history_weight =
                impl_->history_valid ? impl_->temporal_selection.history_weight : 0.0F;
        }
        impl_->immediate_context->CommitShaderResources(
            impl_->temporal_resources[history_index],
            Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        Diligent::DrawAttribs resolve_draw;
        resolve_draw.NumVertices = 3;
        resolve_draw.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
        impl_->immediate_context->Draw(resolve_draw);
        if (measure_temporal) {
            impl_->immediate_context->EndQuery(timing->temporal);
            timing->temporal_pending = true;
        }
        impl_->history_valid = true;
        impl_->frame_metrics.temporal_history_valid = true;
        tone_resource_index = history_index + 1U;
    }
    if (swap_chain_description.Width != 0 && swap_chain_description.Height != 0 &&
        impl_->hdr_shader_resource && impl_->tone_map_pipeline) {
        const bool measure_tone_map = timing != nullptr && !timing->tone_map_pending;
        if (measure_tone_map) {
            impl_->immediate_context->BeginQuery(timing->tone_map);
        }
        auto* render_target = impl_->swap_chain->GetCurrentBackBufferRTV();
        impl_->immediate_context->SetRenderTargets(
            1,
            &render_target,
            nullptr,
            Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        impl_->immediate_context->SetPipelineState(impl_->tone_map_pipeline);
        {
            Diligent::MapHelper<ToneConstants> constants{impl_->immediate_context,
                                                         impl_->tone_constants,
                                                         Diligent::MAP_WRITE,
                                                         Diligent::MAP_FLAG_DISCARD};
            constants->exposure = impl_->exposure;
            constants->inverse_output_width =
                1.0F / static_cast<float>(impl_->output_extent.width);
            constants->inverse_output_height =
                1.0F / static_cast<float>(impl_->output_extent.height);
            constants->sharpness = impl_->temporal_selection.sharpness;
        }
        impl_->immediate_context->CommitShaderResources(
            impl_->tone_map_resources[tone_resource_index],
            Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
        Diligent::DrawAttribs draw;
        draw.NumVertices = 3;
        draw.Flags = Diligent::DRAW_FLAG_VERIFY_ALL;
        impl_->immediate_context->Draw(draw);
        if (measure_tone_map) {
            impl_->immediate_context->EndQuery(timing->tone_map);
            timing->tone_map_pending = true;
        }
    }
    if(!impl_->ui.vertices.empty() && swap_chain_description.Width && swap_chain_description.Height){
        const auto atlas=impl_->textures.find(impl_->ui.atlas);
        if(atlas!=impl_->textures.end()){
            if(!impl_->ui_pipeline){
                constexpr char ui_vs[]=R"(
struct Input {float2 position:ATTRIB0;float2 uv:ATTRIB1;float4 color:ATTRIB2;};
struct Output {float4 position:SV_POSITION;float2 uv:TEX_COORD;float4 color:COLOR;};
Output main(Input v){Output o;o.position=float4(v.position,0,1);o.uv=v.uv;o.color=v.color;return o;}
)";
                constexpr char ui_ps[]=R"(
Texture2D UiAtlas;SamplerState UiAtlas_sampler;
float4 main(float4 position:SV_POSITION,float2 uv:TEX_COORD,float4 color:COLOR):SV_TARGET{
 return UiAtlas.Sample(UiAtlas_sampler,uv)*color;}
)";
                Diligent::ShaderCreateInfo shader;shader.SourceLanguage=Diligent::SHADER_SOURCE_LANGUAGE_HLSL;
                shader.EntryPoint="main";
                shader.Desc.ShaderType=Diligent::SHADER_TYPE_VERTEX;shader.Desc.Name="Gloom UI vertices";shader.Source=ui_vs;
                Diligent::RefCntAutoPtr<Diligent::IShader> vs,ps;impl_->device->CreateShader(shader,&vs);
                shader.Desc.ShaderType=Diligent::SHADER_TYPE_PIXEL;shader.Desc.Name="Gloom UI atlas";shader.Source=ui_ps;
                impl_->device->CreateShader(shader,&ps);
                Diligent::GraphicsPipelineStateCreateInfo pso;pso.PSODesc.Name="Gloom UI after tone mapping";
                pso.PSODesc.PipelineType=Diligent::PIPELINE_TYPE_GRAPHICS;
                pso.PSODesc.ResourceLayout.DefaultVariableType=Diligent::SHADER_RESOURCE_VARIABLE_TYPE_MUTABLE;
                const Diligent::ImmutableSamplerDesc sampler{Diligent::SHADER_TYPE_PIXEL,"UiAtlas_sampler",Diligent::SamplerDesc{}};
                pso.PSODesc.ResourceLayout.ImmutableSamplers=&sampler;pso.PSODesc.ResourceLayout.NumImmutableSamplers=1;
                auto& gp=pso.GraphicsPipeline;gp.NumRenderTargets=1;gp.RTVFormats[0]=swap_chain_description.ColorBufferFormat;
                gp.PrimitiveTopology=Diligent::PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;gp.RasterizerDesc.CullMode=Diligent::CULL_MODE_NONE;
                gp.DepthStencilDesc.DepthEnable=false;
                auto& blend=gp.BlendDesc.RenderTargets[0];blend.BlendEnable=true;
                blend.SrcBlend=Diligent::BLEND_FACTOR_SRC_ALPHA;blend.DestBlend=Diligent::BLEND_FACTOR_INV_SRC_ALPHA;
                blend.SrcBlendAlpha=Diligent::BLEND_FACTOR_ONE;blend.DestBlendAlpha=Diligent::BLEND_FACTOR_INV_SRC_ALPHA;
                const Diligent::LayoutElement layout[]{
                    {0,0,2,Diligent::VT_FLOAT32,false},{1,0,2,Diligent::VT_FLOAT32,false},{2,0,4,Diligent::VT_FLOAT32,false}};
                gp.InputLayout.LayoutElements=layout;gp.InputLayout.NumElements=3;pso.pVS=vs;pso.pPS=ps;
                impl_->device->CreateGraphicsPipelineState(pso,&impl_->ui_pipeline);
                if(!impl_->ui_pipeline)throw std::runtime_error{"UI pipeline creation failed"};
                impl_->ui_pipeline->CreateShaderResourceBinding(&impl_->ui_resources,true);
                Diligent::BufferDesc buffer;buffer.Name="Gloom UI quads";buffer.Size=65536*sizeof(render::UiVertex);
                buffer.Usage=Diligent::USAGE_DEFAULT;buffer.BindFlags=Diligent::BIND_VERTEX_BUFFER;
                impl_->device->CreateBuffer(buffer,nullptr,&impl_->ui_vertices);
                if(!impl_->ui_vertices || !impl_->ui_resources)throw std::runtime_error{"UI resources unavailable"};
            }
            atlas->second.last_used_frame=impl_->frame_index;
            // Mutable descriptors must not be rewritten while earlier frames use
            // them. This immutable atlas binds once; a rare replacement drains
            // outstanding work before creating a fresh binding.
            if(impl_->ui_bound_view!=atlas->second.view){
                if(impl_->ui_bound_view)impl_->immediate_context->WaitForIdle();
                impl_->ui_resources.Release();impl_->ui_pipeline->CreateShaderResourceBinding(&impl_->ui_resources,true);
                impl_->ui_resources->GetVariableByName(Diligent::SHADER_TYPE_PIXEL,"UiAtlas")->Set(atlas->second.view);
                impl_->ui_bound_view=atlas->second.view;
            }
            // Keep large UI uploads out of the shared dynamic constant-buffer heap.
            impl_->immediate_context->UpdateBuffer(impl_->ui_vertices,0,
                impl_->ui.vertices.size()*sizeof(render::UiVertex),impl_->ui.vertices.data(),
                Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            auto* target=impl_->swap_chain->GetCurrentBackBufferRTV();
            impl_->immediate_context->SetRenderTargets(1,&target,nullptr,Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            impl_->immediate_context->SetPipelineState(impl_->ui_pipeline);
            Diligent::IBuffer* vertex_buffer=impl_->ui_vertices;Diligent::Uint64 offset=0;
            impl_->immediate_context->SetVertexBuffers(0,1,&vertex_buffer,&offset,Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,Diligent::SET_VERTEX_BUFFERS_FLAG_RESET);
            impl_->immediate_context->CommitShaderResources(impl_->ui_resources,Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION);
            Diligent::DrawAttribs draw;draw.NumVertices=static_cast<Diligent::Uint32>(impl_->ui.vertices.size());draw.Flags=Diligent::DRAW_FLAG_VERIFY_ALL;
            impl_->immediate_context->Draw(draw);
        }
    }
    if (!impl_->capture_path.empty() && swap_chain_description.Width && swap_chain_description.Height) {
        auto* source = impl_->swap_chain->GetCurrentBackBufferRTV()->GetTexture();
        auto description = source->GetDesc();
        description.Name = "Gloom diagnostic frame readback";
        description.Usage = Diligent::USAGE_STAGING;
        description.BindFlags = Diligent::BIND_NONE;
        description.CPUAccessFlags = Diligent::CPU_ACCESS_READ;
        description.MiscFlags = Diligent::MISC_TEXTURE_FLAG_NONE;
        Diligent::RefCntAutoPtr<Diligent::ITexture> staging;
        impl_->device->CreateTexture(description, nullptr, &staging);
        if (!staging) throw std::runtime_error{"Frame capture staging allocation failed"};
        impl_->immediate_context->SetRenderTargets(0, nullptr, nullptr, Diligent::RESOURCE_STATE_TRANSITION_MODE_NONE);
        impl_->immediate_context->CopyTexture({source, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION,
                                              staging, Diligent::RESOURCE_STATE_TRANSITION_MODE_TRANSITION});
        impl_->immediate_context->WaitForIdle();
        Diligent::MappedTextureSubresource mapped;
        impl_->immediate_context->MapTextureSubresource(staging, 0, 0, Diligent::MAP_READ,
            Diligent::MAP_FLAG_DO_NOT_WAIT, nullptr, mapped);
        if (!mapped.pData) throw std::runtime_error{"Frame capture map failed"};
        const bool bgra = description.Format == Diligent::TEX_FORMAT_BGRA8_UNORM ||
                          description.Format == Diligent::TEX_FORMAT_BGRA8_UNORM_SRGB;
        std::vector<char> rgb(static_cast<std::size_t>(description.Width) * description.Height * 3);
        for (std::uint32_t y = 0; y < description.Height; ++y) {
            const auto* row = static_cast<const unsigned char*>(mapped.pData) + y * mapped.Stride;
            for (std::uint32_t x = 0; x < description.Width; ++x) {
                const auto at = (static_cast<std::size_t>(y) * description.Width + x) * 3;
                rgb[at] = static_cast<char>(row[x * 4 + (bgra ? 2 : 0)]);
                rgb[at + 1] = static_cast<char>(row[x * 4 + 1]);
                rgb[at + 2] = static_cast<char>(row[x * 4 + (bgra ? 0 : 2)]);
            }
        }
        impl_->immediate_context->UnmapTextureSubresource(staging, 0, 0);
        std::ofstream output{impl_->capture_path, std::ios::binary};
        output << "P6\n" << description.Width << ' ' << description.Height << "\n255\n";
        output.write(rgb.data(), static_cast<std::streamsize>(rgb.size()));
        if (!output) throw std::runtime_error{"Frame capture write failed"};
        impl_->capture_path.clear();
    }
    impl_->immediate_context->EnqueueSignal(impl_->residency_fence,
                                            ++impl_->submitted_fence_value);
    impl_->swap_chain->Present(settings_.vertical_sync ? 1 : 0);
    impl_->previous_jitter = impl_->jitter;
    ++impl_->frame_index;
}

} // namespace gloom::backends
