#include <gloom/gameplay/factory_scene.hpp>
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
    scene.scene_id=hash ? hash : 1;
    // The standalone map supplies the warm lava light absent from the multiplayer
    // TXT. Preserve its source position; range/intensity use modern scene units.
    scene.lights.push_back({.position={-125.828F*scale,-44.0043F*scale,-72.072F*scale},
        .range=90.0F,.color={1.0F,0.65F,0.07F},.intensity=1.2F});
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
network::ReplicationSettings factory_movement_settings() {
    network::ReplicationSettings settings;
    settings.resolve_entity_collisions=true;
    auto query=std::make_shared<CollisionQuery>(original_factory().collision);
    const auto speed=settings.movement_speed, jump=settings.jump_speed, gravity=settings.gravity;
    settings.scene_movement=[query,speed,jump,gravity](network::MovementState state,const network::MovementInput& input,double delta) {
        const float length=std::max(1.0F,std::sqrt(input.axis_x*input.axis_x+input.axis_z*input.axis_z));
        const physics::Vec3 velocity{input.axis_x/length*speed,
            (input.jump&&state.grounded?jump:state.grounded?0.0F:state.velocity_y)+gravity*static_cast<float>(delta),input.axis_z/length*speed};
        std::scoped_lock lock{query->mutex};
        const auto moved=query->world.query_character_motion({.position={state.position_x,state.position_y,state.position_z}},velocity,
            static_cast<float>(delta),{0,gravity,0});
        state.position_x=moved.position.x; state.position_y=moved.position.y; state.position_z=moved.position.z;
        state.velocity_x=moved.velocity.x; state.velocity_y=moved.velocity.y; state.velocity_z=moved.velocity.z;
        state.grounded=moved.grounded; state.simulation_tick=input.simulation_tick;
        return state;
    };
    return settings;
}
}
