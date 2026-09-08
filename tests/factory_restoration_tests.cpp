#include <gloom/gameplay/factory_scene.hpp>
#include <gloom/gameplay/vertical_slice_network.hpp>
#include <gloom/physics/triangle_query.hpp>
#include <gloom/assets/gltf_importer.hpp>
#include <gloom/assets/scene_gpu_bridge.hpp>
#include <gloom/network/combat.hpp>
#include <limits>
#include <iostream>
#include <stdexcept>
#include <cmath>

namespace {
void require(bool condition,const char* message) { if (!condition) throw std::runtime_error{message}; }
}
int main() try {
    using namespace gloom;
    const auto& factory=gameplay::original_factory();
    require(factory.entity_count==97 && factory.spawns.size()==9,"Factory entity normalization");
    require(factory.collision->vertices.size()==3564 && factory.collision->indices.size()==17568,"RepX topology changed");
    const auto movement=gameplay::factory_movement_settings();
    network::MovementState jumper{.entity=1,.position_x=factory.jumper.position.x,
        .position_y=factory.jumper.position.y,.position_z=factory.jumper.position.z,.grounded=true};
    jumper=network::simulate_movement(jumper,{.simulation_tick=1},1.0/60,movement);
    require(!jumper.grounded&&jumper.velocity_x>4.0F&&jumper.velocity_y>30.0F,"Factory jumper did not apply its original impulse");
    network::MovementState falling{.entity=1,.position_x=60,.position_y=1};
    for(unsigned i=1;i<=120;++i) falling=network::simulate_movement(falling,{.simulation_tick=i},1.0/60,movement);
    require(falling.position_y<factory.lava_center.y && !falling.grounded,"Lava void was replaced by the old infinite ground plane");
    for (const auto& spawn : factory.spawns) {
        network::MovementState state{.entity=1,.position_x=spawn.position.x,.position_y=spawn.position.y+0.1F,.position_z=spawn.position.z};
        for (std::uint64_t tick=1;tick<=120;++tick)
            state=network::simulate_movement(state,{.simulation_tick=tick},1.0/60,movement);
        std::cout << "Spawn " << spawn.position.x << ',' << spawn.position.y << ',' << spawn.position.z << " => " << state.position_y << " grounded=" << state.grounded << '\n';
        require(state.grounded && std::abs(state.position_y-spawn.position.y)<1.0F,"Original spawn lacks supporting collision");
        const auto down=physics::ray_triangle_distance(*factory.collision,{state.position_x,state.position_y+0.5F,state.position_z},{0,-1,0},10);
        require(down<0.65F,"Hitscan and character support disagree");
        auto replay=state;
        for (std::uint64_t tick=121;tick<=180;++tick) {
            network::MovementInput input{.simulation_tick=tick,.axis_x=0.5F,.axis_z=0.25F,.jump=tick==121};
            state=network::simulate_movement(state,input,1.0/60,movement);
            replay=network::simulate_movement(replay,input,1.0/60,movement);
        }
        require(state.position_x==replay.position_x && state.position_y==replay.position_y && state.position_z==replay.position_z,"Scene movement replay is nondeterministic");
    }
    gameplay::VerticalSliceSimulation simulation{{.opponent_ai_enabled=false,.original_factory=true}};
    for (int i=0;i<120;++i) simulation.tick({});
    require(simulation.snapshot().scene_id==factory.scene_id && simulation.snapshot().factory_lift.entity==0,"Original map contains blockout mechanism");
    require(simulation.snapshot().player.alive && simulation.snapshot().player.grounded,"Original spawn gameplay failed");
    const auto wire=gameplay::encode_slice_snapshot(simulation.snapshot(),0,1);
    require(gameplay::decode_slice_snapshot(wire)->scene_id==factory.scene_id,"Factory wire scene identifier lost");
    auto unknown=wire; unknown.payload.back()=std::byte{42};
    require(!gameplay::decode_slice_snapshot(unknown),"Unknown scene identifier accepted");
    bool wall_checked=false;
    for(std::size_t i=0;i<factory.collision->indices.size() && !wall_checked;i+=3) {
        const auto a=factory.collision->vertices[factory.collision->indices[i]],b=factory.collision->vertices[factory.collision->indices[i+1]],c=factory.collision->vertices[factory.collision->indices[i+2]];
        const physics::Vec3 e{b.x-a.x,b.y-a.y,b.z-a.z},f{c.x-a.x,c.y-a.y,c.z-a.z};
        physics::Vec3 n{e.y*f.z-e.z*f.y,e.z*f.x-e.x*f.z,e.x*f.y-e.y*f.x};
        const float length=std::sqrt(n.x*n.x+n.y*n.y+n.z*n.z);
        if(length<1 || std::abs(n.y)>length*.01F) continue;
        n={n.x/length,n.y/length,n.z/length};
        const physics::Vec3 center{(a.x+b.x+c.x)/3,(a.y+b.y+c.y)/3,(a.z+b.z+c.z)/3};
        network::WorldSnapshot world{.simulation_tick=1,.entities={
            {.entity=1,.position_x=center.x-n.x*2,.position_y=center.y-.9F,.position_z=center.z-n.z*2},
            {.entity=2,.position_x=center.x+n.x*2,.position_y=center.y-.9F,.position_z=center.z+n.z*2}}};
        network::LagCompensatedCombatServer occluded{{.static_mesh=factory.collision}},control;
        occluded.record(world);control.record(world);
        const network::FireCommand shot{.sequence=1,.shooter=1,.estimated_server_tick=1,.aim_x=n.x,.aim_z=n.z,.maximum_distance=10};
        require(control.validate(shot).status==network::FireValidationStatus::hit,"Hitscan wall negative control failed");
        require(occluded.validate(shot).status==network::FireValidationStatus::miss,"Hitscan passed through original Factory wall");
        wall_checked=true;
    }
    require(wall_checked,"No vertical Factory wall tested");
    auto scene=assets::import_gltf(std::filesystem::path{GLOOM_TEST_ASSETS}/"legacy/factory.gltf");
    if (!scene) throw std::runtime_error{scene.error()};
    require(scene->meshes.size()==14,"Factory converted prop inventory changed");
    std::size_t specular=0,emission=0;
    for (const auto& m:scene->materials) {
        specular+=m.extra_textures[3]!=assets::no_asset_index;
        emission+=m.extra_textures[0]!=assets::no_asset_index;
    }
    require(specular>=12 && emission>=3,"Original material maps missing");
    const auto decoded=assets::decode_imported_scene(assets::encode_imported_scene(*scene));
    require(decoded.has_value() && decoded->materials.size()==scene->materials.size(),"Extended material cache round trip");
    for (const auto* name:{"soul_reaper","armour_small"}) {
        const auto recovered=assets::import_gltf(std::filesystem::path{GLOOM_TEST_ASSETS}/"legacy"/(std::string{name}+".gltf"));
        require(recovered.has_value() && !recovered->primitives.empty(),"Normalized legacy mesh cannot be imported");
    }
    auto material_scene=assets::import_gltf(std::filesystem::path{GLOOM_TEST_ASSETS}/"tests/material_surface/scene.gltf");
    if (!material_scene) throw std::runtime_error{material_scene.error()};
    const auto material_roundtrip=assets::decode_imported_scene(assets::encode_imported_scene(*material_scene));
    require(material_roundtrip.has_value(),"Material extension roundtrip failed");
    auto old_version=assets::encode_imported_scene(*material_scene);old_version[8]=std::byte{2};
    require(!assets::decode_imported_scene(old_version),"Old scene payload accepted as extended version");
    const auto& material=material_roundtrip->materials.front();
    require(material.surface.alpha_mode==3 && material.surface.double_sided &&
        std::abs(material.surface.anisotropy_strength-.8F)<1e-5F &&
        std::abs(material.surface.specular_factor-.4F)<1e-5F &&
        std::abs(material.emissive[2]-.9F)<1e-5F,"Standard material extensions were lost");
    require(material.surface.mapping[0].uv_set==1 && material.surface.mapping[0].scale[1]==3 &&
        material.surface.mapping[9].uv_set==1 && material.surface.uv_scroll[1]==.02F,"Material UV controls were lost");
    const std::array image_ids{render::RenderAssetId{400},render::RenderAssetId{401},render::RenderAssetId{402}};
    const auto gpu=assets::build_gpu_scene_uploads(*material_roundtrip,{800},image_ids);
    require(gpu.materials[0].extra_textures[0]==image_ids[0] && gpu.materials[0].extra_textures[1]==image_ids[1] &&
        gpu.materials[0].extra_textures[2]==image_ids[1] && gpu.materials[0].extra_textures[3]==image_ids[0] &&
        gpu.materials[0].extra_textures[4]==image_ids[1] && gpu.materials[0].extra_textures[5]==image_ids[0] &&
        gpu.materials[0].extra_textures[6]==image_ids[0] && gpu.materials[0].surface.alpha_mode==3,"Extended GPU texture bindings are wrong");
    auto reject=[&](auto invalid){bool rejected=false;try{static_cast<void>(assets::encode_imported_scene(invalid));}catch(const std::invalid_argument&){rejected=true;}require(rejected,"Invalid extended material cache accepted");};
    auto invalid=*material_scene;invalid.materials[0].surface.anisotropy_strength=2;reject(invalid);
    invalid=*material_scene;invalid.materials[0].extra_textures[4]=999;reject(invalid);
    invalid=*material_scene;invalid.primitives[0].vertices[0].texture_coordinate_1[0]=std::numeric_limits<float>::quiet_NaN();reject(invalid);
    std::cout << "Factory restoration checks passed\n";
    return 0;
} catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
