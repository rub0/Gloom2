#include <gloom/gameplay/character_animation.hpp>
#include <cmath>
#include <stdexcept>

namespace gloom::gameplay {
namespace {
using render::Vec3;
Vec3 add(Vec3 a,Vec3 b) {return {a.x+b.x,a.y+b.y,a.z+b.z};}
Vec3 sub(Vec3 a,Vec3 b) {return {a.x-b.x,a.y-b.y,a.z-b.z};}
Vec3 mul(Vec3 a,float s) {return {a.x*s,a.y*s,a.z*s};}
float dot(Vec3 a,Vec3 b) {return a.x*b.x+a.y*b.y+a.z*b.z;}
float length(Vec3 a) {return std::sqrt(dot(a,a));}
Vec3 unit(Vec3 a) {return mul(a,1/std::max(length(a),1e-6F));}
Vec3 point(const assets::RigMatrix& m) {return {m[12],m[13],m[14]};}
std::uint32_t node(const assets::ImportedScene& rig,std::string_view name) {
    for (std::uint32_t i=0;i<rig.nodes.size();++i) if (rig.nodes[i].name==name) return i;
    return assets::no_asset_index;
}
render::Quaternion product(render::Quaternion a,render::Quaternion b) {
    return render::attach_transform({.rotation=a},{.rotation=b}).rotation;
}
render::Quaternion inverse(render::Quaternion q) {return {-q.x,-q.y,-q.z,q.w};}
render::Quaternion between(Vec3 a,Vec3 b) {
    a=unit(a);b=unit(b);float w=1+dot(a,b);
    Vec3 v{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};
    if (w<1e-5F) {v=unit(std::abs(a.x)<.9F?Vec3{0,a.z,-a.y}:Vec3{-a.z,0,a.x});w=0;}
    const float n=std::sqrt(dot(v,v)+w*w);return {v.x/n,v.y/n,v.z/n,w/n};
}
void rotate_local(assets::LocalPose& pose,std::uint32_t i,Vec3 axis,float angle) {
    if (i>=pose.size()) return;
    const float s=std::sin(angle*.5F);pose[i].rotation=product(pose[i].rotation,{axis.x*s,axis.y*s,axis.z*s,std::cos(angle*.5F)});
}
void aim_bone(const assets::ImportedScene& rig,assets::LocalPose& pose,std::uint32_t joint,std::uint32_t child,Vec3 target) {
    const auto worlds=assets::pose_worlds(rig,pose);
    const auto delta=between(sub(point(worlds[child]),point(worlds[joint])),sub(target,point(worlds[joint])));
    auto parent=assets::no_asset_index;
    for (std::uint32_t i=0;i<rig.nodes.size();++i) for (auto c:rig.nodes[i].children) if (c==joint) parent=i;
    const auto p=parent==assets::no_asset_index?render::Quaternion{}:assets::rig_transform(worlds[parent]).rotation;
    pose[joint].rotation=product(product(product(inverse(p),delta),p),pose[joint].rotation);
}
void solve_arm(const assets::ImportedScene& rig,assets::LocalPose& pose,bool right,Vec3 target) {
    const std::string side=right?"Bip001 R ":"Bip001 L ";
    const auto upper=node(rig,side+"UpperArm"),fore=node(rig,side+"Forearm"),hand=node(rig,side+"Hand");
    if (upper>=pose.size() || fore>=pose.size() || hand>=pose.size()) return;
    auto w=assets::pose_worlds(rig,pose);const auto shoulder=point(w[upper]);
    const float a=length(sub(point(w[fore]),shoulder)),b=length(sub(point(w[hand]),point(w[fore])));
    const auto direction=unit(sub(target,shoulder));
    const float distance=std::clamp(length(sub(target,shoulder)),std::abs(a-b)+.001F,a+b-.001F);
    const float along=(a*a-b*b+distance*distance)/(2*distance);
    const auto pole=Vec3{right?1.0F:-1.0F,-.8F,-.3F};
    const auto bend=unit(sub(pole,mul(direction,dot(pole,direction))));
    const auto elbow=add(shoulder,add(mul(direction,along),mul(bend,std::sqrt(std::max(0.0F,a*a-along*along)))));
    aim_bone(rig,pose,upper,fore,elbow);aim_bone(rig,pose,fore,hand,target);
}
void fps_shoulder(const assets::ImportedScene& rig,assets::LocalPose& pose,bool right) {
    const auto joint=node(rig,right?"Bip001 R UpperArm":"Bip001 L UpperArm");
    if (joint>=pose.size()) return;
    const auto worlds=assets::pose_worlds(rig,pose);
    for (std::uint32_t p=0;p<rig.nodes.size();++p) for (const auto c:rig.nodes[p].children) if(c==joint) {
        // Authored FPS shoulder position: the cut of the recovered arm remains
        // below the camera, while the unchanged limb lengths reach the gun.
        const render::Transform target{.position={right?.36F:0.F,right?.94F:1.F,right?.10F:.20F}};
        pose[joint].position=point(assets::rig_multiply(assets::rig_inverse(worlds[p]),assets::rig_matrix(target)));
        return;
    }
}
}

void CharacterAnimator::reset() noexcept {*this=CharacterAnimator{};}

CharacterAnimationFrame CharacterAnimator::update(const assets::ImportedScene& rig,const CombatantView& view,
    double seconds,bool discontinuity,bool bind,bool first_person) {
    if (!std::isfinite(seconds) || seconds<0) throw std::invalid_argument{"Invalid animation timestep"};
    const float dt=static_cast<float>(std::min(seconds,.25));
    const bool cut=discontinuity || !initialized_ || previous_.entity!=view.entity || previous_.character!=view.character ||
        previous_.alive!=view.alive || previous_.deaths!=view.deaths ||
        std::hypot(view.position_x-previous_.position_x,view.position_y-previous_.position_y,view.position_z-previous_.position_z)>3;
    if (cut) {
        if(!view.alive) death_pose_=initialized_ && previous_.alive && previous_.entity==view.entity &&
            previous_.character==view.character && last_pose_.size()==rig.nodes.size()?last_pose_:assets::rest_pose(rig);
        time_=0;air_time_=0;death_time_=0;equip_time_=0;movement_=0;landing_=0;recoil_=0;damage_=0;
    }
    if (initialized_ && !cut) {
        if (!previous_.grounded && view.grounded) landing_=1;
        if (view.life<previous_.life || view.shield<previous_.shield) damage_=1;
        if (view.shot_sequence!=previous_.shot_sequence) recoil_=1;
    }
    time_+=dt;equip_time_+=dt;
    air_time_=view.grounded?0:air_time_+dt;
    death_time_=view.alive?0:death_time_+dt;
    const float speed=std::hypot(view.velocity_x,view.velocity_z);
    movement_+=(std::clamp(speed/3.0F,0.0F,1.0F)-movement_)*(1-std::exp(-12*dt));
    landing_=std::max(0.0F,landing_-dt*5);recoil_=std::max(0.0F,recoil_-dt*7);damage_=std::max(0.0F,damage_-dt*5);
    CharacterAnimationFrame frame;frame.cut=cut;
    const auto rest=assets::rest_pose(rig);
    frame.local=rest;
    if (!bind) {
        const float forward=view.velocity_x*view.facing_x+view.velocity_z*view.facing_z;
        const float strafe=view.velocity_x*view.facing_z-view.velocity_z*view.facing_x;
        const double cycle=time_*(.8+std::min(speed/5.0F,1.0F));
        auto idle=assets::sample_animation(rig,"idle",time_);
        auto walk=assets::sample_animation(rig,"forward",forward<0?1000-cycle:cycle);
        auto sideways=assets::sample_animation(rig,"strafe_right",strafe<0?1000-cycle:cycle);
        auto locomotion=assets::blend_poses(walk,sideways,std::abs(strafe)/(std::abs(forward)+std::abs(strafe)+.001F));
        frame.local=assets::blend_poses(idle,locomotion,movement_);
        if (!view.grounded && !rig.animations.empty()) {
            auto jump=assets::sample_animation(rig,"jump",std::min(air_time_,.3),false);
            frame.local=assets::blend_poses(frame.local,jump,.75F);
        }
        // Visual root displacement never drives authoritative locomotion. Keep
        // the recovered pelvis origin (especially Shadow's explicit rebase).
        const auto root=node(rig,"Bip001");if (root<rest.size()) frame.local[root].position=rest[root].position;
        const auto spine=node(rig,"Bip001 Spine"),neck=node(rig,"Bip001 Neck");
        const float breath=.018F*std::sin(static_cast<float>(time_)*2.7F);
        rotate_local(frame.local,spine,{0,0,1},breath-damage_*.10F-.25F*view.aim_pitch);
        rotate_local(frame.local,neck,{0,0,1},-.2F*view.aim_pitch);
        if (view.character==SliceCharacter::shadow) {
            rotate_local(frame.local,node(rig,"Bip001 Tail"),{0,1,0},(.09F+movement_*.18F)*std::sin(static_cast<float>(time_)*4));
            rotate_local(frame.local,node(rig,"Bip001 Tail1"),{0,0,1},(.10F+movement_*.20F)*std::sin(static_cast<float>(time_)*4-1.2F));
            rotate_local(frame.local,spine,{0,1,0},movement_*.07F*std::sin(static_cast<float>(time_)*6));
        }
        frame.local[0].position.y+=.008F*std::sin(static_cast<float>(time_)*3)-landing_*.055F;
        const float pitch=first_person?0:view.aim_pitch;
        const float recoil=recoil_*.055F;
        const float equip=static_cast<float>(std::max(0.0,.25-equip_time_))*.6F;
        Vec3 right{.23F,1.18F-equip,.30F-recoil},left{.20F,1.18F-equip,.49F-recoil};
        auto aim=[&](Vec3 v){const float y=v.y-1.35F,z=v.z;return Vec3{v.x,1.35F+y*std::cos(pitch)+z*std::sin(pitch),z*std::cos(pitch)-y*std::sin(pitch)};};
        right=aim(right);left=aim(left);
        if (view.alive) {
            if(first_person){fps_shoulder(rig,frame.local,true);fps_shoulder(rig,frame.local,false);}
            solve_arm(rig,frame.local,true,right);solve_arm(rig,frame.local,false,left);
        }
        for (std::uint32_t i=0;i<rig.nodes.size();++i) if (rig.nodes[i].name.find("Finger")!=std::string::npos)
            rotate_local(frame.local,i,{0,0,1},rig.nodes[i].name.find(" R ")!=std::string::npos?.6F:-.6F);
        if (!view.alive) {
            // Retain the last live pose through the fall and hold it afterwards;
            // a corpse must not keep looping idle, breathing or arm IK.
            frame.local=death_pose_;
            const float f=std::clamp(static_cast<float>(death_time_)/.65F,0.0F,1.0F);
            // Collapse in normalized presentation space, never rescale a live rig.
            const float angle=1.35F*f;
            const render::Transform collapse{.position={0,.75F*(1-std::cos(angle))-.55F*f,-.75F*std::sin(angle)},
                .rotation={std::sin(angle*.5F),0,0,std::cos(angle*.5F)}};
            frame.local[0]=assets::rig_transform(assets::rig_multiply(assets::rig_matrix(collapse),assets::rig_matrix(frame.local[0])));
        }
    }
    frame.worlds=assets::pose_worlds(rig,frame.local);
    auto anchor=[&](std::string_view name,Vec3 fallback){const auto i=node(rig,name);return i<frame.worlds.size()?point(frame.worlds[i]):fallback;};
    const auto hand=anchor("Bip001 R Hand",{.23F,1.18F,.35F});
    frame.left_grip=anchor("Bip001 L Hand",{-.1F,1.18F,.58F});
    const float pitch=first_person?0:view.aim_pitch;
    frame.weapon={.position=hand,.rotation={-std::sin(pitch*.5F),0,0,std::cos(pitch*.5F)},.scale={.43F,.43F,.43F}};
    // Original Soul Reaper grip is an authored mesh-space socket.
    frame.weapon=render::attach_transform(frame.weapon,{.position={-.13F,-.25F,-.20F}});
    // Recovered barrel tip is x=.21, y=.35, z=.93 in the normalized mesh.
    // Start beyond that surface so depth testing cannot bury the flash in the gun.
    frame.muzzle=render::attach_transform(frame.weapon,{.position={.21F,.35F,1.02F}}).position;
    frame.head=anchor("Bip001 Head",{0,1.7F,0});frame.tail=anchor("Bip001 Tail1",{0,.3F,0});
    frame.left_wing=anchor("Bip001 L Clavicle",{-.3F,1.5F,0});frame.right_wing=anchor("Bip001 R Clavicle",{.3F,1.5F,0});
    last_pose_=frame.local;previous_=view;initialized_=true;return frame;
}
} // namespace gloom::gameplay
