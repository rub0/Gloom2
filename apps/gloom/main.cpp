#include <gloom/platform/movement_actions.hpp>
#include <gloom/assets/asset_loader.hpp>
#include "desktop.hpp"
#include "audio_review.hpp"
#include "game_ui.hpp"
#include <gloom/assets/residency_coordinator.hpp>
#include <gloom/assets/scene_catalog.hpp>
#include <gloom/assets/rig.hpp>
#include <gloom/backends/diligent_renderer.hpp>
#include <gloom/backends/gns_transport.hpp>
#include <gloom/backends/jolt_world.hpp>
#include <gloom/backends/winhttp_match_service.hpp>
#include <gloom/backends/keycloak_identity.hpp>
#include <gloom/backends/sdl_window.hpp>
#include <gloom/core/engine.hpp>
#include <gloom/core/job_system.hpp>
#include <gloom/gameplay/first_person.hpp>
#include <gloom/gameplay/first_person_presentation.hpp>
#include <gloom/gameplay/character_presentation.hpp>
#include <gloom/gameplay/character_animation.hpp>
#include <gloom/gameplay/combat_effects.hpp>
#include <gloom/gameplay/match_discovery.hpp>
#include <gloom/gameplay/vertical_slice.hpp>
#include <gloom/gameplay/vertical_slice_network.hpp>
#include <gloom/network/combat.hpp>
#include <gloom/network/movement_replication.hpp>
#include <gloom/network/network_simulator.hpp>
#include <gloom/network/presentation_smoother.hpp>
#include <gloom/network/protocol.hpp>
#include <gloom/render/visibility.hpp>
#include <gloom/render/lighting.hpp>
#include <gloom/render/particles.hpp>
#include "animation_review.hpp"

#include <algorithm>
#include <chrono>
#include <array>
#include <exception>
#include <cstdlib>
#include <filesystem>
#include <future>
#include <iostream>
#include <memory>
#include <optional>
#include <span>
#include <cmath>
#include <string>
#include <string_view>
#include <thread>
#include <vector>
#include "visual_review.hpp"
#include <gloom/gameplay/factory_scene.hpp>

namespace {

struct VisualBody {
    gloom::physics::BodyId body;
    gloom::render::RenderAssetId mesh{gloom::render::builtin_cube_mesh};
    gloom::render::Transform previous;
    gloom::render::Transform current;
    gloom::render::Color color;
};

constexpr std::array slice_roster{
    gloom::gameplay::SlicePlayerSelection{},
    gloom::gameplay::SlicePlayerSelection{.ability = gloom::gameplay::SliceAbility::guard},
    gloom::gameplay::SlicePlayerSelection{.ability = gloom::gameplay::SliceAbility::none},
    gloom::gameplay::SlicePlayerSelection{
        .character = gloom::gameplay::SliceCharacter::archangel,
        .ability = gloom::gameplay::SliceAbility::none},
    gloom::gameplay::SlicePlayerSelection{
        .character = gloom::gameplay::SliceCharacter::shadow,
        .ability = gloom::gameplay::SliceAbility::none},
};

[[nodiscard]] std::string_view selection_label(
    const gloom::gameplay::SlicePlayerSelection selection) noexcept {
    if(selection.weapon==gloom::gameplay::SliceWeapon::sniper)return "Hound / Sniper";
    if(selection.weapon==gloom::gameplay::SliceWeapon::shotgun)return "Hound / ShotGun";
    if(selection.weapon==gloom::gameplay::SliceWeapon::minigun)return "Hound / MiniGun";
    if(selection.weapon==gloom::gameplay::SliceWeapon::iron_hell_goat)return "Hound / IronHellGoat";
    if (selection.character == gloom::gameplay::SliceCharacter::berserker) {
        return "Hound (legacy Berserker slot) / Soul Reaper / No ability";
    }
    if (selection.character == gloom::gameplay::SliceCharacter::archangel) return "Archangel / Soul Reaper / No ability";
    if (selection.character == gloom::gameplay::SliceCharacter::shadow) return "Shadow / Soul Reaper / No ability";
    if (selection.ability == gloom::gameplay::SliceAbility::guard) {
        return "Hound / Soul Reaper / Guard";
    }
    if (selection.ability == gloom::gameplay::SliceAbility::none) {
        return "Hound / Soul Reaper / No ability";
    }
    return "Hound / Soul Reaper / Bite";
}

[[nodiscard]] std::uint64_t development_account_id(const std::string_view name) noexcept {
    std::uint64_t hash = 1469598103934665603ULL;
    for (const unsigned char character : name) {
        hash = (hash ^ character) * 1099511628211ULL;
    }
    return hash == 0 ? 1 : hash;
}

void print_lobby(const gloom::gameplay::SliceLobbyState& lobby) {
    const char* phase = lobby.phase == gloom::gameplay::SliceMatchPhase::waiting
                            ? "waiting"
                            : lobby.phase == gloom::gameplay::SliceMatchPhase::active
                                  ? "active"
                                  : "completed";
    std::cout << "Lobby " << phase << " (revision " << lobby.revision << ")";
    for (const auto& player : lobby.players) {
        std::cout << "; " << player.identity.display_name << " ["
                  << (player.connected ? "connected" : "disconnected") << ", "
                  << (player.ready ? "ready" : "not ready")
                  << ", "
                  << (player.selection.character == gloom::gameplay::SliceCharacter::berserker
                          ? "Hound (legacy slot)"
                          : player.selection.character == gloom::gameplay::SliceCharacter::archangel ? "Archangel"
                          : player.selection.character == gloom::gameplay::SliceCharacter::shadow ? "Shadow" : "Hound")
                  << " / Soul Reaper / "
                  << (player.selection.ability == gloom::gameplay::SliceAbility::bite
                          ? "Bite]"
                          : player.selection.ability == gloom::gameplay::SliceAbility::guard
                                ? "Guard]"
                                : "No ability]");
    }
    if (lobby.phase == gloom::gameplay::SliceMatchPhase::completed) {
        std::cout << "; winner entity " << lobby.winner_entity << " by abandonment";
    }
    std::cout << ".\n";
}

[[nodiscard]] std::string lobby_title(const gloom::gameplay::SliceLobbyState& lobby) {
    std::string title = lobby.phase == gloom::gameplay::SliceMatchPhase::active
                            ? "Gloom match"
                            : "Gloom lobby";
    for (const auto& player : lobby.players) {
        title += " | " + player.identity.display_name + ": " +
                 std::string{selection_label(player.selection)};
        title += player.ready ? " [ready]" : " [waiting]";
    }
    return title;
}

[[nodiscard]] gloom::render::Transform to_render_transform(const gloom::physics::Transform& transform,
                                                           const gloom::render::Vec3 scale) {
    return {
        .position = {transform.position.x, transform.position.y, transform.position.z},
        .rotation = {transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w},
        .scale = scale,
    };
}

} // namespace

int run_game(const int argument_count, const char* const* arguments, gloom::desktop::Session* desktop_session=nullptr) try {
    const bool factory_review = argument_count > 1 && std::string_view{arguments[1]} == "--factory-review";
    const bool pickup_review = argument_count > 1 && std::string_view{arguments[1]} == "--pickup-review";
    const bool lava_review=argument_count>1 && std::string_view{arguments[1]}=="--lava-review";
    const bool effects_review=argument_count>1 && std::string_view{arguments[1]}=="--effects-review";
    const bool short_review=lava_review || effects_review;
    const bool animation_review=short_review || (argument_count>1 && std::string_view{arguments[1]}=="--animation-review");
    const int animation_fps=short_review?30:animation_review && argument_count>=4?std::stoi(arguments[3]):60;
    const int animation_seconds=short_review?3:40;
    if (animation_review && (argument_count!=(short_review?3:4) || (animation_fps!=30 && animation_fps!=60 && animation_fps!=144)))
        throw std::invalid_argument{"Usage: gloom --animation-review OUTPUT_DIRECTORY 30|60|144"};
    const bool character_review = animation_review || (argument_count > 1 && std::string_view{arguments[1]} == "--character-review");
    const bool visual_review = pickup_review || character_review || factory_review || (argument_count > 1 && std::string_view{arguments[1]} == "--visual-review");
    const bool original_factory = pickup_review || character_review || factory_review || (!visual_review && !(argument_count > 1 && std::string_view{arguments[1]} == "--vertical-slice-smoke"));
    const bool original_characters = original_factory && !factory_review;
    const auto review_views = pickup_review ? std::span<const gloom::review::View>{gloom::review::pickup_views} : character_review ? std::span<const gloom::review::View>{gloom::review::character_views} : factory_review ? std::span<const gloom::review::View>{gloom::review::factory_views} : std::span<const gloom::review::View>{gloom::review::views};
    if (visual_review && !animation_review && argument_count != 3) throw std::invalid_argument{"Usage: gloom --visual-review OUTPUT_DIRECTORY"};
    const std::filesystem::path review_output = visual_review ? arguments[2] : "";
    if (visual_review) std::filesystem::create_directories(review_output);
    const bool list_matches = argument_count > 1 &&
                              std::string_view{arguments[1]} == "--vertical-slice-list";
    std::shared_ptr<gloom::gameplay::SliceMatchDirectory> match_directory=desktop_session?desktop_session->directory:nullptr;
    const auto* game_auth = std::getenv("GLOOM_GAME_AUTH");
    const bool game_tickets = game_auth && std::string_view{game_auth} == "tickets";
    if (game_auth && *game_auth && !game_tickets) throw std::invalid_argument{"Unknown GLOOM_GAME_AUTH mode"};
    std::function<std::expected<std::string, std::string>()> acquire_identity=desktop_session?desktop_session->identity:std::function<std::expected<std::string,std::string>()>{};
    const auto configure_match_directory = [&]() -> gloom::gameplay::SliceMatchDirectory* {
        if (match_directory) return match_directory.get();
        const char* url = std::getenv("GLOOM_MATCH_SERVICE_URL");
        if (url == nullptr || !*url) throw std::runtime_error{"GLOOM_MATCH_SERVICE_URL is required"};
        const auto environment = [](const char* name) -> std::string {
            const auto* value = std::getenv(name); return value ? value : "";
        };
        const auto provider = environment("GLOOM_MATCH_IDENTITY_PROVIDER");
        std::string token;
        if (provider == "keycloak") {
            auto sign_in = std::make_shared<gloom::backends::KeycloakSignIn>(gloom::backends::KeycloakClientSettings{
                environment("GLOOM_KEYCLOAK_ISSUER"), environment("GLOOM_KEYCLOAK_CLIENT_ID"), game_tickets});
            const auto challenge = sign_in->begin();
            if (!challenge) throw std::runtime_error{challenge.error()};
            std::cout << "Sign in at " << challenge->verification_uri << " with code " << challenge->user_code
                      << ". Waiting for approval (Ctrl+C cancels)." << std::endl;
            for (;;) {
                const auto approved = sign_in->poll();
                if (!approved) throw std::runtime_error{approved.error()};
                if (*approved) break;
                std::this_thread::sleep_for(std::chrono::milliseconds{100});
            }
            acquire_identity = [sign_in] { return sign_in->access_token(); };
            const auto acquired = acquire_identity();
            if (!acquired) throw std::runtime_error{acquired.error()};
            token = *acquired;
        } else if (provider.empty() || provider == "registry") {
            token = environment("GLOOM_MATCH_IDENTITY_TOKEN");
            if (token.empty()) throw std::runtime_error{"GLOOM_MATCH_IDENTITY_TOKEN is required for registry mode"};
            acquire_identity = [token] { return std::expected<std::string, std::string>{token}; };
        } else throw std::runtime_error{"Unknown matchmaking identity provider"};
        auto connector = gloom::backends::make_winhttp_match_reader_connector(acquire_identity);
        auto connected = gloom::gameplay::connect_match_directory(
            *connector, {.service_url = url, .bearer_token = token,
                         .wait = [](const std::uint64_t delay) {
                             std::this_thread::sleep_for(std::chrono::milliseconds{delay});
                         }});
        if (!connected) throw std::runtime_error{connected.error()};
        match_directory = std::move(*connected);
        return match_directory.get();
    };
    const auto now_ms = [] {
        return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
    };
    if (list_matches) {
        gloom::gameplay::SliceMatchSelection browser;
        if (auto result = browser.refresh(*configure_match_directory(), now_ms()); !result)
            throw std::runtime_error{result.error()};
        for (const auto& match : browser.matches())
            std::cout << match.match_id << " | " << match.display_name << " | "
                      << match.player_count << '/' << match.capacity << '\n';
        return 0;
    }
    const bool smoke_test = argument_count > 1 && std::string_view{arguments[1]} == "--smoke-test";
    const bool network_demo = argument_count > 1 &&
                              (std::string_view{arguments[1]} == "--network-demo" ||
                               std::string_view{arguments[1]} == "--network-scene-smoke");
    const bool network_scene_smoke = argument_count > 1 &&
                                     std::string_view{arguments[1]} == "--network-scene-smoke";
    const bool vulkan_sync_stress =
        argument_count > 1 && std::string_view{arguments[1]} == "--vulkan-sync-stress";
    const bool vertical_slice = argument_count > 1 &&
                                (visual_review || std::string_view{arguments[1]} == "--vertical-slice" ||
                                 std::string_view{arguments[1]} == "--vertical-slice-smoke" ||
                                 std::string_view{arguments[1]} == "--vertical-slice-host" ||
                                 std::string_view{arguments[1]} == "--vertical-slice-join" ||
                                 std::string_view{arguments[1]} == "--vertical-slice-browse");
    const bool vertical_slice_smoke = argument_count > 1 &&
                                      std::string_view{arguments[1]} == "--vertical-slice-smoke";
    const bool vertical_slice_host = argument_count > 1 &&
                                     std::string_view{arguments[1]} == "--vertical-slice-host";
    const bool vertical_slice_browse = argument_count > 1 &&
                                       std::string_view{arguments[1]} == "--vertical-slice-browse";
    // Device sign-in is a console preflight before creating the graphical window.
    if (vertical_slice_browse) static_cast<void>(configure_match_directory());
    const bool vertical_slice_join = argument_count > 1 &&
                                     (std::string_view{arguments[1]} == "--vertical-slice-join" ||
                                      vertical_slice_browse);
    if ((vertical_slice_host || (vertical_slice_join && !vertical_slice_browse)) && argument_count < 3) {
        throw std::invalid_argument{
            "--vertical-slice-host and --vertical-slice-join require an IP:port endpoint"};
    }
    std::string slice_endpoint =
        vertical_slice_host || (vertical_slice_join && !vertical_slice_browse)
            ? std::string{arguments[2]} : std::string{};
    std::string slice_match_id, slice_game_credential;
    std::future<std::expected<gloom::backends::GameJoinTicket, std::string>> pending_game_ticket;
    if (game_tickets && vertical_slice_host) throw std::invalid_argument{"Ticket hosting requires gloom_slice_server"};
    if (vertical_slice_join && !vertical_slice_browse) {
        auto* directory = slice_endpoint.starts_with("match:")
                              ? configure_match_directory() : nullptr;
        const auto target = gloom::gameplay::resolve_slice_join_target(
            slice_endpoint, directory, now_ms());
        if (!target) {
            throw std::invalid_argument{target.error()};
        }
        slice_endpoint = target->endpoint;
        slice_match_id = target->match_id;
    }
    const auto prepare_game_ticket = [&] {
        if (!game_tickets) return;
        if (slice_match_id.empty()) throw std::invalid_argument{"Ticket mode requires match:<id> or browse"};
        static_cast<void>(configure_match_directory());
        if (pending_game_ticket.valid()) return;
        pending_game_ticket = std::async(std::launch::async,
            [acquire = acquire_identity, url = std::string{std::getenv("GLOOM_MATCH_SERVICE_URL")}, id = slice_match_id]()
                -> std::expected<gloom::backends::GameJoinTicket, std::string> {
                try {
                    const auto identity = acquire();
                    if (!identity) return std::unexpected{identity.error()};
                    return gloom::backends::request_game_ticket(url, *identity, id);
                } catch (...) { return std::unexpected{"Game authorization request failed"}; }
            });
    };
    const std::string_view slice_player_name =
        vertical_slice_browse && argument_count > 2 ? std::string_view{arguments[2]} :
        vertical_slice_join && argument_count > 3 ? std::string_view{arguments[3]} : std::string_view{};
    const std::string_view slice_loadout =
        argument_count>2 && std::string_view{arguments[1]}=="--vertical-slice" ? std::string_view{arguments[2]} :
        vertical_slice_browse && argument_count > 3 ? std::string_view{arguments[3]} :
        vertical_slice_join && argument_count > 4 ? std::string_view{arguments[4]} : std::string_view{"hound-bite"};
    const bool interactive_slice_selection = vertical_slice_join &&
        (desktop_session || (vertical_slice_browse ? argument_count <= 3 : argument_count <= 4));
    const gloom::gameplay::SlicePlayerSelection slice_selection{
        .character = slice_loadout == "berserker-reaper"
                         ? gloom::gameplay::SliceCharacter::berserker
                         : slice_loadout == "archangel-reaper" ? gloom::gameplay::SliceCharacter::archangel
                         : slice_loadout == "shadow-reaper" ? gloom::gameplay::SliceCharacter::shadow
                         : gloom::gameplay::SliceCharacter::hound,
        .weapon = slice_loadout=="hound-sniper"?gloom::gameplay::SliceWeapon::sniper:
                  slice_loadout=="hound-shotgun"?gloom::gameplay::SliceWeapon::shotgun:
                  slice_loadout=="hound-minigun"?gloom::gameplay::SliceWeapon::minigun:
                  slice_loadout=="hound-iron-hell-goat"?gloom::gameplay::SliceWeapon::iron_hell_goat:gloom::gameplay::SliceWeapon::soul_reaper,
        .ability = (slice_loadout == "hound-reaper" || slice_loadout == "archangel-reaper" || slice_loadout == "shadow-reaper" ||
                    slice_loadout=="hound-sniper" || slice_loadout=="hound-shotgun" || slice_loadout=="hound-minigun" || slice_loadout=="hound-iron-hell-goat")
                       ? gloom::gameplay::SliceAbility::none
                       : slice_loadout == "berserker-reaper"
                             ? gloom::gameplay::SliceAbility::none
                       : slice_loadout == "hound-guard"
                             ? gloom::gameplay::SliceAbility::guard
                             : gloom::gameplay::SliceAbility::bite};
    if (vertical_slice_join && slice_loadout != "hound-bite" &&
        slice_loadout != "hound-reaper" && slice_loadout != "hound-guard" &&
        slice_loadout != "berserker-reaper" && slice_loadout != "archangel-reaper" && slice_loadout != "shadow-reaper" &&
        slice_loadout!="hound-sniper" && slice_loadout!="hound-shotgun" && slice_loadout!="hound-minigun" && slice_loadout!="hound-iron-hell-goat") {
        throw std::invalid_argument{
            "Slice loadout must be hound-bite, hound-guard, hound-reaper or "
            "berserker-reaper, archangel-reaper, shadow-reaper, hound-sniper, "
            "hound-shotgun, hound-minigun or hound-iron-hell-goat"};
    }
    const bool automated_graphics = smoke_test || network_scene_smoke ||
                                    vertical_slice_smoke || vulkan_sync_stress || visual_review;

    gloom::core::Engine engine;
    auto jobs = std::make_unique<gloom::core::JobSystem>(
        gloom::core::JobSystemSettings{.worker_threads = 2});
    auto* jobs_view = jobs.get();
    auto window = std::make_unique<gloom::backends::SdlWindow>(gloom::platform::WindowDesc{
        .title = vertical_slice
                     ? vertical_slice_host
                           ? "Gloom authoritative slice host"
                           : "Gloom - Factory - WASD, Space, fire LMB, Ability Q/RMB"
                     : network_demo
                     ? "Gloom Network Lab - blue: predicted local, orange: interpolated remote"
                     : "Gloom - SDL3 + Vulkan",
        .width = animation_review?640U:1280U,
        .height = animation_review?360U:720U,
        .resizable = true,
    });
    auto* window_view = window.get();
    auto renderer = std::make_unique<gloom::backends::DiligentRenderer>(
        *window_view,
        gloom::render::RendererSettings{
            .vertical_sync = !automated_graphics,
            // One cube plus tiny textures on the first smoke frame; defer the
            // quad. Derive the budget from the vertex layout (now eight weights).
            .upload_budget_bytes_per_frame = smoke_test ? 24U*sizeof(gloom::render::GpuVertex)+36U*sizeof(std::uint32_t)+48U : 16U * 1024U * 1024U,
            // Builtins plus one 16x16 RGBA texture; a second must be evicted.
            .resident_budget_bytes = smoke_test ? 28U*sizeof(gloom::render::GpuVertex)+42U*sizeof(std::uint32_t)+8U+1024U+20U : 512U * 1024U * 1024U,
            .temporal = {.render_scale =
                             automated_graphics && !visual_review ? 0.75F : 1.0F,
                         .dynamic_resolution = {
                             .enabled = automated_graphics && !visual_review,
                             .minimum_scale = 0.6F,
                             .target_frame_milliseconds = 100.0F,
                             .scale_step = 0.05F,
                             .settle_frames = 2}},
        });
    auto* renderer_view = renderer.get();
    constexpr gloom::render::RenderAssetId streaming_smoke_texture{0x600d600d};
    constexpr gloom::render::RenderAssetId streaming_budget_texture_a{0x600d600e};
    constexpr gloom::render::RenderAssetId streaming_budget_texture_b{0x600d600f};
    if (smoke_test) {
        renderer_view->enqueue(gloom::render::TextureUpload{
            .id = streaming_smoke_texture,
            .mip_levels = {{.width = 2,
                            .height = 2,
                            .data = std::vector<std::byte>(16, std::byte{0x7f})}},
        });
    }
    auto physics = std::make_unique<gloom::backends::JoltWorld>();
    auto* physics_view = physics.get();

    engine.add(std::move(jobs));
    engine.add(std::move(window));
    engine.add(std::move(renderer));
    engine.add(std::move(physics));
    auto transport = std::make_unique<gloom::backends::GnsTransport>();
    auto* transport_view = transport.get();
    engine.add(std::move(transport));

    engine.start();
    std::unique_ptr<gloom::assets::VirtualFileSystem> factory_filesystem;
    std::unique_ptr<gloom::assets::AssetCatalog> factory_catalog;
    std::unique_ptr<gloom::assets::AsyncAssetLoader> factory_asset_loader;
    std::unique_ptr<gloom::assets::AssetResidencyCoordinator> factory_residency;
    std::unique_ptr<gloom::assets::AssetCatalog> factory_surface_catalog;
    std::unique_ptr<gloom::assets::AsyncAssetLoader> factory_surface_loader;
    std::unique_ptr<gloom::assets::AssetResidencyCoordinator> factory_surface_residency;
    std::optional<gloom::assets::SceneTicket> factory_lift_ticket;
    std::optional<gloom::assets::SceneTicket> factory_surface_ticket;
    std::array<std::unique_ptr<gloom::assets::AssetCatalog>, 4> character_catalogs;
    std::array<std::unique_ptr<gloom::assets::AsyncAssetLoader>, 4> character_loaders;
    std::array<std::unique_ptr<gloom::assets::AssetResidencyCoordinator>, 4>
        character_residencies;
    std::array<std::optional<gloom::assets::SceneTicket>, 4> character_tickets;
    std::array<std::uint64_t, 4> character_generations{};
    std::array<bool, 4> character_failure_reported{};
    std::array<std::array<gloom::render::RenderAssetId, 9>, 4> character_meshes;
    std::unique_ptr<gloom::assets::AssetCatalog> weapon_catalog;
    std::unique_ptr<gloom::assets::AsyncAssetLoader> weapon_loader;
    std::unique_ptr<gloom::assets::AssetResidencyCoordinator> weapon_residency;
    std::optional<gloom::assets::SceneTicket> weapon_ticket;
    std::array<gloom::render::RenderAssetId, 2> weapon_meshes{
        gloom::render::builtin_cube_mesh, gloom::render::builtin_cube_mesh};
    std::uint64_t weapon_generation = 0;
    bool weapon_failure_reported = false;
    std::array<std::unique_ptr<gloom::assets::AssetCatalog>,4> arsenal_catalogs;
    std::array<std::unique_ptr<gloom::assets::AsyncAssetLoader>,4> arsenal_loaders;
    std::array<std::unique_ptr<gloom::assets::AssetResidencyCoordinator>,4> arsenal_residencies;
    std::array<std::optional<gloom::assets::SceneTicket>,4> arsenal_tickets;
    std::array<std::uint64_t,4> arsenal_generations{};
    std::array<bool,4> arsenal_failure_reported{};
    std::unique_ptr<gloom::assets::AssetCatalog> ability_catalog;
    std::unique_ptr<gloom::assets::AsyncAssetLoader> ability_loader;
    std::unique_ptr<gloom::assets::AssetResidencyCoordinator> ability_residency;
    std::optional<gloom::assets::SceneTicket> ability_ticket;
    std::array<gloom::render::RenderAssetId, 2> ability_meshes{
        gloom::render::builtin_cube_mesh, gloom::render::builtin_cube_mesh};
    std::uint64_t ability_generation = 0;
    bool ability_failure_reported = false;
    std::unique_ptr<gloom::assets::AssetCatalog> effects_catalog;
    std::unique_ptr<gloom::assets::AsyncAssetLoader> effects_loader;
    std::unique_ptr<gloom::assets::AssetResidencyCoordinator> effects_residency;
    std::optional<gloom::assets::SceneTicket> effects_ticket;
    for (auto& meshes : character_meshes) {
        meshes.fill(gloom::render::builtin_cube_mesh);
    }
    std::uint64_t factory_lift_generation = 0;
    std::uint64_t factory_surface_generation = 0;
    bool factory_lift_failure_reported = false;
    bool factory_surface_failure_reported = false;
    if (vertical_slice) {
        factory_filesystem = std::make_unique<gloom::assets::VirtualFileSystem>();
        factory_filesystem->mount("game", std::filesystem::path{GLOOM_SOURCE_ROOT} / "assets");
        factory_filesystem->mount("cache", std::filesystem::path{GLOOM_BINARY_ROOT} / "content");
        const auto source = gloom::assets::VirtualPath::parse(
            "game:/factory/cargo_lift.gltf");
        const auto cooked = gloom::assets::VirtualPath::parse(
            "cache:/factory/cargo_lift.gasset");
        if (source && cooked) {
            auto discovered = gloom::assets::discover_cooked_scene(
                *factory_filesystem, *source, *cooked);
            if (discovered) {
                const auto scene = discovered->scene;
                factory_catalog = std::make_unique<gloom::assets::AssetCatalog>(
                    std::move(discovered->catalog));
                factory_asset_loader = std::make_unique<gloom::assets::AsyncAssetLoader>(
                    *jobs_view, *factory_filesystem, *factory_catalog);
                factory_residency =
                    std::make_unique<gloom::assets::AssetResidencyCoordinator>(
                        *jobs_view, *factory_asset_loader, *factory_catalog, *renderer_view);
                factory_lift_ticket = factory_residency->request_scene(
                    scene, gloom::assets::AssetPriority::critical);
            } else {
                std::cerr << "Factory authored lift unavailable; using logical blockout: "
                          << discovered.error() << '\n';
            }
        }
        const auto surface_source = gloom::assets::VirtualPath::parse(
            original_factory ? "game:/legacy/factory.gltf" : "game:/factory/surface_modules.gltf");
        const auto surface_cooked = gloom::assets::VirtualPath::parse(
            original_factory ? "cache:/legacy/factory.gasset" : "cache:/factory/surface_modules.gasset");
        if (surface_source && surface_cooked) {
            auto discovered = gloom::assets::discover_cooked_scene(
                *factory_filesystem, *surface_source, *surface_cooked);
            if (discovered) {
                const auto scene = discovered->scene;
                factory_surface_catalog = std::make_unique<gloom::assets::AssetCatalog>(
                    std::move(discovered->catalog));
                factory_surface_loader = std::make_unique<gloom::assets::AsyncAssetLoader>(
                    *jobs_view, *factory_filesystem, *factory_surface_catalog);
                factory_surface_residency =
                    std::make_unique<gloom::assets::AssetResidencyCoordinator>(
                        *jobs_view, *factory_surface_loader, *factory_surface_catalog,
                        *renderer_view);
                factory_surface_ticket = factory_surface_residency->request_scene(
                    scene, gloom::assets::AssetPriority::high);
            } else {
                if (original_factory) throw std::runtime_error{"Factory original content unavailable: " + discovered.error()};
                std::cerr << "Factory authored surfaces unavailable; using logical blockout: "
                          << discovered.error() << '\n';
            }
        }
        for (const auto character : {gloom::gameplay::SliceCharacter::hound,
                                     gloom::gameplay::SliceCharacter::berserker,
                                     gloom::gameplay::SliceCharacter::archangel,
                                     gloom::gameplay::SliceCharacter::shadow}) {
            if (!original_characters && static_cast<unsigned>(character)>1) continue;
            const auto index = static_cast<std::size_t>(character);
            const auto recipe = gloom::gameplay::character_presentation_recipe(character, original_characters);
            const auto character_source =
                gloom::assets::VirtualPath::parse(recipe.authored_scene_uri);
            const auto character_cooked =
                gloom::assets::VirtualPath::parse(recipe.cooked_scene_uri);
            if (!character_source || !character_cooked) {
                continue;
            }
            auto discovered = gloom::assets::discover_cooked_scene(
                *factory_filesystem, *character_source, *character_cooked);
            if (!discovered) {
                std::cerr << "Authored character unavailable; using recipe fallback: "
                          << discovered.error() << '\n';
                continue;
            }
            const auto scene = discovered->scene;
            character_catalogs[index] = std::make_unique<gloom::assets::AssetCatalog>(
                std::move(discovered->catalog));
            character_loaders[index] = std::make_unique<gloom::assets::AsyncAssetLoader>(
                *jobs_view, *factory_filesystem, *character_catalogs[index]);
            character_residencies[index] =
                std::make_unique<gloom::assets::AssetResidencyCoordinator>(
                    *jobs_view, *character_loaders[index], *character_catalogs[index],
                    *renderer_view);
            character_tickets[index] = character_residencies[index]->request_scene(
                scene, gloom::assets::AssetPriority::high);
        }
        const auto discover_presentation = [&] (
            const std::string_view source_text, const std::string_view cooked_text,
            std::unique_ptr<gloom::assets::AssetCatalog>& catalog,
            std::unique_ptr<gloom::assets::AsyncAssetLoader>& loader,
            std::unique_ptr<gloom::assets::AssetResidencyCoordinator>& residency,
            std::optional<gloom::assets::SceneTicket>& ticket) {
            const auto source = gloom::assets::VirtualPath::parse(source_text);
            const auto cooked = gloom::assets::VirtualPath::parse(cooked_text);
            if (!source || !cooked) {
                return;
            }
            auto discovered = gloom::assets::discover_cooked_scene(
                *factory_filesystem, *source, *cooked);
            if (!discovered) {
                std::cerr << "Authored first-person content unavailable; using fallback: "
                          << discovered.error() << '\n';
                return;
            }
            const auto scene = discovered->scene;
            catalog = std::make_unique<gloom::assets::AssetCatalog>(
                std::move(discovered->catalog));
            loader = std::make_unique<gloom::assets::AsyncAssetLoader>(
                *jobs_view, *factory_filesystem, *catalog);
            residency = std::make_unique<gloom::assets::AssetResidencyCoordinator>(
                *jobs_view, *loader, *catalog, *renderer_view);
            ticket = residency->request_scene(scene, gloom::assets::AssetPriority::high);
        };
        const auto weapon_slot=original_characters ? gloom::gameplay::original_weapon_presentation(slice_selection.weapon) : gloom::gameplay::soul_reaper_presentation;
        discover_presentation(weapon_slot.source_uri,
                              weapon_slot.cooked_uri,
                              weapon_catalog, weapon_loader, weapon_residency,
                              weapon_ticket);
        if(original_characters)for(std::size_t i=1;i<gloom::gameplay::slice_weapon_count;++i){const auto slot=gloom::gameplay::original_weapon_presentation(static_cast<gloom::gameplay::SliceWeapon>(i));discover_presentation(slot.source_uri,slot.cooked_uri,arsenal_catalogs[i-1],arsenal_loaders[i-1],arsenal_residencies[i-1],arsenal_tickets[i-1]);}
        discover_presentation(gloom::gameplay::hound_ability_presentation.source_uri,
                              gloom::gameplay::hound_ability_presentation.cooked_uri,
                              ability_catalog, ability_loader, ability_residency,
                              ability_ticket);
        discover_presentation("game:/effects/effects.gltf","cache:/effects/effects.gasset",
                              effects_catalog,effects_loader,effects_residency,effects_ticket);
    }
    std::unique_ptr<gloom::backends::JoltWorld> slice_authoritative_physics;
    if (vertical_slice && !vertical_slice_join) {
        slice_authoritative_physics = std::make_unique<gloom::backends::JoltWorld>();
        slice_authoritative_physics->start();
    }
    if (vertical_slice && !vertical_slice_smoke && !vertical_slice_host &&
        !interactive_slice_selection && !vertical_slice_browse) {
        window_view->set_relative_mouse_mode(true);
    }
    std::optional<gloom::gameplay::VerticalSliceRemoteHost> slice_remote_host;
    std::optional<gloom::gameplay::VerticalSliceRemoteClient> slice_remote_client;
    gloom::network::ConnectionId slice_remote_connection = gloom::network::invalid_connection;
    bool slice_reconnect_pending = false;
    auto slice_reconnect_at = std::chrono::steady_clock::time_point{};
    std::optional<std::chrono::steady_clock::time_point> slice_admission_deadline;
    std::string slice_bound_endpoint;
    gloom::gameplay::AsyncSliceMatchBrowser slice_match_browser;
    bool slice_match_selected = !vertical_slice_browse;
    std::size_t slice_match_index = 0;
    if (vertical_slice_host) {
        slice_remote_host.emplace(gloom::gameplay::SliceRemoteHostSettings{
            .authoritative_physics = slice_authoritative_physics.get(), .original_factory = original_factory});
        slice_bound_endpoint = transport_view->listen(slice_endpoint);
        std::cout << "Authoritative slice listening on " << slice_bound_endpoint << ".\n";
    } else if (vertical_slice_join) {
        if (slice_player_name.empty()) {
            slice_remote_client.emplace(slice_selection, !interactive_slice_selection);
        } else {
            slice_remote_client.emplace(
                gloom::gameplay::SlicePlayerIdentity{
                    .account_id = development_account_id(slice_player_name),
                    .display_name = std::string{slice_player_name},
                },
                slice_selection, !interactive_slice_selection);
        }
        if (vertical_slice_browse) {
            static_cast<void>(slice_match_browser.begin_refresh(
                *configure_match_directory(), now_ms()));
            window_view->set_title("Gloom matches - loading...");
        } else {
            prepare_game_ticket();
            if (!game_tickets) slice_remote_connection = transport_view->connect(slice_endpoint);
            slice_admission_deadline=std::chrono::steady_clock::now()+std::chrono::seconds{10};
            std::cout << "Joining authoritative slice at " << slice_endpoint << " as "
                      << slice_remote_client->identity().display_name << ".\n";
        }
        if (interactive_slice_selection && !vertical_slice_browse) {
            std::cout << "Choose loadout with Left/Right and confirm with Enter.\n";
            window_view->set_title("Gloom roster: < " +
                                   std::string{selection_label(slice_selection)} +
                                   " >  [Enter to confirm]");
        }
    }
    gloom::render::VisibilitySystem visibility{*jobs_view};
    gloom::render::ClusteredLightingBuilder lighting_builder;
    std::vector point_lights{
        gloom::render::PointLight{.position = {-2.5F, 3.0F, -1.0F},
                                  .range = 7.0F,
                                  .color = {1.0F, 0.20F, 0.08F},
                                  .intensity = 18.0F},
        gloom::render::PointLight{.position = {2.5F, 2.0F, 1.5F},
                                  .range = 6.0F,
                                  .color = {0.08F, 0.35F, 1.0F},
                                  .intensity = 16.0F},
    };

    if (vertical_slice && original_factory) point_lights=gloom::gameplay::original_factory().lights;
    if (character_review) {
        // Neutral inspection lights expose metal/normal response in both views.
        point_lights.push_back({.position={-33.5F,3.5F,8.0F},.range=10,.color={1,.92F,.8F},.intensity=12});
        point_lights.push_back({.position={-37.5F,2.5F,4.0F},.range=8,.color={.5F,.7F,1},.intensity=8});
    }
    std::vector<VisualBody> visual_bodies;
    visual_bodies.reserve(12);
    const auto add_box = [&](const gloom::physics::Vec3 position,
                             const gloom::physics::Vec3 half_extent,
                             const gloom::physics::MotionType motion,
                             const gloom::render::Color color) {
        const auto body = physics_view->create_body(gloom::physics::BodyDesc{
            .shape = {.type = gloom::physics::ShapeType::box, .half_extent = half_extent},
            .transform = {.position = position},
            .motion = motion,
            .friction = 0.6F,
            .restitution = 0.08F,
        });
        const auto transform = to_render_transform(
            physics_view->body_transform(body), {half_extent.x, half_extent.y, half_extent.z});
        visual_bodies.push_back({body, gloom::render::builtin_cube_mesh,
                                 transform, transform, color});
    };
    const auto add_visual = [&](const gloom::render::Vec3 position,
                                const gloom::render::Vec3 scale,
                                const gloom::render::Color color,
                                const gloom::render::RenderAssetId mesh) {
        const gloom::render::Transform transform{
            .position = position, .scale = scale};
        visual_bodies.push_back({{}, mesh, transform, transform, color});
    };
    std::optional<std::size_t> factory_lift_visual;
    std::vector<std::size_t> factory_floor_visuals;
    std::vector<std::size_t> factory_industrial_visuals;
    std::vector<std::size_t> factory_trim_visuals;
    std::vector<std::size_t> factory_lava_visuals;

    std::optional<gloom::gameplay::VerticalSliceSimulation> slice_simulation;
    if (vertical_slice) {
        if (!vertical_slice_host) {
            // Join mode only uses this local instance as immutable arena metadata;
            // gameplay state comes exclusively from the authoritative host.
            slice_simulation.emplace(gloom::gameplay::VerticalSliceSettings{
                .opponent_ai_enabled = !vertical_slice_smoke && !vertical_slice_join && !visual_review,
                .authoritative_physics = vertical_slice_join
                                             ? nullptr
                                             : slice_authoritative_physics.get(), .original_factory = original_factory});
            if(!vertical_slice_join && !visual_review && !vertical_slice_smoke)
                static_cast<void>(slice_simulation->set_selection(gloom::gameplay::VerticalSliceSimulation::player_entity,slice_selection));
        }
        const auto arena = vertical_slice_host ? slice_remote_host->arena()
                                               : slice_simulation->arena();
        for (const auto& box : arena) {
            const auto color = [&] {
                switch (box.material) {
                    case gloom::gameplay::ArenaMaterial::floor:
                        return gloom::render::Color{0.055F, 0.065F, 0.085F, 1.0F};
                    case gloom::gameplay::ArenaMaterial::industrial:
                        return gloom::render::Color{0.19F, 0.21F, 0.24F, 1.0F};
                    case gloom::gameplay::ArenaMaterial::trim:
                        return gloom::render::Color{0.38F, 0.16F, 0.08F, 1.0F};
                    case gloom::gameplay::ArenaMaterial::lava:
                        return gloom::render::Color{1.0F, 0.16F, 0.015F, 1.0F};
                }
                return gloom::render::Color{0.2F, 0.2F, 0.2F, 1.0F};
            }();
            const auto visual_index = visual_bodies.size();
            add_visual({box.center_x, box.center_y, box.center_z},
                       {box.half_x, box.half_y, box.half_z}, color,
                       gloom::render::builtin_cube_mesh);
            if (box.material == gloom::gameplay::ArenaMaterial::floor) {
                factory_floor_visuals.push_back(visual_index);
            } else if (box.material == gloom::gameplay::ArenaMaterial::industrial) {
                factory_industrial_visuals.push_back(visual_index);
            } else if (box.material == gloom::gameplay::ArenaMaterial::trim) {
                factory_trim_visuals.push_back(visual_index);
            }
        }
        const auto surfaces = vertical_slice_host ? slice_remote_host->surfaces()
                                                  : slice_simulation->surfaces();
        if (!original_factory) {
        for (const auto& surface : surfaces) {
            const auto visual_index = visual_bodies.size();
            add_visual({surface.center_x, surface.center_y, surface.center_z},
                       {surface.half_x, 1.0F, surface.half_z},
                       {1.0F, 0.16F, 0.015F, 1.0F},
                       gloom::render::builtin_horizontal_quad_mesh);
            if (surface.material == gloom::gameplay::ArenaMaterial::lava) {
                factory_lava_visuals.push_back(visual_index);
            }
        }
        factory_lift_visual = visual_bodies.size();
        add_visual({-8.0F, 0.25F, 0.0F}, {1.0F, 0.25F, 1.0F},
                   {0.72F, 0.46F, 0.10F, 1.0F},
                   gloom::render::builtin_cube_mesh);
        }
    } else if (network_demo) {
        add_box({0.0F, -0.5F, 0.0F},
                {6.0F, 0.5F, 6.0F},
                gloom::physics::MotionType::static_body,
                {0.18F, 0.22F, 0.30F, 1.0F});
    }
    std::optional<gloom::gameplay::SliceSnapshot> review_snapshot;
    const auto current_slice_snapshot = [&]() -> const gloom::gameplay::SliceSnapshot& {
        if (review_snapshot) return *review_snapshot;
        if (vertical_slice_host) {
            return slice_remote_host->snapshot();
        }
        if (vertical_slice_join && slice_remote_client->has_snapshot()) {
            return slice_remote_client->snapshot();
        }
        return slice_simulation->snapshot();
    };
    constexpr std::array colors{
        gloom::render::Color{0.90F, 0.20F, 0.24F, 1.0F},
        gloom::render::Color{0.20F, 0.72F, 0.95F, 1.0F},
        gloom::render::Color{0.35F, 0.85F, 0.48F, 1.0F},
        gloom::render::Color{0.95F, 0.68F, 0.20F, 1.0F},
    };
    if (!network_demo && !vertical_slice) {
        for (std::size_t index = 0; index < 8; ++index) {
            const float horizontal_offset = index % 2 == 0 ? -0.18F : 0.18F;
            add_box({horizontal_offset, 0.65F + static_cast<float>(index) * 1.08F, 0.0F},
                    {0.5F, 0.5F, 0.5F},
                    gloom::physics::MotionType::dynamic,
                    colors[index % colors.size()]);
        }
    } else if (network_demo) {
        add_box({2.0F, 0.375F, 0.0F},
                {0.65F, 0.375F, 1.25F},
                gloom::physics::MotionType::static_body,
                {0.42F, 0.48F, 0.58F, 1.0F});
        add_box({-2.0F, 0.75F, 2.0F},
                {1.2F, 0.75F, 0.55F},
                gloom::physics::MotionType::static_body,
                {0.34F, 0.40F, 0.50F, 1.0F});
    }

    gloom::render::Camera camera = vertical_slice
                                      ? gloom::render::Camera{
                                            .position = {0.0F, 0.9F, 0.0F},
                                            .target = {1.0F, 0.9F, 0.0F},
                                            .vertical_field_of_view_radians = 1.30899694F,
                                            .near_plane = 0.05F,
                                            .far_plane = 100.0F,
                                        }
                                             : network_demo
                                             ? gloom::render::Camera{
                                                   .position = {10.0F, 9.0F, -14.0F},
                                                   .target = {0.0F, 0.5F, 0.0F},
                                               }
                                             : gloom::render::Camera{
                                                   .position = {9.0F, 6.5F, -12.0F},
                                                   .target = {0.0F, 2.8F, 0.0F},
                                               };

    constexpr gloom::network::NetworkEntityId local_network_entity = 1;
    constexpr gloom::network::NetworkEntityId remote_network_entity = 2;
    constexpr double network_fixed_delta = 1.0 / 60.0;
    const gloom::network::ReplicationSettings replication_settings{
        .tick_rate = 60,
        .snapshot_rate = 20,
        .movement_speed = 5.0F,
        .interpolation_delay_seconds = 0.12,
        .jump_speed = 5.5F,
        .gravity = -15.0F,
        .character_radius = 0.4F,
        .static_obstacles = {
            {.minimum_x = 1.35F,
             .maximum_x = 2.65F,
             .minimum_z = -1.25F,
             .maximum_z = 1.25F,
             .top_y = 0.75F},
            {.minimum_x = -3.2F,
             .maximum_x = -0.8F,
             .minimum_z = 1.45F,
             .maximum_z = 2.55F,
             .top_y = 1.5F},
        },
    };
    std::optional<gloom::network::AuthoritativeMovementServer> network_server;
    std::optional<gloom::network::PredictedMovementClient> network_client;
    std::optional<gloom::network::NetworkSimulator> upstream;
    std::optional<gloom::network::NetworkSimulator> downstream;
    std::optional<gloom::network::LagCompensatedCombatServer> combat_server;
    gloom::physics::CharacterId local_character;
    gloom::network::MovementState remote_render_state{
        .entity = remote_network_entity, .position_x = -3.0F};
    double network_accumulator = 0.0;
    bool previous_jump_pressed = false;
    gloom::network::PresentationSmoother presentation_smoother;
    if (network_demo) {
        const gloom::network::MovementState local_initial{.entity = local_network_entity};
        const gloom::network::MovementState remote_initial{
            .entity = remote_network_entity, .position_x = -3.0F};
        network_server.emplace(replication_settings, local_network_entity);
        network_server->add_entity(local_initial);
        network_server->add_entity(remote_initial);
        network_client.emplace(replication_settings, local_network_entity, local_initial);
        combat_server.emplace(gloom::network::CombatSettings{
            .history_ticks = 120,
            .maximum_range = 100.0F,
            .static_obstacles = replication_settings.static_obstacles,
        });
        combat_server->record(network_server->capture_world_snapshot());
        const gloom::network::NetworkSimulationSettings link_settings{
            .latency_seconds = 0.055,
            .jitter_seconds = 0.012,
            .loss_probability = 0.02,
            .duplicate_probability = 0.01,
            .reorder_probability = 0.05,
            .maximum_reorder_delay_seconds = 0.025,
            .bandwidth_bytes_per_second = 64 * 1024,
            .queue_capacity_packets = 256,
            .random_seed = 0x600d,
        };
        upstream.emplace(link_settings);
        auto reverse_settings = link_settings;
        reverse_settings.random_seed = 0xbeef;
        downstream.emplace(reverse_settings);
        local_character = physics_view->create_character({
            .position = {0.0F, 0.05F, 0.0F},
            .radius = 0.4F,
            .cylinder_half_height = 0.5F,
        });
    }
    std::vector<gloom::render::RenderInstance> render_instances;
    constexpr std::size_t slice_instance_count = 18;
    render_instances.reserve(visual_bodies.size() + (network_demo ? 2 : 0) +
                             (vertical_slice ? slice_instance_count : 0));
    render_instances.resize(visual_bodies.size() + (network_demo ? 2 : 0) +
                            (vertical_slice ? slice_instance_count : 0));
    auto physics_to_render_group = jobs_view->create_group();
    auto snapshot_group = jobs_view->create_group();

    auto previous_frame = std::chrono::steady_clock::now();
    auto drawable_size = window_view->drawable_size();
    std::uint32_t frame_count = 0;
    double slice_accumulator = 0.0;
    bool slice_saw_kill = false;
    bool slice_saw_respawn = false;
    bool slice_opponent_was_dead = false;
    gloom::platform::MovementActions slice_actions;
    bool slice_previous_ability_pressed = false;
    bool slice_previous_menu_left = false;
    bool slice_previous_menu_right = false;
    bool slice_previous_menu_confirm = false;
    bool slice_selection_confirmed = !interactive_slice_selection;
    std::size_t slice_roster_index = 0;
    gloom::gameplay::FirstPersonController slice_first_person;
    if (vertical_slice && original_factory) slice_first_person=gloom::gameplay::FirstPersonController{3.14159265F,0.0F};
    gloom::gameplay::SlicePresentationFeedback slice_feedback;
    std::size_t review_frames = 0;
    std::vector<gloom::render::Transform> previous_original_transforms;
    auto previous_original_character=gloom::gameplay::SliceCharacter::hound;
    double presentation_seconds=0;
    gloom::gameplay::CharacterAnimator opponent_animator,local_animator;
    std::vector<gloom::render::RenderInstance> previous_animated_instances;
    gloom::render::ParticleSystem particles{gloom::render::load_particle_recipes(
        std::filesystem::path{GLOOM_SOURCE_ROOT}/"assets/effects/recipes.json")};
    gloom::gameplay::CombatEffects combat_effects;
    const bool graphical_ui=vertical_slice && !automated_graphics;
    std::shared_ptr<gloom::gameplay::AudioPresentation> game_audio;
    if(graphical_ui){
        game_audio=desktop_session?desktop_session->audio:nullptr;
        if(!game_audio){
            gloom::assets::VirtualFileSystem fs;fs.mount("game",std::filesystem::path{GLOOM_SOURCE_ROOT}/"assets");fs.mount("cache",std::filesystem::path{GLOOM_BINARY_ROOT}/"content");
            game_audio=std::make_shared<gloom::gameplay::AudioPresentation>(fs,true);
            if(desktop_session)desktop_session->audio=game_audio;
        }
        game_audio->scene_reset();std::cout<<game_audio->diagnostic()<<'\n';
    }
    // Scope exit also handles admission failures and exceptions.
    struct AudioSceneGuard {std::shared_ptr<gloom::gameplay::AudioPresentation> value;~AudioSceneGuard(){if(value)value->scene_reset();}} audio_guard{game_audio};
    auto audio_time=std::chrono::steady_clock::now();
    bool audio_waiting_for_snapshot=vertical_slice_join;
    std::optional<gloom::desktop::GameUi> game_ui;
    bool ui_return_to_menu=false,ui_mouse_captured=false,ui_suppress_game_input=true;
    std::string ui_notice;
    unsigned ui_flow_stage=0;std::array<bool,9> ui_flow_captured{};
    const bool ui_flow=desktop_session && !desktop_session->flow_output.empty();
    const auto ui_flow_start=std::chrono::steady_clock::now();auto ui_flow_next=ui_flow_start;
    gloom::network::NetworkEntityId ui_flow_entity=0;
    if(graphical_ui){
        game_ui.emplace();renderer_view->enqueue(game_ui->canvas.atlas_upload());
        for(std::size_t i=0;i<slice_roster.size();++i)if(slice_roster[i]==slice_selection)slice_roster_index=i;
        window_view->set_relative_mouse_mode(false);
    }
    while (window_view->poll_events()) {
        if(game_ui){
            const auto size=window_view->drawable_size();
            const auto* lobby=vertical_slice_host?&slice_remote_host->lobby():vertical_slice_join?&slice_remote_client->lobby():nullptr;
            const bool connected=!vertical_slice_join || (slice_remote_connection!=gloom::network::invalid_connection && slice_remote_client->active() && !slice_reconnect_pending);
            auto ui_input=window_view->input_state();
            if(ui_flow){
                ui_input={};const auto now=std::chrono::steady_clock::now();
                if(now-ui_flow_start>std::chrono::seconds{100})throw std::runtime_error{"Graphical two-client flow timed out at stage "+std::to_string(ui_flow_stage)};
                if(frame_count%8==4){
                    if(ui_flow_stage==0 && connected){ui_input.mouse_x=500;ui_input.mouse_y=617;ui_input.mouse_primary=true;ui_flow_stage=1;}
                    else if(ui_flow_stage==1 && lobby && lobby->phase==gloom::gameplay::SliceMatchPhase::active && factory_surface_generation){ui_flow_entity=current_slice_snapshot().player.entity;ui_flow_stage=2;ui_flow_next=now+std::chrono::seconds{2};}
                    else if(ui_flow_stage==2 && now>=ui_flow_next){ui_input.menu_back=true;ui_flow_stage=3;}
                    else if(ui_flow_stage==3 && game_ui->paused){ui_input.mouse_x=640;ui_input.mouse_y=411;ui_input.mouse_primary=true;ui_flow_stage=4;}
                    else if(ui_flow_stage==4 && connected && lobby && lobby->phase==gloom::gameplay::SliceMatchPhase::active){
                        if(current_slice_snapshot().player.entity!=ui_flow_entity)throw std::runtime_error{"UI reconnect changed player identity"};
                        ui_flow_stage=5;ui_flow_next=now+std::chrono::seconds{2};}
                    else if(ui_flow_stage==5 && now>=ui_flow_next){ui_input.menu_back=true;ui_flow_stage=6;}
                    else if(ui_flow_stage==6 && game_ui->paused){ui_input.mouse_x=600;ui_input.mouse_y=480;ui_input.mouse_primary=true;ui_flow_stage=7;}
                    else if(ui_flow_stage==7 && game_ui->confirm_leave){ui_input.mouse_x=800;ui_input.mouse_y=422;ui_input.mouse_primary=true;ui_flow_stage=8;}
                }
            }
            const auto action=game_ui->draw(size.first,size.second,ui_input,current_slice_snapshot(),lobby,vertical_slice_host,
                factory_surface_generation!=0,connected,slice_reconnect_pending,slice_selection_confirmed,slice_roster_index,
                vertical_slice_host?slice_bound_endpoint:slice_endpoint,ui_notice);
            if(action.leave){ui_return_to_menu=true;break;}
            if(action.selection && slice_remote_client){slice_roster_index=*action.selection;static_cast<void>(slice_remote_client->set_desired_selection(slice_roster[slice_roster_index]));}
            if(action.ready && slice_remote_client){
                if(auto outgoing=slice_remote_client->confirm_selection();outgoing && slice_remote_connection!=gloom::network::invalid_connection){
                    const auto bytes=gloom::network::encode_message(outgoing->message);
                    transport_view->send(slice_remote_connection,{.payload=bytes,.delivery=outgoing->delivery});
                    slice_selection_confirmed=true;
                }
            }
            if(action.reconnect && slice_remote_client){
                if(slice_remote_connection!=gloom::network::invalid_connection)transport_view->disconnect(slice_remote_connection);
                slice_remote_connection=gloom::network::invalid_connection;slice_reconnect_pending=true;
                slice_reconnect_at=std::chrono::steady_clock::now()+std::chrono::milliseconds{300};ui_notice.clear();
                local_animator.reset();opponent_animator.reset();combat_effects.reset(particles);previous_animated_instances.clear();
            }
            const bool capture=!game_ui->blocked;
            if(capture!=ui_mouse_captured){window_view->set_relative_mouse_mode(capture);ui_mouse_captured=capture;ui_suppress_game_input=true;}
        }
        if (effects_residency) {
            effects_residency->update();
            if (effects_ticket && effects_residency->state(*effects_ticket)==gloom::assets::SceneResidencyState::failed)
                throw std::runtime_error{"Effects failed to become resident: "+std::string{effects_residency->error(*effects_ticket)}};
        }
        if (slice_reconnect_pending) {combat_effects.reset(particles);opponent_animator.reset();local_animator.reset();previous_animated_instances.clear();}
        if (factory_residency && factory_lift_ticket) {
            factory_residency->update();
            const auto state = factory_residency->state(*factory_lift_ticket);
            if (state == gloom::assets::SceneResidencyState::failed &&
                !factory_lift_failure_reported) {
                std::cerr << "Factory authored lift failed to become resident; using logical "
                             "blockout: "
                          << factory_residency->error(*factory_lift_ticket) << '\n';
                factory_lift_failure_reported = true;
            }
            const auto* scene = factory_residency->scene(*factory_lift_ticket);
            if (scene != nullptr && !scene->instances.empty() &&
                scene->generation != factory_lift_generation && factory_lift_visual) {
                auto& visual = visual_bodies[*factory_lift_visual];
                visual.mesh = scene->instances.front().mesh;
                visual.previous.scale = {1.0F, 1.0F, 1.0F};
                visual.current.scale = {1.0F, 1.0F, 1.0F};
                factory_lift_generation = scene->generation;
            }
        }
        if (factory_surface_residency && factory_surface_ticket) {
            factory_surface_residency->update();
            const auto state = factory_surface_residency->state(*factory_surface_ticket);
            if (state == gloom::assets::SceneResidencyState::failed &&
                !factory_surface_failure_reported) {
                if (original_factory) throw std::runtime_error{"Original Factory streaming failed: " + std::string{factory_surface_residency->error(*factory_surface_ticket)}};
                std::cerr << "Factory authored surfaces failed to become resident; using "
                             "logical blockout: "
                          << factory_surface_residency->error(*factory_surface_ticket) << '\n';
                factory_surface_failure_reported = true;
            }
            const auto* scene = factory_surface_residency->scene(*factory_surface_ticket);
            if (scene != nullptr && scene->instances.size() >= 4 &&
                scene->generation != factory_surface_generation) {
                const auto apply_module = [&](const std::span<const std::size_t> visuals,
                                              const gloom::render::RenderAssetId mesh) {
                    for (const auto index : visuals) {
                        auto& visual = visual_bodies[index];
                        visual.mesh = mesh;
                        if (factory_surface_generation == 0) {
                            visual.previous.scale.y *= 4.0F;
                            visual.current.scale.y *= 4.0F;
                        }
                    }
                };
                apply_module(factory_floor_visuals, scene->instances[0].mesh);
                apply_module(factory_industrial_visuals, scene->instances[1].mesh);
                apply_module(factory_trim_visuals, scene->instances[2].mesh);
                for (const auto index : factory_lava_visuals) {
                    visual_bodies[index].mesh = scene->instances[3].mesh;
                }
                factory_surface_generation = scene->generation;
            }
        }
        for (std::size_t index = 0; index < character_residencies.size(); ++index) {
            if (!character_residencies[index] || !character_tickets[index]) {
                continue;
            }
            auto& residency = *character_residencies[index];
            residency.update();
            const auto state = residency.state(*character_tickets[index]);
            if (state == gloom::assets::SceneResidencyState::failed &&
                !character_failure_reported[index]) {
                std::cerr << "Authored character failed to become resident; using recipe "
                             "fallback: "
                          << residency.error(*character_tickets[index]) << '\n';
                character_failure_reported[index] = true;
            }
            const auto* scene = residency.scene(*character_tickets[index]);
            if (original_characters) {
                if (scene && !scene->instances.empty() && scene->bind_rig) character_generations[index]=scene->generation;
                continue;
            }
            if (scene == nullptr || scene->instances.size() != character_meshes[index].size() ||
                scene->generation == character_generations[index]) {
                continue;
            }
            for (std::size_t part = 0; part < character_meshes[index].size(); ++part) {
                character_meshes[index][part] = scene->instances[part].mesh;
            }
            character_generations[index] = scene->generation;
        }
        const auto update_two_part_scene = [&] (
            std::unique_ptr<gloom::assets::AssetResidencyCoordinator>& residency,
            const std::optional<gloom::assets::SceneTicket>& ticket,
            std::array<gloom::render::RenderAssetId, 2>& meshes,
            std::uint64_t& generation, bool& failure_reported,
            const std::string_view label) {
            if (!residency || !ticket) {
                return;
            }
            residency->update();
            const auto state = residency->state(*ticket);
            if (state == gloom::assets::SceneResidencyState::failed &&
                !failure_reported) {
                std::cerr << label << " failed to become resident; using fallback: "
                          << residency->error(*ticket) << '\n';
                failure_reported = true;
            }
            const auto* scene = residency->scene(*ticket);
            if (original_characters && &meshes==&weapon_meshes) {
                if (scene && !scene->instances.empty()) generation=scene->generation;
                return;
            }
            if (scene == nullptr || scene->instances.size() != meshes.size() ||
                scene->generation == generation) {
                return;
            }
            for (std::size_t part = 0; part < meshes.size(); ++part) {
                meshes[part] = scene->instances[part].mesh;
            }
            generation = scene->generation;
        };
        update_two_part_scene(weapon_residency, weapon_ticket, weapon_meshes,
                              weapon_generation, weapon_failure_reported,
                              "Authored Soul Reaper");
        for(std::size_t i=0;i<arsenal_residencies.size();++i)if(arsenal_residencies[i]&&arsenal_tickets[i]){arsenal_residencies[i]->update();const auto state=arsenal_residencies[i]->state(*arsenal_tickets[i]);if(state==gloom::assets::SceneResidencyState::failed&&!arsenal_failure_reported[i]){std::cerr<<"Original weapon "<<i+1<<" failed to become resident: "<<arsenal_residencies[i]->error(*arsenal_tickets[i])<<'\n';arsenal_failure_reported[i]=true;}if(const auto* scene=arsenal_residencies[i]->scene(*arsenal_tickets[i]))arsenal_generations[i]=scene->generation;}
        const auto resident_weapon=[&](gloom::gameplay::SliceWeapon weapon)->const gloom::assets::ResidentScene*{const auto i=static_cast<std::size_t>(weapon);if(i==0)return weapon_ticket&&weapon_residency?weapon_residency->scene(*weapon_ticket):nullptr;return i<=arsenal_tickets.size()&&arsenal_tickets[i-1]&&arsenal_residencies[i-1]?arsenal_residencies[i-1]->scene(*arsenal_tickets[i-1]):nullptr;};
        update_two_part_scene(ability_residency, ability_ticket, ability_meshes,
                              ability_generation, ability_failure_reported,
                              "Authored Hound ability content");
        const bool review_ready = visual_review && (original_factory || factory_lift_generation) && factory_surface_generation &&
            character_generations[0] && character_generations[1] && weapon_generation && ability_generation &&
            (!character_review || (character_generations[2] && character_generations[3])) &&
            (!animation_review || (effects_ticket && effects_residency->scene(*effects_ticket)));
        const auto& review_view = lava_review?gloom::review::factory_views[4]:review_views[animation_review?0:review_frames / 32];
        if (visual_review) {
            review_snapshot = slice_simulation->snapshot();
            review_snapshot->player.position_x = review_view.x;
            review_snapshot->player.position_y = review_view.y;
            review_snapshot->player.position_z = review_view.z;
            review_snapshot->player.ability = review_view.ability;
            review_snapshot->hud.primary_ability_active = review_view.ability != gloom::gameplay::SliceAbility::none;
            review_snapshot->hud.weapon_ready_fraction = 1.0F;
            if(pickup_review) {
                const auto& definitions=gloom::gameplay::original_factory().pickups;
                const auto found=std::ranges::find_if(definitions,[](const auto& p){return p.kind==gloom::gameplay::PickupKind::shield && p.reward==50;});
                if(found==definitions.end())throw std::runtime_error{"Pickup review shield missing"};
                auto& p=review_snapshot->pickups[static_cast<std::size_t>(found-definitions.begin())];
                const std::string_view name=review_view.name;
                if(name=="pickup-pulling") {p.phase=gloom::gameplay::PickupPhase::pulling;p.pulling_player=1;p.position.z+=1.5F;p.position.y+=.5F;}
                if(name=="pickup-collected") {p.phase=gloom::gameplay::PickupPhase::respawning;p.respawn_remaining=1500;}
            }
            if (character_review) {
                const auto view=review_frames/32;
                review_snapshot->opponent.character = view==2 || view==3 ? gloom::gameplay::SliceCharacter::shadow : gloom::gameplay::SliceCharacter::archangel;
                review_snapshot->opponent.position_x=review_view.x;
                review_snapshot->opponent.position_y=.01F;
                review_snapshot->opponent.position_z=review_view.z-3.2F;
                review_snapshot->opponent.facing_x=0;
                review_snapshot->opponent.facing_z=view==1 || view==3 ? -1.0F : 1.0F;
            }
            slice_first_person = gloom::gameplay::FirstPersonController{review_view.yaw, review_view.pitch};
            if (animation_review) {
                gloom::review::animate_snapshot(*review_snapshot,review_frames,animation_fps);
                const double shot_time=static_cast<double>(review_frames%static_cast<std::size_t>(animation_fps*10))/animation_fps;
                if (shot_time>=8 && shot_time<9) {
                    const float pitch=static_cast<float>(std::sin((shot_time-8)*6.2831853)*.85);
                    slice_first_person=gloom::gameplay::FirstPersonController{review_view.yaw,pitch};
                }
                if (review_ready && review_frames%static_cast<std::size_t>(animation_fps*10)==0) {
                    opponent_animator.reset();local_animator.reset();combat_effects.reset(particles);previous_animated_instances.clear();
                }
                point_lights=gloom::gameplay::original_factory().lights;
                if (!lava_review && review_frames<static_cast<std::size_t>(animation_fps*20)) {
                    point_lights.push_back({.position={-33.5F,3.5F,8.0F},.range=10,.color={1,.92F,.8F},.intensity=12});
                    point_lights.push_back({.position={-37.5F,2.5F,4.0F},.range=8,.color={.5F,.7F,1},.intensity=8});
                }
            }
        }
        const auto current_frame = std::chrono::steady_clock::now();
        const std::chrono::duration<double> measured_elapsed = current_frame - previous_frame;
        previous_frame = current_frame;
        const double elapsed = game_ui && game_ui->paused && !vertical_slice_host && !vertical_slice_join ? 0.0 : animation_review && review_ready ? 1.0/animation_fps : (visual_review || (vertical_slice && original_factory && !factory_surface_generation)) ? 0.0 : (network_scene_smoke || vertical_slice_smoke)
                                   ? network_fixed_delta
                                   : measured_elapsed.count();
        presentation_seconds+=elapsed;

        if (vertical_slice) {
            slice_accumulator = std::min(slice_accumulator + elapsed, 0.25);
            auto input_state = window_view->input_state();
            if(game_ui && !input_state.fire_primary && !input_state.fire_secondary && !input_state.menu_confirm && !input_state.jump && !input_state.use_primary_ability)ui_suppress_game_input=false;
            if(game_ui && (game_ui->blocked || ui_suppress_game_input)){input_state.move_left=input_state.move_right=input_state.move_forward=input_state.move_backward=false;
                input_state.dodge=input_state.jump=input_state.fire_primary=input_state.fire_secondary=input_state.use_primary_ability=false;input_state.look_delta_x=input_state.look_delta_y=0;}
            if (vertical_slice_browse && !slice_match_selected) {
                if (slice_match_browser.poll()) {
                    if (!slice_match_browser.error().empty()) {
                        window_view->set_title("Gloom matches - error; press Enter to retry");
                    } else if (slice_match_browser.matches().empty()) {
                        window_view->set_title("Gloom matches - none available; press Enter to refresh");
                    } else {
                        slice_match_index = std::min(slice_match_index,
                            slice_match_browser.matches().size() - 1);
                        const auto& chosen = slice_match_browser.matches()[slice_match_index];
                        window_view->set_title("Gloom matches: < " + chosen.display_name + " " +
                            std::to_string(chosen.player_count) + "/" +
                            std::to_string(chosen.capacity) + " > [Enter]");
                    }
                }
                const bool previous = input_state.menu_previous && !slice_previous_menu_left;
                const bool next = input_state.menu_next && !slice_previous_menu_right;
                if ((previous || next) && !slice_match_browser.matches().empty()) {
                    const auto count = slice_match_browser.matches().size();
                    slice_match_index = previous ? (slice_match_index + count - 1) % count
                                                 : (slice_match_index + 1) % count;
                    const auto& chosen = slice_match_browser.matches()[slice_match_index];
                    window_view->set_title("Gloom matches: < " + chosen.display_name + " " +
                        std::to_string(chosen.player_count) + "/" +
                        std::to_string(chosen.capacity) + " > [Enter]");
                }
                if (input_state.menu_confirm && !slice_previous_menu_confirm) {
                    if (slice_match_browser.matches().empty()) {
                        static_cast<void>(slice_match_browser.begin_refresh(
                            *configure_match_directory(), now_ms()));
                        window_view->set_title("Gloom matches - loading...");
                    } else {
                        const auto& chosen = slice_match_browser.matches()[slice_match_index];
                        const auto resolved = configure_match_directory()->resolve(
                            chosen.match_id, now_ms());
                        if (!resolved) {
                            window_view->set_title("Gloom match changed; press Enter to refresh");
                        } else {
                            slice_endpoint = resolved->endpoint;
                            slice_match_id = chosen.match_id;
                            prepare_game_ticket();
                            if (!game_tickets) slice_remote_connection = transport_view->connect(slice_endpoint);
                            slice_match_selected = true;
                            if (!interactive_slice_selection)
                                window_view->set_relative_mouse_mode(true);
                            window_view->set_title(interactive_slice_selection
                                ? "Gloom roster - choose with Left/Right, Enter"
                                : "Gloom lobby - connecting");
                        }
                    }
                }
            }
            if (!game_ui && slice_match_selected && interactive_slice_selection && !slice_selection_confirmed) {
                const bool previous = input_state.menu_previous && !slice_previous_menu_left;
                const bool next = input_state.menu_next && !slice_previous_menu_right;
                if (previous || next) {
                    slice_roster_index = previous
                                             ? (slice_roster_index + slice_roster.size() - 1) %
                                                   slice_roster.size()
                                             : (slice_roster_index + 1) % slice_roster.size();
                    static_cast<void>(slice_remote_client->set_desired_selection(
                        slice_roster[slice_roster_index]));
                    window_view->set_title(
                        "Gloom roster: < " +
                        std::string{selection_label(slice_roster[slice_roster_index])} +
                        " >  [Enter to confirm]");
                }
                if (input_state.menu_confirm && !slice_previous_menu_confirm) {
                    slice_selection_confirmed = true;
                    if (auto outgoing = slice_remote_client->confirm_selection();
                        outgoing &&
                        slice_remote_connection != gloom::network::invalid_connection) {
                        const auto bytes = gloom::network::encode_message(outgoing->message);
                        transport_view->send(slice_remote_connection,
                                             {.payload = bytes,
                                              .delivery = outgoing->delivery});
                    }
                    window_view->set_relative_mouse_mode(true);
                    window_view->set_title(
                        "Gloom lobby - selection submitted; waiting for authority");
                }
            }
            slice_previous_menu_left = input_state.menu_previous;
            slice_previous_menu_right = input_state.menu_next;
            slice_previous_menu_confirm = input_state.menu_confirm;
            if (!vertical_slice_smoke && !vertical_slice_host && !visual_review) {
                slice_first_person.look(input_state.look_delta_x, input_state.look_delta_y);
            }
            slice_actions.update(input_state,!game_ui || (!game_ui->blocked && !ui_suppress_game_input));
            bool jump_requested = slice_actions.jump;
            bool ability_requested = input_state.use_primary_ability &&
                                     !slice_previous_ability_pressed;
            slice_previous_ability_pressed = input_state.use_primary_ability;
            while (slice_accumulator + 1.0e-12 >= network_fixed_delta) {
                auto axes = gloom::platform::movement_axes(input_state);
                const auto movement = slice_first_person.movement(axes.x, axes.z);
                float movement_x = movement.world_x;
                float movement_z = movement.world_z;
                auto aim = slice_first_person.forward();
                bool fire = input_state.fire_primary;
                if (vertical_slice_smoke) {
                    const auto& before_tick = slice_simulation->snapshot();
                    const auto tick = before_tick.simulation_tick;
                    // Sidestep the central occluder before exercising the
                    // authoritative hitscan/respawn loop.
                    movement_x = 0.0F;
                    movement_z = tick < 45 ? -1.0F : 0.0F;
                    fire = tick >= 45;
                    jump_requested = tick == 12;
                    const float difference_x =
                        before_tick.opponent.position_x - before_tick.player.position_x;
                    const float difference_y =
                        before_tick.opponent.position_y - before_tick.player.position_y;
                    const float difference_z =
                        before_tick.opponent.position_z - before_tick.player.position_z;
                    const float length_squared = difference_x * difference_x +
                                                 difference_y * difference_y +
                                                 difference_z * difference_z;
                    if (length_squared > 1.0e-6F) {
                        const float inverse_length = 1.0F / std::sqrt(length_squared);
                        aim = {.x = difference_x * inverse_length,
                               .y = difference_y * inverse_length,
                               .z = difference_z * inverse_length};
                    }
                }
                const gloom::gameplay::SliceInput slice_input{
                    .axis_x = movement_x,
                    .axis_z = movement_z,
                    .aim_x = aim.x,
                    .aim_y = aim.y,
                    .aim_z = aim.z,
                    .jump = jump_requested,
                    .fire_primary = fire,
                    .fire_secondary = input_state.fire_secondary,
                    .use_primary_ability = ability_requested,
                    .dodge = slice_actions.dodge,
                    .weapon_selection = input_state.weapon_selection,
                };
                if (vertical_slice_host) {
                    for (const auto& outgoing : slice_remote_host->tick_clients()) {
                        const auto bytes = gloom::network::encode_message(outgoing.message);
                        transport_view->send(outgoing.connection,
                                             {.payload = bytes,
                                              .delivery = outgoing.delivery});
                    }
                } else if (vertical_slice_join) {
                    if (slice_remote_connection != gloom::network::invalid_connection) {
                        for (const auto& outgoing :
                             slice_remote_client->create_input(slice_input)) {
                            const auto bytes = gloom::network::encode_message(outgoing.message);
                            transport_view->send(slice_remote_connection,
                                                 {.payload = bytes,
                                                  .delivery = outgoing.delivery});
                        }
                    }
                } else {
                    slice_simulation->tick(slice_input);
                }
                jump_requested = false;
                slice_actions.consume();
                ability_requested = false;
                if (!vertical_slice_host && !vertical_slice_join) {
                    const auto& slice = slice_simulation->snapshot();
                    slice_saw_kill = slice_saw_kill || slice.player.kills != 0;
                    slice_saw_respawn = slice_saw_respawn ||
                                        (slice_opponent_was_dead && slice.opponent.alive);
                    slice_opponent_was_dead = !slice.opponent.alive;
                }
                slice_accumulator -= network_fixed_delta;
            }
        }

        if (network_demo) {
            network_accumulator = std::min(network_accumulator + elapsed, 0.25);
            const auto input_state = window_view->input_state();
            bool jump_requested = input_state.jump && !previous_jump_pressed;
            previous_jump_pressed = input_state.jump;
            while (network_accumulator + 1.0e-12 >= network_fixed_delta) {
                auto axes = gloom::platform::movement_axes(input_state);
                if (network_scene_smoke) {
                    const std::uint64_t tick = network_server->simulation_tick();
                    axes = tick < 90 ? gloom::platform::MovementAxes{1.0F, 0.0F}
                                     : gloom::platform::MovementAxes{-0.5F, 0.5F};
                    jump_requested = tick == 30 || tick == 120;
                }
                const bool jump_this_tick = jump_requested;
                jump_requested = false;
                slice_actions.consume();
                const bool was_grounded = network_client->local_state().grounded;
                auto input = network_client->create_input(axes.x, axes.z, jump_this_tick);
                auto encoded_input = gloom::network::encode_message(input);
                upstream->submit(
                    {.flow = 1, .sequence = input.sequence, .payload = encoded_input});

                const double phase = static_cast<double>(network_server->simulation_tick()) * 0.025;
                network_server->set_entity_input(remote_network_entity,
                                                 static_cast<float>(std::cos(phase) * 0.65),
                                                 static_cast<float>(std::sin(phase) * 0.65));
                upstream->advance(network_fixed_delta);
                downstream->advance(network_fixed_delta);
                while (const auto packet = upstream->receive()) {
                    const auto message = gloom::network::decode_message(packet->payload);
                    if (message) {
                        network_server->receive(*message);
                    }
                }
                const auto snapshot = network_server->tick();
                combat_server->record(network_server->capture_world_snapshot());
                if (snapshot) {
                    auto encoded_snapshot = gloom::network::encode_message(*snapshot);
                    downstream->submit({.flow = 2,
                                        .sequence = snapshot->sequence,
                                        .payload = encoded_snapshot});
                }
                if (network_scene_smoke &&
                    (network_server->simulation_tick() == 60 ||
                     network_server->simulation_tick() == 120)) {
                    const auto& shooter = network_server->entity(local_network_entity);
                    const auto& target = network_server->entity(remote_network_entity);
                    const float difference_x = target.position_x - shooter.position_x;
                    const float difference_y = target.position_y - shooter.position_y;
                    const float difference_z = target.position_z - shooter.position_z;
                    const float length_squared =
                        difference_x * difference_x + difference_y * difference_y +
                        difference_z * difference_z;
                    if (length_squared > 1.0e-6F) {
                        const float inverse_length = 1.0F / std::sqrt(length_squared);
                        static_cast<void>(combat_server->validate({
                            .sequence = static_cast<std::uint32_t>(
                                network_server->simulation_tick() / 60),
                            .shooter = local_network_entity,
                            .estimated_server_tick = network_server->simulation_tick(),
                            .aim_x = difference_x * inverse_length,
                            .aim_y = difference_y * inverse_length,
                            .aim_z = difference_z * inverse_length,
                            .maximum_distance = 100.0F,
                        }));
                    }
                }
                while (const auto packet = downstream->receive()) {
                    const auto message = gloom::network::decode_message(packet->payload);
                    if (message) {
                        const auto previous_reconciliations =
                            network_client->reconciliation_count();
                        network_client->receive(*message);
                        if (network_client->reconciliation_count() > previous_reconciliations) {
                            presentation_smoother.observe_correction(
                                network_client->reconciliation_metrics());
                        }
                    }
                }
                const auto& predicted = network_client->local_state();
                physics_view->set_character_horizontal_velocity(
                    local_character, {predicted.velocity_x, 0.0F, predicted.velocity_z});
                if (jump_this_tick && was_grounded && predicted.velocity_y > 0.0F) {
                    physics_view->jump_character(local_character, replication_settings.jump_speed);
                }
                network_accumulator -= network_fixed_delta;
            }
            presentation_smoother.advance(elapsed);
        }

        engine.tick(elapsed);
        if (vertical_slice_host || vertical_slice_join) {
            if (pending_game_ticket.valid() && pending_game_ticket.wait_for(std::chrono::seconds{0}) == std::future_status::ready) {
                const auto ticket = pending_game_ticket.get();
                if (!ticket) {
                    ui_notice="No se pudo autorizar el acceso: "+ticket.error();
                    slice_reconnect_pending = false;
                    window_view->set_relative_mouse_mode(false);
                    if (vertical_slice_browse) slice_match_selected = false;
                    window_view->set_title("Gloom game authorization failed: " + ticket.error());
                    std::cerr << "Game authorization failed: " << ticket.error() << '\n';
                } else {
                    slice_endpoint = ticket->endpoint;
                    slice_game_credential = ticket->credential;
                    slice_remote_connection = transport_view->connect(slice_endpoint);
                    slice_admission_deadline = current_frame + std::chrono::seconds{10};
                }
            }
            const double network_now =
                std::chrono::duration<double>(current_frame.time_since_epoch()).count();
            if (vertical_slice_join && slice_match_selected && slice_reconnect_pending &&
                current_frame >= slice_reconnect_at) {
                prepare_game_ticket();
                if (!game_tickets) slice_remote_connection = transport_view->connect(slice_endpoint);
                slice_reconnect_pending = false;
                slice_admission_deadline=current_frame+std::chrono::seconds{10};
                std::cout << "Reconnecting authoritative slice at " << slice_endpoint << ".\n";
            }
            while (auto event = transport_view->poll_event()) {
                if (vertical_slice_host && event->incoming) {
                    if (event->state == gloom::network::ConnectionState::connected) {
                        slice_remote_host->connected(event->connection);
                    } else if (event->state == gloom::network::ConnectionState::disconnected ||
                               event->state == gloom::network::ConnectionState::failed) {
                        slice_remote_host->disconnected(event->connection);
                    }
                } else if (vertical_slice_join && !event->incoming &&
                           event->connection == slice_remote_connection) {
                    if (event->state == gloom::network::ConnectionState::connected) {
                        const bool resume = slice_remote_client->session().resume_token() != 0;
                        const auto hello = game_tickets
                            ? (resume ? slice_remote_client->reconnect_with_credential(std::move(slice_game_credential))
                                      : slice_remote_client->begin_with_credential(std::move(slice_game_credential)))
                            : (resume ? slice_remote_client->reconnect() : slice_remote_client->begin());
                        const auto bytes = gloom::network::encode_message(hello.message);
                        transport_view->send(slice_remote_connection,
                                             {.payload = bytes,
                                              .delivery = hello.delivery});
                    } else if (event->state == gloom::network::ConnectionState::disconnected ||
                               event->state == gloom::network::ConnectionState::failed) {
                        slice_remote_connection = gloom::network::invalid_connection;
                        slice_admission_deadline.reset();
                        slice_reconnect_pending = true;
                        audio_waiting_for_snapshot=true;
                        if(game_audio)game_audio->scene_reset();
                        slice_reconnect_at = current_frame + std::chrono::seconds{1};
                        std::cout << "Authoritative slice connection lost ("
                                  << event->description << "); retrying in one second.\n";
                    }
                }
            }
            while (auto packet = transport_view->receive()) {
                const auto decoded = gloom::network::decode_message(packet->payload);
                if (!decoded) {
                    continue;
                }
                if (vertical_slice_host) {
                    for (const auto& response : slice_remote_host->receive(
                             packet->connection, *decoded, network_now)) {
                        const auto bytes = gloom::network::encode_message(response.message);
                        transport_view->send(response.connection,
                                             {.payload = bytes,
                                              .delivery = response.delivery});
                    }
                } else if (packet->connection == slice_remote_connection) {
                    const auto previous_lobby_revision =
                        slice_remote_client->lobby().revision;
                    if (auto response = slice_remote_client->receive(*decoded, network_now)) {
                        const auto bytes = gloom::network::encode_message(response->message);
                        transport_view->send(slice_remote_connection,
                                             {.payload = bytes,
                                              .delivery = response->delivery});
                    }
                    if (decoded->kind == gloom::network::MessageKind::server_welcome) {
                        slice_admission_deadline.reset();audio_waiting_for_snapshot=true;
                        if(game_audio)game_audio->scene_reset();
                    }
                    if(decoded->kind==gloom::network::MessageKind::gameplay_snapshot&&slice_remote_client->has_snapshot()&&slice_remote_client->snapshot().simulation_tick==decoded->simulation_tick)
                        audio_waiting_for_snapshot=false;
                    if (slice_remote_client->lobby().revision > previous_lobby_revision) {
                        print_lobby(slice_remote_client->lobby());
                        if (!interactive_slice_selection || slice_selection_confirmed) {
                            window_view->set_title(lobby_title(slice_remote_client->lobby()));
                        }
                    }
                }
            }
            if (slice_admission_deadline && current_frame >= *slice_admission_deadline) {
                ui_notice="No se pudo entrar en la sala. Comprueba la dirección y el acceso, o reintenta.";
                const auto failed_connection = slice_remote_connection;
                slice_remote_connection = gloom::network::invalid_connection;
                slice_admission_deadline.reset();
                slice_reconnect_pending = false;
                if (vertical_slice_browse) slice_match_selected = false;
                transport_view->disconnect(failed_connection);
                window_view->set_relative_mouse_mode(false);
                window_view->set_title("Gloom game admission timed out; select again or restart to retry");
                std::cerr << "Game admission timed out; account, ticket or session was not accepted.\n";
            }
        }
        if (vertical_slice) {
            const auto& slice = current_slice_snapshot();
            slice_feedback.advance(elapsed);
            slice_feedback.observe(slice);
            const auto forward = slice_first_person.forward();
            camera.position = {slice.player.position_x,
                               slice.player.position_y + 0.9F,
                               slice.player.position_z};
            camera.target = {camera.position.x + forward.x,
                             camera.position.y + forward.y,
                             camera.position.z + forward.z};
            if(game_audio){const auto now=std::chrono::steady_clock::now();
                if(audio_waiting_for_snapshot)game_audio->menu(std::chrono::duration<double>(now-audio_time).count());
                else game_audio->update(slice,{.position={camera.position.x,camera.position.y,camera.position.z},
                    .forward={forward.x,forward.y,forward.z},.velocity={slice.player.velocity_x,slice.player.velocity_y,slice.player.velocity_z}},
                    game_ui&&game_ui->paused,std::chrono::duration<double>(now-audio_time).count());audio_time=now;}
        }
        const auto physics_stats = physics_view->statistics();
        if (!vulkan_sync_stress) {
            if (vertical_slice && factory_lift_visual) {
                const auto& lift = current_slice_snapshot().factory_lift;
                auto& visual = visual_bodies[*factory_lift_visual];
                visual.previous = visual.current;
                visual.current.position = {lift.position_x, lift.position_y,
                                           lift.position_z};
            }
            if (physics_stats.last_sub_steps > 0) {
                jobs_view->parallel_for(
                    physics_to_render_group, visual_bodies.size(), 3, [&](const std::size_t begin, const std::size_t end) {
                        for (std::size_t index = begin; index < end; ++index) {
                            auto& visual = visual_bodies[index];
                            if (visual.body.valid()) {
                                visual.previous = visual.current;
                                visual.current = to_render_transform(
                                    physics_view->body_transform(visual.body),
                                    visual.current.scale);
                            }
                        }
                    });
                jobs_view->wait(physics_to_render_group);
            }
            jobs_view->parallel_for(
                snapshot_group, visual_bodies.size(), 3, [&](const std::size_t begin, const std::size_t end) {
                    for (std::size_t index = begin; index < end; ++index) {
                        const auto& visual = visual_bodies[index];
                        const auto previous_render_transform = render_instances[index].transform;
                        render_instances[index] = {
                            .mesh = visual.mesh,
                            .transform = gloom::render::interpolate(
                                visual.previous,
                                visual.current,
                                static_cast<float>(physics_stats.interpolation_alpha)),
                            .previous_transform = previous_render_transform,
                            .has_previous_transform = frame_count != 0,
                            .color = visual.color,
                        };
                    }
                });
            jobs_view->wait(snapshot_group);
            if (network_demo) {
                const auto character = physics_view->character_state(local_character);
                const auto& predicted = network_client->local_state();
                const float correction_x = predicted.position_x - character.position.x;
                const float correction_y = predicted.position_y - character.position.y;
                const float correction_z = predicted.position_z - character.position.z;
                if (correction_x * correction_x + correction_y * correction_y +
                        correction_z * correction_z >
                    0.0625F) {
                    physics_view->set_character_position(
                        local_character,
                        {predicted.position_x, predicted.position_y, predicted.position_z});
                }
                const double server_time =
                    static_cast<double>(network_server->simulation_tick()) /
                    static_cast<double>(replication_settings.tick_rate);
                if (const auto remote =
                        network_client->sample_remote(remote_network_entity, server_time)) {
                    remote_render_state = remote->state;
                }
                const auto presented_character = presentation_smoother.apply(
                    {.entity = local_network_entity,
                     .position_x = character.position.x,
                     .position_y = character.position.y,
                     .position_z = character.position.z});
                const auto local_index = visual_bodies.size();
                const auto remote_index = local_index + 1;
                const auto previous_local = render_instances[local_index].transform;
                const auto previous_remote = render_instances[remote_index].transform;
                render_instances[local_index] = {
                    .mesh = gloom::render::builtin_cube_mesh,
                    .transform = {.position = {presented_character.position_x,
                                               presented_character.position_y + 0.9F,
                                               presented_character.position_z},
                                  .scale = {0.4F, 0.9F, 0.4F}},
                    .previous_transform = previous_local,
                    .has_previous_transform = frame_count != 0,
                    .color = {0.15F, 0.55F, 1.0F, 1.0F},
                };
                render_instances[remote_index] = {
                    .mesh = gloom::render::builtin_cube_mesh,
                    .transform = {.position = {remote_render_state.position_x,
                                               remote_render_state.position_y + 0.9F,
                                               remote_render_state.position_z},
                                  .scale = {0.4F, 0.9F, 0.4F}},
                    .previous_transform = previous_remote,
                    .has_previous_transform = frame_count != 0,
                    .color = {1.0F, 0.42F, 0.12F, 1.0F},
                };
            } else if (vertical_slice) {
                const auto& slice = current_slice_snapshot();
                const auto base = visual_bodies.size();
                const float forward_x = slice.opponent.facing_x;
                const float forward_z = slice.opponent.facing_z;
                const float right_x = -forward_z;
                const float right_z = forward_x;
                const float yaw = std::atan2(forward_x, forward_z);
                const auto& feedback = slice_feedback.state();
                const bool damage_flash = feedback.opponent_damage_remaining_seconds > 0.0F &&
                                          std::fmod(feedback.opponent_damage_remaining_seconds *
                                                        50.0F,
                                                    2.0F) >= 1.0F;
                const gloom::render::Quaternion character_rotation{
                    0.0F, std::sin(yaw * 0.5F), 0.0F, std::cos(yaw * 0.5F)};
                const auto recipe = gloom::gameplay::character_presentation_recipe(
                    slice.opponent.character);
                const auto make_character_part = [&] (
                    const std::size_t part,
                    const gloom::gameplay::CharacterPresentationPart& recipe_part) {
                    const float forward_offset = recipe_part.forward_offset;
                    const float right_offset = recipe_part.right_offset;
                    const float height = recipe_part.height;
                    gloom::render::Vec3 scale{
                        recipe_part.scale_x, recipe_part.scale_y, recipe_part.scale_z};
                    const auto character_index =
                        static_cast<std::size_t>(recipe.character);
                    const auto mesh = character_meshes[character_index][part];
                    if (mesh != gloom::render::builtin_cube_mesh) {
                        scale.y *= 4.0F;
                    }
                    auto color = gloom::render::Color{
                        recipe_part.red, recipe_part.green, recipe_part.blue, 1.0F};
                    if (recipe_part.ability_tint &&
                        slice.opponent.primary_ability_active) {
                        color = slice.opponent.ability == gloom::gameplay::SliceAbility::guard
                                    ? gloom::render::Color{0.08F, 0.38F, 0.78F, 1.0F}
                                    : gloom::render::Color{0.72F, 0.11F, 0.025F, 1.0F};
                    }
                    const auto index = base + part;
                    const auto previous = render_instances[index].transform;
                    const bool dead = !slice.opponent.alive;
                    const auto presented_scale = dead
                        ? gloom::render::Vec3{scale.x, std::max(0.025F, scale.y * 0.08F), scale.z}
                        : scale;
                    const float presented_height = dead ? 0.055F + height * 0.055F : height;
                    const auto presented_color = [&] {
                        if (dead) {
                            return feedback.opponent_death_remaining_seconds > 0.0F
                                ? gloom::render::Color{0.75F, 0.035F, 0.01F, 1.0F}
                                : gloom::render::Color{0.07F, 0.012F, 0.01F, 1.0F};
                        }
                        if (feedback.opponent_respawn_remaining_seconds > 0.0F) {
                            return gloom::render::Color{0.30F, 0.82F, 1.0F, 1.0F};
                        }
                        if (damage_flash) {
                            return gloom::render::Color{1.0F, 0.72F, 0.48F, 1.0F};
                        }
                        return color;
                    }();
                    render_instances[index] = {
                        .mesh = mesh,
                        .transform = {.position = {
                                          slice.opponent.position_x +
                                              forward_x * forward_offset +
                                              right_x * right_offset,
                                          slice.opponent.position_y + presented_height,
                                          slice.opponent.position_z +
                                              forward_z * forward_offset +
                                              right_z * right_offset},
                                      .rotation = character_rotation,
                                      .scale = presented_scale},
                        .previous_transform = previous,
                        .has_previous_transform = frame_count != 0,
                        .color = presented_color,
                    };
                };
                const auto hide = [&](const std::size_t part) {
                    render_instances[base + part] = {
                        .mesh = gloom::render::builtin_cube_mesh,
                        .transform = {.scale = {}},
                    };
                };
                for (std::size_t part = 0; part < recipe.fallback_parts.size(); ++part) {
                    make_character_part(part, recipe.fallback_parts[part]);
                }

                // A small world-space reticle follows the exact authoritative aim ray.
                const auto aim = slice_first_person.forward();
                render_instances[base + 9] = {
                    .mesh = gloom::render::builtin_cube_mesh,
                    .transform = {.position = {camera.position.x + aim.x * 2.0F,
                                               camera.position.y + aim.y * 2.0F,
                                               camera.position.z + aim.z * 2.0F},
                                  .scale = slice.hud.dead
                                               ? gloom::render::Vec3{0.05F, 0.05F, 0.05F}
                                               : gloom::render::Vec3{0.012F, 0.012F, 0.012F}},
                    .color = slice.hud.dead
                                 ? gloom::render::Color{0.65F, 0.0F, 0.0F, 1.0F}
                                  : slice.hud.hit_marker
                                  ? gloom::render::Color{1.0F, 0.12F, 0.06F, 1.0F}
                                  : slice.hud.primary_ability_active
                                  ? slice.player.ability == gloom::gameplay::SliceAbility::guard
                                        ? gloom::render::Color{0.16F, 0.62F, 1.0F, 1.0F}
                                        : gloom::render::Color{1.0F, 0.48F, 0.04F, 1.0F}
                                  : gloom::render::Color{1.0F, 1.0F, 1.0F, 1.0F},
                };
                const float opponent_life = std::clamp(
                    slice.opponent.life / gloom::gameplay::legacy_default_life, 0.0F, 1.0F);
                render_instances[base + 10] = {
                    .mesh = gloom::render::builtin_cube_mesh,
                    .transform = {.position = {slice.opponent.position_x,
                                               slice.opponent.position_y + 2.05F,
                                               slice.opponent.position_z},
                                  .scale = slice.opponent.alive
                                               ? gloom::render::Vec3{0.65F, 0.06F, 0.06F}
                                               : gloom::render::Vec3{}},
                    .color = {0.03F, 0.03F, 0.03F, 1.0F},
                };
                render_instances[base + 11] = {
                    .mesh = gloom::render::builtin_cube_mesh,
                    .transform = {.position = {slice.opponent.position_x -
                                                   0.65F * (1.0F - opponent_life),
                                               slice.opponent.position_y + 2.12F,
                                               slice.opponent.position_z - 0.01F},
                                  .scale = slice.opponent.alive
                                               ? gloom::render::Vec3{0.65F * opponent_life,
                                                                    0.055F,
                                                                    0.055F}
                                               : gloom::render::Vec3{}},
                    .color = {0.95F, 0.08F, 0.04F, 1.0F},
                };
                const auto view_prop = [&](std::size_t part, gloom::render::RenderAssetId mesh,
                                           gloom::render::Vec3 offset, gloom::render::Vec3 scale,
                                           gloom::render::Color color) {
                    const auto previous = render_instances[base + part].transform;
                    render_instances[base + part] = {
                        .mesh = mesh,
                        .transform = gloom::render::camera_relative_transform(camera, offset, scale),
                        .previous_transform = previous,
                        .has_previous_transform = frame_count != 0,
                        .color = color,
                        .view_model = true,
                    };
                };
                if (slice.hud.primary_ability_active &&
                    slice.player.ability == gloom::gameplay::SliceAbility::bite) {
                    for (std::size_t hand = 0; hand < 2; ++hand)
                        view_prop(12 + hand, ability_meshes[0], {hand == 0 ? -0.24F : 0.24F, -0.20F, 0.58F},
                                  {0.06F, ability_meshes[0] == gloom::render::builtin_cube_mesh ? 0.07F : 0.28F, 0.16F},
                                  {1.0F, 0.12F, 0.02F, 1.0F});
                } else if (slice.hud.primary_ability_active &&
                           slice.player.ability == gloom::gameplay::SliceAbility::guard) {
                    for (std::size_t hand = 0; hand < 2; ++hand)
                        view_prop(12 + hand, ability_meshes[1], {hand == 0 ? -0.25F : 0.25F, -0.12F, 0.57F},
                                  {0.10F, ability_meshes[1] == gloom::render::builtin_cube_mesh ? 0.16F : 0.64F, 0.035F},
                                  {0.08F, 0.46F, 1.0F, 1.0F});
                } else {
                    view_prop(12, weapon_meshes[0], {0.22F, -0.23F, 0.58F},
                              {0.085F, weapon_meshes[0] == gloom::render::builtin_cube_mesh ? 0.08F : 0.32F, 0.20F},
                              {0.24F, 0.27F, 0.32F, 1.0F});
                    const bool muzzle_flash = slice.hud.weapon_ready_fraction < 0.34F && !slice.hud.dead;
                    view_prop(13, weapon_meshes[1], {0.22F, -0.20F, 0.88F},
                              {0.032F, weapon_meshes[1] == gloom::render::builtin_cube_mesh ? 0.032F : 0.128F, 0.19F},
                              muzzle_flash ? gloom::render::Color{1.0F, 0.34F, 0.025F, 1.0F}
                                           : gloom::render::Color{0.46F, 0.24F, 0.12F, 1.0F});
                }
                // Four short-lived dots form a confirmed-hit marker around the
                // aim point. They are driven only by the authoritative HUD bit.
                const float horizontal_aim_length =
                    std::sqrt(aim.x * aim.x + aim.z * aim.z);
                const float marker_right_x = -aim.z / horizontal_aim_length;
                const float marker_right_z = aim.x / horizontal_aim_length;
                const gloom::render::Vec3 marker_up{
                    -marker_right_z * aim.y,
                    marker_right_z * aim.x - marker_right_x * aim.z,
                    marker_right_x * aim.y};
                constexpr std::array marker_offsets{
                    std::pair{-1.0F, -1.0F}, std::pair{-1.0F, 1.0F},
                    std::pair{1.0F, -1.0F}, std::pair{1.0F, 1.0F}};
                for (std::size_t marker = 0; marker < marker_offsets.size(); ++marker) {
                    if (!slice.hud.hit_marker) {
                        hide(14 + marker);
                        continue;
                    }
                    const auto [right_sign, up_sign] = marker_offsets[marker];
                    render_instances[base + 14 + marker] = {
                        .mesh = gloom::render::builtin_cube_mesh,
                        .transform = {
                            .position = {
                                camera.position.x + aim.x * 1.98F +
                                    marker_right_x * right_sign * 0.038F +
                                    marker_up.x * up_sign * 0.038F,
                                camera.position.y + aim.y * 1.98F +
                                    marker_up.y * up_sign * 0.038F,
                                camera.position.z + aim.z * 1.98F +
                                    marker_right_z * right_sign * 0.038F +
                                    marker_up.z * up_sign * 0.038F},
                            .scale = {0.010F, 0.010F, 0.010F}},
                        .color = feedback.opponent_death_remaining_seconds > 0.0F
                                     ? gloom::render::Color{1.0F, 0.72F, 0.08F, 1.0F}
                                     : gloom::render::Color{1.0F, 0.12F, 0.04F, 1.0F},
                    };
                }
                // Reticle, health bars and hit markers are presentation, not occluders.
                for (std::size_t part = 9; part < 18; ++part)
                    render_instances[base + part].casts_shadow = false;
            }
        }
        const auto current_drawable_size = window_view->drawable_size();
        if (current_drawable_size != drawable_size) {
            drawable_size = current_drawable_size;
            renderer_view->resize(drawable_size.first, drawable_size.second);
        }
        renderer_view->begin_frame();
        if (frame_count == 0 &&
            (renderer_view->asset_state(gloom::render::builtin_cube_mesh) !=
                 gloom::render::GpuAssetState::resident ||
             renderer_view->asset_state(gloom::render::builtin_default_material) !=
                 gloom::render::GpuAssetState::resident ||
             renderer_view->asset_state(gloom::render::builtin_white_texture) !=
                 gloom::render::GpuAssetState::resident ||
             renderer_view->asset_state(gloom::render::builtin_flat_normal_texture) !=
                 gloom::render::GpuAssetState::resident ||
             (smoke_test && renderer_view->asset_state(streaming_smoke_texture) !=
                                gloom::render::GpuAssetState::resident))) {
            throw std::runtime_error{"Built-in GPU assets did not become resident"};
        }
        auto complete_instances = render_instances;
        if(vertical_slice) {
            const gloom::gameplay::SliceSnapshot& state=current_slice_snapshot();
            for(std::size_t i=0;i<state.projectile_count;++i) {
                const gloom::gameplay::WeaponProjectileView& p=state.projectiles[i];
                const bool fireball=p.weapon==gloom::gameplay::SliceWeapon::iron_hell_goat;
                const gloom::render::Color color=fireball?gloom::render::Color{2.5F,.4F,.025F,1}:gloom::render::Color{.25F,.85F,1,1};
                const float scale=fireball?p.radius*.28F:p.radius;
                complete_instances.push_back({.mesh=gloom::render::builtin_cube_mesh,
                    .transform={.position={p.position_x,p.position_y,p.position_z},.scale={scale,scale,scale}},
                    .color=color,.particle=true,.soft_distance=.2F});
                if(fireball)particles.emitter(0x100000000ULL+p.id,0x100000000ULL+p.id,"fireball_trail",{p.position_x,p.position_y,p.position_z});
            }
        }
        if(game_ui && complete_instances.size()>=visual_bodies.size()+18){
            complete_instances[visual_bodies.size()+9].transform.scale={};
            for(std::size_t i=14;i<18;++i)complete_instances[visual_bodies.size()+i].transform.scale={};
        }
        if (vertical_slice && original_characters && (!visual_review || animation_review)) {
            const auto& slice=current_slice_snapshot();
            const auto base=visual_bodies.size();
            const auto resident_character=[&](gloom::gameplay::SliceCharacter character) -> const gloom::assets::ResidentScene* {
                const auto i=static_cast<std::size_t>(character);
                return character_tickets[i]?character_residencies[i]->scene(*character_tickets[i]):nullptr;
            };
            const auto* opponent_scene=resident_character(slice.opponent.character);
            const auto* local_scene=resident_character(slice.player.character);
            const auto* local_weapon_scene=resident_weapon(slice.player.weapon);
            const auto* opponent_weapon_scene=resident_weapon(slice.opponent.weapon);
            std::vector<gloom::render::RenderInstance> current;
            const auto append=[&](const gloom::assets::ResidentScene& scene,const gloom::render::Transform& parent,
                                  bool fps,const gloom::gameplay::CharacterAnimationFrame* frame,bool cut) {
                for (auto instance:scene.instances) {
                    if (frame && scene.bind_rig) {
                        if (fps) {
                            if (!instance.arms_mesh.value) continue;
                            instance.mesh=instance.arms_mesh;instance.lod_count=1;instance.lod_meshes={};
                        }
                        const auto& rig=*scene.bind_rig;
                        instance.transform=gloom::assets::rig_transform(frame->worlds[instance.source_node]);
                        instance.pose=gloom::assets::skin_pose(rig,instance.source_node,frame->worlds);
                        instance.local_bounds=gloom::assets::skinned_bounds(rig.primitives[instance.source_primitive],*instance.pose);
                    }
                    instance.transform=gloom::render::attach_transform(parent,instance.transform);
                    instance.view_model=fps;instance.casts_shadow=!fps;
                    const auto i=current.size();
                    instance.has_previous_transform=!cut && i<previous_animated_instances.size() &&
                        previous_animated_instances[i].mesh==instance.mesh && previous_animated_instances[i].view_model==fps;
                    if (instance.has_previous_transform) {
                        instance.previous_transform=previous_animated_instances[i].transform;
                        instance.previous_pose=previous_animated_instances[i].pose;
                    }
                    current.push_back(instance);
                }
            };
            if (opponent_scene && opponent_scene->bind_rig) {
                for (std::size_t i=0;i<9;++i) complete_instances[base+i].transform.scale={};
                const auto frame=opponent_animator.update(*opponent_scene->bind_rig,slice.opponent,elapsed);
                const float yaw=std::atan2(slice.opponent.facing_x,slice.opponent.facing_z);
                const gloom::render::Transform parent{
                    .position={slice.opponent.position_x,slice.opponent.position_y,slice.opponent.position_z},
                    .rotation={0,std::sin(yaw*.5F),0,std::cos(yaw*.5F)}};
                append(*opponent_scene,parent,false,&frame,frame.cut);
                combat_effects.observe(particles,slice.opponent,frame,parent,slice.simulation_tick,false);
                if (opponent_weapon_scene && slice.opponent.alive)
                    append(*opponent_weapon_scene,gloom::render::attach_transform(parent,frame.weapon),false,nullptr,frame.cut);
            }
            if (local_weapon_scene && local_scene && local_scene->bind_rig) {
                const auto frame=local_animator.update(*local_scene->bind_rig,slice.player,elapsed,false,false,true);
                const auto parent=gloom::render::camera_relative_transform(camera,{.039F,-1.106F,.355F},{.70F,.70F,.70F});
                combat_effects.observe(particles,slice.player,frame,parent,slice.simulation_tick,true);
                if (!slice.hud.dead && !slice.hud.primary_ability_active) {
                    complete_instances[base+12].transform.scale={};complete_instances[base+13].transform.scale={};
                    append(*local_scene,parent,true,&frame,frame.cut);
                    append(*local_weapon_scene,gloom::render::attach_transform(parent,frame.weapon),true,nullptr,frame.cut);
                }
            }
            previous_animated_instances=current;
            complete_instances.insert(complete_instances.end(),current.begin(),current.end());
            if (original_factory) {
                const auto& lava=gloom::gameplay::original_factory();
                const int cell_x=static_cast<int>(std::floor(camera.position.x/4)),cell_z=static_cast<int>(std::floor(camera.position.z/4));
                for (int x=cell_x-1;x<=cell_x+1;++x) for (int z=cell_z-1;z<=cell_z+1;++z) {
                    const gloom::render::Vec3 p{static_cast<float>(x)*4,lava.lava_center.y+.04F,static_cast<float>(z)*4};
                    if (std::abs(p.x-lava.lava_center.x)>lava.lava_half_width || std::abs(p.z-lava.lava_center.z)>lava.lava_half_width) continue;
                    const std::uint64_t id=0x100000000ULL+static_cast<std::uint64_t>(x+2048)*8192+static_cast<std::uint64_t>(z+2048)*2;
                    particles.emitter(id,0,"lava",p);particles.emitter(id+1,0,"heat",p);
                }
            }
            if (animation_review && review_ready) {
                const auto f=review_frames%static_cast<std::size_t>(animation_fps*10);
                const gloom::render::Vec3 sample{slice.opponent.position_x+(effects_review?-.8F:.8F),.65F,slice.opponent.position_z+.4F};
                if (f==static_cast<std::size_t>(animation_fps*(effects_review?1:6))) particles.burst(0,"blood_review",sample,{0,1,0},6301);
                if (f==static_cast<std::size_t>(animation_fps*(effects_review?2:7))) particles.burst(0,"explosion_review",sample,{0,1,0},6302);
                if (effects_review || (f>=static_cast<std::size_t>(animation_fps*6) && f<static_cast<std::size_t>(animation_fps*8)))
                    particles.emitter(0x200000000ULL,0,"heat",sample);
            }
            particles.advance(std::min(elapsed,.25));
            if (effects_ticket) if (const auto* scene=effects_residency->scene(*effects_ticket)) {
                const auto effects=particles.render(camera,scene->instances);
                complete_instances.insert(complete_instances.end(),effects.begin(),effects.end());
            }
        }
        if (vertical_slice && original_characters && visual_review && !animation_review) {
            const auto& slice=current_slice_snapshot();
            const auto base=visual_bodies.size();
            const auto character_index=static_cast<std::size_t>(slice.opponent.character);
            const auto* character_scene=character_tickets[character_index]
                ? character_residencies[character_index]->scene(*character_tickets[character_index]) : nullptr;
            const auto* weapon_scene=resident_weapon(slice.opponent.weapon);
            const auto* local_weapon_scene=resident_weapon(slice.player.weapon);
            std::vector<gloom::render::Transform> current_transforms;
            const auto append=[&](const gloom::assets::ResidentScene& scene,const gloom::render::Transform& parent,
                                  bool first_person,gloom::render::Color tint=gloom::render::Color{}) {
                for (auto instance:scene.instances) {
                    instance.transform=gloom::render::attach_transform(parent,instance.transform);
                    const auto i=current_transforms.size();
                    instance.has_previous_transform=previous_original_character==slice.opponent.character && i<previous_original_transforms.size();
                    if (instance.has_previous_transform) instance.previous_transform=previous_original_transforms[i];
                    current_transforms.push_back(instance.transform);
                    instance.view_model=first_person;instance.casts_shadow=!first_person;instance.color=tint;
                    complete_instances.push_back(instance);
                }
            };
            if (character_scene) {
                for (std::size_t i=0;i<9;++i) complete_instances[base+i].transform.scale={};
                const float yaw=std::atan2(slice.opponent.facing_x,slice.opponent.facing_z);
                const float size=slice.opponent.alive ? 1.0F : .05F;
                const gloom::render::Transform parent{
                    .position={slice.opponent.position_x,slice.opponent.position_y,slice.opponent.position_z},
                    .rotation={0,std::sin(yaw*.5F),0,std::cos(yaw*.5F)},.scale={size,size,size}};
                const auto& feedback=slice_feedback.state();
                const auto tint=!slice.opponent.alive ? gloom::render::Color{.15F,.025F,.02F,1}
                    : feedback.opponent_respawn_remaining_seconds>0 ? gloom::render::Color{.3F,.82F,1,1}
                    : feedback.opponent_damage_remaining_seconds>0 ? gloom::render::Color{1,.65F,.65F,1}
                    : slice.opponent.primary_ability_active ? (slice.opponent.ability==gloom::gameplay::SliceAbility::guard
                        ? gloom::render::Color{.16F,.62F,1,1} : gloom::render::Color{1,.48F,.12F,1})
                    : gloom::render::Color{};
                append(*character_scene,parent,false,tint);
                if (weapon_scene && slice.opponent.alive && character_scene->bind_rig) {
                    const auto& rig=*character_scene->bind_rig;
                    const auto matrices=gloom::assets::bind_node_transforms(rig);
                    for (std::size_t i=0;matrices && i<rig.nodes.size();++i) if (rig.nodes[i].name=="Bip001 R Hand") {
                        const auto& hand=(*matrices)[i];
                        // Grip offset is art metadata; it never relocates an authoritative ray.
                        const gloom::render::Transform grip{.position={hand[12]-.035F,hand[13]-.07F,hand[14]+.12F},.scale={.43F,.43F,.43F}};
                        append(*weapon_scene,gloom::render::attach_transform(parent,grip),false);
                        break;
                    }
                }
            }
            if (local_weapon_scene && !slice.hud.primary_ability_active) {
                complete_instances[base+12].transform.scale={};complete_instances[base+13].transform.scale={};
                if (!slice.hud.dead) {
                const auto hand=gloom::render::camera_relative_transform(camera,{.20F,-.28F,.60F},{.30F,.30F,.30F});
                append(*local_weapon_scene,hand,true);
                if (slice.hud.weapon_ready_fraction<.34F) {
                    const auto muzzle=gloom::render::attach_transform(hand,{.position={.13F,.40F,.90F}});
                    complete_instances.push_back({.transform={.position=muzzle.position,.scale={.015F,.015F,.035F}},
                        .color={1,.55F,.08F,1},.view_model=true,.casts_shadow=false});
                }
                }
            }
            previous_original_transforms=std::move(current_transforms);
            previous_original_character=slice.opponent.character;
        }
        if (vertical_slice && original_factory && factory_surface_residency && factory_surface_ticket) {
            if (const auto* scene = factory_surface_residency->scene(*factory_surface_ticket)) {
                const auto& definitions=gloom::gameplay::original_factory().pickups;
                const auto& state=current_slice_snapshot();
                for(auto instance:scene->instances) {
                    const auto found=std::ranges::find(definitions,instance.source_node,&gloom::gameplay::PickupDefinition::source_node);
                    if(found!=definitions.end() && instance.source_node!=~0U && !factory_review) {
                        const auto i=static_cast<std::size_t>(found-definitions.begin());
                        if(i>=state.pickup_count || state.pickups[i].phase==gloom::gameplay::PickupPhase::respawning)continue;
                        const auto& p=state.pickups[i].position;
                        instance.transform.position.x+=p.x-found->position.x;
                        instance.transform.position.y+=p.y-found->position.y;
                        instance.transform.position.z+=p.z-found->position.z;
                    }
                    complete_instances.push_back(instance);
                }
                // Original modifier archetypes are particles. The skinned MiniGun
                // uses the already resident arsenal mesh omitted by the map importer.
                if(!factory_review)for(std::size_t i=0;i<state.pickup_count;++i) {
                    if(definitions[i].source_node!=~0U || state.pickups[i].phase==gloom::gameplay::PickupPhase::respawning)continue;
                    const auto& p=state.pickups[i].position;
                    if(definitions[i].kind==gloom::gameplay::PickupKind::weapon) {
                        if(const auto* weapon=resident_weapon(definitions[i].weapon)) {
                            for(auto instance:weapon->instances) {
                                instance.transform=gloom::render::attach_transform({.position={p.x,p.y,p.z}},instance.transform);
                                complete_instances.push_back(instance);
                            }
                            continue;
                        }
                    }
                    const auto color=definitions[i].kind==gloom::gameplay::PickupKind::damage?
                        gloom::render::Color{1,.18F,.08F,1}:gloom::render::Color{.15F,.6F,1,1};
                    complete_instances.push_back({.mesh=gloom::render::builtin_cube_mesh,
                        .transform={.position={p.x,p.y,p.z},.scale={.22F,.22F,.22F}},.color=color,.particle=true,.soft_distance=.15F});
                }
            }
        }
        const auto source_instances = std::span<const gloom::render::RenderInstance>{complete_instances};
        const float render_aspect = drawable_size.second == 0
                                        ? 1.0F
                                        : static_cast<float>(drawable_size.first) /
                                              static_cast<float>(drawable_size.second);
        const auto visible = visibility.build(camera, render_aspect, source_instances);
        const auto lighting = lighting_builder.build(camera, render_aspect, point_lights,
            vertical_slice && original_factory ? gloom::render::DirectionalLight{.intensity=1.08F} : gloom::render::DirectionalLight{},
            vertical_slice ? gloom::render::EnvironmentLighting{
                .sky_radiance = {0.20F, 0.24F, 0.30F},
                .ground_radiance = {0.08F, 0.075F, 0.07F},
                .intensity = original_factory ? 0.35F : 1.0F,
                .probe = original_factory ? gloom::gameplay::original_factory().environment_probe : nullptr,
                .ambient_fill = original_factory ? 0.65F : 0.0F} : gloom::render::EnvironmentLighting{});
        const auto lighting_view = lighting.view();
        auto snapshot = visible.snapshot();
        snapshot.presentation_seconds = visual_review && !animation_review ? 45.0F : static_cast<float>(presentation_seconds);
        snapshot.lighting = &lighting_view;
        snapshot.shadow_instances = source_instances;
        if(game_ui)snapshot.ui=&game_ui->canvas.data();
        renderer_view->draw(snapshot);
        if(ui_flow && frame_count>15 && frame_count%8==5 && !ui_flow_captured[ui_flow_stage]){
            renderer_view->capture_next_frame(desktop_session->flow_output/("game-stage-"+std::to_string(ui_flow_stage)+".ppm"));ui_flow_captured[ui_flow_stage]=true;
        }
        if (review_ready && animation_review && (review_frames==0 ||
            review_frames*30/static_cast<std::size_t>(animation_fps)!=(review_frames-1)*30/static_cast<std::size_t>(animation_fps))) {
            const auto number=review_frames*30/static_cast<std::size_t>(animation_fps);
            auto name=std::to_string(number);name=std::string(4-name.size(),'0')+name;
            renderer_view->capture_next_frame(review_output/("frame-"+name+".ppm"));
        }
        if (review_ready && !animation_review && review_frames % 32 == 31)
            renderer_view->capture_next_frame(review_output / (std::string{review_view.name} + ".ppm"));
        renderer_view->end_frame();
        if (review_ready && ++review_frames == (animation_review?static_cast<std::size_t>(animation_fps*animation_seconds):review_views.size()*32)) break;
        if (visual_review && !review_ready && frame_count > 1800) throw std::runtime_error{"Visual review assets did not become resident"};
        if (smoke_test && frame_count == 0) {
            renderer_view->release(streaming_smoke_texture);
            const auto enqueue_budget_texture = [&](const gloom::render::RenderAssetId id) {
                renderer_view->enqueue(gloom::render::TextureUpload{
                    .id = id,
                    .mip_levels = {{.width = 16,
                                    .height = 16,
                                    .data = std::vector<std::byte>(1024, std::byte{0x5a})}},
                });
            };
            enqueue_budget_texture(streaming_budget_texture_a);
            enqueue_budget_texture(streaming_budget_texture_b);
        }
        ++frame_count;

        if (smoke_test && frame_count == 3) {
            const auto residency = renderer_view->residency_metrics();
            if (renderer_view->asset_state(streaming_smoke_texture) !=
                    gloom::render::GpuAssetState::missing ||
                residency.uploaded_bytes == 0 || residency.deferred_releases == 0 ||
                residency.completed_releases == 0 || residency.budget_limited_frames == 0 ||
                residency.evictions == 0 ||
                renderer_view->asset_state(streaming_budget_texture_a) !=
                    gloom::render::GpuAssetState::missing ||
                renderer_view->asset_state(streaming_budget_texture_b) !=
                    gloom::render::GpuAssetState::resident) {
                throw std::runtime_error{"GPU streaming release did not complete safely"};
            }
        }
        if (smoke_test && frame_count == 8) {
            const auto capabilities = renderer_view->capabilities();
            const auto frame_metrics = renderer_view->frame_metrics();
            if (frame_metrics.point_lights != point_lights.size() ||
                !capabilities.temporal.motion_vectors || !capabilities.temporal.taa ||
                !capabilities.temporal.depth_disocclusion ||
                !capabilities.temporal.reactive_history ||
                !capabilities.temporal.contrast_adaptive_sharpening ||
                !capabilities.temporal.dynamic_resolution ||
                frame_metrics.render_width == 0 || frame_metrics.render_height == 0 ||
                frame_metrics.render_width >= frame_metrics.output_width ||
                frame_metrics.render_height >= frame_metrics.output_height ||
                frame_metrics.output_width != drawable_size.first ||
                frame_metrics.output_height != drawable_size.second ||
                !frame_metrics.temporal_history_valid ||
                (capabilities.gpu_timestamps &&
                 frame_metrics.dynamic_resolution_changes == 0) ||
                (capabilities.gpu_timestamps &&
                 (!frame_metrics.gpu_timing_supported ||
                  (frame_metrics.shadow_nanoseconds == 0 &&
                   frame_metrics.opaque_nanoseconds == 0 &&
                   frame_metrics.tone_map_nanoseconds == 0 &&
                   frame_metrics.temporal_resolve_nanoseconds == 0)))) {
                throw std::runtime_error{
                    "Modern lighting or temporal telemetry did not become available"};
            }
        }

        if (vulkan_sync_stress && frame_count == 60) {
            window_view->set_minimized(true);
            renderer_view->resize(0, 0);
        } else if (vulkan_sync_stress && frame_count == 90) {
            window_view->set_minimized(false);
            drawable_size = window_view->drawable_size();
            renderer_view->resize(drawable_size.first, drawable_size.second);
        }

        if ((smoke_test && frame_count >= 10) ||
            (network_scene_smoke && frame_count >= 180) ||
            (vertical_slice_smoke && frame_count >= 360) ||
            (vulkan_sync_stress && frame_count >= 600)) {
            break;
        }
        if (!vulkan_sync_stress && !network_scene_smoke && !vertical_slice_smoke && !visual_review) {
            std::this_thread::sleep_for(std::chrono::milliseconds{16});
        }
    }

    const auto job_metrics = jobs_view->metrics();
    if(ui_flow){
        if(ui_flow_stage!=8 || !ui_return_to_menu || !ui_flow_entity)throw std::runtime_error{"UI flow did not finish through the leave confirmation"};
        std::cout<<"UI flow passed: browser -> selection -> ready -> play -> reconnect -> leave; entity="<<ui_flow_entity<<", character="<<static_cast<unsigned>(slice_remote_client->desired_selection().character)<<'\n';
    }
    if (visual_review && review_frames != (animation_review?static_cast<std::size_t>(animation_fps*animation_seconds):review_views.size()*32))
        throw std::runtime_error{"Visual review closed before every view was captured"};
    if (vertical_slice_smoke && (!slice_saw_kill || !slice_saw_respawn)) {
        throw std::runtime_error{"Vertical slice did not complete its kill/respawn loop"};
    }
    if (vertical_slice_smoke && factory_lift_generation == 0) {
        throw std::runtime_error{"Authored Factory cargo lift did not become resident"};
    }
    if (vertical_slice_smoke && factory_surface_generation == 0) {
        throw std::runtime_error{"Authored Factory surface modules did not become resident"};
    }
    if (vertical_slice_smoke &&
        (character_generations[0] == 0 || character_generations[1] == 0)) {
        throw std::runtime_error{"Authored roster characters did not become resident"};
    }
    if (vertical_slice_smoke && (weapon_generation == 0 || ability_generation == 0)) {
        throw std::runtime_error{"Authored first-person content did not become resident"};
    }
    if (vertical_slice && !vertical_slice_smoke && !vertical_slice_host) {
        window_view->set_relative_mouse_mode(false);
    }
    factory_surface_residency.reset();
    factory_surface_loader.reset();
    factory_surface_catalog.reset();
    factory_residency.reset();
    factory_asset_loader.reset();
    factory_catalog.reset();
    for (auto& residency : character_residencies) {
        residency.reset();
    }
    for (auto& loader : character_loaders) {
        loader.reset();
    }
    for (auto& catalog : character_catalogs) {
        catalog.reset();
    }
    ability_residency.reset();
    ability_loader.reset();
    ability_catalog.reset();
    combat_effects.reset(particles);
    effects_residency.reset();effects_loader.reset();effects_catalog.reset();
    for(auto& value:arsenal_residencies)value.reset();for(auto& value:arsenal_loaders)value.reset();for(auto& value:arsenal_catalogs)value.reset();
    weapon_residency.reset();
    weapon_loader.reset();
    weapon_catalog.reset();
    factory_filesystem.reset();
    if(desktop_session && !ui_return_to_menu)desktop_session->quit=true;
    engine.stop();

    std::cout << "Gloom Vulkan/Jolt scene completed successfully.\n";
    std::cout << "Jobs: " << job_metrics.completed_jobs << " completed, "
              << job_metrics.caller_executed_jobs << " helped by the waiting thread, peak queue "
              << job_metrics.peak_queue_depth << ", "
              << job_metrics.job_execution_nanoseconds / 1'000 << " us executing and "
              << job_metrics.wait_nanoseconds / 1'000 << " us waiting.\n";
    if (network_demo) {
        const auto& reconciliation = network_client->reconciliation_metrics();
        const auto& smoothing = presentation_smoother.metrics();
        const auto& client_inputs = network_client->input_metrics();
        const auto& server_inputs = network_server->input_metrics();
        const auto& snapshots = network_server->snapshot_metrics();
        const auto& combat = combat_server->metrics();
        std::cout << "Network corrections: " << reconciliation.count << ", maximum "
                  << reconciliation.maximum_distance << " m, accumulated "
                  << reconciliation.accumulated_distance << " m; presentation smoothed "
                  << smoothing.smoothed_corrections << " and snapped "
                  << smoothing.snapped_corrections << ".\n";
        std::cout << "Input batches: " << client_inputs.batches << " sent with "
                  << client_inputs.commands << " commands (maximum "
                  << client_inputs.maximum_batch_size << "), "
                  << server_inputs.redundant_commands << " redundant commands accepted and "
                  << server_inputs.duplicate_commands << " duplicates discarded.\n";
        std::cout << "Snapshots: " << snapshots.full_snapshots << " full, "
                  << snapshots.delta_snapshots << " delta, " << snapshots.records_encoded
                  << " records and " << snapshots.payload_bytes << " payload bytes.\n";
        std::cout << "Combat history: " << combat_server->history_size() << " retained of "
                  << combat.history_frames << " recorded frames, " << combat.accepted_commands
                  << " validated shots, " << combat.hits << " hits and " << combat.misses
                  << " misses.\n";
    }
    if (animation_review) std::cout<<"Animation review: "<<animation_fps<<" FPS, "<<review_frames<<" frames, "<<combat_effects.events()<<" events, particles spawned="<<particles.metrics().spawned<<", expired="<<particles.metrics().expired<<", dropped="<<particles.metrics().dropped<<'\n';
    if (vertical_slice) {
        const auto& slice = current_slice_snapshot();
        std::cout << "Vertical slice: " << slice.player.kills << " kills, "
                  << slice.player.deaths << " deaths, opponent life "
                  << slice.opponent.life << ".\n";
        std::cout << "Authoritative sessions: " << slice.network.active_sessions
                  << " active, " << slice.network.authorized_input_batches
                  << " input batches and " << slice.network.authorized_fire_commands
                  << " fire commands authorized, " << slice.network.rejected_commands
                  << " rejected, " << slice.network.reconciliation_count
                  << " reconciliations.\n";
        if (vertical_slice_host || vertical_slice_join) {
            const auto link = transport_view->metrics();
            std::cout << "Remote slice transport: " << link.sent_packets << " packets/"
                      << link.sent_bytes << " bytes sent, " << link.received_packets
                      << " packets/" << link.received_bytes << " bytes received";
            if (vertical_slice_host) {
                std::cout << ", " << slice_remote_host->active_clients()
                          << " remote clients active";
            } else {
                std::cout << ", client session "
                          << (slice_remote_client->active() ? "active" : "inactive");
                const auto& correction =
                    slice_remote_client->reconciliation_metrics();
                std::cout << ", correction max " << correction.maximum_distance
                          << " m, accumulated " << correction.accumulated_distance << " m";
            }
            std::cout << ".\n";
        }
    }
    return 0;
} catch (const std::exception& error) {
    if(desktop_session)throw;
    std::cerr << "Gloom startup failed: " << error.what() << '\n';
    return 1;
}

int main(int argc,const char* const* argv) try {
    if(argc>1 && std::string_view{argv[1]}=="--audio-review"){
        if(argc<3||argc>4)throw std::invalid_argument{"Usage: gloom --audio-review DIR [--device]"};
        return gloom::review::audio_review(argv[2],argc==4&&std::string_view{argv[3]}=="--device");
    }
    if(argc>1 && std::string_view{argv[1]}=="--ui-flow-review"){
        if(argc!=6)throw std::invalid_argument{"Usage: gloom --ui-flow-review ENDPOINT OUTPUT NAME archangel|shadow"};
        gloom::desktop::Session session;session.flow_output=argv[3];session.flow_name=argv[4];session.flow_selection=std::string_view{argv[5]}=="shadow"?4:3;
        std::filesystem::create_directories(session.flow_output);
        session.directory=gloom::gameplay::make_in_memory_match_directory();
        const auto now=static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
        const auto advertised=session.directory->advertise({.match_id="ui-factory",.instance_id="ui-review",.display_name="Factory UI acceptance",.endpoint=argv[2]},now,600000);
        if(!advertised)throw std::runtime_error{advertised.error()};
        auto launch=gloom::desktop::menu(session);if(launch.empty())throw std::runtime_error{"UI review did not enter the selected match"};
        launch.insert(launch.begin(),"gloom");std::vector<const char*> arguments;for(const auto& a:launch)arguments.push_back(a.c_str());
        return run_game(static_cast<int>(arguments.size()),arguments.data(),&session);
    }
    if(argc>1 && std::string_view{argv[1]}=="--ui-review"){
        if(argc!=3)throw std::invalid_argument{"Usage: gloom --ui-review OUTPUT"};
        gloom::desktop::Session session;
        for(const auto size:std::array{std::pair{1280U,720U},std::pair{1920U,1080U},std::pair{2560U,1080U}})
            static_cast<void>(gloom::desktop::menu(session,std::filesystem::path{argv[2]}/std::to_string(size.first),size.first,size.second));
        return 0;
    }
    if(argc>1 && std::string_view{argv[1]}!="--desktop")return run_game(argc,argv);
    gloom::desktop::Session session;
    while(!session.quit){
        auto launch=gloom::desktop::menu(session);if(launch.empty())break;
        launch.insert(launch.begin(),"gloom");std::vector<const char*> arguments;
        for(const auto& argument:launch)arguments.push_back(argument.c_str());
        try{static_cast<void>(run_game(static_cast<int>(arguments.size()),arguments.data(),&session));}
        catch(const std::exception& e){session.notice=e.what();}
    }
    return 0;
}catch(const std::exception& e){std::cerr<<"Gloom: "<<e.what()<<'\n';return 1;}
