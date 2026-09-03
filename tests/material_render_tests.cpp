#include <gloom/backends/sdl_window.hpp>
#include <gloom/backends/diligent_renderer.hpp>
#include <gloom/render/lighting.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <stdexcept>

int main(int argc,char** argv) try {
    using namespace gloom;
    const auto output=argc>1?std::filesystem::path{argv[1]}:std::filesystem::path{"material-review"};
    std::filesystem::create_directories(output);
    backends::SdlWindow window{{.title="Gloom material acceptance",.width=512,.height=512,.resizable=false}};
    window.start();
    backends::DiligentRenderer renderer{window,{.vertical_sync=false,.temporal={.technique=render::TemporalTechnique::disabled}}};renderer.start();
    render::MeshUpload sphere{.id={100}};
    constexpr unsigned columns=48,rows=24;
    for (unsigned y=0;y<=rows;++y) for (unsigned x=0;x<=columns;++x) {
        const float u=float(x)/columns,v=float(y)/rows,p=u*6.2831853F,t=v*3.14159265F;
        const std::array normal{std::sin(t)*std::cos(p),std::cos(t),std::sin(t)*std::sin(p)};
        sphere.vertices.push_back({.position=normal,.normal=normal,.texture_coordinate={u,v},
            .tangent={-std::sin(p),0,std::cos(p),1},.texture_coordinate_1={v,u}});
    }
    for (unsigned y=0;y<rows;++y) for (unsigned x=0;x<columns;++x) {
        const auto a=y*(columns+1)+x,b=a+columns+1;
        sphere.indices.insert(sphere.indices.end(),{a,a+1,b,b,a+1,b+1});
    }
    renderer.enqueue(std::move(sphere));
    renderer.enqueue(render::TextureUpload{.id={101},.srgb=true,.mip_levels={{.width=2,.height=2,.data={
        std::byte{255},std::byte{40},std::byte{10},std::byte{0},std::byte{10},std::byte{255},std::byte{20},std::byte{255},
        std::byte{20},std::byte{10},std::byte{255},std::byte{255},std::byte{255},std::byte{255},std::byte{255},std::byte{0}}}}});
    render::ClusteredLightingBuilder builder;
    render::Camera camera{.position={0,0,-4},.target={0,0,0}};
    const auto lights=builder.build(camera,1,{},render::DirectionalLight{.direction={-.4F,-.6F,.6F},.intensity=4,.casts_shadows=true});
    const auto light_view=lights.view();
    auto capture=[&](const char* name,render::MaterialUpload material,bool reverse=false,bool second=false) {
        material.id={102};renderer.enqueue(material);
        render::MaterialUpload back{.id={103},.base_color={.1F,.4F,.8F,.5F},.roughness=.6F};back.surface.alpha_mode=2;renderer.enqueue(back);
        std::vector<render::RenderInstance> instances{{.mesh={100},.material={102}}};
        if(second) instances.push_back({.mesh={100},.material={103},.transform={.position={0,0,.7F}}});
        if(reverse) std::reverse(instances.begin(),instances.end());
        for(int frame=0;frame<32;++frame) {
            static_cast<void>(window.poll_events());renderer.begin_frame();
            renderer.draw({.presentation_seconds=2,.camera=camera,.instances=instances,.lighting=&light_view,.shadow_instances=instances});
            if(frame==31) renderer.capture_next_frame(output/(std::string{name}+".ppm"));
            renderer.end_frame();
        }
    };
    render::MaterialUpload material{.base_color={.65F,.4F,.15F,1},.metallic=1,.roughness=.3F};
    capture("isotropic",material);
    material.surface.anisotropy_strength=.9F;capture("anisotropic",material);
    material.surface.anisotropy_rotation=1.5707963F;capture("anisotropic-rotated",material);
    material.surface.anisotropy_strength=0;capture("anisotropic-zero",material);
    material.metallic=0;capture("specular",material);
    material.surface.specular_factor=0;capture("specular-zero",material);
    material.surface.specular_factor=1;
    material.extra_textures[1]={101};capture("occlusion",material);
    material.extra_textures[1]=render::builtin_white_texture;
    material.metallic=0;material.base_color_texture={101};material.surface.alpha_mode=1;material.surface.double_sided=true;capture("alpha-mask",material);
    material.surface.mapping[0].scale={4,2};material.surface.mapping[0].rotation=.4F;material.surface.uv_scroll={.1F,.2F};capture("uv-transform",material);
    material.surface.mapping[0].uv_set=1;capture("uv1",material);
    material.emissive={3,1,.1F};material.extra_textures[0]={101};capture("emission",material);
    material.emissive={};material.base_color_texture=render::builtin_white_texture;material.surface={};material.base_color[3]=.35F;material.surface.alpha_mode=2;
    capture("blend-forward",material,false,true);capture("blend-reverse",material,true,true);
    material.surface.alpha_mode=3;capture("additive",material,false,true);
    renderer.stop();window.stop();
    auto bytes=[&](const char* name){std::ifstream stream{output/(std::string{name}+".ppm"),std::ios::binary};return std::string{std::istreambuf_iterator<char>{stream},{}};};
    auto differs=[&](const char* a,const char* b){if(bytes(a)==bytes(b))throw std::runtime_error{std::string{a}+" has no visible effect versus "+b};};
    differs("isotropic","anisotropic");differs("anisotropic","anisotropic-rotated");differs("alpha-mask","uv-transform");differs("uv-transform","uv1");differs("uv1","emission");differs("blend-forward","additive");
    differs("specular","specular-zero");differs("specular","occlusion");
    if(bytes("isotropic")!=bytes("anisotropic-zero")) throw std::runtime_error{"Zero anisotropy does not recover isotropic response"};
    if(bytes("blend-forward")!=bytes("blend-reverse")) throw std::runtime_error{"Transparent result depends on submission order"};
    std::cout << "Material GPU acceptance passed\n";return 0;
} catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
