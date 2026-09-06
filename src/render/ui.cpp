#include <gloom/render/ui.hpp>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace gloom::render {
UiCanvas::UiCanvas(const std::filesystem::path& directory):directory_{directory}{
    std::ifstream in{directory_/"atlas.txt"};in>>atlas_width_>>atlas_height_;
    if(!in || atlas_width_>8192 || atlas_height_>8192 || !atlas_width_ || !atlas_height_)throw std::runtime_error{"Invalid UI atlas"};
    std::string name;Sprite s{};
    while(in>>name>>s.x>>s.y>>s.w>>s.h>>s.advance){
        if(s.x<0 || s.y<0 || s.w<=0 || s.h<=0 || s.x+s.w>atlas_width_ || s.y+s.h>atlas_height_)throw std::runtime_error{"Invalid UI sprite"};
        sprites_.emplace(name,s);
    }
    if(!sprites_.contains("white") || !sprites_.contains("g63"))throw std::runtime_error{"Incomplete UI atlas"};
}
TextureUpload UiCanvas::atlas_upload() const {
    TextureUpload out;out.id=ui_atlas_id;out.mip_levels.push_back({atlas_width_,atlas_height_,{}});
    std::ifstream in{directory_/"atlas.rgba",std::ios::binary};
    auto& pixels=out.mip_levels[0].data;pixels.resize(static_cast<std::size_t>(atlas_width_)*atlas_height_*4);
    in.read(reinterpret_cast<char*>(pixels.data()),static_cast<std::streamsize>(pixels.size()));
    if(!in || in.peek()!=std::char_traits<char>::eof())throw std::runtime_error{"UI atlas byte count mismatch"};
    return out;
}
void UiCanvas::begin(std::uint32_t width,std::uint32_t height,const platform::InputState& input){
    width_=static_cast<float>(std::max(1U,width));height_=static_cast<float>(std::max(1U,height));
    scale_=std::min(width_/1280,height_/720);left_=(width_-1280*scale_)*.5F;top_=(height_-720*scale_)*.5F;
    input_=input;if(first_){previous_=input;first_=false;}
    mouse_x_=(input.mouse_x-left_)/scale_;mouse_y_=(input.mouse_y-top_)/scale_;
    click_=input.mouse_primary && !previous_.mouse_primary && input.focused;
    confirm_=input.menu_confirm && !previous_.menu_confirm && input.focused;
    escape_=input.menu_back && !previous_.menu_back;
    previous_controls_=std::move(controls_);controls_.clear();data_.vertices.clear();text_focused_=false;
    if(!previous_controls_.empty() && std::find(previous_controls_.begin(),previous_controls_.end(),focus_)==previous_controls_.end())focus_=previous_controls_.front();
    const bool forward=(input.menu_tab && !previous_.menu_tab)||(input.menu_down && !previous_.menu_down);
    const bool backward=(input.menu_up && !previous_.menu_up)||(forward && input.menu_shift);
    if((forward || backward) && !previous_controls_.empty()){
        auto it=std::find(previous_controls_.begin(),previous_controls_.end(),focus_);
        auto i=it==previous_controls_.end()?0:static_cast<std::size_t>(it-previous_controls_.begin());
        i=backward?(i+previous_controls_.size()-1)%previous_controls_.size():(i+1)%previous_controls_.size();focus_=previous_controls_[i];
    }
    previous_=input;
}
void UiCanvas::image(std::string_view name,UiRect r,UiColor c){
    if(r.w<=0 || r.h<=0 || data_.vertices.size()+6>65536)return;
    const auto it=sprites_.find(std::string{name});if(it==sprites_.end())return;const auto s=it->second;
    const float x0=(left_+r.x*scale_)/width_*2-1,x1=(left_+(r.x+r.w)*scale_)/width_*2-1;
    const float y0=1-(top_+r.y*scale_)/height_*2,y1=1-(top_+(r.y+r.h)*scale_)/height_*2;
    const float aw=static_cast<float>(atlas_width_),ah=static_cast<float>(atlas_height_);
    const float u0=(s.x+.5F)/aw,u1=(s.x+s.w-.5F)/aw,v0=(s.y+.5F)/ah,v1=(s.y+s.h-.5F)/ah;
    for(const auto& p:std::array{std::array{x0,y0,u0,v0},std::array{x1,y0,u1,v0},std::array{x0,y1,u0,v1},
                               std::array{x0,y1,u0,v1},std::array{x1,y0,u1,v0},std::array{x1,y1,u1,v1}})
        data_.vertices.push_back({p[0],p[1],p[2],p[3],c[0],c[1],c[2],c[3]});
}
void UiCanvas::rect(UiRect r,UiColor c){image("white",r,c);}
void UiCanvas::ring(float x,float y,float radius,float fraction,UiColor c,float thickness){
    const auto s=sprites_.at("white");
    const float u=(s.x+.5F)/atlas_width_,v=(s.y+.5F)/atlas_height_;
    const float end=std::clamp(fraction,0.F,1.F)*64;
    for(unsigned i=0;i<static_cast<unsigned>(std::ceil(end));++i){
        const float a=-1.57079633F+i*6.28318531F/64,b=-1.57079633F+std::min(static_cast<float>(i+1),end)*6.28318531F/64;
        auto vertex=[&](float angle,float r){return UiVertex{(left_+(x+std::cos(angle)*r)*scale_)/width_*2-1,
            1-(top_+(y+std::sin(angle)*r)*scale_)/height_*2,u,v,c[0],c[1],c[2],c[3]};};
        if(data_.vertices.size()+6>65536)return;
        const auto p=vertex(a,radius),q=vertex(b,radius),r=vertex(a,radius-thickness),t=vertex(b,radius-thickness);
        for(auto point:{p,q,r,r,q,t})data_.vertices.push_back(point);
    }
}
void UiCanvas::text(float x,float y,std::string_view value,float size,UiColor c,bool heading){
    const float start=x;
    for(std::size_t i=0;i<value.size();++i){
        unsigned code=static_cast<unsigned char>(value[i]);
        if(code=='\n'){x=start;y+=size*1.4F;continue;}
        if(code>=192 && code<224 && i+1<value.size()){code=((code&31)<<6)|(static_cast<unsigned char>(value[++i])&63);}
        else if(code>=128)code=63;
        const std::string key=(heading?"h":"g")+std::to_string(code);
        auto it=sprites_.find(key);if(it==sprites_.end())it=sprites_.find("g63");
        const auto s=it->second;const float factor=size/32;
        image(it->first,{x,y,s.w*factor,s.h*factor},c);x+=s.advance*factor;
    }
}
void UiCanvas::paragraph(float x,float y,std::string_view value,float width,float size,UiColor color){
    std::istringstream words{std::string{value}};std::string word,line;
    const auto max=static_cast<std::size_t>(width/(size*.56F));
    while(words>>word){
        if(line.size()+word.size()+1>max && !line.empty()){text(x,y,line,size,color);line.clear();y+=size*1.45F;}
        if(!line.empty())line+=' ';line+=word;
    }
    if(!line.empty())text(x,y,line,size,color);
}
void UiCanvas::panel(UiRect r){rect(r,{.025F,.07F,.08F,.95F});rect({r.x,r.y,r.w,2},ui_cyan);rect({r.x,r.y+r.h-1,r.w,1},{.14F,.32F,.33F,1});}
void UiCanvas::ornament(UiRect r){const float edge=std::min(r.w*.5F,r.h*96/126);
    image("frame_left",{r.x,r.y,edge,r.h});image("frame_middle",{r.x+edge,r.y,r.w-2*edge,r.h});image("frame_right",{r.x+r.w-edge,r.y,edge,r.h});}
void UiCanvas::background(std::string_view title,std::string_view subtitle){
    rect({-left_/scale_,-top_/scale_,width_/scale_,height_/scale_},{.015F,.025F,.03F,1});
    image("background",{0,0,1280,720},{.10F,.15F,.15F,1});
    image("art",{520,80,740,555},{.35F,.45F,.42F,.16F});
    image("logo",{64,34,200,94});text(68,145,title,48,ui_ink,true);text(70,210,subtitle,20,ui_muted);
    ornament({58,649,1164,50});
    text(150,670,"GLOOM  /  FACTORY",16,ui_cyan);text(800,670,"Tab  ·  Flechas  ·  Intro  ·  Esc",16,ui_muted);
}
bool UiCanvas::control(std::string_view id,UiRect r,bool enabled){
    if(!enabled)return false;
    controls_.emplace_back(id);
    if(focus_.empty())focus_=id;
    const bool hover=mouse_x_>=r.x && mouse_y_>=r.y && mouse_x_<r.x+r.w && mouse_y_<r.y+r.h;
    if(hover && click_)focus_=id;
    return (hover && click_) || (focus_==id && confirm_);
}
bool UiCanvas::button(std::string_view id,UiRect r,std::string_view label,bool enabled){
    const bool activated=control(id,r,enabled);const bool selected=focus_==id;
    const bool hover=mouse_x_>=r.x && mouse_y_>=r.y && mouse_x_<r.x+r.w && mouse_y_<r.y+r.h;
    rect(r,enabled?(selected || hover?UiColor{.07F,.25F,.27F,1}:UiColor{.035F,.12F,.14F,.95F}):UiColor{.07F,.08F,.09F,.8F});
    rect({r.x,r.y,selected?4.0F:1.0F,r.h},enabled?ui_cyan:ui_muted);
    text(r.x+20,r.y+(r.h-30)*.5F,label,22,enabled?ui_ink:ui_muted);
    return activated;
}
bool UiCanvas::field(std::string_view id,UiRect r,std::string& value,std::size_t limit){
    static_cast<void>(control(id,r,true));const bool active=focus_==id;bool changed=false;
    if(active){
        text_focused_=true;
        if(input_.select_all){changed=!value.empty();value.clear();}
        if(input_.backspace && !value.empty()){do{const auto c=static_cast<unsigned char>(value.back());value.pop_back();if((c&192)!=128)break;}while(!value.empty());changed=true;}
        for(std::size_t i=0;i<input_.text.size() && input_.text[i];){
            const auto ch=static_cast<unsigned char>(input_.text[i]);
            const std::size_t count=ch<128?1:ch>=194 && ch<=223?2:ch>=224 && ch<=239?3:ch>=240 && ch<=244?4:0;
            if(!count || i+count>input_.text.size()){++i;continue;}
            bool valid=true;for(std::size_t j=1;j<count;++j)valid &= (static_cast<unsigned char>(input_.text[i+j])&192)==128;
            if(valid && ch>=32 && ch!=127 && value.size()+count<=limit){value.append(input_.text.data()+i,count);changed=true;}
            i+=count;
        }
    }
    rect(r,{.015F,.035F,.045F,1});rect({r.x,r.y+r.h-2,r.w,2},active?ui_cyan:ui_muted);
    const auto max=static_cast<std::size_t>((r.w-30)/13);auto start=value.size()>max?value.size()-max:0;
    while(start<value.size() && (static_cast<unsigned char>(value[start])&192)==128)++start;
    auto shown=value.substr(start);
    text(r.x+12,r.y+8,shown+(active?"|":""),22);return changed;
}
} // namespace gloom::render
