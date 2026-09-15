#include "desktop.hpp"
#include "game_ui.hpp"
#include <gloom/backends/sdl_window.hpp>
#include <gloom/backends/diligent_renderer.hpp>
#include <gloom/backends/keycloak_identity.hpp>
#include <gloom/backends/winhttp_match_service.hpp>
#include <gloom/render/ui.hpp>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <mutex>
#include <thread>

namespace gloom::desktop {
namespace {
std::string env(const char* key){const auto* v=std::getenv(key);return v?v:"";}
std::uint64_t now_ms(){return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());}
struct Login {
    std::mutex mutex;std::string message;std::atomic_bool cancel{false};
    std::shared_ptr<gameplay::SliceMatchDirectory> directory;
    std::function<std::expected<std::string,std::string>()> identity;
    std::future<std::string> future;
    void say(std::string text){std::lock_guard lock{mutex};message=std::move(text);}
    std::string read(){std::lock_guard lock{mutex};return message;}
    void begin(){
        cancel=false;say("Conectando con el servicio de partidas...");
        future=std::async(std::launch::async,[this]()->std::string{try{
            const auto url=env("GLOOM_MATCH_SERVICE_URL");
            if(url.empty())return "El servicio de partidas no está configurado. Puedes usar Conexión directa.";
            const auto provider=env("GLOOM_MATCH_IDENTITY_PROVIDER");
            if(provider=="keycloak"){
                auto sign=std::make_shared<backends::KeycloakSignIn>(backends::KeycloakClientSettings{env("GLOOM_KEYCLOAK_ISSUER"),env("GLOOM_KEYCLOAK_CLIENT_ID"),env("GLOOM_GAME_AUTH")=="tickets"});
                const auto challenge=sign->begin();if(!challenge)return challenge.error();
                say("Inicia sesión en "+challenge->verification_uri+"   Código: "+challenge->user_code);
                for(;;){if(cancel){sign->cancel();return "Inicio de sesión cancelado.";}const auto r=sign->poll();if(!r)return r.error();if(*r)break;std::this_thread::sleep_for(std::chrono::milliseconds{100});}
                identity=[sign]{return sign->access_token();};
            }else if(provider.empty() || provider=="registry"){
                const auto token=env("GLOOM_MATCH_IDENTITY_TOKEN");
                if(token.empty())return "Falta la credencial de acceso al navegador. Revisa la configuración del servicio.";
                identity=[token]{return std::expected<std::string,std::string>{token};};
            }else return "Proveedor de autenticación desconocido.";
            if(cancel)return "Inicio de sesión cancelado.";
            say("Cargando partidas disponibles...");const auto token=identity();if(!token)return token.error();
            auto connector=backends::make_winhttp_match_reader_connector(identity);
            auto result=gameplay::connect_match_directory(*connector,{.service_url=url,.bearer_token=*token,.maximum_attempts=1});
            if(!result)return result.error();directory=std::move(*result);return {};
        }catch(const std::exception& e){return e.what();}});
    }
};
}
std::vector<std::string> menu(Session& session,const std::filesystem::path& review,unsigned width,unsigned height){
    if(review.empty()&&!session.audio){
        assets::VirtualFileSystem fs;fs.mount("game",std::filesystem::path{GLOOM_SOURCE_ROOT}/"assets");fs.mount("cache",std::filesystem::path{GLOOM_BINARY_ROOT}/"content");
        session.audio=std::make_shared<gameplay::AudioPresentation>(fs,true);
    }
    if(session.audio)session.audio->scene_reset();
    auto audio_time=std::chrono::steady_clock::now();
    backends::SdlWindow window{{.title="Gloom",.width=width,.height=height}};window.start();
    render::RendererSettings settings;settings.upload_budget_bytes_per_frame=64U*1024*1024;
    backends::DiligentRenderer renderer{window,settings};renderer.start();
    render::UiCanvas ui{std::filesystem::path{GLOOM_SOURCE_ROOT}/"assets/ui/original"};renderer.enqueue(ui.atlas_upload());
    enum class Page{main,browser,direct,host,roster,exit};Page page=Page::main;
    std::string endpoint="127.0.0.1:27020",name=session.flow_name.empty()?"Player":session.flow_name,notice=std::move(session.notice),mode="local";
    std::string target;std::size_t selected=0,page_index=0;bool closing=false;
    Login login;gameplay::AsyncSliceMatchBrowser browser;
    const std::array<std::string,5> loadouts{"hound-bite","hound-guard","hound-reaper","archangel-reaper","shadow-reaper"};
    const std::array<std::string,5> labels{"Hound · Mordisco","Hound · Escudo","Hound · Soul Reaper","Archangel","Shadow"};
    std::vector<std::string> launch;
    unsigned flow_stage=0;
    unsigned frame=0;const unsigned review_pages=18;
    std::optional<GameUi> review_game;if(!review.empty())review_game.emplace();
    if(!review.empty())std::filesystem::create_directories(review);
    while(window.poll_events()){
        const auto audio_now=std::chrono::steady_clock::now();
        if(session.audio)session.audio->menu(std::chrono::duration<double>(audio_now-audio_time).count());
        audio_time=audio_now;
        if(login.future.valid() && login.future.wait_for(std::chrono::seconds{0})==std::future_status::ready){
            notice=login.future.get();if(notice.empty() && !login.cancel){session.directory=login.directory;session.identity=login.identity;static_cast<void>(browser.begin_refresh(*session.directory,now_ms()));}
        }
        static_cast<void>(browser.poll());
        if(closing && !login.future.valid() && !browser.refreshing()){session.quit=true;break;}
        const auto [w,h]=window.drawable_size();renderer.resize(w,h);auto input=window.input_state();
        if(!session.flow_output.empty()){
            input={};
            if(frame>12 && frame%8==4){
                if(page==Page::main && flow_stage==0){input.mouse_x=200;input.mouse_y=375;input.mouse_primary=true;++flow_stage;}
                else if(page==Page::browser && !browser.refreshing() && !browser.matches().empty() && flow_stage==1){input.mouse_x=300;input.mouse_y=310;input.mouse_primary=true;++flow_stage;}
                else if(page==Page::roster && flow_stage==2){input.mouse_x=300;input.mouse_y=298+static_cast<float>(session.flow_selection)*58;input.mouse_primary=true;++flow_stage;}
                else if(page==Page::roster && flow_stage==3){input.mouse_x=750;input.mouse_y=605;input.mouse_primary=true;++flow_stage;}
            }
            if(frame>1200)throw std::runtime_error{"UI browser/selection driver timed out"};
        }
        // Gallery fixtures must not depend on the desktop pointer/focus left by
        // another graphical test. Interactive and flow paths keep their input.
        if(!review.empty())input={};
        ui.begin(w,h,input);
        const unsigned review_page=frame/8;
        if(!review.empty()){
            switch(review_page){case 0:page=Page::main;break;case 1:page=Page::browser;notice="";break;
                case 2:page=Page::direct;break;case 3:page=Page::host;break;case 4:page=Page::roster;selected=3;break;
                case 5:page=Page::roster;selected=4;break;case 6:page=Page::browser;notice="No se pudo conectar. Revisa la conexión y vuelve a intentarlo.";break;
                default:page=Page::exit;break;}
        }
        if(ui.back()){if(page==Page::main)page=Page::exit;else{page=Page::main;login.cancel=true;}}
        if(page==Page::main){
            ui.background("VUELVE A LA ARENA","Elige cómo entrar en Factory.");
            if(ui.button("local",{70,282,400,54},"Jugar en local")){mode="local";page=Page::roster;}
            if(ui.button("browser",{70,350,400,54},"Buscar partidas")){page=Page::browser;notice.clear();if(session.directory)static_cast<void>(browser.begin_refresh(*session.directory,now_ms()));else if(!login.future.valid())login.begin();}
            if(ui.button("direct",{70,418,400,54},"Conexión directa")){page=Page::direct;mode="join";}
            if(ui.button("host",{70,486,400,54},"Crear sala local")){page=Page::host;mode="host";}
            if(ui.button("exit",{70,554,400,54},"Salir"))page=Page::exit;
            ui.paragraph(540,530,notice,620,20);
        }else if(page==Page::browser){
            ui.background("PARTIDAS","Factory · Salas disponibles");ui.panel({70,266,1140,298});
            if(login.future.valid())ui.paragraph(100,305,login.read(),1050,23,render::ui_cyan);
            else if(browser.refreshing())ui.text(100,305,"Buscando partidas...",26);
            else if(!notice.empty() || !browser.error().empty())ui.paragraph(100,305,!notice.empty()?notice:std::string{browser.error()},1040,23,{1,.6F,.45F,1});
            else if(browser.matches().empty())ui.paragraph(100,305,"No hay partidas disponibles. Actualiza la lista o conecta directamente a una sala conocida.",1020,23);
            else{
                const auto matches=browser.matches();if(page_index*4>=matches.size())page_index=0;
                for(std::size_t i=page_index*4;i<std::min(matches.size(),page_index*4+4);++i){const auto& m=matches[i];
                    if(ui.button("match"+std::to_string(i),{94,285+static_cast<float>(i%4)*63,1092,54},m.display_name+"    "+std::to_string(m.player_count)+" / "+std::to_string(m.capacity))){mode="join";target="match:"+m.match_id;page=Page::roster;}}
            }
            if(ui.button("refresh",{70,582,230,48},"Actualizar",!login.future.valid()&&!browser.refreshing())){notice.clear();if(session.directory)static_cast<void>(browser.begin_refresh(*session.directory,now_ms()));else login.begin();}
            if(browser.matches().size()>4){if(ui.button("next",{320,582,200,48},"Más salas"))page_index=(page_index+1)%((browser.matches().size()+3)/4);}
            else if(ui.button("login",{320,582,200,48},"Acceso",!login.future.valid()&&!browser.refreshing())){
                session.directory.reset();session.identity={};login.directory.reset();login.identity={};notice.clear();login.begin();
            }
            if(ui.button("manual",{540,582,290,48},"Conexión directa")){page=Page::direct;mode="join";}
            if(ui.button("back",{970,582,240,48},"Volver")){login.cancel=true;page=Page::main;}
        }else if(page==Page::direct || page==Page::host){
            const bool host=page==Page::host;ui.background(host?"CREAR SALA":"CONEXIÓN DIRECTA",host?"Servidor local · Dos plazas para jugadores remotos":"Conecta a una dirección conocida");
            ui.panel({70,274,700,280});ui.text(96,294,host?"Dirección de escucha":"Dirección del servidor",20);
            ui.field("endpoint",{96,332,648,48},endpoint,120);ui.text(96,395,"Nombre del jugador",20);ui.field("name",{96,430,648,48},name,24);
            ui.paragraph(810,300,host?"Mantén esta ventana abierta mientras juegan los clientes. Los servidores publicados se gestionan con el dedicado existente.":"Ejemplo: 127.0.0.1:27020. Las partidas con acceso verificado se abren desde Buscar partidas.",360,21);
            if(ui.button("connect",{70,580,350,50},host?"Abrir sala":"Elegir personaje",!endpoint.empty()&&!name.empty())){
                if(host){launch={"--vertical-slice-host",endpoint};break;}
                const auto resolved=gameplay::resolve_slice_join_target(endpoint,nullptr,now_ms());
                if(!resolved)notice="Dirección no válida. Usa IP:puerto.";else{mode="join";target=endpoint;page=Page::roster;notice.clear();}
            }
            if(ui.button("back",{450,580,320,50},"Volver"))page=Page::main;ui.text(96,502,notice,18,{1,.6F,.45F,1});
        }else if(page==Page::roster){
            ui.background("ELIGE TU PERSONAJE",mode=="local"?"Partida local · Factory":"Tu selección se confirmará en la sala");
            for(std::size_t i=0;i<labels.size();++i)if(ui.button("roster"+std::to_string(i),{70,274+static_cast<float>(i)*58,450,50},labels[i]))selected=i;
            ui.panel({570,264,640,290});ui.image(selected==4?"shadow":"archangel",{606,283,164,243});
            ui.text(805,292,selected==4?"SHADOW":selected==3?"ARCHANGEL":"HOUND",38,render::ui_cyan,true);
            ui.paragraph(805,352,selected==4?"Armadura oscura, cola espectral y ojos rojos.":"Armadura dorada y energía cian. Entra en la arena de Factory.",350,22);
            ui.text(805,459,selected<2?(selected==0?"Habilidad: Mordisco":"Habilidad: Escudo"):"Arma: Soul Reaper",20);
            if(mode!="local")ui.field("player-name",{805,499,370,44},name,24);
            if(ui.button("play",{570,580,400,50},mode=="local"?"Entrar en Factory":"Entrar en la sala",!login.future.valid()&&!name.empty())){
                if(mode=="local")launch={"--vertical-slice",loadouts[selected]};
                else launch={"--vertical-slice-join",target,name,loadouts[selected]};break;
            }
            if(ui.button("back",{990,580,220,50},"Volver"))page=Page::main;
        }else{
            ui.background("SALIR DE GLOOM","¿Quieres cerrar el juego?");ui.panel({250,300,780,230});
            ui.paragraph(285,325,closing?"Cancelando la solicitud pendiente...":"Puedes volver al menú para entrar en otra partida.",700,24);
            if(ui.button("cancel",{280,430,335,60},"Seguir jugando",!closing))page=Page::main;
            if(ui.button("quit",{655,430,335,60},"Salir",!closing)){login.cancel=true;closing=true;}
        }
        const render::UiDrawData* draw_data=&ui.data();
        if(!review.empty() && ((review_page>=8 && review_page<=13) || review_page==16 || review_page==17)){
            gameplay::SliceSnapshot fixture;fixture.player.character=gameplay::SliceCharacter::archangel;
            fixture.player.life=175;fixture.player.shield=35;fixture.hud.life_fraction=.7F;fixture.hud.shield_fraction=.35F;
            fixture.hud.kills=3;fixture.hud.deaths=1;
            if(review_page==16){
                fixture.player.entity=1;fixture.opponent.entity=2;fixture.player.kills=3;fixture.player.deaths=1;
                fixture.opponent.kills=5;fixture.opponent.deaths=2;fixture.opponent.character=gameplay::SliceCharacter::shadow;
            }
            if(review_page==17){
                fixture.player.entity=1;fixture.opponent.entity=2;fixture.player.life=120;fixture.hud.life_fraction=.48F;
                fixture.opponent.position_x=3;fixture.opponent.position_z=3;
            }
            gameplay::SliceLobbyState lobby;lobby.phase=gameplay::SliceMatchPhase::active;
            lobby.players={{.entity=1,.identity={1,"Nyx"},
                                .selection={.character=gameplay::SliceCharacter::archangel,.ability=gameplay::SliceAbility::diamond_skin},
                                .connected=true,.ready=true},
                           {.entity=2,.identity={2,"Rook"},
                                .selection={.character=gameplay::SliceCharacter::shadow,.ability=gameplay::SliceAbility::invisibility},
                                .connected=true,.ready=false}};
            if(review_page==9){fixture.hud.dead=true;fixture.player.life=0;fixture.hud.life_fraction=0;fixture.player.respawn_remaining_seconds=3;}
            if(review_page==10)lobby.phase=gameplay::SliceMatchPhase::waiting;
            platform::InputState neutral;neutral.menu_tab=review_page==16;review_game->paused=review_page==12;review_game->was_playing=true;
            static_cast<void>(review_game->draw(w,h,neutral,fixture,&lobby,false,review_page!=13,review_page!=11,review_page==11,review_page!=10,3,"127.0.0.1:27020",{},120));
            draw_data=&review_game->canvas.data();
        }else if(!review.empty() && review_page==14){
            ui.background("INICIAR SESIÓN","Acceso al navegador de partidas");ui.panel({70,270,1140,300});
            ui.text(100,306,"Abre la dirección de tu proveedor y confirma el código.",25);
            ui.text(100,370,"https://cuentas.example/activate",27,render::ui_cyan);
            ui.text(100,430,"GLOOM-6400",44,render::ui_ink,true);ui.button("cancel-login",{840,490,330,52},"Cancelar");
        }else if(!review.empty() && review_page==15){
            ui.background("PARTIDAS","Factory · Salas disponibles");ui.panel({70,270,1140,300});
            ui.button("sample-1",{94,290,1092,60},"Factory / Europa    1 / 2");ui.button("sample-2",{94,365,1092,60},"Arena nocturna    0 / 2");
            ui.button("refresh",{70,592,280,48},"Actualizar");ui.button("back",{940,592,270,48},"Volver");
        }
        renderer.begin_frame();renderer.draw({.ui=draw_data});
        if(!session.flow_output.empty() && frame%8==3)renderer.capture_next_frame(session.flow_output/("menu-stage-"+std::to_string(flow_stage)+".ppm"));
        if(!review.empty() && frame%8==7)renderer.capture_next_frame(review/("page-"+std::to_string(review_page)+".ppm"));
        renderer.end_frame();++frame;
        if(!review.empty() && frame==review_pages*8)break;
    }
    login.cancel=true;
    if(launch.empty() && review.empty())session.quit=true;
    renderer.stop();window.stop();return launch;
}
} // namespace gloom::desktop
