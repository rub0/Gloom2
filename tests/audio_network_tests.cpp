#include <gloom/gameplay/audio_presentation.hpp>
#include <gloom/gameplay/vertical_slice_network.hpp>
#include <iostream>
#include <set>
#include <stdexcept>
using namespace gloom;
void require(bool v,const char* why){if(!v)throw std::runtime_error{why};}
network::ProtocolMessage wire(const network::ProtocolMessage& m){auto r=network::decode_message(network::encode_message(m));require(r.has_value(),"Protocol roundtrip failed");return *r;}
void admit(gameplay::VerticalSliceRemoteHost& host,gameplay::VerticalSliceRemoteClient& client,network::ConnectionId connection,bool reconnect=false){
    host.connected(connection);auto responses=host.receive(connection,wire((reconnect?client.reconnect():client.begin()).message),0);
    require(responses.size()==1,"Admission failed");auto request=client.receive(wire(responses[0].message),.001);require(request.has_value(),"Clock request missing");
    responses=host.receive(connection,wire(request->message),.002);require(responses.size()==1,"Clock reply missing");
    auto selection=client.receive(wire(responses[0].message),.003);if(reconnect)return;
    require(selection.has_value(),"Selection missing");static_cast<void>(host.receive(connection,wire(selection->message),.004));
    auto ready=client.receive(wire(gameplay::encode_lobby_state(host.lobby())),.005);require(ready.has_value(),"Ready missing");
    static_cast<void>(host.receive(connection,wire(ready->message),.006));
}
int main()try{
    gameplay::VerticalSliceRemoteHost host{gameplay::SliceRemoteHostSettings{}};
    gameplay::VerticalSliceRemoteClient first{{.account_id=7101,.display_name="AudioA"}},second{{.account_id=7102,.display_name="AudioB"}};
    admit(host,first,81);admit(host,second,82);
    audio::EventCursor a,b;std::set<std::uint64_t> heard_a,heard_b;
    assets::VirtualFileSystem fs;fs.mount("cache",std::filesystem::path{GLOOM_BINARY_ROOT}/"content");
    gameplay::AudioPresentation presentation_a{fs,false},presentation_b{fs,false};
    for(unsigned tick=0;tick<150;++tick){
        for(const auto& message:first.create_input({.aim_z=1,.jump=tick==20,.fire_primary=tick>5&&tick%31==0}))static_cast<void>(host.receive(81,wire(message.message),tick/60.0));
        for(const auto& message:second.create_input({.aim_z=-1,.jump=tick==50,.fire_primary=tick>5&&tick%37==0}))static_cast<void>(host.receive(82,wire(message.message),tick/60.0));
        for(const auto& message:host.tick_clients()){
            if(message.connection==82&&tick>=40&&tick<70)continue; // lost snapshots recovered from bounded journal
            auto& client=message.connection==81?first:second;static_cast<void>(client.receive(wire(message.message),(tick+1)/60.0));
            if(!client.has_snapshot())continue;const auto& state=client.snapshot();
            auto& cursor=message.connection==81?a:b;auto& heard=message.connection==81?heard_a:heard_b;
            for(const auto& event:cursor.observe(state.audio_events,state.simulation_tick))require(heard.insert(event.sequence).second,"Remote event duplicated");
            require(cursor.observe(state.audio_events,state.simulation_tick).empty(),"Duplicate snapshot replayed audio");
            auto& presentation=message.connection==81?presentation_a:presentation_b;
            presentation.update(state,{.position={state.player.position_x,state.player.position_y,state.player.position_z}},false,.05);
        }
    }
    require(!heard_a.empty()&&heard_a==heard_b,"Two perspectives did not receive each semantic event once");
    require(presentation_a.events_played()==heard_a.size()&&presentation_b.events_played()==heard_b.size(),"Presentation duplicated or lost events");
    audio::Mixer left,right;left.listener({.position={-10,0,0}});right.listener({.position={10,0,0}});
    auto ga=left.spatial_gains({.spatial=true}),gb=right.spatial_gains({.spatial=true});require(ga[1]>ga[0]&&gb[0]>gb[1],"Cameras did not spatialize independently");
    // A new observer starts from a late full snapshot without past combat sounds.
    audio::EventCursor late;require(late.observe(first.snapshot().audio_events,first.snapshot().simulation_tick).empty(),"Late join replayed old audio");
    const auto old_epoch=first.snapshot().audio_epoch;const auto heard_before=presentation_a.events_played();
    host.disconnected(81);for(unsigned i=0;i<20;++i)static_cast<void>(host.tick_clients());
    admit(host,first,83,true);
    for(unsigned tick=0;tick<12;++tick)for(const auto& m:host.tick_clients())if(m.connection==83){static_cast<void>(first.receive(wire(m.message),3+tick/60.0));presentation_a.update(first.snapshot(),{},false,.05);}
    require(first.snapshot().audio_epoch!=old_epoch,"Reconnect generation did not change");
    require(presentation_a.events_played()==heard_before,"Reconnect replayed historical events");
    require(!presentation_a.device_available()&&!presentation_b.device_available(),"Network tests opened devices");
    std::cout<<"Two clients: "<<heard_a.size()<<" unique events each, loss recovery, independent cameras, late baseline and reconnect passed\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
