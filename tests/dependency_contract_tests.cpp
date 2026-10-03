#include <gloom/core/types.hpp>
#include <Jolt/Jolt.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/ObjectLayerPairFilterTable.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayerInterfaceTable.h>
#include <Jolt/Physics/Collision/BroadPhase/ObjectVsBroadPhaseLayerFilterTable.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>
#include <fastgltf/core.hpp>
#include <GameNetworkingSockets/steam/steamnetworkingsockets.h>
#include <GameNetworkingSockets/steam/steamnetworkingsockets_flat.h>
#define DILIGENT_C_INTERFACE 1
#include <Graphics/GraphicsEngine/interface/RenderDevice.h>
#include <stdio.h>
#include <stdlib.h>

namespace {
void require(bool value, const char* message) {
    if (!value) {
        fprintf(stderr, "%s\n", message);
        exit(1);
    }
}
}
int main() {
    SteamNetworkingErrMsg network_error{};
    require(GameNetworkingSockets_Init(nullptr, network_error), network_error);
    require(SteamAPI_SteamNetworkingSockets_v009() != nullptr, "GNS flat interface unavailable");
    GameNetworkingSockets_Kill();
    fastgltf::Parser parser;
    parser.setUserPointer(nullptr);
    require(sizeof(IRenderDevice) > 0, "Diligent C interface unavailable");
    JPH::RegisterDefaultAllocator();
    JPH::Factory::sInstance = new JPH::Factory{};
    JPH::RegisterTypes();
    {
        JPH::BroadPhaseLayerInterfaceTable broad{2, 2};
        broad.MapObjectToBroadPhaseLayer(0, JPH::BroadPhaseLayer{0});
        broad.MapObjectToBroadPhaseLayer(1, JPH::BroadPhaseLayer{1});
        JPH::ObjectLayerPairFilterTable pairs{2};
        pairs.EnableCollision(0, 1);
        pairs.EnableCollision(1, 1);
        JPH::ObjectVsBroadPhaseLayerFilterTable filter{broad, 2, pairs, 2};
        for (gloom::uint32 first = 0; first < 2; ++first)
            for (gloom::uint32 second = 0; second < 2; ++second) {
                require(pairs.ShouldCollide(static_cast<JPH::ObjectLayer>(first), static_cast<JPH::ObjectLayer>(second)) == (first != 0 || second != 0),
                    "Built-in object filter differs from Gloom");
                require(filter.ShouldCollide(static_cast<JPH::ObjectLayer>(first), JPH::BroadPhaseLayer{static_cast<JPH::BroadPhaseLayer::Type>(second)}) ==
                            (first != 0 || second != 0),
                    "Built-in broadphase filter differs from Gloom");
            }
        JPH::PhysicsSystem physics;
        physics.Init(16, 0, 64, 64, broad, filter, pairs);
        physics.SetGravity(JPH::Vec3::sZero());
        JPH::TempAllocatorImpl temporary{4 * 1024 * 1024};
        JPH::JobSystemThreadPool jobs{1024, 16, 2};
        JPH::BodyInterface& bodies = physics.GetBodyInterface();
        JPH::BodyCreationSettings sensor{new JPH::BoxShape{JPH::Vec3{1, 1, 1}}, JPH::RVec3::sZero(), JPH::Quat::sIdentity(), JPH::EMotionType::Static, 0};
        sensor.mIsSensor = true;
        JPH::BodyCreationSettings moving{new JPH::SphereShape{.5F}, JPH::RVec3::sZero(), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, 1};
        moving.mAllowSleeping = false;
        const JPH::BodyID sensor_id = bodies.CreateAndAddBody(sensor, JPH::EActivation::DontActivate);
        const JPH::BodyID moving_id = bodies.CreateAndAddBody(moving, JPH::EActivation::Activate);
        gloom::uint32 entered = 0, stayed = 0, exited = 0;
        bool previous = false;
        for (gloom::uint32 step = 0; step < 4; ++step) {
            if (step == 3)
                bodies.SetPosition(moving_id, JPH::RVec3{5, 0, 0}, JPH::EActivation::Activate);
            require(physics.Update(1.0F / 60.0F, 1, &temporary, &jobs) == JPH::EPhysicsUpdateError::None, "Jolt update failed");
            const bool contact = physics.WereBodiesInContact(sensor_id, moving_id);
            if (contact && !previous)
                ++entered;
            else if (contact)
                ++stayed;
            else if (previous)
                ++exited;
            previous = contact;
        }
        require(entered == 1 && stayed == 2 && exited == 1, "Simple polling transitions changed");
        bodies.RemoveBody(moving_id);
        bodies.DestroyBody(moving_id);
        bodies.RemoveBody(sensor_id);
        bodies.DestroyBody(sensor_id);
    }
    JPH::UnregisterTypes();
    delete JPH::Factory::sInstance;
    JPH::Factory::sInstance = nullptr;
    puts("Dependency contract: Jolt filters and simple two-worker poll; Diligent C header; fastgltf construction; GNS initialization and flat interface.");
    puts("This does not prove equivalence to Gloom listeners for sleeping, transient, character or destroyed-body contacts.");
    return 0;
}
