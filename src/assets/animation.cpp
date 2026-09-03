#include <gloom/assets/animation.hpp>
#include <set>
#include <stdexcept>

namespace gloom::assets {
namespace {
render::Quaternion slerp(render::Quaternion a, render::Quaternion b, float t) {
    float d=a.x*b.x+a.y*b.y+a.z*b.z+a.w*b.w;
    if (d<0) {b={-b.x,-b.y,-b.z,-b.w};d=-d;}
    if (d>.9995F) return render::interpolate({.rotation=a},{.rotation=b},t).rotation;
    const float angle=std::acos(std::clamp(d,-1.0F,1.0F));
    const float u=std::sin((1-t)*angle)/std::sin(angle),v=std::sin(t*angle)/std::sin(angle);
    return {a.x*u+b.x*v,a.y*u+b.y*v,a.z*u+b.z*v,a.w*u+b.w*v};
}
}

RigMatrix rig_matrix(const render::Transform& t) {
    const auto [x,y,z,w]=t.rotation;
    return {(1-2*(y*y+z*z))*t.scale.x,2*(x*y+z*w)*t.scale.x,2*(x*z-y*w)*t.scale.x,0,
        2*(x*y-z*w)*t.scale.y,(1-2*(x*x+z*z))*t.scale.y,2*(y*z+x*w)*t.scale.y,0,
        2*(x*z+y*w)*t.scale.z,2*(y*z-x*w)*t.scale.z,(1-2*(x*x+y*y))*t.scale.z,0,
        t.position.x,t.position.y,t.position.z,1};
}

RigMatrix rig_inverse(const RigMatrix& m) {
    const float d=m[0]*(m[5]*m[10]-m[9]*m[6])-m[4]*(m[1]*m[10]-m[9]*m[2])+m[8]*(m[1]*m[6]-m[5]*m[2]);
    if (!std::isfinite(d) || std::abs(d)<1e-12F) throw std::invalid_argument{"Singular rig matrix"};
    RigMatrix r{(m[5]*m[10]-m[9]*m[6])/d,(m[9]*m[2]-m[1]*m[10])/d,(m[1]*m[6]-m[5]*m[2])/d,0,
        (m[8]*m[6]-m[4]*m[10])/d,(m[0]*m[10]-m[8]*m[2])/d,(m[4]*m[2]-m[0]*m[6])/d,0,
        (m[4]*m[9]-m[8]*m[5])/d,(m[8]*m[1]-m[0]*m[9])/d,(m[0]*m[5]-m[4]*m[1])/d,0,0,0,0,1};
    for (std::size_t i=0;i<3;++i) r[12+i]=-r[i]*m[12]-r[4+i]*m[13]-r[8+i]*m[14];
    return r;
}

render::Transform rig_transform(const RigMatrix& m) {
    render::Vec3 scale{std::hypot(m[0],m[1],m[2]),std::hypot(m[4],m[5],m[6]),std::hypot(m[8],m[9],m[10])};
    if (scale.x<1e-8F || scale.y<1e-8F || scale.z<1e-8F) throw std::invalid_argument{"Zero rig scale"};
    if (m[0]*(m[5]*m[10]-m[9]*m[6])-m[4]*(m[1]*m[10]-m[9]*m[2])+m[8]*(m[1]*m[6]-m[5]*m[2])<0) scale.x=-scale.x;
    const render::Camera c{.position={},.target={m[8]/scale.z,m[9]/scale.z,m[10]/scale.z},.up={m[4]/scale.y,m[5]/scale.y,m[6]/scale.y}};
    const auto rotation=render::camera_relative_transform(c,{}, {1,1,1}).rotation;
    return {.position={m[12],m[13],m[14]},.rotation=rotation,.scale=scale};
}

bool valid_animations(const ImportedScene& scene) {
    if (scene.animations.size()>256) return false;
    if (scene.animations.empty()) return true;
    if (!bind_node_transforms(scene)) return false;
    std::set<std::string> names;
    std::size_t keys=0;
    for (const auto& clip:scene.animations) {
        if (clip.name.empty() || !names.insert(clip.name).second || !std::isfinite(clip.duration) ||
            clip.duration<=0 || clip.duration>3600 || clip.channels.empty() || clip.channels.size()>scene.nodes.size()*3) return false;
        std::set<std::pair<std::uint32_t,AnimationPath>> targets;
        for (const auto& channel:clip.channels) {
            if (channel.node>=scene.nodes.size() || channel.path>AnimationPath::scale ||
                channel.interpolation>AnimationInterpolation::step || !targets.emplace(channel.node,channel.path).second ||
                channel.times.empty() || channel.times.size()!=channel.values.size()) return false;
            keys+=channel.times.size();if (keys>1'000'000) return false;
            try {
                const auto& matrix=scene.nodes[channel.node].local_transform;
                const auto rebuilt=rig_matrix(rig_transform(matrix));
                for (std::size_t i=0;i<16;++i) if (std::abs(matrix[i]-rebuilt[i])>1e-3F) return false;
            } catch (...) {return false;}
            float previous=-1;
            for (std::size_t i=0;i<channel.times.size();++i) {
                const float t=channel.times[i];const auto& v=channel.values[i];
                if (!std::isfinite(t) || t<0 || t<=previous || t>clip.duration ||
                    !std::ranges::all_of(v,[](float x){return std::isfinite(x);})) return false;
                previous=t;
                if (channel.path==AnimationPath::rotation && std::abs(v[0]*v[0]+v[1]*v[1]+v[2]*v[2]+v[3]*v[3]-1)>1e-3F) return false;
                if (channel.path==AnimationPath::scale && (v[0]<1e-4F || v[1]<1e-4F || v[2]<1e-4F)) return false;
            }
        }
    }
    return true;
}

LocalPose rest_pose(const ImportedScene& scene) {
    LocalPose result;result.reserve(scene.nodes.size());
    for (const auto& node:scene.nodes) result.push_back(rig_transform(node.local_transform));
    return result;
}

LocalPose sample_animation(const ImportedScene& scene, std::string_view name, double seconds, bool loop) {
    if (!std::isfinite(seconds)) throw std::invalid_argument{"Invalid animation time"};
    auto pose=rest_pose(scene);
    const auto clip=std::ranges::find(scene.animations,name,&AnimationClip::name);
    if (clip==scene.animations.end()) return pose;
    if (clip->duration<=0) throw std::invalid_argument{"Invalid clip duration"};
    const float t=static_cast<float>(loop ? std::fmod(std::max(0.0,seconds),clip->duration) : std::clamp(seconds,0.0,static_cast<double>(clip->duration)));
    for (const auto& c:clip->channels) {
        const auto hi=std::upper_bound(c.times.begin(),c.times.end(),t);
        const auto a=hi==c.times.begin()?0:static_cast<std::size_t>(hi-c.times.begin()-1);
        const auto b=std::min(a+1,c.times.size()-1);
        const float w=a==b || c.interpolation==AnimationInterpolation::step?0:std::clamp((t-c.times[a])/(c.times[b]-c.times[a]),0.0F,1.0F);
        const auto& x=c.values[a];const auto& y=c.values[b];
        const render::Vec3 v{x[0]+(y[0]-x[0])*w,x[1]+(y[1]-x[1])*w,x[2]+(y[2]-x[2])*w};
        if (c.path==AnimationPath::translation) pose[c.node].position=v;
        else if (c.path==AnimationPath::scale) pose[c.node].scale=v;
        else pose[c.node].rotation=slerp({x[0],x[1],x[2],x[3]},{y[0],y[1],y[2],y[3]},w);
    }
    return pose;
}

LocalPose blend_poses(const LocalPose& a, const LocalPose& b, float weight) {
    if (a.size()!=b.size() || !std::isfinite(weight)) throw std::invalid_argument{"Invalid pose blend"};
    const float t=std::clamp(weight,0.0F,1.0F);LocalPose result(a.size());
    for (std::size_t i=0;i<a.size();++i) {
        result[i]=render::interpolate(a[i],b[i],t);
        result[i].rotation=slerp(a[i].rotation,b[i].rotation,t);
    }
    return result;
}

std::vector<RigMatrix> pose_worlds(const ImportedScene& scene, const LocalPose& pose) {
    if (pose.size()!=scene.nodes.size()) throw std::invalid_argument{"Pose/rig size mismatch"};
    std::vector<std::uint32_t> parents(pose.size(),no_asset_index),queue;
    for (std::uint32_t i=0;i<pose.size();++i) for (auto child:scene.nodes[i].children) {
        if (child>=pose.size() || parents[child]!=no_asset_index) throw std::invalid_argument{"Invalid pose hierarchy"};
        parents[child]=i;
    }
    for (std::uint32_t i=0;i<pose.size();++i) if (parents[i]==no_asset_index) queue.push_back(i);
    std::vector<RigMatrix> result(pose.size());
    for (std::size_t h=0;h<queue.size();++h) {
        const auto i=queue[h];const auto local=rig_matrix(pose[i]);
        result[i]=parents[i]==no_asset_index?local:rig_multiply(result[parents[i]],local);
        queue.insert(queue.end(),scene.nodes[i].children.begin(),scene.nodes[i].children.end());
    }
    if (queue.size()!=pose.size()) throw std::invalid_argument{"Cyclic pose hierarchy"};
    return result;
}

std::shared_ptr<render::SkinPose> skin_pose(const ImportedScene& scene, std::uint32_t mesh_node, const std::vector<RigMatrix>& worlds) {
    if (worlds.size()!=scene.nodes.size() || mesh_node>=scene.nodes.size() || scene.nodes[mesh_node].skin>=scene.skins.size())
        throw std::invalid_argument{"Invalid skin pose binding"};
    auto result=std::make_shared<render::SkinPose>();
    const auto inverse=rig_inverse(worlds[mesh_node]);const auto& skin=scene.skins[scene.nodes[mesh_node].skin];
    for (std::size_t i=0;i<skin.joints.size();++i) {
        const auto m=rig_multiply(inverse,rig_multiply(worlds[skin.joints[i]],skin.inverse_bind_matrices[i]));
        result->matrices.push_back(m);const auto inv=rig_inverse(m);RigMatrix n{};
        for (std::size_t c=0;c<4;++c) for (std::size_t r=0;r<4;++r) n[c*4+r]=inv[r*4+c];
        result->normal_matrices.push_back(n);
    }
    return result;
}

render::Vec3 skinned_position(const ImportedVertex& v,const render::SkinPose& pose) {
    render::Vec3 p{};
    for (std::size_t i=0;i<8;++i) if (v.weights[i]>0) {
        const auto& m=pose.matrices.at(v.joints[i]);const float w=v.weights[i];
        p.x+=w*(m[0]*v.position[0]+m[4]*v.position[1]+m[8]*v.position[2]+m[12]);
        p.y+=w*(m[1]*v.position[0]+m[5]*v.position[1]+m[9]*v.position[2]+m[13]);
        p.z+=w*(m[2]*v.position[0]+m[6]*v.position[1]+m[10]*v.position[2]+m[14]);
    }
    return p;
}

render::BoundingSphere skinned_bounds(const ImportedPrimitive& primitive,const render::SkinPose& pose) {
    render::Vec3 lo{1e30F,1e30F,1e30F},hi{-1e30F,-1e30F,-1e30F};
    for (const auto& v:primitive.vertices) {
        const auto p=skinned_position(v,pose);
        lo={std::min(lo.x,p.x),std::min(lo.y,p.y),std::min(lo.z,p.z)};
        hi={std::max(hi.x,p.x),std::max(hi.y,p.y),std::max(hi.z,p.z)};
    }
    return {.center={(lo.x+hi.x)*.5F,(lo.y+hi.y)*.5F,(lo.z+hi.z)*.5F},.radius=std::hypot(hi.x-lo.x,hi.y-lo.y,hi.z-lo.z)*.5F+1e-3F};
}
} // namespace gloom::assets
