#include <gloom/gameplay/legacy_movement.hpp>
#include <gloom/gameplay/factory_scene.hpp>
#include <gloom/network/movement_replication.hpp>
#include <gloom/platform/movement_actions.hpp>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <limits>

using namespace gloom;
void require(bool value,const char* why){if(!value)throw std::runtime_error{why};}
void close(float a,float b,const char* why){require(std::abs(a-b)<.0002F,why);}
int main()try{
    platform::DoubleTap tap;
    require(!tap.press(0) && !tap.press(100,true) && tap.press(450),"450 ms double Space or repeat filtering failed");
    require(!tap.press(460),"Third tap repeated the dodge");
    tap.clear();require(!tap.press(500),"Focus reset retained first Space");
    require(!tap.press(951),"Expired Space tap accepted");
    platform::MovementActions actions;
    actions.update({.jump=true,.dodge=true}); // a render frame with no tick
    actions.update({.jump=true});
    require(actions.jump && actions.dodge,"Input edge lost before fixed tick");
    actions.consume();actions.update({.jump=true});
    require(!actions.jump && !actions.dodge,"Held input repeated after consumption");
    actions.update({});actions.update({.jump=true});actions.update({.jump=true},false);
    require(!actions.jump,"Menu/focus transition kept a buffered jump");
    constexpr float units=.15F/.016F;
    for(auto character:{gameplay::SliceCharacter::hound,gameplay::SliceCharacter::archangel,gameplay::SliceCharacter::shadow}){
        const auto p=gameplay::legacy_movement_profile(character);
        // Independent transcription of AvatarController at its actual 16 ms tick.
        float original_x=0;physics::Vec3 velocity{};
        for(unsigned tick=0;tick<250;++tick){
            const float dir=tick<100?1.F:tick<150?-1.F:0.F;
            const float coefficient=dir==0?.8F:p.maximum_momentum/(p.maximum_momentum+.5F*p.acceleration*16.F);
            original_x=(original_x+dir*p.acceleration*16.F*.5F)*coefficient;
            velocity=gameplay::legacy_motion(velocity,true,dir,0,false,false,.016F,p).velocity;
            close(velocity.x,original_x*units,"Native ground acceleration/friction differs from Legacy");
        }
        original_x=.4F;float original_y=p.jump_momentum;velocity={original_x*units,original_y*units,0};
        for(unsigned tick=0;tick<180;++tick){
            original_x=original_x*.98F+p.acceleration*2.F;
            original_y=std::max(-p.maximum_momentum*6.F,original_y-.007F*8.F);
            velocity=gameplay::legacy_motion(velocity,false,1,0,false,false,.016F,p).velocity;
            close(velocity.x,original_x*units,"Native air control differs from Legacy");
            close(velocity.y,original_y*units,"Native gravity or terminal speed differs from Legacy");
        }
        for(unsigned hz:{30,60,144}){
            velocity={};for(unsigned tick=0;tick<hz*2;++tick)
                velocity=gameplay::legacy_motion(velocity,true,1,0,false,false,1.F/hz,p).velocity;
            const float retain=std::pow(p.maximum_momentum/(p.maximum_momentum+8.F*p.acceleration),125.F);
            close(velocity.x,p.maximum_momentum*units*(1-retain),"Ground response changed with tick rate");
        }
        const auto jump=gameplay::legacy_motion({},true,0,0,true,false,.016F,p);
        close(jump.velocity.y,p.jump_momentum*units*.9F+gameplay::legacy_gravity*.016F,"Normal jump impulse is not reduced by 10 percent");
        auto brake=gameplay::legacy_motion({10,0,0},true,0,0,false,false,.016F,p);
        close(brake.velocity.x,8,"Release must decelerate instead of stopping instantly");
        auto dodge=gameplay::legacy_motion({},true,1,0,false,true,.016F,p);
        require(dodge.airborne && dodge.velocity.x>p.maximum_momentum*units && dodge.velocity.y>0,"Ground dodge missing impulse");
        const auto air=gameplay::legacy_motion(dodge.velocity,false,1,0,false,false,.016F,p);
        const auto denied=gameplay::legacy_motion(dodge.velocity,false,1,0,true,true,.016F,p);
        close(air.velocity.y,denied.velocity.y,"Airborne dodge/jump granted an extra impulse");
        const auto diagonal=gameplay::legacy_motion({},true,1,1,false,false,.016F,p);
        const auto straight=gameplay::legacy_motion({},true,1,0,false,false,.016F,p);
        close(std::hypot(diagonal.velocity.x,diagonal.velocity.z),straight.velocity.x,"Diagonal motion is faster");
    }
    network::MovementInput input{.sequence=1,.simulation_tick=1,.axis_x=1,.jump=true,.dodge=true};
    const auto decoded=network::decode_movement_input(network::encode_movement_input(input));
    require(decoded && decoded->jump && decoded->dodge,"Dodge was lost on the wire");
    auto invalid=network::encode_movement_input(input);invalid.payload.back()=std::byte{4};
    require(!network::decode_movement_input(invalid),"Reserved movement flag accepted");
    const auto& factory=gameplay::original_factory();
    auto settings=gameplay::factory_movement_settings([](auto){return gameplay::SliceCharacter::shadow;});
    network::MovementState state{.entity=1,.position_x=factory.spawns[0].position.x,
        .position_y=factory.spawns[0].position.y,.position_z=factory.spawns[0].position.z};
    for(unsigned tick=1;tick<=30;++tick)state=network::simulate_movement(state,{.simulation_tick=tick},1./60,settings);
    require(state.grounded,"Original capsule did not settle on Factory");
    const auto rest=state;
    for(unsigned tick=0;tick<600;++tick)state=network::simulate_movement(state,{},1./60,settings);
    close(state.position_x,rest.position_x,"Idle character drifts in X");
    close(state.position_z,rest.position_z,"Idle character drifts in Z");
    close(state.position_y,rest.position_y,"Idle character drifts in Y");
    auto second_space=network::simulate_movement(state,{.axis_x=1,.jump=true},1./60,settings);
    for(unsigned tick=0;tick<14;++tick)second_space=network::simulate_movement(second_space,{.axis_x=1},1./60,settings);
    require(second_space.air_dodge_available,"Jump did not arm second Space");
    second_space=network::simulate_movement(second_space,{.axis_x=1,.jump=true,.dodge=true},1./60,settings);
    require(!second_space.air_dodge_available && second_space.velocity_y>8,"Second Space did not dodge during jump");
    auto repeat=network::simulate_movement(second_space,{.axis_x=1,.dodge=true},1./60,settings);
    require(repeat.velocity_y<second_space.velocity_y,"Repeated airborne dodge grants infinite flight");
    // Actual Factory staircase collision ramp: (-44,-0.3,-29) to (-24,7,-29).
    auto stairs=gameplay::factory_movement_settings();
    auto settle=[&](float x,float y){
        network::MovementState result{.entity=1,.position_x=x,.position_y=y,.position_z=-29};
        for(unsigned i=0;i<60;++i)result=network::simulate_movement(result,{},1./60,stairs);
        require(result.grounded,"Factory stairs did not settle");return result;
    };
    auto uphill=settle(-42,2);const auto bottom=uphill;
    for(unsigned i=0;i<90;++i){
        uphill=network::simulate_movement(uphill,{.axis_x=1},1./60,stairs);
        if(i>30)require(uphill.velocity_x>10,"Factory stairs drain uphill momentum");
    }
    require(uphill.position_x>bottom.position_x+10 && uphill.position_y>bottom.position_y+3,
        "Factory staircase climb stalled");
    auto downhill=settle(-26,8);
    for(unsigned i=0;i<60;++i){
        downhill=network::simulate_movement(downhill,{.axis_x=-1},1./60,stairs);
        require(downhill.grounded,"Descending Factory stairs lost jump support");
        const auto jumping=network::simulate_movement(downhill,{.axis_x=-1,.jump=true},1./60,stairs);
        require(jumping.velocity_y>10 && !jumping.grounded,"Jump rejected while descending Factory stairs");
    }
    const float downhill_speed=std::abs(downhill.velocity_x);
    downhill=network::simulate_movement(downhill,{},1./60,stairs);
    require(std::abs(downhill.velocity_x)>downhill_speed*.7F && std::abs(downhill.velocity_x)<downhill_speed,
        "Release on stairs must preserve decaying momentum");
    for(unsigned i=0;i<180;++i)downhill=network::simulate_movement(downhill,{},1./60,stairs);
    const auto stopped=downhill;
    for(unsigned i=0;i<600;++i)downhill=network::simulate_movement(downhill,{},1./60,stairs);
    close(downhill.position_x,stopped.position_x,"Rest on Factory slope drifts");
    close(downhill.position_y,stopped.position_y,"Rest on Factory slope loses height");
    const auto landed_dodge=gameplay::legacy_motion({20,0,0},true,0,0,false,false,.016F,
        gameplay::legacy_movement_profile(gameplay::SliceCharacter::hound));
    close(landed_dodge.velocity.x,16,"Landing after dodge erased momentum");
    const auto start=state;float peak=state.position_y;
    settings.snapshot_rate=60;
    auto authority_settings=settings;unsigned dodge_commands=0;
    authority_settings.scene_movement=[motion=settings.scene_movement,&dodge_commands](auto s,const auto& command,double dt){
        if(command.dodge)++dodge_commands;return motion(s,command,dt);
    };
    network::AuthoritativeMovementServer server{authority_settings,1};server.add_entity(start);
    network::PredictedMovementClient client{settings,1,start};
    static_cast<void>(client.create_input(1,0,false,true)); // deliberately lost
    client.receive(*server.tick());
    require(dodge_commands==0,"Authority executed a lost dodge");
    const auto recovered=client.create_input(1,0);
    server.receive(recovered);server.receive(recovered);
    client.receive(*server.tick());
    require(dodge_commands==1 && !server.entity(1).grounded && client.pending_input_count()==0,
        "Redundant dodge did not recover exactly once and reconcile");
    server.receive(recovered);client.receive(*server.tick());
    require(dodge_commands==1,"Acknowledged dodge replayed on authority");
    for(unsigned tick=31;tick<=150;++tick){
        state=network::simulate_movement(state,{.simulation_tick=tick,.jump=tick==31},1./60,settings);
        peak=std::max(peak,state.position_y);
    }
    require(peak>start.position_y+1.5F && state.grounded,"Original jump arc/landing failed against Factory");
    bool rejected=false;try{static_cast<void>(gameplay::legacy_motion({},true,0,0,false,false,std::numeric_limits<float>::quiet_NaN(),gameplay::legacy_movement_profile(gameplay::SliceCharacter::hound)));}catch(const std::invalid_argument&){rejected=true;}
    require(rejected,"Nonfinite movement time accepted");
    std::cout<<"Legacy 16 ms oracle, 30/60/144 Hz response, profiles, dodge wire flags and Factory stairs, slope rest, reduced jump and dodge landing passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
