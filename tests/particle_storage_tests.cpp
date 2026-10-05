#include <gloom/render/particles.hpp>
#include <gloom/gameplay/combat_effects.hpp>
#include <gloom/core/allocation_profile.hpp>
#include <math.h>
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
void require(bool condition, const char* message) {
    if (!condition) {
        fprintf(stderr, "Particle contract: %s\n", message);
        exit(1);
    }
}
void allocation_window(bool active) {
    allocation_profile_window(false);
    if (active) {
        allocation_profile_reset();
        allocation_profile_window(true);
    } else {
        uint64 calls = 0, bytes = 0;
        for (uint32 phase = 0; phase < static_cast<uint32>(AllocationPhase::count); ++phase) {
            const AllocationStats stats = allocation_profile_read(static_cast<AllocationPhase>(phase));
            calls += stats.calls;
            bytes += stats.bytes;
        }
        printf("Particle allocation window: own_new=%llu bytes=%llu\n", calls, bytes);
        require(calls == 0 && bytes == 0, "Warm update/render/effects allocated");
    }
}
int main(int argc, const char* const* argv) {
    using namespace render;
#ifdef _DEBUG
    if (argc == 2) {
        _set_error_mode(_OUT_TO_STDERR);
        _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
        _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
        const ParticleRecipe recipe{.name = "fixture", .rate = 1};
        ParticleSystem p{{&recipe, 1}};
        if (strcmp(argv[1], "timestep") == 0)
            p.advance(NAN);
        else if (strcmp(argv[1], "identity") == 0) {
            p.emitter(1, 10, "fixture", {});
            p.emitter(1, 11, "fixture", {});
        } else if (strcmp(argv[1], "overlap") == 0) {
            Array<RenderInstance> output;
            output.push_back({});
            p.render(output, {}, {output.data(), output.size()});
        }
        return 0;
    }
    char executable[MAX_PATH];
    require(GetModuleFileNameA(nullptr, executable, MAX_PATH) > 0, "Could not locate assertion fixture");
    const char* modes[]{"timestep", "identity", "overlap"};
    for (const char* mode : modes) {
        char command[MAX_PATH + 32];
        snprintf(command, sizeof(command), "\"%s\" %s", executable, mode);
        STARTUPINFOA startup{.cb = sizeof(startup)};
        PROCESS_INFORMATION process{};
        require(CreateProcessA(nullptr, command, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process),
            "Could not start assertion fixture");
        require(WaitForSingleObject(process.hProcess, 5000) == WAIT_OBJECT_0, "Programmer misuse hung instead of asserting");
        DWORD result = 0;
        GetExitCodeProcess(process.hProcess, &result);
        CloseHandle(process.hThread);
        CloseHandle(process.hProcess);
        require(result == 3, "Programmer precondition was not enforced");
    }
#else
    static_cast<void>(argc);
    static_cast<void>(argv);
#endif
    const ParticleRecipe recipes[2]{
        {.name = "one", .burst = 1}, {.name = "many", .burst = 512, .life = 10, .speed = 2, .trail_seconds = .1F, .distortion = .01F}};
    ParticleSystem particles{recipes};
    RenderInstance material{.mesh = {.value = 77}, .material = {.value = 88}, .color = {.red = 2}};
    const Camera camera{.position = {0, 0, 5}, .target = {}};
    Array<RenderInstance> result;
    result.reserve(2049);
    result.push_back({.mesh = {.value = 99}});
    particles.render(result, camera, {&material, 1});
    require(result.size() == 1 && result[0].mesh.value == 99, "Empty render lost scene prefix");
    particles.burst(1, "one", {}, {0, 0, 1}, 123, true);
    particles.render(result, camera, {&material, 1});
    require(result.size() == 2 && result[1].color.alpha == 0 && result[1].view_model && !result[1].casts_shadow && result[1].soft_distance == 0,
        "Single particle birth/material policy changed");
    particles.advance(.025);
    result.resize(1);
    particles.render(result, camera, {&material, 1});
    require(result[1].color.alpha > .9F, "Soft birth did not ramp");
    particles.cancel(1);
    particles.clear();
    for (uint32 burst = 0; burst < 4; ++burst)
        particles.burst(10, "many", {}, {0, 0, 1}, 123 + burst);
    particles.burst(20, "one", {}, {0, 1, 0}, 99);
    require(particles.particles().size() == 2048 && particles.metrics().dropped == 1, "Default capacity/saturation changed");
    RenderInstance* storage = result.data();
    allocation_window(true);
    for (uint32 frame = 0; frame < 1000; ++frame) {
        particles.advance(.001);
        result.resize(1);
        particles.render(result, camera, {&material, 1});
    }
    allocation_window(false);
    require(result.data() == storage && result.size() == 2049 && result[0].mesh.value == 99, "Persistent output lost prefix or capacity");
    require(result[1].mesh.value == 77 && result[1].material.value == 88 && result[1].particle && result[1].lod_count == 1 && !result[1].casts_shadow &&
                result[1].soft_distance == .15F && result[1].distortion == .01F && isfinite(result[1].transform.rotation.w),
        "Parallel-camera trail or material fields changed");
    require(particles.translate(10, {1, 2, 3}) && particles.particles()[0].position.x == 1, "Translation lost owner");
    require(!particles.translate(20, {}), "Dropped particle has an owner");
    particles.cancel(10);
    require(particles.particles().empty(), "Cancel did not remove particles");
    particles.burst(30, "one", {}, {0, 1, 0}, 1);
    result.resize(1);
    particles.render(result, camera, {});
    require(result.size() == 1, "Missing material emitted an instance");
    particles.advance(1);
    require(particles.particles().empty() && particles.metrics().expired == 1, "Natural expiry changed");
    particles.burst(30, "one", {}, {0, 1, 0}, 1);
    Array<RenderInstance> growing;
    allocation_profile_window(false);
    allocation_profile_reset();
    allocation_profile_window(true);
    particles.render(growing, camera, {&material, 1});
    allocation_profile_window(false);
    require(allocation_profile_read(AllocationPhase::particles).calls > 0 && growing.size() == 1, "Growth positive control did not detect allocation");

    Array<ParticleRecipe> loaded;
    require(load_particle_recipes(GLOOM_TEST_ASSETS "/effects/recipes.json", loaded) == nullptr, "Real recipes did not load");
    ParticleSystem events_system{{loaded.data(), loaded.size()}};
    gameplay::CombatEffects events;
    gameplay::CombatantView view{.entity = 2, .character = gameplay::SliceCharacter::archangel};
    gameplay::CharacterAnimationFrame frame;
    events.observe(events_system, view, frame, {}, 1, true);
    frame.cut = false;
    result.reserve(1 + events_system.capacity());
    allocation_window(true);
    for (uint64 tick = 2; tick < 1002; ++tick) {
        view.shot_sequence = static_cast<uint32>(tick);
        view.shot_tick = tick;
        events.observe(events_system, view, frame, {}, tick, true);
        events_system.advance(.001);
        result.resize(1);
        events_system.render(result, camera, {&material, 1});
    }
    allocation_window(false);
    require(events.events() == 1000, "Confirmed shots lost or duplicated events");
    events.reset(events_system);
    require(events_system.particles().empty() && events_system.time() == 0, "Reset retained effects");

    ParticleSystem emitter_system{{loaded.data(), loaded.size()}};
    for (uint64 id = 1; id <= 65; ++id)
        emitter_system.emitter(id, id, "shadow_smoke", {});
    require(emitter_system.metrics().dropped == 1, "Emitter capacity changed");
    emitter_system.advance(1);
    require(emitter_system.metrics().spawned == 64 * 28, "Emitter refresh/rate changed");
    emitter_system.advance(2);
    require(emitter_system.particles().empty(), "Missing refresh leaked emitters");
    emitter_system.emitter(1, 100, "shadow_smoke", {});
    emitter_system.cancel(100);
    emitter_system.emitter(1, 101, "shadow_smoke", {});
    emitter_system.advance(.1);
    for (const Particle& particle : emitter_system.particles())
        require(particle.owner == 101, "Recycled attachment retained prior entity lifetime");

    char directory[MAX_PATH], path[MAX_PATH];
    require(GetTempPathA(MAX_PATH, directory) && GetTempFileNameA(directory, "gpf", 0, path), "Could not initialize JSON fixture");
    const char* valid = "{\"name\":\"x\",\"material\":0,\"burst\":1,\"rate\":1,\"life\":1,\"speed\":0,\"spread\":0,\"gravity\":0,"
                        "\"size_start\":1,\"size_end\":1,\"trail_seconds\":0,\"distortion\":0,\"color_start\":[1,1,1,1],\"color_end\":[1,1,1,0]}";
    char json[4096];
    const char* malformed[]{"{", "{\"version\":2,\"recipes\":[]}", "{\"version\":1,\"recipes\":[]}", "{\"version\":1,\"recipes\":[{\"name\":\"x\"}]}"};
    const size_t saved = loaded.size();
    for (const char* text : malformed) {
        FILE* file = nullptr;
        require(fopen_s(&file, path, "wb") == 0, "Could not write JSON fixture");
        fputs(text, file);
        fclose(file);
        require(load_particle_recipes(path, loaded) != nullptr && loaded.size() == saved, "Malformed recipe replaced valid data");
    }
    snprintf(json, sizeof(json), "{\"version\":1,\"recipes\":[%s,%s]}", valid, valid);
    FILE* file = nullptr;
    require(fopen_s(&file, path, "wb") == 0, "Could not write duplicate fixture");
    fputs(json, file);
    fclose(file);
    require(load_particle_recipes(path, loaded) != nullptr && loaded.size() == saved, "Duplicate recipe accepted");
    snprintf(json, sizeof(json), "{\"version\":1,\"recipes\":[%s]}", valid);
    require(fopen_s(&file, path, "wb") == 0, "Could not write valid fixture");
    fputs(json, file);
    fclose(file);
    require(load_particle_recipes(path, loaded) == nullptr && loaded.size() == 1, "Valid minimal recipe rejected");
    char* life = strstr(json, "\"life\":1");
    require(life != nullptr, "Invalid life fixture");
    life[7] = '0';
    require(fopen_s(&file, path, "wb") == 0, "Could not write invalid life fixture");
    fputs(json, file);
    fclose(file);
    require(load_particle_recipes(path, loaded) != nullptr && loaded.size() == 1, "Invalid life in external JSON accepted");
    require(fopen_s(&file, path, "wb") == 0, "Could not write recipe count fixture");
    fputs("{\"version\":1,\"recipes\":[", file);
    for (uint32 index = 0; index < 65; ++index) {
        if (index)
            fputc(',', file);
        fputs(valid, file);
    }
    fputs("]}", file);
    fclose(file);
    require(load_particle_recipes(path, loaded) != nullptr && loaded.size() == 1, "More than 64 recipes accepted");
    require(remove(path) == 0, "Could not remove JSON fixture");
    require(load_particle_recipes(path, loaded) != nullptr && loaded.size() == 1, "Missing file erased valid recipes");
    puts("Particle storage, lifetime, saturation, effects and external-data checks passed.");
    return 0;
}
