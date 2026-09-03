#include <gloom/render/particles.hpp>
#include <simdjson.h>
#include <algorithm>
#include <cmath>
#include <set>
#include <stdexcept>

namespace gloom::render {
namespace {
Vec3 add(Vec3 a,Vec3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
Vec3 mul(Vec3 a,float s){return {a.x*s,a.y*s,a.z*s};}
Vec3 mix(Vec3 a,Vec3 b,float t){return {a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t};}
std::uint64_t hash(std::uint64_t x){x^=x>>30;x*=0xbf58476d1ce4e5b9ULL;x^=x>>27;x*=0x94d049bb133111ebULL;return x^(x>>31);}
float random(std::uint64_t& seed){seed=hash(seed+0x9e3779b97f4a7c15ULL);return static_cast<float>(seed>>40)/16777216.0F;}
Vec3 position(const Particle& p,const ParticleRecipe& r,double time){const float age=static_cast<float>(std::max(0.0,time-p.birth));return add(p.position,add(mul(p.velocity,age),{0,.5F*r.gravity*age*age,0}));}
bool finite(Vec3 p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);}
}

bool valid_particle_recipe(const ParticleRecipe& r) noexcept {
    for (float f:{r.rate,r.life,r.speed,r.spread,r.gravity,r.size_start,r.size_end,r.trail_seconds,r.distortion,
        r.color_start.red,r.color_start.green,r.color_start.blue,r.color_start.alpha,r.color_end.red,r.color_end.green,r.color_end.blue,r.color_end.alpha})
        if (!std::isfinite(f)) return false;
    return !r.name.empty() && r.name.size()<=64 && r.material_slot<64 && r.burst<=512 && r.rate>=0 && r.rate<=500 && r.life>0 && r.life<=10 &&
        r.speed>=0 && r.speed<=100 && r.spread>=0 && r.spread<=20 && r.size_start>0 && r.size_start<=10 &&
        r.size_end>0 && r.size_end<=10 && r.trail_seconds>=0 && r.trail_seconds<=r.life && r.distortion>=0 && r.distortion<=.02F &&
        std::abs(r.gravity)<=100 && std::min({r.color_start.red,r.color_start.green,r.color_start.blue,r.color_end.red,r.color_end.green,r.color_end.blue})>=0 &&
        r.color_start.alpha>=0 && r.color_start.alpha<=1 && r.color_end.alpha>=0 && r.color_end.alpha<=1;
}

std::vector<ParticleRecipe> load_particle_recipes(const std::filesystem::path& path) {
    simdjson::dom::parser parser;auto doc=parser.load(path.string());
    if (doc.error()) throw std::runtime_error{"Cannot load particle recipes"};
    if (std::uint64_t(doc["version"])!=1) throw std::runtime_error{"Unsupported particle recipe version"};
    std::vector<ParticleRecipe> recipes;
    for (auto value:doc["recipes"].get_array()) {
        ParticleRecipe r;r.name=std::string{std::string_view(value["name"])};
        const auto material=std::uint64_t(value["material"]),burst=std::uint64_t(value["burst"]);
        if(material>=64 || burst>512) throw std::runtime_error{"Particle recipe index/count exceeds capacity"};
        r.material_slot=static_cast<std::uint32_t>(material);r.burst=static_cast<std::uint32_t>(burst);
        r.rate=static_cast<float>(double(value["rate"]));r.life=static_cast<float>(double(value["life"]));
        r.speed=static_cast<float>(double(value["speed"]));r.spread=static_cast<float>(double(value["spread"]));
        r.gravity=static_cast<float>(double(value["gravity"]));r.size_start=static_cast<float>(double(value["size_start"]));
        r.size_end=static_cast<float>(double(value["size_end"]));r.trail_seconds=static_cast<float>(double(value["trail_seconds"]));
        r.distortion=static_cast<float>(double(value["distortion"]));
        auto color=[&](const char* name){auto a=value[name].get_array();if (a.size()!=4) throw std::runtime_error{"Invalid particle color"};
            std::array<float,4> v{};std::size_t i=0;for (auto x:a) v[i++]=static_cast<float>(double(x));return Color{v[0],v[1],v[2],v[3]};};
        r.color_start=color("color_start");r.color_end=color("color_end");
        if (!valid_particle_recipe(r) || recipes.size()>=64) throw std::runtime_error{"Invalid particle recipe"};
        recipes.push_back(r);
    }
    return recipes;
}

ParticleSystem::ParticleSystem(std::vector<ParticleRecipe> recipes,std::size_t capacity):recipes_{std::move(recipes)},capacity_{capacity} {
    if (!capacity || capacity>16384 || recipes_.empty() || recipes_.size()>64) throw std::invalid_argument{"Invalid particle capacity"};
    std::set<std::string> names;
    for (const auto& r:recipes_) if (!valid_particle_recipe(r)||!names.insert(r.name).second) throw std::invalid_argument{"Invalid particle recipe"};
    particles_.reserve(capacity);emitters_.reserve(64);
}
std::uint32_t ParticleSystem::find(std::string_view name) const {
    for (std::uint32_t i=0;i<recipes_.size();++i) if (recipes_[i].name==name) return i;
    throw std::invalid_argument{"Unknown particle recipe"};
}
void ParticleSystem::emitter(std::uint64_t id,std::uint64_t owner,std::string_view name,Vec3 p,Vec3 d) {
    if (!id || !finite(p)||!finite(d)) throw std::invalid_argument{"Invalid emitter"};
    const auto recipe=find(name);if (recipes_[recipe].rate<=0) throw std::invalid_argument{"Continuous emitter requires rate"};
    for (auto& e:emitters_) if (e.id==id) {
        if (e.owner!=owner || e.recipe!=recipe) throw std::invalid_argument{"Emitter identity changed without cancel"};
        e.position=p;e.direction=d;e.refreshed=true;return;
    }
    if (emitters_.size()>=64) {++metrics_.dropped;return;}
    emitters_.push_back({id,owner,0,recipe,p,p,d,time_+1.0/recipes_[recipe].rate,true});
}
void ParticleSystem::spawn(std::uint64_t owner,std::uint32_t recipe,Vec3 p,Vec3 d,std::uint64_t seed,double birth,bool fps) {
    if (particles_.size()>=capacity_) {++metrics_.dropped;return;}
    const auto& r=recipes_[recipe];const float n=std::hypot(d.x,d.y,d.z);if (n>1e-6F) d=mul(d,1/n);
    Vec3 noise{random(seed)*2-1,random(seed)*2-1,random(seed)*2-1};
    particles_.push_back({owner,recipe,birth,p,add(mul(d,r.speed),mul(noise,r.spread)),random(seed)*6.2831853F,fps});++metrics_.spawned;
}
void ParticleSystem::burst(std::uint64_t owner,std::string_view name,Vec3 p,Vec3 d,std::uint64_t seed,bool fps) {
    if (!finite(p)||!finite(d)) throw std::invalid_argument{"Invalid burst"};
    const auto recipe=find(name);
    for (std::uint32_t i=0;i<recipes_[recipe].burst;++i) spawn(owner,recipe,p,d,hash(seed+i),time_,fps);
}
void ParticleSystem::advance(double seconds) {
    if (!std::isfinite(seconds)||seconds<0||seconds>10) throw std::invalid_argument{"Invalid particle timestep"};
    const double end=time_+seconds;
    const auto expire=[&](double time) {
        const auto before=particles_.size();
        std::erase_if(particles_,[&](const auto& p){return time-p.birth>=recipes_[p.recipe].life-1e-9;});
        metrics_.expired+=before-particles_.size();
    };
    std::erase_if(emitters_,[](const auto& e){return !e.refreshed;});
    // Process births chronologically (ID breaks ties), including capacity decisions.
    // Expiring at the end of the frame before all births changes saturation at 30/144 Hz.
    for (;;) {
        auto next=std::min_element(emitters_.begin(),emitters_.end(),[](const auto& a,const auto& b){
            const auto ta=std::llround(a.next*1e9),tb=std::llround(b.next*1e9);
            return ta==tb?a.id<b.id:ta<tb;
        });
        if (next==emitters_.end() || next->next>end+1e-9) break;
        auto& e=*next;
        const auto& r=recipes_[e.recipe];
        expire(e.next);
        const float t=seconds>0?static_cast<float>(std::clamp((e.next-time_)/seconds,0.0,1.0)):1;
        spawn(e.owner,e.recipe,mix(e.previous,e.position,t),e.direction,hash(e.id+e.serial),e.next,false);
        ++e.serial;e.next+=1.0/r.rate;
    }
    expire(end);
    for (auto& e:emitters_) {
        e.previous=e.position;e.refreshed=false;
    }
    time_=end;
}
void ParticleSystem::cancel(std::uint64_t owner) {std::erase_if(emitters_,[&](const auto& e){return e.owner==owner;});std::erase_if(particles_,[&](const auto& p){return p.owner==owner;});}
void ParticleSystem::clear() noexcept {particles_.clear();emitters_.clear();time_=0;}

std::vector<RenderInstance> ParticleSystem::render(const Camera& camera,std::span<const RenderInstance> materials) const {
    std::vector<RenderInstance> result;result.reserve(particles_.size());
    const auto camera_basis=camera_relative_transform(camera,{}, {1,1,1});
    for (const auto& p:particles_) {
        const auto& r=recipes_[p.recipe];if (r.material_slot>=materials.size()) continue;
        const float age=static_cast<float>(time_-p.birth),t=std::clamp(age/r.life,0.0F,1.0F);
        auto instance=materials[r.material_slot];const float size=r.size_start+(r.size_end-r.size_start)*t;
        instance.transform={.position=position(p,r,time_),.rotation=camera_basis.rotation,.scale={size,size,size}};
        if (r.trail_seconds>0 && age>0) {
            const auto start=position(p,r,time_-std::min(age,r.trail_seconds));const auto end=instance.transform.position;
            const float distance=std::hypot(end.x-start.x,end.y-start.y,end.z-start.z);
            if (distance>1e-4F) {
                // Ribbon quads have their long Y axis along motion and face camera.
                Camera ribbon{.position=mul(add(start,end),.5F),.target=camera.position,.up={end.x-start.x,end.y-start.y,end.z-start.z}};
                const auto cross=Vec3{(camera.position.y-ribbon.position.y)*ribbon.up.z-(camera.position.z-ribbon.position.z)*ribbon.up.y,
                    (camera.position.z-ribbon.position.z)*ribbon.up.x-(camera.position.x-ribbon.position.x)*ribbon.up.z,
                    (camera.position.x-ribbon.position.x)*ribbon.up.y-(camera.position.y-ribbon.position.y)*ribbon.up.x};
                if (std::hypot(cross.x,cross.y,cross.z)>1e-5F) instance.transform=camera_relative_transform(ribbon,{}, {size,distance+size,size});
            }
        }
        instance.color={r.color_start.red+(r.color_end.red-r.color_start.red)*t,r.color_start.green+(r.color_end.green-r.color_start.green)*t,
            r.color_start.blue+(r.color_end.blue-r.color_start.blue)*t,r.color_start.alpha+(r.color_end.alpha-r.color_start.alpha)*t};
        instance.color.alpha*=std::min(1.0F,age/.025F); // soft birth, no single-frame bloom pop
        instance.view_model=p.view_model;instance.casts_shadow=false;instance.lod_count=1;
        instance.particle=true;instance.distortion=r.distortion;instance.soft_distance=p.view_model?0:.15F;
        result.push_back(std::move(instance));
    }
    return result;
}
} // namespace gloom::render
