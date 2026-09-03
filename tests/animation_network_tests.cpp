#include <gloom/backends/gns_transport.hpp>
#include <gloom/backends/jolt_world.hpp>
#include <gloom/gameplay/vertical_slice_network.hpp>
#include <gloom/gameplay/combat_effects.hpp>
#include <gloom/assets/animation.hpp>
#include <chrono>
#include <thread>
#include <iostream>
#include <stdexcept>
#include <cmath>

using namespace gloom;
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error{message};}
struct Driver {
    std::size_t waypoint=0;
    bool damaged=false,dead=false,respawned=false;
    std::uint64_t death_tick=0,respawn_tick=0;
    // Walk the original collision geometry, from spawn 0 around the lava to spawn 1.
    static constexpr std::array path{
        std::array{-35.75F,9.F},std::array{-35.75F,1.5F},std::array{-1.5F,1.5F},
        std::array{-1.5F,5.25F},std::array{1.F,5.25F},std::array{1.F,-11.25F},
        std::array{10.F,-11.25F},std::array{10.F,-12.5F},std::array{14.5F,-12.5F},
        std::array{14.5F,-19.75F},std::array{15.75F,-20.5F},std::array{15.75F,-23.F},std::array{21.F,-23.F}};
    gameplay::SliceInput input(const gameplay::SliceSnapshot& s) {
        const auto& p=s.player;const auto& o=s.opponent;
        damaged|=o.life>0 && o.life<gameplay::legacy_default_life;
        if(!o.alive && !dead){dead=true;death_tick=s.simulation_tick;}
        if(dead && o.alive && !respawned){respawned=true;respawn_tick=s.simulation_tick;}
        if(dead)return {};
        if(waypoint<path.size()) {
            const auto target=path[waypoint];const float x=target[0]-p.position_x,z=target[1]-p.position_z;
            if(std::hypot(x,z)<.13F){++waypoint;return {};}
            return {.axis_x=std::clamp(x*4,-1.F,1.F),.axis_z=std::clamp(z*4,-1.F,1.F)};
        }
        return {.aim_x=o.position_x-p.position_x,.aim_y=o.position_y-p.position_y,.aim_z=o.position_z-p.position_z,.fire_primary=true};
    }
    void verify(const gameplay::SliceSnapshot& s) const {
        require(damaged && dead && respawned,"Missing original Factory damage/death/respawn cycle");
        require(s.player.kills==1 && s.opponent.deaths==1,"Authoritative kill/death accounting changed");
        require(respawn_tick-death_tick>=237 && respawn_tick-death_tick<=243,"Respawn is not four seconds");
        std::cout<<"Combat: shots="<<s.player.shot_sequence<<", kills="<<s.player.kills<<", death tick="<<death_tick<<", respawn tick="<<respawn_tick<<'\n';
    }
};
void local() {
    gameplay::VerticalSliceSimulation sim{{.opponent_ai_enabled=false,.original_factory=true}};Driver driver;
    for(int i=0;i<4000 && !driver.respawned;++i) {
        sim.tick(driver.input(sim.snapshot()),{});
        if(i%600==0) std::cout<<"Local waypoint="<<driver.waypoint<<" position="<<sim.snapshot().player.position_x<<','<<sim.snapshot().player.position_y<<','<<sim.snapshot().player.position_z<<'\n';
    }
    driver.verify(sim.snapshot());
}
void send(network::Transport& transport,network::ConnectionId c,const gameplay::SliceWireMessage& msg){
    const auto b=network::encode_message(msg.message);transport.send(c,{.payload=b,.delivery=msg.delivery});
}
void clients(const std::string& external) {
    const bool embedded=external.empty();
    backends::JoltWorld physics;std::unique_ptr<backends::GnsTransport> server;std::unique_ptr<gameplay::VerticalSliceRemoteHost> host;
    std::string endpoint=external;
    if(embedded){physics.start();host=std::make_unique<gameplay::VerticalSliceRemoteHost>(gameplay::SliceRemoteHostSettings{.authoritative_physics=&physics,.original_factory=true});
        server=std::make_unique<backends::GnsTransport>();server->start();endpoint=server->listen("127.0.0.1:0");}
    backends::GnsTransport a,b;a.start();b.start();
    gameplay::VerticalSliceRemoteClient first{{.character=gameplay::SliceCharacter::archangel,.ability=gameplay::SliceAbility::none},true};
    gameplay::VerticalSliceRemoteClient second{{.character=gameplay::SliceCharacter::shadow,.ability=gameplay::SliceAbility::none},true};
    auto ca=a.connect(endpoint);network::ConnectionId cb=network::invalid_connection;
    bool hello_a=false,hello_b=false,reconnecting=false,reconnected=false;int resumed=0;
    Driver driver;
    gameplay::CombatEffects effects;render::ParticleSystem particles{render::load_particle_recipes(std::filesystem::path{GLOOM_TEST_ASSETS}/"effects/recipes.json")};
    gameplay::CharacterAnimator animator;const auto rig=assets::import_gltf(std::filesystem::path{GLOOM_TEST_ASSETS}/"characters/original/archangel.gltf");
    const auto start=std::chrono::steady_clock::now();std::uint64_t last_tick=0,last_event_count=0;
    auto next_input=start;
    for(int pump=0;pump<120000 && !reconnected;++pump) {
        const auto now_clock=std::chrono::steady_clock::now();
        const double now=std::chrono::duration<double>(now_clock-start).count();
        require(now<80,"Real-client review timed out");
        if(server) {
            server->tick(1.0/60);
            while(auto e=server->poll_event()) {
                if(e->incoming && e->state==network::ConnectionState::connected)host->connected(e->connection);
                else if(e->incoming && (e->state==network::ConnectionState::disconnected || e->state==network::ConnectionState::failed))host->disconnected(e->connection);
            }
            while(auto p=server->receive()) {
                const auto m=network::decode_message(p->payload);require(m.has_value(),"Malformed host packet");
                for(const auto& out:host->receive(p->connection,*m,now))send(*server,out.connection,out);
            }
        }
        a.tick(1.0/60);b.tick(1.0/60);
        while(auto e=a.poll_event()) if(e->connection==ca && e->state==network::ConnectionState::connected && !hello_a) {
            send(a,ca,reconnecting?first.reconnect():first.begin());hello_a=true;
        }
        // Admit first deterministically so route belongs to spawn 0.
        if(first.active() && cb==network::invalid_connection)cb=b.connect(endpoint);
        while(auto e=b.poll_event()) if(e->connection==cb && e->state==network::ConnectionState::connected && !hello_b){send(b,cb,second.begin());hello_b=true;}
        auto receive=[&](auto& transport,auto& client,auto connection){while(auto p=transport.receive()){
            const auto m=network::decode_message(p->payload);require(m.has_value(),"Malformed client packet");
            if(auto reply=client.receive(*m,now))send(transport,connection,*reply);
        }};
        receive(a,first,ca);receive(b,second,cb);
        if(first.has_snapshot() && first.snapshot().simulation_tick!=last_tick && (!reconnecting || hello_a)) {
            const auto& s=first.snapshot();last_tick=s.simulation_tick;
            const auto frame=animator.update(*rig,s.player,1.0/60);
            effects.observe(particles,s.player,frame,{},last_tick,true);particles.advance(1.0/60);
            if(reconnecting && first.active() && first.lobby().phase==gameplay::SliceMatchPhase::active && ++resumed>10) {
                require(effects.events()==last_event_count,"Reconnect replayed old confirmed shots");reconnected=true;
            }
        }
        if(first.active() && second.active() && (embedded || now_clock>=next_input)) {
            next_input=now_clock+std::chrono::microseconds(16667);
            auto input=driver.input(first.snapshot());
            if(!reconnecting)for(const auto& out:first.create_input(input))send(a,ca,out);
            for(const auto& out:second.create_input({}))send(b,cb,out);
            if(server)for(const auto& out:host->tick_clients())send(*server,out.connection,out);
        }
        if(driver.respawned && !reconnecting) {
            driver.verify(first.snapshot());
            require(first.snapshot().opponent.character==gameplay::SliceCharacter::shadow && second.snapshot().opponent.character==gameplay::SliceCharacter::archangel,"Character IDs changed");
            require(effects.events()>=4,"Confirmed muzzle/impact events did not reach presentation");
            last_event_count=effects.events();effects.reset(particles);animator.reset();
            a.disconnect(ca);ca=a.connect(endpoint);hello_a=false;reconnecting=true;last_tick=0;
        }
        if(pump%3000==0 && first.has_snapshot())std::cout<<"GNS waypoint="<<driver.waypoint<<" tick="<<first.snapshot().simulation_tick<<" position="<<first.snapshot().player.position_x<<','<<first.snapshot().player.position_y<<','<<first.snapshot().player.position_z<<'\n';
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    require(reconnected,"Real GNS reconnect failed");
    require(first.reconciliation_metrics().count>0,"No reconciliation exercised");
    std::cout<<(embedded?"Host/join":"Dedicated")<<": two real GNS clients, events="<<effects.events()<<", reconnect preserved entity="<<first.snapshot().player.entity<<", reconciliations="<<first.reconciliation_metrics().count<<", packets="<<a.metrics().received_packets+b.metrics().received_packets<<'\n';
    effects.reset(particles);require(particles.particles().empty(),"Scene exit leaked effects");
    a.disconnect(ca);b.disconnect(cb);a.stop();b.stop();if(server)server->stop();
}
}
int main(int argc,char** argv)try {
    if(argc==1){local();clients("");}
    else if(argc==3 && std::string_view{argv[1]}=="--clients")clients(argv[2]);
    else throw std::invalid_argument{"Usage: animation_network_tests [--clients endpoint]"};
    return 0;
}catch(const std::exception& e){std::cerr<<"Animation network review: "<<e.what()<<'\n';return 1;}
