#pragma once
#include <gloom/render/ui.hpp>
#include <gloom/gameplay/vertical_slice.hpp>
#include <gloom/gameplay/match_lobby.hpp>
#include <gloom/gameplay/legacy_arsenal.hpp>
#include <optional>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace gloom::desktop {
struct GameUiAction {bool leave{},ready{},reconnect{};std::optional<std::size_t> selection;};
class GameUi {
public:
    render::UiCanvas canvas{std::filesystem::path{GLOOM_SOURCE_ROOT}/"assets/ui/original"};
    std::chrono::steady_clock::time_point fps_start{std::chrono::steady_clock::now()};
    unsigned fps_frames{};float fps{};
    bool paused{},confirm_leave{},blocked{},was_playing{};
    GameUiAction draw(unsigned width,unsigned height,const platform::InputState& input,
        const gameplay::SliceSnapshot& s,const gameplay::SliceLobbyState* lobby,bool hosting,
        bool loaded,bool connected,bool reconnecting,bool selection_confirmed,
        std::size_t selection,std::string_view endpoint,std::string_view error,float review_fps=-1){
        canvas.begin(width,height,input);GameUiAction action;
        const bool playing=!hosting && (!lobby || (lobby->phase==gameplay::SliceMatchPhase::active && connected));
        if(!input.focused && playing)paused=true;
        if(canvas.back()){if(confirm_leave)confirm_leave=false;else if(playing)paused=!paused;else confirm_leave=true;}
        if(playing && !was_playing)paused=false;was_playing=playing;
        blocked=!playing || !loaded || paused || confirm_leave || !input.focused;
        if(playing){
            canvas.ornament({158,573,964,132});
            const auto now=std::chrono::steady_clock::now();
            const float seconds=std::chrono::duration<float>(now-fps_start).count();
            if(++fps_frames && seconds>=.5F){fps=fps_frames/seconds;fps_frames=0;fps_start=now;}
            std::ostringstream stats;stats<<std::fixed<<std::setprecision(0)<<(review_fps>=0?review_fps:fps)<<" FPS\n"<<std::setprecision(2)
                <<"X "<<s.player.position_x<<"  Y "<<s.player.position_y<<"  Z "<<s.player.position_z;
            canvas.rect({24,20,400,89},{.01F,.025F,.03F,.72F});
            canvas.text(36,26,stats.str(),17);canvas.text(36,78,"Esc · Pausa",14,render::ui_muted);
            for(unsigned i=0;i<2;++i){
                const float x=70+i*46.F, fraction=std::clamp(i?s.hud.shield_fraction:s.hud.life_fraction,0.F,1.F);
                const render::UiColor color=i?render::UiColor{.12F,.65F,.82F,1}:render::UiColor{.85F,.18F,.13F,1};
                canvas.rect({x-3,516,26,155},{.04F,.10F,.11F,.95F});
                canvas.rect({x,519,20,149},{.015F,.03F,.035F,1});
                canvas.rect({x,519+149*(1-fraction),20,149*fraction},color);
                canvas.image(i?"shield_icon":"life_icon",{x-4,478,28,31});
                canvas.text(x-4,677,std::to_string(static_cast<int>(i?s.player.shield:s.player.life)),15);
            }
            const std::array<std::string_view,5> icons{"reaper","sniper","shotgun","minigun","goat"};
            for(unsigned i=0;i<icons.size();++i){
                const float x=268+i*146.F;const bool selected=static_cast<unsigned>(s.player.weapon)==i;
                const bool owned=(s.player.owned_weapons&(1U<<i))!=0;
                canvas.image(icons[i],{x,644,86,43},owned?render::UiColor{1,1,1,1}:render::UiColor{.22F,.27F,.27F,.65F});
                canvas.text(x+3,620,std::to_string(i+1),14,selected?render::ui_cyan:render::ui_muted);
                if(selected){canvas.text(x+23,620,std::to_string(s.hud.ammunition)+" / "+std::to_string(s.hud.maximum_ammunition),14);
                    canvas.rect({x,691,86*std::clamp(s.hud.weapon_ready_fraction,0.F,1.F),3},render::ui_cyan);
                    if(s.hud.weapon_charge_fraction>0)canvas.rect({x,697,86*s.hud.weapon_charge_fraction,3},{1,.32F,.12F,1});}
            }
            if(s.player.ability!=gameplay::SliceAbility::none){
                const std::string_view icon=s.player.ability==gameplay::SliceAbility::bite?"bite":
                    s.player.ability==gameplay::SliceAbility::invisibility?"shadow":"heal";
                canvas.image(icon,{174,587,42,42});
                canvas.ring(195,608,25,s.hud.primary_ability_ready_fraction,
                    s.hud.primary_ability_active?render::UiColor{1,.8F,.2F,1}:render::ui_cyan);
                canvas.text(188,555,"Q",15,render::ui_cyan);
            }
            if(s.player.secondary_ability!=gameplay::SliceSecondaryAbility::none){
                const std::string_view icon=s.player.secondary_ability==gameplay::SliceSecondaryAbility::life_dome?"heal":
                    s.player.secondary_ability==gameplay::SliceSecondaryAbility::flash?"shield_icon":"bite";
                canvas.image(icon,{220,587,42,42});
                canvas.ring(241,608,25,s.hud.secondary_ability_ready_fraction,
                    s.hud.secondary_ability_active?render::UiColor{1,.45F,.12F,1}:render::ui_cyan);
                canvas.text(235,555,"E",15,render::ui_cyan);
            }
            canvas.panel({478,22,324,59});canvas.text(506,37,std::to_string(s.hud.kills)+"  BAJAS    /    "+std::to_string(s.hud.deaths)+"  MUERTES",20);
            if(s.player.damage_modifier_ticks)canvas.text(920,90,"DAÑO x3  "+std::to_string((s.player.damage_modifier_ticks+59)/60)+" s",20);
            if(s.player.cooldown_modifier_ticks)canvas.text(920,116,"CADENCIA  "+std::to_string((s.player.cooldown_modifier_ticks+59)/60)+" s",20);
            if(!s.hud.dead)canvas.image("crosshair",{623,343,34,34},s.hud.hit_marker?render::UiColor{1,.25F,.15F,1}:render::ui_ink);
            else {canvas.panel({340,264,600,170});canvas.text(486,286,"HAS CAÍDO",46,render::ui_cyan,true);
                canvas.text(413,366,"Reaparición en "+std::to_string(static_cast<int>(std::ceil(s.player.respawn_remaining_seconds)))+" s",25);}
            if(s.player.flash_factor>1.0F)canvas.rect({0,0,static_cast<float>(width),static_cast<float>(height)},
                {1,1,1,std::clamp(s.player.flash_factor/50.0F,0.18F,1.0F)});

        }
        if(!playing && !confirm_leave){
            canvas.background(hosting?"SALA LOCAL":reconnecting?"RECONECTANDO":"SALA DE PARTIDA",hosting?"Servidor de Factory · Dos plazas remotas":"Elige tu personaje y confirma cuando estés listo.");
            canvas.panel({70,272,690,276});
            if(lobby && !lobby->players.empty()){
                for(std::size_t i=0;i<lobby->players.size() && i<2;++i){const auto& player=lobby->players[i];
                    const float y=296+static_cast<float>(i)*110;
                    canvas.text(96,y,player.identity.display_name,27);canvas.text(96,y+45,player.connected?(player.ready?"LISTO":"ELIGIENDO PERSONAJE"):"DESCONECTADO",18,render::ui_cyan);
                }
            }else canvas.text(96,320,"Esperando jugadores...",26);
            canvas.panel({794,272,416,276});
            if(!error.empty())canvas.paragraph(820,293,error,358,21,{1,.55F,.4F,1});
            else if(reconnecting)canvas.paragraph(820,293,"Se ha perdido la conexión. Intentando recuperar tu sesión...",358,23);
            else if(!connected && !hosting)canvas.paragraph(820,293,"Conectando con la sala y verificando el acceso...",358,23);
            else if(lobby && lobby->phase==gameplay::SliceMatchPhase::completed)canvas.paragraph(820,293,"La partida ha terminado. Puedes volver al menú y elegir otra sala.",358,23);
            else canvas.paragraph(820,293,hosting?std::string{"Sala abierta en "+std::string{endpoint}}:"La partida comienza cuando ambos jugadores confirman su selección.",358,22);
            if(!hosting && !selection_confirmed && connected){
                const std::array<std::string_view,5> labels{"Hound / Mordisco","Hound / Escudo","Hound / Soul Reaper","Archangel","Shadow"};
                canvas.text(92,556,labels[selection%5],24);
                if(canvas.button("previous",{70,595,150,45},"Anterior"))action.selection=(selection+4)%5;
                if(canvas.button("next",{232,595,150,45},"Siguiente"))action.selection=(selection+1)%5;
                if(canvas.button("ready",{400,595,260,45},"Estoy listo"))action.ready=true;
            }
            if(!hosting && !connected && !reconnecting && !error.empty() && canvas.button("retry",{800,480,380,48},"Reintentar"))action.reconnect=true;
            if(canvas.button("leave",{964,595,246,45},"Volver al menú"))confirm_leave=true;
        }
        if(!loaded && playing){canvas.panel({300,284,680,150});canvas.text(400,328,"Cargando Factory...",40,render::ui_cyan,true);}
        if(paused && playing && !confirm_leave){
            canvas.rect({0,0,1280,720},{.01F,.025F,.03F,.68F});canvas.panel({380,175,520,380});canvas.text(432,198,"PAUSA",48,render::ui_cyan,true);
            canvas.text(432,262,lobby?"La partida online continúa.":"Partida local pausada.",20);
            if(canvas.button("resume",{425,316,430,54},"Continuar"))paused=false;
            if(lobby && canvas.button("reconnect",{425,384,430,54},"Reconectar a la sala")){action.reconnect=true;paused=false;}
            if(canvas.button("leave",{425,452,430,54},"Volver al menú"))confirm_leave=true;
        }
        if(confirm_leave){
            canvas.rect({0,0,1280,720},{.01F,.025F,.03F,.8F});canvas.panel({290,247,700,230});
            canvas.text(334,277,"¿ABANDONAR LA PARTIDA?",37,render::ui_cyan,true);
            canvas.text(335,334,"Volverás al menú principal.",23);
            if(canvas.button("stay",{325,396,305,54},"Continuar"))confirm_leave=false;
            if(canvas.button("confirm-leave",{650,396,305,54},"Abandonar"))action.leave=true;
        }
        return action;
    }
};
}
