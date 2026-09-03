cmake_minimum_required(VERSION 3.28)

project(Gloom VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
include(FetchContent)
find_package(SDL3 CONFIG REQUIRED)
find_package(Jolt CONFIG REQUIRED)
find_package(GameNetworkingSockets CONFIG REQUIRED)
find_package(fastgltf CONFIG REQUIRED)
find_package(Ktx CONFIG REQUIRED)
find_package(meshoptimizer CONFIG REQUIRED)
find_package(mikktspace CONFIG REQUIRED)
find_package(Stb REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(simdjson CONFIG REQUIRED)

set(HTTPLIB_REQUIRE_OPENSSL ON CACHE BOOL "" FORCE)
set(HTTPLIB_USE_ZLIB_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
set(HTTPLIB_USE_BROTLI_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
set(HTTPLIB_USE_ZSTD_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
FetchContent_Declare(httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG 0d3a3d805c8979e3220b42fcbc0ff95980eabc06
)
FetchContent_MakeAvailable(httplib)

set(DILIGENT_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(DILIGENT_NO_DIRECT3D11 ON CACHE BOOL "" FORCE)
set(DILIGENT_NO_DIRECT3D12 ON CACHE BOOL "" FORCE)
set(DILIGENT_NO_OPENGL ON CACHE BOOL "" FORCE)
set(DILIGENT_NO_VULKAN OFF CACHE BOOL "" FORCE)
set(DILIGENT_NO_WEBGPU ON CACHE BOOL "" FORCE)
set(DILIGENT_NO_ARCHIVER ON CACHE BOOL "" FORCE)
set(DILIGENT_MSVC_RELEASE_COMPILE_OPTIONS "" CACHE STRING "" FORCE)

FetchContent_Declare(
    DiligentCore
    GIT_REPOSITORY https://github.com/DiligentGraphics/DiligentCore.git
    # v2.5.6 baseline plus the upstream Vulkan swapchain semaphore fix (#682).
    GIT_TAG a255d24d365e13edeade3b02676f444a493bc883
    GIT_SUBMODULES_RECURSE TRUE
)
FetchContent_MakeAvailable(DiligentCore)

foreach(unused_target IN ITEMS
    Diligent-GraphicsEngineVk-shared
    Diligent-GraphicsTools
    SPIRV-Tools-diff
    glslang-default-resource-limits
)
    if(TARGET ${unused_target})
        set_property(TARGET ${unused_target} PROPERTY EXCLUDE_FROM_ALL TRUE)
    endif()
endforeach()

option(GLOOM_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

add_library(gloom_engine STATIC
    src/assets/asset.cpp
    src/assets/animation.cpp
    src/assets/asset_loader.cpp
    src/assets/gltf_importer.cpp
    src/assets/residency_coordinator.cpp
    src/assets/scene_catalog.cpp
    src/assets/scene_gpu_bridge.cpp
    src/assets/texture_asset.cpp
    src/core/engine.cpp
    src/core/entity.cpp
    src/core/job_system.cpp
    src/gameplay/vertical_slice.cpp
    src/gameplay/character_animation.cpp
    src/gameplay/combat_effects.cpp
    src/gameplay/factory_scene.cpp
    src/gameplay/match_lobby.cpp
    src/gameplay/match_discovery.cpp
    src/gameplay/first_person.cpp
    src/gameplay/vertical_slice_network.cpp
    src/network/clock_sync.cpp
    src/network/combat.cpp
    src/network/movement_replication.cpp
    src/network/network_simulator.cpp
    src/network/protocol.cpp
    src/network/session.cpp
    src/render/scene.cpp
    src/render/particles.cpp
    src/render/temporal.cpp
    src/render/lighting.cpp
    src/render/visibility.cpp
    src/render/render_graph.cpp
    src/backends/placeholder.cpp
    src/backends/sdl_window.cpp
    src/backends/diligent_renderer.cpp
    src/backends/jolt_world.cpp
    src/backends/gns_transport.cpp
    src/backends/winhttp_match_service.cpp
)

target_include_directories(gloom_engine
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)

target_compile_features(gloom_engine PUBLIC cxx_std_23)
target_compile_definitions(gloom_engine PRIVATE GLOOM_SOURCE_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(gloom_engine PRIVATE simdjson::simdjson)
add_library(gloom_match_storage STATIC
    src/gameplay/durable_match_service.cpp
    src/gameplay/match_service_wire.cpp
)
target_include_directories(gloom_match_storage PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_features(gloom_match_storage PUBLIC cxx_std_23)
add_library(gloom_match_identity STATIC src/backends/keycloak_identity.cpp
    src/backends/game_ticket.cpp
    src/backends/winhttp_identity.cpp src/backends/match_reader_grants.cpp)
target_include_directories(gloom_match_identity PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_features(gloom_match_identity PUBLIC cxx_std_23)
target_link_libraries(gloom_match_identity PRIVATE simdjson::simdjson OpenSSL::Crypto)
if(WIN32)
    target_link_libraries(gloom_match_identity PRIVATE winhttp)
endif()
if(MSVC)
    target_compile_options(gloom_match_identity PRIVATE /W4 /WX /EHsc /permissive-)
endif()
target_link_libraries(gloom_engine PUBLIC gloom_match_storage gloom_match_identity)
if(MSVC)
    target_compile_options(gloom_match_storage PRIVATE /W4 /WX /EHsc /permissive-)
endif()
target_link_libraries(gloom_engine
    PRIVATE
        SDL3::SDL3
        Diligent-Common
        Diligent-GraphicsEngineVk-static
        Diligent-GraphicsTools
        Jolt::Jolt
        GameNetworkingSockets::GameNetworkingSockets
        KTX::ktx
)
if(WIN32)
    target_link_libraries(gloom_engine PRIVATE winhttp)
endif()

if(MSVC)
    target_compile_options(gloom_engine PRIVATE /W4 /permissive- /EHsc /Zc:__cplusplus)
    if(GLOOM_WARNINGS_AS_ERRORS)
        target_compile_options(gloom_engine PRIVATE /WX)
    endif()
else()
    target_compile_options(gloom_engine PRIVATE -Wall -Wextra -Wpedantic)
    if(GLOOM_WARNINGS_AS_ERRORS)
        target_compile_options(gloom_engine PRIVATE -Werror)
    endif()
endif()

add_library(gloom_texture_cooker OBJECT src/assets/texture_asset.cpp)
target_include_directories(gloom_texture_cooker PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${Stb_INCLUDE_DIR}"
)
target_compile_features(gloom_texture_cooker PRIVATE cxx_std_23)
target_compile_definitions(gloom_texture_cooker PRIVATE GLOOM_TEXTURE_COOKER=1)
target_link_libraries(gloom_texture_cooker PRIVATE KTX::ktx)

add_library(gloom_gltf_importer OBJECT src/assets/gltf_importer.cpp)
target_include_directories(gloom_gltf_importer PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)
target_compile_features(gloom_gltf_importer PRIVATE cxx_std_23)
target_compile_definitions(gloom_gltf_importer PRIVATE GLOOM_GLTF_IMPORTER=1)
target_link_libraries(gloom_gltf_importer PRIVATE fastgltf::fastgltf simdjson::simdjson)
target_link_libraries(gloom_gltf_importer PRIVATE meshoptimizer::meshoptimizer mikktspace::mikktspace)

add_library(gloom_asset_pipeline STATIC
    src/assets/asset_cooker.cpp
    src/assets/mesh_processing.cpp
    $<TARGET_OBJECTS:gloom_gltf_importer>
    $<TARGET_OBJECTS:gloom_texture_cooker>
)
target_compile_features(gloom_asset_pipeline PUBLIC cxx_std_23)
target_link_libraries(gloom_asset_pipeline
    PUBLIC gloom_engine
    PRIVATE fastgltf::fastgltf
            KTX::ktx
            meshoptimizer::meshoptimizer
            mikktspace::mikktspace
)
if(MSVC)
    target_compile_options(
        gloom_asset_pipeline PRIVATE /W4 /permissive- /EHsc /Zc:__cplusplus
    )
    if(GLOOM_WARNINGS_AS_ERRORS)
        target_compile_options(gloom_asset_pipeline PRIVATE /WX)
    endif()
else()
    target_compile_options(gloom_asset_pipeline PRIVATE -Wall -Wextra -Wpedantic)
    if(GLOOM_WARNINGS_AS_ERRORS)
        target_compile_options(gloom_asset_pipeline PRIVATE -Werror)
    endif()
endif()

add_executable(gloom apps/gloom/main.cpp)
target_link_libraries(gloom PRIVATE gloom_engine)
target_compile_definitions(gloom PRIVATE
    GLOOM_SOURCE_ROOT="${CMAKE_CURRENT_SOURCE_DIR}"
    GLOOM_BINARY_ROOT="${CMAKE_CURRENT_BINARY_DIR}"
)

add_executable(gloom_slice_server apps/gloom_slice_server/main.cpp)
target_link_libraries(gloom_slice_server PRIVATE gloom_engine)

add_library(gloom_match_https STATIC src/backends/match_https_host.cpp)
target_link_libraries(gloom_match_https PUBLIC gloom_match_storage gloom_match_identity PRIVATE httplib::httplib)
target_compile_features(gloom_match_https PUBLIC cxx_std_23)
add_executable(gloom_match_service apps/gloom_match_service/main.cpp)
target_link_libraries(gloom_match_service PRIVATE gloom_match_https)
add_custom_target(gloom_match_service_package
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/package/match-service/$<CONFIG>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:gloom_match_service>
            "${CMAKE_BINARY_DIR}/package/match-service/$<CONFIG>"
    COMMAND ${CMAKE_COMMAND}
            "-DOPENSSL_BIN=${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/$<IF:$<CONFIG:Debug>,debug/bin,bin>"
            "-DPACKAGE_DIR=${CMAKE_BINARY_DIR}/package/match-service/$<CONFIG>"
            -P "${CMAKE_SOURCE_DIR}/cmake/PackageMatchRuntime.cmake"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/docs/MATCH_SERVICE.md"
            "${CMAKE_BINARY_DIR}/package/match-service/$<CONFIG>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/docs/KEYCLOAK.md"
            "${CMAKE_BINARY_DIR}/package/match-service/$<CONFIG>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${CMAKE_SOURCE_DIR}/docs/GAME_AUTH.md"
            "${CMAKE_BINARY_DIR}/package/match-service/$<CONFIG>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${httplib_SOURCE_DIR}/LICENSE"
            "${CMAKE_BINARY_DIR}/package/match-service/$<CONFIG>/cpp-httplib-LICENSE"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/openssl/copyright"
            "${CMAKE_BINARY_DIR}/package/match-service/$<CONFIG>/OpenSSL-LICENSE"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/simdjson/copyright"
            "${CMAKE_BINARY_DIR}/package/match-service/$<CONFIG>/simdjson-LICENSE"
    DEPENDS gloom_match_service
    COMMAND_EXPAND_LISTS VERBATIM
)
if(MSVC)
    target_compile_options(gloom_match_https PRIVATE /W4 /WX /EHsc /permissive-)
    target_compile_options(gloom_match_service PRIVATE /W4 /WX /EHsc /permissive-)
endif()

add_executable(gloom_asset_cooker apps/gloom_asset_cooker/main.cpp)
target_link_libraries(gloom_asset_cooker PRIVATE gloom_asset_pipeline)

set(GLOOM_FACTORY_CONTENT_DIR "${CMAKE_CURRENT_BINARY_DIR}/content")
set(GLOOM_FACTORY_LIFT_ASSET
    "${GLOOM_FACTORY_CONTENT_DIR}/factory/cargo_lift.gasset")
set(GLOOM_FACTORY_SURFACE_ASSET
    "${GLOOM_FACTORY_CONTENT_DIR}/factory/surface_modules.gasset")
set(GLOOM_HOUND_CHARACTER_ASSET
    "${GLOOM_FACTORY_CONTENT_DIR}/characters/hound.gasset")
set(GLOOM_BERSERKER_CHARACTER_ASSET
    "${GLOOM_FACTORY_CONTENT_DIR}/characters/berserker.gasset")
set(GLOOM_SOUL_REAPER_ASSET
    "${GLOOM_FACTORY_CONTENT_DIR}/weapons/soul_reaper.gasset")
set(GLOOM_HOUND_ABILITIES_ASSET
    "${GLOOM_FACTORY_CONTENT_DIR}/abilities/hound_abilities.gasset")
set(GLOOM_EFFECTS_ASSET "${GLOOM_FACTORY_CONTENT_DIR}/effects/effects.gasset")
file(GLOB GLOOM_EFFECT_TEXTURES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/effects/textures/*")
add_custom_command(OUTPUT "${GLOOM_EFFECTS_ASSET}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${GLOOM_FACTORY_CONTENT_DIR}/effects"
    COMMAND $<TARGET_FILE:gloom_asset_cooker> "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "${GLOOM_FACTORY_CONTENT_DIR}" game:/effects/effects.gltf cache:/effects/effects.gasset
    DEPENDS gloom_asset_cooker "${CMAKE_CURRENT_SOURCE_DIR}/assets/effects/effects.gltf"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/effects/effects.bin" ${GLOOM_EFFECT_TEXTURES} VERBATIM)
add_custom_command(
    OUTPUT "${GLOOM_FACTORY_LIFT_ASSET}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${GLOOM_FACTORY_CONTENT_DIR}/factory"
    COMMAND $<TARGET_FILE:gloom_asset_cooker>
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "${GLOOM_FACTORY_CONTENT_DIR}"
            game:/factory/cargo_lift.gltf
            cache:/factory/cargo_lift.gasset
    DEPENDS gloom_asset_cooker
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/factory/cargo_lift.gltf"
    VERBATIM
)
add_custom_command(
    OUTPUT "${GLOOM_FACTORY_SURFACE_ASSET}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${GLOOM_FACTORY_CONTENT_DIR}/factory"
    COMMAND $<TARGET_FILE:gloom_asset_cooker>
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "${GLOOM_FACTORY_CONTENT_DIR}"
            game:/factory/surface_modules.gltf
            cache:/factory/surface_modules.gasset
    DEPENDS gloom_asset_cooker
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/factory/surface_modules.gltf"
    VERBATIM
)
add_custom_command(
    OUTPUT "${GLOOM_HOUND_CHARACTER_ASSET}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${GLOOM_FACTORY_CONTENT_DIR}/characters"
    COMMAND $<TARGET_FILE:gloom_asset_cooker>
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "${GLOOM_FACTORY_CONTENT_DIR}"
            game:/characters/hound.gltf
            cache:/characters/hound.gasset
    DEPENDS gloom_asset_cooker
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/characters/hound.gltf"
    VERBATIM
)
add_custom_command(
    OUTPUT "${GLOOM_BERSERKER_CHARACTER_ASSET}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${GLOOM_FACTORY_CONTENT_DIR}/characters"
    COMMAND $<TARGET_FILE:gloom_asset_cooker>
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "${GLOOM_FACTORY_CONTENT_DIR}"
            game:/characters/berserker.gltf
            cache:/characters/berserker.gasset
    DEPENDS gloom_asset_cooker
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/characters/berserker.gltf"
    VERBATIM
)
add_custom_command(
    OUTPUT "${GLOOM_SOUL_REAPER_ASSET}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${GLOOM_FACTORY_CONTENT_DIR}/weapons"
    COMMAND $<TARGET_FILE:gloom_asset_cooker>
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "${GLOOM_FACTORY_CONTENT_DIR}"
            game:/weapons/soul_reaper.gltf
            cache:/weapons/soul_reaper.gasset
    DEPENDS gloom_asset_cooker
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/weapons/soul_reaper.gltf"
    VERBATIM
)
add_custom_command(
    OUTPUT "${GLOOM_HOUND_ABILITIES_ASSET}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory
            "${GLOOM_FACTORY_CONTENT_DIR}/abilities"
    COMMAND $<TARGET_FILE:gloom_asset_cooker>
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "${GLOOM_FACTORY_CONTENT_DIR}"
            game:/abilities/hound_abilities.gltf
            cache:/abilities/hound_abilities.gasset
    DEPENDS gloom_asset_cooker
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/abilities/hound_abilities.gltf"
    VERBATIM
)
file(GLOB GLOOM_ORIGINAL_TEXTURES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/legacy/textures/*.png")
add_custom_command(OUTPUT "${GLOOM_FACTORY_CONTENT_DIR}/legacy/factory.gasset"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${GLOOM_FACTORY_CONTENT_DIR}/legacy"
    COMMAND $<TARGET_FILE:gloom_asset_cooker> "${CMAKE_CURRENT_SOURCE_DIR}/assets" "${GLOOM_FACTORY_CONTENT_DIR}"
            game:/legacy/factory.gltf cache:/legacy/factory.gasset
    DEPENDS gloom_asset_cooker "${CMAKE_CURRENT_SOURCE_DIR}/assets/legacy/factory.gltf"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/legacy/factory.bin" ${GLOOM_ORIGINAL_TEXTURES}
    VERBATIM)
file(GLOB GLOOM_CHARACTER_TEXTURES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/assets/characters/original/textures/*.png")
foreach(character archangel shadow soul_reaper)
    set(character_source "${CMAKE_CURRENT_SOURCE_DIR}/assets/characters/original/${character}")
    add_custom_command(OUTPUT "${GLOOM_FACTORY_CONTENT_DIR}/characters/original/${character}.gasset"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${GLOOM_FACTORY_CONTENT_DIR}/characters/original"
        COMMAND $<TARGET_FILE:gloom_asset_cooker> "${CMAKE_CURRENT_SOURCE_DIR}/assets" "${GLOOM_FACTORY_CONTENT_DIR}"
                "game:/characters/original/${character}.gltf" "cache:/characters/original/${character}.gasset"
        DEPENDS gloom_asset_cooker "${character_source}.gltf" "${character_source}.bin" ${GLOOM_CHARACTER_TEXTURES}
        VERBATIM)
endforeach()
add_custom_command(OUTPUT "${GLOOM_FACTORY_CONTENT_DIR}/legacy/soul_reaper.gasset"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${GLOOM_FACTORY_CONTENT_DIR}/legacy"
    COMMAND $<TARGET_FILE:gloom_asset_cooker> "${CMAKE_CURRENT_SOURCE_DIR}/assets" "${GLOOM_FACTORY_CONTENT_DIR}"
            game:/legacy/soul_reaper.gltf cache:/legacy/soul_reaper.gasset
    DEPENDS gloom_asset_cooker "${CMAKE_CURRENT_SOURCE_DIR}/assets/legacy/soul_reaper.gltf"
            "${CMAKE_CURRENT_SOURCE_DIR}/assets/legacy/soul_reaper.bin" ${GLOOM_ORIGINAL_TEXTURES} VERBATIM)
add_custom_target(gloom_project_content ALL DEPENDS "${GLOOM_FACTORY_CONTENT_DIR}/legacy/factory.gasset"
    "${GLOOM_FACTORY_CONTENT_DIR}/legacy/soul_reaper.gasset"
    "${GLOOM_FACTORY_CONTENT_DIR}/characters/original/archangel.gasset"
    "${GLOOM_FACTORY_CONTENT_DIR}/characters/original/shadow.gasset"
    "${GLOOM_FACTORY_CONTENT_DIR}/characters/original/soul_reaper.gasset"
    "${GLOOM_FACTORY_LIFT_ASSET}"
    "${GLOOM_FACTORY_SURFACE_ASSET}"
    "${GLOOM_HOUND_CHARACTER_ASSET}"
    "${GLOOM_BERSERKER_CHARACTER_ASSET}"
    "${GLOOM_SOUL_REAPER_ASSET}"
    "${GLOOM_HOUND_ABILITIES_ASSET}"
    "${GLOOM_EFFECTS_ASSET}"
)
add_dependencies(gloom gloom_project_content)

add_executable(gloom_scene_viewer apps/gloom_scene_viewer/main.cpp)
target_link_libraries(gloom_scene_viewer PRIVATE gloom_engine)

if(BUILD_TESTING)
    if(WIN32)
        add_executable(gloom_keycloak_identity_tests tests/keycloak_identity_tests.cpp)
        add_executable(gloom_game_ticket_tests tests/game_ticket_tests.cpp)
        target_link_libraries(gloom_game_ticket_tests PRIVATE gloom_engine OpenSSL::Crypto)
        add_test(NAME gloom.game_tickets COMMAND gloom_game_ticket_tests)
        set_tests_properties(gloom.game_tickets PROPERTIES LABELS network TIMEOUT 30)
        target_link_libraries(gloom_keycloak_identity_tests PRIVATE gloom_match_https httplib::httplib)
        add_test(NAME gloom.keycloak_identity COMMAND gloom_keycloak_identity_tests)
        set_tests_properties(gloom.keycloak_identity PROPERTIES LABELS network TIMEOUT 40)
        add_executable(gloom_match_https_tests tests/match_https_tests.cpp)
        target_link_libraries(gloom_match_https_tests PRIVATE gloom_match_https httplib::httplib)
        add_dependencies(gloom_match_https_tests gloom_match_service)
        add_test(NAME gloom.match_https COMMAND gloom_match_https_tests $<TARGET_FILE:gloom_match_service>)
        set_tests_properties(gloom.match_https PROPERTIES LABELS network TIMEOUT 60)
    endif()
    add_executable(gloom_tests tests/engine_tests.cpp)
    target_link_libraries(gloom_tests PRIVATE gloom_engine)
    add_test(NAME gloom.unit COMMAND gloom_tests)

    add_executable(gloom_job_tests tests/job_system_tests.cpp)
    target_link_libraries(gloom_job_tests PRIVATE gloom_engine)
    add_test(NAME gloom.jobs COMMAND gloom_job_tests)

    add_executable(gloom_entity_tests tests/entity_tests.cpp)
    target_link_libraries(gloom_entity_tests PRIVATE gloom_engine)
    add_test(NAME gloom.entities COMMAND gloom_entity_tests)

    add_executable(gloom_physics_tests tests/physics_tests.cpp)
    target_link_libraries(gloom_physics_tests PRIVATE gloom_engine)
    add_test(NAME gloom.physics COMMAND gloom_physics_tests)

    add_executable(gloom_render_scene_tests tests/render_scene_tests.cpp)
    target_link_libraries(gloom_render_scene_tests PRIVATE gloom_engine)
    add_test(NAME gloom.render_scene COMMAND gloom_render_scene_tests)

    add_executable(gloom_render_graph_tests tests/render_graph_tests.cpp)
    target_link_libraries(gloom_render_graph_tests PRIVATE gloom_engine)
    add_test(NAME gloom.render_graph COMMAND gloom_render_graph_tests)

    add_executable(gloom_gpu_asset_tests tests/gpu_asset_tests.cpp)
    target_link_libraries(gloom_gpu_asset_tests PRIVATE gloom_engine)
    add_test(NAME gloom.gpu_assets COMMAND gloom_gpu_asset_tests)

    add_executable(gloom_visibility_tests tests/visibility_tests.cpp)
    target_link_libraries(gloom_visibility_tests PRIVATE gloom_engine)
    add_test(NAME gloom.visibility COMMAND gloom_visibility_tests)

    add_executable(gloom_lighting_tests tests/lighting_tests.cpp)
    target_link_libraries(gloom_lighting_tests PRIVATE gloom_engine)
    add_test(NAME gloom.lighting COMMAND gloom_lighting_tests)

    add_executable(gloom_temporal_tests tests/temporal_tests.cpp)
    target_link_libraries(gloom_temporal_tests PRIVATE gloom_engine)
    add_test(NAME gloom.temporal COMMAND gloom_temporal_tests)

    add_executable(gloom_vertical_slice_tests tests/vertical_slice_tests.cpp)
    target_link_libraries(gloom_vertical_slice_tests PRIVATE gloom_engine)
    add_test(NAME gloom.vertical_slice COMMAND gloom_vertical_slice_tests)

    add_executable(gloom_vertical_slice_network_tests tests/vertical_slice_network_tests.cpp)
    target_link_libraries(gloom_vertical_slice_network_tests PRIVATE gloom_engine)
    add_test(NAME gloom.vertical_slice_network COMMAND gloom_vertical_slice_network_tests)

    add_executable(gloom_match_lobby_tests tests/match_lobby_tests.cpp)
    target_link_libraries(gloom_match_lobby_tests PRIVATE gloom_engine)
    add_test(NAME gloom.match_lobby COMMAND gloom_match_lobby_tests)

    add_executable(gloom_match_discovery_tests tests/match_discovery_tests.cpp)
    target_link_libraries(gloom_match_discovery_tests PRIVATE gloom_engine)
    add_test(NAME gloom.match_discovery COMMAND gloom_match_discovery_tests)

    add_executable(gloom_vertical_slice_transport_tests tests/vertical_slice_transport_tests.cpp)
    target_link_libraries(gloom_vertical_slice_transport_tests PRIVATE gloom_engine)
    add_test(NAME gloom.vertical_slice_transport COMMAND gloom_vertical_slice_transport_tests)
    set_tests_properties(gloom.vertical_slice_transport PROPERTIES LABELS network TIMEOUT 15)

    add_test(NAME gloom.slice_server_smoke
             COMMAND gloom_slice_server 127.0.0.1:0 --ticks 3)
    set_tests_properties(gloom.slice_server_smoke PROPERTIES LABELS network TIMEOUT 5)

    add_executable(gloom_network_tests tests/network_transport_tests.cpp)
    target_link_libraries(gloom_network_tests PRIVATE gloom_engine)
    add_test(NAME gloom.network COMMAND gloom_network_tests)
    set_tests_properties(gloom.network PROPERTIES TIMEOUT 15)

    add_executable(gloom_network_protocol_tests tests/network_protocol_tests.cpp)
    target_link_libraries(gloom_network_protocol_tests PRIVATE gloom_engine)
    add_test(NAME gloom.network_protocol COMMAND gloom_network_protocol_tests)

    add_executable(gloom_network_replication_tests tests/network_replication_tests.cpp)
    target_link_libraries(gloom_network_replication_tests PRIVATE gloom_engine)
    add_test(NAME gloom.network_replication COMMAND gloom_network_replication_tests)

    add_executable(gloom_combat_tests tests/combat_tests.cpp)
    target_link_libraries(gloom_combat_tests PRIVATE gloom_engine)
    add_test(NAME gloom.combat COMMAND gloom_combat_tests)

    add_executable(gloom_session_tests tests/session_tests.cpp)
    target_link_libraries(gloom_session_tests PRIVATE gloom_engine)
    add_test(NAME gloom.session COMMAND gloom_session_tests)

    add_executable(gloom_factory_restoration_tests tests/factory_restoration_tests.cpp)
    add_executable(gloom_character_restoration_tests tests/character_restoration_tests.cpp)
    add_executable(gloom_animation_vfx_tests tests/animation_vfx_tests.cpp)
    target_link_libraries(gloom_animation_vfx_tests PRIVATE gloom_asset_pipeline)
    target_compile_definitions(gloom_animation_vfx_tests PRIVATE GLOOM_TEST_ASSETS="${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_test(NAME gloom.animation_vfx COMMAND gloom_animation_vfx_tests)
    add_executable(gloom_animation_network_tests tests/animation_network_tests.cpp)
    target_link_libraries(gloom_animation_network_tests PRIVATE gloom_asset_pipeline)
    target_compile_definitions(gloom_animation_network_tests PRIVATE GLOOM_TEST_ASSETS="${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_test(NAME gloom.animation_network COMMAND gloom_animation_network_tests)
    set_tests_properties(gloom.animation_network PROPERTIES LABELS network TIMEOUT 90)
    find_program(GLOOM_POWERSHELL NAMES pwsh powershell REQUIRED)
    add_test(NAME gloom.animation_dedicated COMMAND "${GLOOM_POWERSHELL}" -NoProfile -ExecutionPolicy Bypass
        -File "${CMAKE_CURRENT_SOURCE_DIR}/tools/review/animation_dedicated.ps1"
        -Server "$<TARGET_FILE:gloom_slice_server>" -Clients "$<TARGET_FILE:gloom_animation_network_tests>"
        -Output "${CMAKE_CURRENT_BINARY_DIR}/animation-dedicated/$<CONFIG>")
    set_tests_properties(gloom.animation_dedicated PROPERTIES LABELS network TIMEOUT 95)

    target_link_libraries(gloom_character_restoration_tests PRIVATE gloom_asset_pipeline)
    target_compile_definitions(gloom_character_restoration_tests PRIVATE GLOOM_TEST_ASSETS="${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_test(NAME gloom.character_restoration COMMAND gloom_character_restoration_tests)
    add_executable(gloom_material_render_tests tests/material_render_tests.cpp)
    target_link_libraries(gloom_material_render_tests PRIVATE gloom_engine)
    add_test(NAME gloom.material_render COMMAND "${CMAKE_COMMAND}"
        "-DGLOOM_EXECUTABLE=$<TARGET_FILE:gloom_material_render_tests>"
        "-DGLOOM_CAPTURE_DIR=${CMAKE_BINARY_DIR}/material-review"
        -P "${CMAKE_SOURCE_DIR}/cmake/RunMaterialReview.cmake")
    set_tests_properties(gloom.material_render PROPERTIES LABELS "graphics;visual" TIMEOUT 60)
    target_link_libraries(gloom_factory_restoration_tests PRIVATE gloom_asset_pipeline)
    target_compile_definitions(gloom_factory_restoration_tests PRIVATE GLOOM_TEST_ASSETS="${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_test(NAME gloom.factory_restoration COMMAND gloom_factory_restoration_tests)

    add_executable(gloom_asset_tests tests/asset_pipeline_tests.cpp)
    target_link_libraries(gloom_asset_tests PRIVATE gloom_asset_pipeline)
    target_compile_definitions(gloom_asset_tests PRIVATE GLOOM_TEST_ASSETS="${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_test(NAME gloom.assets COMMAND gloom_asset_tests)
    set_tests_properties(gloom.assets PROPERTIES TIMEOUT 20)

    add_test(NAME gloom.sdl_smoke COMMAND gloom --smoke-test)
    add_executable(gloom_visual_capture_tests tests/visual_capture_tests.cpp)
    target_compile_features(gloom_visual_capture_tests PRIVATE cxx_std_23)
    target_include_directories(gloom_visual_capture_tests PRIVATE include apps/gloom)
    add_test(NAME gloom.visual_review COMMAND "${CMAKE_COMMAND}"
        "-DGLOOM_EXECUTABLE=$<TARGET_FILE:gloom>"
        "-DGLOOM_COMPARATOR=$<TARGET_FILE:gloom_visual_capture_tests>"
        "-DGLOOM_CAPTURE_DIR=${CMAKE_BINARY_DIR}/visual-review/$<CONFIG>"
        "-DGLOOM_REFERENCE_DIR=${CMAKE_SOURCE_DIR}/tests/visual-references"
        -P "${CMAKE_SOURCE_DIR}/cmake/RunVisualReview.cmake")
    set_tests_properties(gloom.visual_review PROPERTIES LABELS "graphics;visual" TIMEOUT 50)
    add_test(NAME gloom.factory_visual_review COMMAND "${CMAKE_COMMAND}"
        "-DGLOOM_EXECUTABLE=$<TARGET_FILE:gloom>"
        "-DGLOOM_COMPARATOR=$<TARGET_FILE:gloom_visual_capture_tests>"
        "-DGLOOM_CAPTURE_DIR=${CMAKE_BINARY_DIR}/factory-review/$<CONFIG>"
        "-DGLOOM_REFERENCE_DIR=${CMAKE_SOURCE_DIR}/tests/factory-visual-references"
        -DGLOOM_FACTORY_REVIEW=ON
        -P "${CMAKE_SOURCE_DIR}/cmake/RunVisualReview.cmake")
    set_tests_properties(gloom.factory_visual_review PROPERTIES LABELS "graphics;visual" TIMEOUT 50)
    add_test(NAME gloom.character_visual_review COMMAND "${CMAKE_COMMAND}"
        "-DGLOOM_EXECUTABLE=$<TARGET_FILE:gloom>"
        "-DGLOOM_COMPARATOR=$<TARGET_FILE:gloom_visual_capture_tests>"
        "-DGLOOM_CAPTURE_DIR=${CMAKE_BINARY_DIR}/character-review/$<CONFIG>"
        "-DGLOOM_REFERENCE_DIR=${CMAKE_SOURCE_DIR}/tests/character-visual-references"
        -DGLOOM_CHARACTER_REVIEW=ON -P "${CMAKE_SOURCE_DIR}/cmake/RunVisualReview.cmake")
    set_tests_properties(gloom.character_visual_review PROPERTIES LABELS "graphics;visual" TIMEOUT 50)
    set_tests_properties(gloom.sdl_smoke PROPERTIES LABELS graphics TIMEOUT 15)

    add_test(NAME gloom.network_scene_smoke COMMAND gloom --network-scene-smoke)
    set_tests_properties(
        gloom.network_scene_smoke
        PROPERTIES LABELS "graphics;network" TIMEOUT 20
    )

    add_test(NAME gloom.vertical_slice_smoke COMMAND gloom --vertical-slice-smoke)
    set_tests_properties(
        gloom.vertical_slice_smoke
        PROPERTIES LABELS "graphics;gameplay" TIMEOUT 25
    )

    add_test(
        NAME gloom.vulkan_sync
        COMMAND
            "${CMAKE_COMMAND}"
            "-DGLOOM_EXECUTABLE=$<TARGET_FILE:gloom>"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/RunGraphicsValidation.cmake"
    )
    set_tests_properties(gloom.vulkan_sync PROPERTIES LABELS graphics TIMEOUT 35)
endif()
