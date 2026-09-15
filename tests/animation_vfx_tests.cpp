#include <gloom/assets/animation.hpp>
#include <gloom/assets/scene_gpu_bridge.hpp>
#include <gloom/gameplay/combat_effects.hpp>
#include <gloom/gameplay/vertical_slice_network.hpp>
#include <gloom/render/particles.hpp>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
void check(bool v,const char* text){if(!v)throw std::runtime_error{text};}
template<class F> void rejects(F f){bool rejected=false;try{f();}catch(const std::exception&){rejected=true;}check(rejected,"Malformed data accepted");}
float distance(gloom::render::Vec3 a,gloom::render::Vec3 b){return std::hypot(a.x-b.x,a.y-b.y,a.z-b.z);}
}
int main() try {
    using namespace gloom;
    const std::filesystem::path root{GLOOM_TEST_ASSETS};
    for (const auto* name:{"archangel","shadow"}) {
        const auto imported=assets::import_gltf(root/"characters/original"/(std::string{name}+".gltf"));
        if (!imported) throw std::runtime_error{imported.error()};const auto& rig=*imported;
        check(assets::valid_animations(rig),"Converted animations invalid");
        check(rig.animations.size()==(std::string_view{name}=="shadow"?0:4),"Original clips lost or fabricated");
        const auto encoded=assets::encode_imported_scene(rig);const auto decoded=assets::decode_imported_scene(encoded);
        check(decoded && decoded->animations.size()==rig.animations.size(),"Clips lost during serialization");
        const auto bind=assets::skin_pose(rig,1,*assets::bind_node_transforms(rig));
        for (const auto& v:rig.primitives[0].vertices)
            check(distance(assets::skinned_position(v,*bind),{v.position[0],v.position[1],v.position[2]})<.002F,"Eight-weight bind skin differs from static mesh");
        for (const auto& clip:rig.animations) {
            check(clip.channels.size()==129,"Ogre channels lost");
            const auto before=assets::pose_worlds(rig,assets::sample_animation(rig,clip.name,.13));
            const auto after=assets::pose_worlds(*decoded,assets::sample_animation(*decoded,clip.name,.13));
            check(before==after,"Clip serialization changed evaluation");
            const auto looping=assets::pose_worlds(rig,assets::sample_animation(rig,clip.name,.13+clip.duration));
            for(std::size_t i=0;i<before.size();++i) for(std::size_t k=0;k<16;++k)
                check(std::abs(looping[i][k]-before[i][k])<.0001F,"Loop wrap changed pose");
        }
        if (!rig.animations.empty()) {
            auto bad=rig;bad.animations[0].channels[0].times[1]=bad.animations[0].channels[0].times[0];rejects([&]{static_cast<void>(assets::encode_imported_scene(bad));});
            bad=rig;bad.animations[0].channels[0].node=9999;rejects([&]{static_cast<void>(assets::encode_imported_scene(bad));});
            bad=rig;bad.animations[0].channels[1].values[0]={0,0,0,0};rejects([&]{static_cast<void>(assets::encode_imported_scene(bad));});
            bad=rig;bad.animations[0].channels[0].interpolation=static_cast<assets::AnimationInterpolation>(7);rejects([&]{static_cast<void>(assets::encode_imported_scene(bad));});
            bad=rig;bad.animations[0].channels.push_back(bad.animations[0].channels[0]);rejects([&]{static_cast<void>(assets::encode_imported_scene(bad));});
            auto short_bytes=encoded;short_bytes.resize(short_bytes.size()-7);check(!assets::decode_imported_scene(short_bytes),"Truncated clip accepted");
        }
        gameplay::CharacterAnimator animator;
        gameplay::CombatantView view{.entity=2,.velocity_z=3,.facing_x=0,.facing_z=1,
            .character=std::string_view{name}=="shadow"?gameplay::SliceCharacter::shadow:gameplay::SliceCharacter::archangel};
        auto frame=animator.update(rig,view,1.0/60);check(frame.cut,"First pose retained motion history");
        const auto first=assets::skin_pose(rig,1,frame.worlds);
        for (int i=0;i<40;++i) frame=animator.update(rig,view,1.0/60);
        check(!frame.cut,"Continuous motion reset history");
        const auto animated=assets::skin_pose(rig,1,frame.worlds);
        float deformation=0;
        const auto bounds=assets::skinned_bounds(rig.primitives[0],*animated);
        for (const auto& v:rig.primitives[0].vertices) {
            const auto p=assets::skinned_position(v,*animated);
            deformation=std::max(deformation,distance(p,assets::skinned_position(v,*first)));
            check(distance(p,bounds.center)<=bounds.radius+.001F,"Animated bounds lost a vertex");
        }
        check(deformation>.01F,"Character animation did not deform the recovered mesh");
        check(distance(frame.left_grip,{.20F,1.18F,.49F})<.08F,"Left hand does not reach grip");
        assets::LocalPose timed_reference;
        for(int fps:{30,60,144}) {
            gameplay::CharacterAnimator timed;
            for(int i=0;i<fps*2;++i) frame=timed.update(rig,view,1.0/fps,false,false,true);
            check(distance(frame.left_grip,{.20F,1.18F,.49F})<.08F,"FPS hand cannot reach weapon");
            if(timed_reference.empty())timed_reference=frame.local;
            else {
                const auto reference_world=assets::pose_worlds(rig,timed_reference);
                for(std::size_t n=0;n<frame.worlds.size();++n)for(std::size_t k=0;k<16;++k)
                    check(std::abs(reference_world[n][k]-frame.worlds[n][k])<.002F,"Pose changes with render frequency");
            }
        }
        view.position_x=20;check(animator.update(rig,view,1.0/60).cut,"Teleport retained skin history");
        view.alive=false;check(animator.update(rig,view,1.0/60).cut,"Death retained skin history");
        for(int i=0;i<60;++i) frame=animator.update(rig,view,1.0/60);
        const auto corpse=frame.worlds;
        for(int i=0;i<60;++i) frame=animator.update(rig,view,1.0/60);
        check(corpse==frame.worlds,"Corpse kept looping idle after its death animation");
        view.alive=true;++view.deaths;check(animator.update(rig,view,1.0/60).cut,"Respawn retained skin history");
        const auto uploads=assets::build_gpu_scene_uploads(rig,{123});
        check(uploads.primitives[0].arms_mesh.value!=0,"No original FPS arm geometry");
        check(uploads.meshes.back().vertices[0].weights==rig.primitives[0].vertices[0].weights,"GPU bridge dropped bone weights");
        std::cout<<name<<": original clips="<<rig.animations.size()<<", deformation="<<deformation<<", grip="<<frame.left_grip.x<<','<<frame.left_grip.y<<','<<frame.left_grip.z<<'\n';
    }
    const auto recipes=render::load_particle_recipes(root/"effects/recipes.json");
    const render::ParticleRecipe* rocket_smoke=nullptr;const render::ParticleRecipe* rocket_explosion=nullptr;
    for(const auto& recipe:recipes){if(recipe.name=="rocket_smoke")rocket_smoke=&recipe;else if(recipe.name=="explosion_review")rocket_explosion=&recipe;}
    check(rocket_smoke && rocket_smoke->rate==55 && rocket_smoke->life==1.1F && rocket_smoke->size_end==.62F,"Rocket smoke recipe lost");
    check(rocket_explosion && rocket_explosion->burst==28 && rocket_explosion->size_end==1.05F,"Rocket explosion emphasis lost");
    render::ParticleSystem rocket_particles{recipes};
    for(int i=0;i<60;++i){rocket_particles.emitter(9,9,"rocket_smoke",{static_cast<float>(i)/60,0,0});rocket_particles.advance(1.0/60);}
    check(rocket_particles.metrics().spawned==55,"Rocket smoke emission rate changed");
    rocket_particles.burst(9,"explosion_review",{},{0,1,0},1);check(rocket_particles.metrics().spawned==83,"Rocket explosion particle count changed");
    std::vector<render::Particle> reference;
    for (int fps:{30,60,144}) {
        render::ParticleSystem p{recipes};
        for(int i=0;i<fps;++i){p.emitter(1,7,"shadow_smoke",{1,2,3});p.advance(1.0/fps);}
        check(p.metrics().spawned==28,"Emitter rate depends on render FPS");
        if(reference.empty()) reference.assign(p.particles().begin(),p.particles().end());
        else {
            check(reference.size()==p.particles().size(),"Lifetime depends on render FPS");
            for(std::size_t i=0;i<reference.size();++i) check(distance(reference[i].velocity,p.particles()[i].velocity)<1e-6F,"Random seed depends on render FPS");
        }
        p.advance(2);check(p.particles().empty(),"Unrefreshed emitters leaked particles");
        p.burst(7,"impact",{}, {0,1,0},123);p.cancel(7);check(p.particles().empty(),"Entity removal leaked particles");
    }
    render::ParticleSystem bounded{recipes,5};bounded.burst(1,"impact",{}, {0,1,0},1);
    check(bounded.particles().size()==5 && bounded.metrics().dropped==9,"Particle saturation not bounded");
    std::vector<render::Particle> saturated_reference;std::uint64_t dropped_reference=0;
    for(int fps:{30,60,144}) {
        render::ParticleSystem saturated{recipes,5};
        for(int i=0;i<fps*3;++i) {
            saturated.emitter(2,8,"shadow_smoke",{1,2,3});saturated.emitter(1,7,"archangel_energy",{});
            saturated.advance(1.0/fps);
        }
        if(saturated_reference.empty()){saturated_reference.assign(saturated.particles().begin(),saturated.particles().end());dropped_reference=saturated.metrics().dropped;}
        else {
            check(saturated.metrics().dropped==dropped_reference && saturated.particles().size()==saturated_reference.size(),"Saturation depends on FPS");
            for(std::size_t i=0;i<saturated_reference.size();++i)
                check(saturated_reference[i].owner==saturated.particles()[i].owner && std::abs(saturated_reference[i].birth-saturated.particles()[i].birth)<1e-8,"Saturation changed retained particles");
        }
    }
    rejects([&]{bounded.advance(std::numeric_limits<double>::quiet_NaN());});
    auto bad=recipes;bad[0].life=-1;rejects([&]{render::ParticleSystem p{bad};});
    const auto rig=assets::import_gltf(root/"characters/original/archangel.gltf");
    gameplay::CharacterAnimator animator;gameplay::CombatEffects events;render::ParticleSystem p{recipes};
    gameplay::CombatantView v{.entity=2,.facing_x=0,.facing_z=1,.character=gameplay::SliceCharacter::archangel,.ability=gameplay::SliceAbility::diamond_skin,
        .secondary_ability=gameplay::SliceSecondaryAbility::life_dome};
    auto frame=animator.update(*rig,v,.01);events.observe(p,v,frame,{},100,true);
    v.shot_sequence=1;v.shot_tick=101;v.shot_contact=true;
    events.observe(p,v,frame,{},101,true);const auto count=p.metrics().spawned;
    const auto muzzle_before=p.particles().front().position;render::Transform moved{.position={1,2,3}};
    events.observe(p,v,frame,moved,101,true);check(distance(p.particles().front().position,muzzle_before)>3.7F,"Muzzle flash did not follow the weapon");
    for(int i=0;i<20;++i) events.observe(p,v,frame,{},102+i,true);
    check(p.metrics().spawned==count && events.events()==2,"Confirmed/replayed shot duplicated effects");
    auto stale=v;stale.shot_sequence=0;events.observe(p,stale,frame,{},90,true);events.observe(p,v,frame,{},122,true);
    check(p.metrics().spawned==count,"Reordered snapshot replayed an old effect");
    events.reset(p);events.observe(p,v,frame,{},130,true);check(p.particles().empty(),"Reconnect replayed an old shot");
    v.alive=false;events.observe(p,v,frame,{},131,true);p.advance(2);check(p.particles().empty(),"Death effect did not expire");
    events.reset(p);check(p.particles().empty(),"Scene exit leaked effects");
    gameplay::VerticalSliceSimulation sim{false};auto snapshot=sim.snapshot();snapshot.player=v;snapshot.player.alive=true;
    snapshot.player.aim_pitch=.5F;
    const auto wire=gameplay::decode_slice_snapshot(gameplay::encode_slice_snapshot(snapshot,0,1));
    check(wire && wire->player.shot_sequence==v.shot_sequence && wire->player.aim_pitch==.5F,"Wire lost cosmetic event identity or aim");
    std::cout<<"Animation, skinning, FPS grip, particle lifetime/capacity and event checks passed\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
