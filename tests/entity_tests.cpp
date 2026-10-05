#include <gloom/core/entity.hpp>
#include <gloom/gameplay/component_replication.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _DEBUG
#include <crtdbg.h>
#endif
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

using namespace gloom;
using namespace gloom::core;
namespace {
void require(bool condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "Entity contract: %s\n", message);
        exit(1);
    }
}
struct Transform {
    static constexpr ComponentType component_id = ComponentType::transform;
    float x{0}, y{0}, z{0};
};
struct Health {
    static constexpr ComponentType component_id = ComponentType::health;
    float current{100};
};
struct Lifetime {
    static constexpr ComponentType component_id = ComponentType::score;
    uint32* destroyed;
    explicit Lifetime(uint32& count) noexcept : destroyed{&count} {}
    ~Lifetime() {
        ++*destroyed;
    }
    Lifetime(const Lifetime&) = delete;
    Lifetime& operator=(const Lifetime&) = delete;
};
struct alignas(64) Aligned {
    static constexpr ComponentType component_id = ComponentType::damage_volume;
    uint64 value{17};
};
void no_allocations() {
    allocation_profile_window(false);
    uint64 calls = 0, bytes = 0;
    for (uint32 phase = 0; phase < static_cast<uint32>(AllocationPhase::count); ++phase) {
        const AllocationStats stats = allocation_profile_read(static_cast<AllocationPhase>(phase));
        calls += stats.calls;
        bytes += stats.bytes;
    }
    require(calls == 0 && bytes == 0, "Warm entity lifecycle allocated");
    printf("Entity allocation window: own_new=%llu bytes=%llu\n", calls, bytes);
}
}
int main(int argc, const char* const* argv) {
#ifdef _DEBUG
    if (argc == 2) {
        _set_error_mode(_OUT_TO_STDERR);
        _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
        _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
        EntityRegistry r;
        const EntityId e = r.create();
        r.emplace<Transform>(e);
        if (strcmp(argv[1], "duplicate") == 0)
            r.emplace<Transform>(e);
        else if (strcmp(argv[1], "stale") == 0) {
            require(r.destroy(e), "Fixture destroy");
            r.emplace<Health>(e);
        }
        return 0;
    }
    char executable[MAX_PATH];
    require(GetModuleFileNameA(nullptr, executable, MAX_PATH) > 0, "Locate fixture");
    const char* modes[]{"duplicate", "stale"};
    for (const char* mode : modes) {
        char command[MAX_PATH + 32];
        snprintf(command, sizeof(command), "\"%s\" %s", executable, mode);
        STARTUPINFOA startup{.cb = sizeof(startup)};
        PROCESS_INFORMATION process{};
        require(CreateProcessA(nullptr, command, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process), "Start fixture");
        require(WaitForSingleObject(process.hProcess, 5000) == WAIT_OBJECT_0, "Assert hung");
        DWORD result = 0;
        GetExitCodeProcess(process.hProcess, &result);
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        require(result == 3, "Programmer precondition accepted");
    }
#else
    static_cast<void>(argc);
    static_cast<void>(argv);
#endif
    uint32 destroyed = 0;
    {
        EntityRegistry r;
        const EntityId lava = r.create(), hound = r.create();
        Transform& transform = r.emplace<Transform>(lava, Transform{.x = 1, .y = -.5F, .z = 7.4F});
        r.emplace<Health>(hound, Health{.current = 120});
        r.emplace<Lifetime>(lava, destroyed);
        Aligned* aligned = &r.emplace<Aligned>(lava);
        require(reinterpret_cast<size_t>(aligned) % 64 == 0, "Component alignment lost");
        transform.y = -.6F;
        for (uint32 i = 0; i < 2048; ++i) {
            const EntityId e = r.create();
            r.emplace<Transform>(e);
        }
        require(
            r.size() == 2050 && r.get<Transform>(lava) == &transform && transform.y == -.6F && r.get<Aligned>(lava) == aligned, "Growth invalidated pointers");
        require(!r.get<Health>(lava) && r.component_count<Transform>() == 2049, "Ownership/count changed");
        require(r.remove<Lifetime>(lava) && !r.remove<Lifetime>(lava) && destroyed == 1, "Removal destruction not once");
        r.emplace<Lifetime>(lava, destroyed);
        require(r.destroy(lava) && !r.destroy(lava) && !r.get<Transform>(lava) && !r.remove<Health>(lava) && destroyed == 2, "Destroy/stale contract");
        const EntityId replacement = r.create();
        require(replacement.index == lava.index && replacement.generation != lava.generation && !r.alive(lava), "Generation reuse revived old handle");
        r.emplace<Lifetime>(replacement, destroyed);
        const EntityRegistry& read = r;
        require(read.get<Health>(hound)->current == 120, "Const lookup");
        r.clear();
        r.clear();
        require(destroyed == 3 && !r.size() && !r.alive(hound) && !r.component_count<Transform>(), "Clear destruction/generations");
        const EntityId after = r.create();
        require(after.valid() && !r.alive(hound) && !r.alive(replacement), "Clear revived handle");
        r.emplace<Lifetime>(after, destroyed);
    }
    require(destroyed == 4, "Registry teardown destruction not once");

    using namespace gameplay;
    {
        EntityRegistry r;
        EntityId ids[1024];
        for (uint32 i = 0; i < 8; ++i)
            ids[i] = compose_slice_character(r, {.network_entity = i + 1, .spawn_x = static_cast<float>(i)});
        const void* pointers[12]{r.get<TransformComponent>(ids[0]), r.get<CharacterPhysicsComponent>(ids[0]), r.get<HealthComponent>(ids[0]),
            r.get<ShieldComponent>(ids[0]), r.get<CharacterMovementComponent>(ids[0]), r.get<WeaponComponent>(ids[0]), r.get<AbilityComponent>(ids[0]),
            r.get<CharacterLoadoutComponent>(ids[0]), r.get<AuthorityComponent>(ids[0]), r.get<ReplicationComponent>(ids[0]),
            r.get<CharacterPresentationComponent>(ids[0]), r.get<ScoreComponent>(ids[0])};
        for (uint32 i = 8; i < 1024; ++i)
            ids[i] = compose_slice_character(r, {.network_entity = i + 1});
        const void* grown[12]{r.get<TransformComponent>(ids[0]), r.get<CharacterPhysicsComponent>(ids[0]), r.get<HealthComponent>(ids[0]),
            r.get<ShieldComponent>(ids[0]), r.get<CharacterMovementComponent>(ids[0]), r.get<WeaponComponent>(ids[0]), r.get<AbilityComponent>(ids[0]),
            r.get<CharacterLoadoutComponent>(ids[0]), r.get<AuthorityComponent>(ids[0]), r.get<ReplicationComponent>(ids[0]),
            r.get<CharacterPresentationComponent>(ids[0]), r.get<ScoreComponent>(ids[0])};
        for (uint32 i = 0; i < 12; ++i)
            require(pointers[i] == grown[i], "Combatant cached pointer moved");
        r.get<TransformComponent>(ids[0])->velocity_x = 3;
        const network::WorldSnapshot captured = capture_component_snapshot(r, {ids, 1024}, 42, 9);
        require(captured.entities.size() == 1024 && captured.entities[0].velocity_x == 3, "Replication after growth changed simulation data");
        r.get<AuthorityComponent>(ids[0])->mode = ComponentAuthority::interpolated_remote;
        r.get<TransformComponent>(ids[0])->position_x = 99;
        const ComponentSnapshotApplyResult applied = apply_component_snapshot(r, {ids, 1024}, captured);
        require(applied.applied == 1 && applied.ignored_authoritative == 1023 && r.get<TransformComponent>(ids[0])->position_x == 0,
            "Growth broke authority/replication");
    }
    const uint32 capacities[]{8, 64, 1024};
    for (uint32 capacity : capacities) {
        EntityRegistry r;
        r.reserve(capacity);
        EntityId ids[1024];
        for (uint32 i = 0; i < capacity; ++i)
            ids[i] = compose_slice_character(r, {.network_entity = i + 1});
        r.clear();
        allocation_profile_window(false);
        allocation_profile_reset();
        allocation_profile_window(true);
        for (uint32 cycle = 0; cycle < 1000; ++cycle) {
            for (uint32 i = 0; i < capacity; ++i) {
                ids[i] = compose_slice_character(r, {.network_entity = i + 1});
                require(r.get<AuthorityComponent>(ids[i])->network_entity == i + 1, "Reuse changed composition");
            }
            if (cycle % 2 == 0)
                r.clear();
            else
                for (uint32 i = 0; i < capacity; ++i)
                    require(r.destroy(ids[i]), "Lifecycle destroy");
            require(!r.alive(ids[0]) && !r.get<TransformComponent>(ids[0]), "Lifecycle retained stale component");
        }
        no_allocations();
        printf("Entity reuse: count=%u cycles=1000\n", capacity);
    }
    EntityRegistry cold;
    const EntityId cold_entity = cold.create();
    allocation_profile_window(false);
    allocation_profile_reset();
    allocation_profile_window(true);
    cold.emplace<Transform>(cold_entity);
    allocation_profile_window(false);
    require(allocation_profile_read(AllocationPhase::entities).calls > 0, "Cold page positive control missed allocations");
    puts("Entity generations, stable composition, replication, lifetime and reuse passed.");
}
