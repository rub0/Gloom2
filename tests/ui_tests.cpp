#include <gloom/render/ui.hpp>
#include <stdexcept>
#include <iostream>
#include <cmath>

using namespace gloom;
void require(bool ok,const char* message){if(!ok)throw std::runtime_error{message};}
int main()try{
    render::UiCanvas ui{GLOOM_UI_ASSETS};platform::InputState input;
    const auto atlas=ui.atlas_upload();require(atlas.mip_levels.size()==1 && atlas.mip_levels[0].data.size()==4096ULL*2048*4,"Atlas dimensions lost");
    const auto draw=[&]{const bool a=ui.button("a",{100,100,300,60},"Primero");const bool b=ui.button("b",{100,180,300,60},"Segundo");return std::pair{a,b};};
    ui.begin(1280,720,input);require(!draw().first,"First frame activated a button");
    input.menu_tab=true;ui.begin(1280,720,input);draw();require(ui.focus()=="b","Tab did not move focus");
    input.menu_tab=false;input.menu_confirm=true;ui.begin(1280,720,input);require(draw().second,"Enter did not activate focus");
    ui.begin(1280,720,input);require(!draw().second,"Held Enter repeats actions");
    input={};input.mouse_x=200;input.mouse_y=130;input.mouse_primary=true;ui.begin(1280,720,input);require(draw().first,"Mouse click missed button");
    input={};ui.begin(2560,1080,input);draw();
    // 1280-wide safe area at 1.5 scale, centered inside an ultrawide drawable.
    input.mouse_x=320+200*1.5F;input.mouse_y=130*1.5F;input.mouse_primary=true;
    ui.begin(2560,1080,input);require(draw().first,"Ultrawide/DPI hit transform diverged");
    input={};ui.begin(1920,1080,input);ui.background("MENÚ","Escala 150% · Texto original");
    for(const auto& v:ui.data().vertices)require(std::isfinite(v.x)&&std::isfinite(v.y)&&v.x>=-1.001F&&v.x<=1.001F&&v.y>=-1.001F&&v.y<=1.001F,"UI escaped drawable");
    std::string name="Player";input={};ui.begin(1280,720,input);ui.field("name",{100,100,300,60},name);
    input.mouse_x=200;input.mouse_y=120;input.mouse_primary=true;input.select_all=true;input.text={'N','y','x',0};
    ui.begin(1280,720,input);ui.field("name",{100,100,300,60},name);require(name=="Nyx" && ui.text_focused(),"Text field did not accept replacement");
    input={};input.backspace=true;ui.begin(1280,720,input);ui.field("name",{100,100,300,60},name);require(name=="Ny","Backspace failed");
    name="A";input={};input.text={static_cast<char>(0xC3),static_cast<char>(0xB1),0};
    ui.begin(1280,720,input);ui.field("name",{100,100,300,60},name,2);require(name=="A","Byte limit split a UTF-8 character");
    ui.begin(1280,720,input);ui.field("name",{100,100,300,60},name,3);require(name.size()==3,"Complete UTF-8 character rejected");
    input={};input.backspace=true;ui.begin(1280,720,input);ui.field("name",{100,100,300,60},name);require(name=="A","Backspace split a UTF-8 character");
    input={};input.focused=false;input.menu_confirm=true;ui.begin(1280,720,input);require(!ui.button("name",{100,100,300,60},"No activar"),"Unfocused window activated UI");
    ui.begin(1280,720,{});ui.vignette({1,0,0,.5F});require(ui.data().vertices.size()==24,"Damage vignette geometry changed");
    ui.arc(640,360,48,0,.14F,{1,0,0,1},6);require(ui.data().vertices.size()==78,"Directional damage arc geometry changed");
    std::cout<<"UI atlas, keyboard edges/focus, text editing, mouse and DPI/ultrawide checks passed\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
