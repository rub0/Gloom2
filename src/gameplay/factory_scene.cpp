#include <gloom/gameplay/factory_scene.hpp>
#include <gloom/gameplay/legacy_movement.hpp>
#include <gloom/backends/jolt_world.hpp>
#include <simdjson.h>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <bit>
#include <mutex>
#include <stdexcept>

namespace gloom::gameplay {
namespace {
physics::Vec3 vector(simdjson::dom::element element,float scale=1.0F) {
    const auto array=element.get_array().value();
    if (array.size()!=3) throw std::runtime_error{"Factory vector must have three values"};
    physics::Vec3 result;
    auto fields=std::array{&result.x,&result.y,&result.z};
    std::size_t index=0;
    for (const auto item : array) {
        const auto number=static_cast<float>(item.get_double().value())*scale;
        if (!std::isfinite(number)) throw std::runtime_error{"Factory vector is not finite"};
        *fields[index++]=number;
    }
    return result;
}
FactoryScene load_factory() {
    simdjson::dom::parser parser;
    const auto document=parser.load((std::filesystem::path{GLOOM_SOURCE_ROOT}/"assets/legacy/factory_scene.json").string()).value();
    if (document["version"].get_uint64().value()!=1) throw std::runtime_error{"Unknown Factory scene version"};
    const float scale=static_cast<float>(document["unit_scale"].get_double().value());
    if (std::abs(scale-0.15F)>1e-6F) throw std::runtime_error{"Factory unit policy mismatch"};
    FactoryScene scene;
    auto probe=std::make_shared<render::EnvironmentProbe>();
    std::ifstream stream{std::filesystem::path{GLOOM_SOURCE_ROOT}/"assets/legacy/factory.ibl",std::ios::binary};
    std::array<char,4> magic{};
    stream.read(magic.data(),4);
    stream.read(reinterpret_cast<char*>(&probe->size),4);
    stream.read(reinterpret_cast<char*>(&probe->mip_levels),4);
    if (!stream || magic!=std::array{'G','I','B','L'} || probe->size!=32 || probe->mip_levels!=6)
        throw std::runtime_error{"Invalid Factory environment probe"};
    std::size_t texels=0;
    for (std::uint32_t level=0;level<probe->mip_levels;++level) texels+=6*(probe->size>>level)*(probe->size>>level);
    probe->radiance.resize(texels);
    stream.read(reinterpret_cast<char*>(probe->radiance.data()),static_cast<std::streamsize>(texels*sizeof(probe->radiance.front())));
    if (!stream || stream.peek()!=std::char_traits<char>::eof()) throw std::runtime_error{"Invalid Factory probe payload"};
    for (const auto& pixel:probe->radiance) for (const auto channel:pixel)
        if (!std::isfinite(channel)||channel<0) throw std::runtime_error{"Invalid Factory probe radiance"};
    scene.environment_probe=std::move(probe);
    auto mesh=std::make_shared<physics::TriangleMesh>();
    for (const auto vertex : document["collision"]["vertices"].get_array()) mesh->vertices.push_back(vector(vertex));
    for (const auto item : document["collision"]["indices"].get_array()) {
        const auto index=item.get_uint64().value();
        if (index>=mesh->vertices.size()) throw std::runtime_error{"Factory collision index out of range"};
        mesh->indices.push_back(static_cast<std::uint32_t>(index));
    }
    if (mesh->vertices.empty()||mesh->indices.empty()||mesh->indices.size()%3) throw std::runtime_error{"Empty or invalid Factory collision"};
    scene.collision=std::move(mesh);
    for (const auto entity : document["entities"].get_array()) {
        ++scene.entity_count;
        const auto type=entity["type"].get_string().value();
        const auto source=entity["source"];
        if (type=="SpawnPoint") scene.spawns.push_back({vector(source["position"],scale),static_cast<float>(source["yaw"].get_double().value())});
        else if (type=="Lava") {
            scene.lava_center=vector(source["position"],scale);
            scene.lava_half_width=static_cast<float>(source["plane_width"].get_double().value())*scale*0.5F;
        }
        const auto resolved=entity["resolved"];
        double reward{};
        // Generic Ammo (SniperAmmo1) has no recovered archetype and is excluded
        // by the gameplay audit's 14-type/73-location contract.
        if(type!="Ammo" && resolved["reward"].get_double().get(reward)==simdjson::SUCCESS) {
            const auto id=resolved["id"].get_string().value();
            PickupKind kind;
            if(id=="orb")kind=PickupKind::life;
            else if(id=="armor")kind=PickupKind::shield;
            else if(id=="weapon")kind=PickupKind::weapon;
            else if(id=="ammo")kind=PickupKind::ammo;
            else if(id=="damageAmplifier")kind=PickupKind::damage;
            else if(id=="cooldownReducer")kind=PickupKind::cooldown;
            else throw std::runtime_error{"Unknown Factory pickup reward"};
            double weapon=0,radius=3;
            if(kind==PickupKind::weapon || kind==PickupKind::ammo)weapon=resolved["weaponType"].get_double().value();
            double authored_radius{};
            if(resolved["physic_radius"].get_double().get(authored_radius)==simdjson::SUCCESS)radius=authored_radius;
            const double ticks=resolved["respawnTime"].get_double().value()*60;
            if(!std::isfinite(reward)||reward<0||reward>200||!std::isfinite(ticks)||ticks<1||ticks>7200||weapon<0||weapon>=slice_weapon_count||!std::isfinite(radius)||radius<=0)
                throw std::runtime_error{"Invalid Factory pickup rule"};
            scene.pickups.push_back({.name=std::string{entity["id"].get_string().value()},.kind=kind,
                .weapon=static_cast<SliceWeapon>(static_cast<unsigned>(weapon)),.position=vector(resolved["position"],scale),
                .reward=static_cast<std::uint16_t>(reward),.respawn_ticks=static_cast<std::uint16_t>(ticks),.radius=static_cast<float>(radius)*scale});
        }
    }
    if(scene.pickups.size()!=factory_pickup_count)throw std::runtime_error{"Factory pickup manifest is incomplete"};
    simdjson::dom::parser visual_parser;
    const auto visuals=visual_parser.load((std::filesystem::path{GLOOM_SOURCE_ROOT}/"assets/legacy/factory.gltf").string()).value();
    std::uint32_t node_index=0;
    for(const auto node:visuals["nodes"].get_array()) {
        const auto name=node["name"].get_string().value();
        for(auto& pickup:scene.pickups)if(pickup.name==name)pickup.source_node=node_index;
        ++node_index;
    }
    if (scene.spawns.size()!=9 || scene.entity_count!=97) throw std::runtime_error{"Factory entity manifest is incomplete"};
    // The wire scene ID includes the actual collision/spawn data. A client with
    // a different import cannot silently predict against another level.
    std::uint32_t hash=2166136261U;
    const auto word=[&](std::uint32_t value){for(unsigned i=0;i<4;++i){hash=(hash^(value&255U))*16777619U;value>>=8;}};
    const auto point=[&](physics::Vec3 value){word(std::bit_cast<std::uint32_t>(value.x));word(std::bit_cast<std::uint32_t>(value.y));word(std::bit_cast<std::uint32_t>(value.z));};
    for (const auto p:scene.collision->vertices) point(p);
    for (const auto i:scene.collision->indices) word(i);
    for (const auto spawn:scene.spawns) {point(spawn.position);word(std::bit_cast<std::uint32_t>(spawn.yaw));}
    point(scene.lava_center);word(std::bit_cast<std::uint32_t>(scene.lava_half_width));
    // Original movement contract revision: incompatible prediction must not join.
    word(68);
    word(67); // Pickup rule/identity contract, including source-level overrides.
    for(const auto& p:scene.pickups) {
        for(unsigned char c:p.name)word(c);
        word(static_cast<std::uint32_t>(p.kind));word(static_cast<std::uint32_t>(p.weapon));
        point(p.position);word(p.reward);word(p.respawn_ticks);word(std::bit_cast<std::uint32_t>(p.radius));word(p.source_node);
    }
    scene.scene_id=hash ? hash : 1;
    // The standalone map supplies the warm lava light absent from the multiplayer
    // TXT. Preserve its source position; range/intensity use modern scene units.
    scene.lights.push_back({.position={-125.828F*scale,-44.0043F*scale,-72.072F*scale},
        .range=90.0F,.color={1.0F,0.65F,0.07F},.intensity=1.2F});
    for(auto& light:scene.lights)light.intensity*=1.2F;
    return scene;
}
struct CollisionQuery {
    backends::JoltWorld world{{.worker_threads=1}};
    std::mutex mutex;
    explicit CollisionQuery(std::shared_ptr<const physics::TriangleMesh> mesh) {
        world.start();
        static_cast<void>(world.create_body({.shape={.type=physics::ShapeType::triangle_mesh,.triangle_mesh=std::move(mesh)},.friction=0.6F}));
    }
};
}
const FactoryScene& original_factory() {
    static const auto scene=load_factory();
    return scene;
}
network::ReplicationSettings factory_movement_settings(std::function<SliceCharacter(network::NetworkEntityId)> character) {
    network::ReplicationSettings settings;
    settings.resolve_entity_collisions=true;
    settings.character_radius=.45F;
    settings.movement_speed=1.3F*legacy_unit_scale/legacy_motion_step;
    settings.jump_speed=1.8F*legacy_unit_scale/legacy_motion_step*normal_jump_scale;
    settings.gravity=legacy_gravity;
    auto query=std::make_shared<CollisionQuery>(original_factory().collision);
    settings.scene_movement=[query,character=std::move(character)](network::MovementState state,const network::MovementInput& input,double delta) {
        const bool dodge=input.dodge && (input.axis_x!=0 || input.axis_z!=0) && (state.grounded || state.air_dodge_available);
        if(state.grounded)state.air_dodge_available=input.jump && !dodge;
        if(dodge)state.air_dodge_available=false;
        auto motion=legacy_motion({state.velocity_x,state.velocity_y,state.velocity_z},state.grounded || dodge,
            input.axis_x,input.axis_z,input.jump,dodge,static_cast<float>(delta),
            legacy_movement_profile(character?character(state.entity):SliceCharacter::hound));
        std::scoped_lock lock{query->mutex};
        const auto moved=query->world.query_character_motion({.position={state.position_x,state.position_y,state.position_z},
            .radius=.45F,.cylinder_half_height=.45F,.max_slope_angle_radians=std::acos(.707F),
            .step_up_height=.075F,.step_down_height=.075F},motion.velocity,
            static_cast<float>(delta),{0,legacy_gravity,0},.075F);
        state.position_x=moved.position.x; state.position_y=moved.position.y; state.position_z=moved.position.z;
        state.velocity_x=moved.velocity.x;
        state.velocity_y=moved.grounded?0.F:moved.velocity.y;
        state.velocity_z=moved.velocity.z;
        state.grounded=moved.grounded; state.simulation_tick=input.simulation_tick;
        return state;
    };
    return settings;
}
}
