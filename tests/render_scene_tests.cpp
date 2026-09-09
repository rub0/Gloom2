#include <gloom/render/scene.hpp>
#include <gloom/render/shadow_visibility.hpp>
#include <stdio.h>

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace {

void expect_near(const float actual, const float expected, const char* message) {
    if (std::abs(actual - expected) > 0.0001F) {
        throw std::runtime_error{message};
    }
}

} // namespace

int main() try {
    using namespace gloom::render;
    const float identity_clip[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    const BoundingSphere outside[6] = {
        {.center={-2,0,.5F},.radius=.1F}, {.center={2,0,.5F},.radius=.1F},
        {.center={0,-2,.5F},.radius=.1F}, {.center={0,2,.5F},.radius=.1F},
        {.center={0,0,-1},.radius=.1F}, {.center={0,0,2},.radius=.1F}};
    for (gloom::uint32 i = 0; i < 6; ++i) {
        if (shadow_visible(outside[i], identity_clip)) {
            fprintf(stderr, "Shadow culling retained outside plane %u\n", i);
            return 1;
        }
    }
    const float stretched_clip[16] = {-4,0,0,0, 0,.2F,0,0, 0,0,1,0, 2,0,.5F,1};
    if (!shadow_visible({.center={0,0,.5F},.radius=.1F}, identity_clip) ||
        !shadow_visible({.center={1.1F,0,.5F},.radius=.1F}, identity_clip) ||
        !shadow_visible({.radius=.3F}, stretched_clip) || shadow_visible({.radius=.1F}, stretched_clip)) {
        fprintf(stderr, "Shadow culling lost an intersecting or mirrored/scaled sphere\n");
        return 1;
    }
    const auto rotate = [](Quaternion q, Vec3 v) {
        const Vec3 t{2*(q.y*v.z-q.z*v.y), 2*(q.z*v.x-q.x*v.z), 2*(q.x*v.y-q.y*v.x)};
        return Vec3{v.x+q.w*t.x+q.y*t.z-q.z*t.y, v.y+q.w*t.y+q.z*t.x-q.x*t.z,
                    v.z+q.w*t.z+q.x*t.y-q.y*t.x};
    };
    // Camera-relative placement and mesh orientation must agree through a full
    // yaw turn and steep pitch, including quaternion trace-negative branches.
    for (const float yaw : {0.0F, 1.57F, 3.14F, 4.71F}) {
        for (const float pitch : {-1.35F, 0.0F, 1.35F}) {
            const Vec3 forward{std::sin(yaw)*std::cos(pitch), std::sin(pitch), std::cos(yaw)*std::cos(pitch)};
            const Camera camera{.position = {3, 2, -7}, .target = {3+forward.x, 2+forward.y, -7+forward.z}};
            const auto prop = camera_relative_transform(camera, {0.22F, -0.23F, 0.58F}, {0.08F, 0.32F, 0.20F});
            const auto axis = rotate(prop.rotation, {0, 0, 1});
            expect_near(axis.x, forward.x, "View model yaw did not follow camera");
            expect_near(axis.y, forward.y, "View model pitch did not follow camera");
            expect_near(axis.z, forward.z, "View model forward axis changed");
            const auto offset = rotate(prop.rotation, {0.22F, -0.23F, 0.58F});
            expect_near(prop.position.x-camera.position.x, offset.x, "View model position/orientation disagreed");
            expect_near(prop.position.y-camera.position.y, offset.y, "View model vertical offset changed");
            expect_near(prop.position.z-camera.position.z, offset.z, "View model depth offset changed");
        }
    }
    // A sloped normal must remain perpendicular to a stretched/mirrored tangent.
    const Transform stretched{.rotation = {0, 0.70710678F, 0, 0.70710678F}, .scale = {-8, 0.2F, 3}};
    const auto normal = normal_transform(stretched);
    const auto n = rotate(normal.rotation, {normal.scale.x, normal.scale.y, 0});
    const auto tangent = rotate(stretched.rotation, {stretched.scale.x, -stretched.scale.y, 0});
    expect_near(n.x*tangent.x+n.y*tangent.y+n.z*tangent.z, 0, "Nonuniform normal transform broke perpendicularity");
    const gloom::render::Transform previous{
        .position = {0.0F, 2.0F, 4.0F},
        .rotation = {0.0F, 0.0F, 0.0F, 1.0F},
        .scale = {1.0F, 2.0F, 3.0F},
    };
    const gloom::render::Transform current{
        .position = {10.0F, 6.0F, 0.0F},
        .rotation = {0.0F, 0.0F, 0.0F, -1.0F},
        .scale = {3.0F, 4.0F, 5.0F},
    };

    const auto halfway = gloom::render::interpolate(previous, current, 0.5F);
    expect_near(halfway.position.x, 5.0F, "Position interpolation failed");
    expect_near(halfway.position.y, 4.0F, "Position interpolation failed");
    expect_near(halfway.scale.z, 4.0F, "Scale interpolation failed");
    expect_near(halfway.rotation.w, 1.0F, "Quaternion shortest-path interpolation failed");

    const auto before = gloom::render::interpolate(previous, current, -1.0F);
    const auto after = gloom::render::interpolate(previous, current, 2.0F);
    expect_near(before.position.x, previous.position.x, "Interpolation did not clamp below zero");
    expect_near(after.position.x, current.position.x, "Interpolation did not clamp above one");

    std::cout << "Gloom render-scene tests completed successfully.\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << "Render-scene test failure: " << error.what() << '\n';
    return 1;
}
