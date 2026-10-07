#include <gloom/assets/gltf_importer.hpp>
#include <gloom/assets/rig.hpp>
#include <gloom/assets/scene_gpu_bridge.hpp>
#include <gloom/gameplay/character_presentation.hpp>
#include <gloom/gameplay/vertical_slice_network.hpp>
#include <gloom/render/scene.hpp>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdio.h>
#include <stdlib.h>

namespace {
void require(bool value, const char* message) {
    if (!value) {
        fputs(message, stderr);
        fputc('\n', stderr);
        exit(1);
    }
}
}
int main() {
    using namespace gloom;
    for (const char* name : {"archangel", "shadow"}) {
        const std::expected<assets::ImportedScene, std::string> source =
            assets::import_gltf(std::filesystem::path{GLOOM_TEST_ASSETS} / "characters/original" / (std::string{name} + ".gltf"));
        require(source.has_value(), source ? "" : source.error().c_str());
        const bool shadow = std::string_view{name} == "shadow";
        require(source->skins.size() == 1 && source->skins[0].joints.size() == (shadow ? 17 : 43), "Original rig was lost");
        std::vector<std::byte> bytes = assets::encode_imported_scene(*source);
        const std::expected<assets::ImportedScene, std::string> roundtrip = assets::decode_imported_scene(bytes);
        require(roundtrip.has_value() && assets::valid_bind_rigs(*roundtrip), "Cooked rig bind pose is invalid");
        require(roundtrip->skins[0].inverse_bind_matrices == source->skins[0].inverse_bind_matrices, "Inverse bind matrix roundtrip changed");
        Array<assets::RigMatrix> transforms;
        require(assets::bind_node_transforms(*roundtrip, transforms), "Rig hierarchy is invalid");
        float head_y = -100, hand_y = -100;
        for (size_t i = 0; i < roundtrip->nodes.size(); ++i) {
            if (roundtrip->nodes[i].name == "Bip001 Head")
                head_y = transforms[i][13];
            if (roundtrip->nodes[i].name == "Bip001 R Hand")
                hand_y = transforms[i][13];
        }
        require(head_y > hand_y && head_y > 1.3F && head_y < 1.9F && hand_y > .4F && hand_y < 1.3F, "Head/hand anchors are inverted or displaced");
        unsigned maximum = 0;
        for (size_t p = 0; p < source->primitives.size(); ++p) {
            require(source->primitives[p].vertices.size() == roundtrip->primitives[p].vertices.size(), "Cooked skin vertex count changed");
            for (size_t i = 0; i < source->primitives[p].vertices.size(); ++i) {
                const assets::ImportedVertex& before = source->primitives[p].vertices[i];
                const assets::ImportedVertex& after = roundtrip->primitives[p].vertices[i];
                require(before.weights == after.weights && before.joints == after.joints, "Bone influences changed during cooking");
                unsigned active = 0;
                float sum = 0;
                for (float weight : after.weights) {
                    active += weight > 0;
                    sum += weight;
                }
                require(std::abs(sum - 1) < .0001F, "Unnormalized weights");
                maximum = maximum > active ? maximum : active;
            }
        }
        require(maximum == (shadow ? 5 : 4), "Original influences were truncated");
        const assets::ImportedMaterial& material = roundtrip->materials.front();
        require(material.metallic_roughness_texture != assets::no_asset_index && material.normal_texture != assets::no_asset_index &&
                    material.extra_textures[0] != assets::no_asset_index && material.emissive[0] >= 4,
            "Character metal/normal/glow material lost");
        const assets::GpuSceneUploads uploads = assets::build_gpu_scene_uploads(*roundtrip, {.value = 700});
        require(uploads.meshes.size() != 0 && uploads.materials[0].metallic == 1, "Character presentation upload failed");
        assets::ImportedScene bad = *source;
        bad.primitives[0].vertices[0].weights[0] = std::numeric_limits<float>::quiet_NaN();
        require(!assets::validate_imported_scene(bad), "Malformed rig accepted");
        bad = *source;
        bad.primitives[0].vertices[0].joints[0] = 255;
        require(!assets::validate_imported_scene(bad), "Malformed rig accepted");
        bad = *source;
        bad.nodes[1].skin = assets::no_asset_index;
        require(!assets::validate_imported_scene(bad), "Malformed rig accepted");
        bad = *source;
        bad.skins[0].inverse_bind_matrices[0][12] += 10;
        require(!assets::validate_imported_scene(bad), "Malformed rig accepted");
        bad = *source;
        bad.skins[0].joints[1] = bad.skins[0].joints[0];
        require(!assets::validate_imported_scene(bad), "Malformed rig accepted");
        bad = *source;
        bad.nodes[2].children.push_back(0);
        require(!assets::bind_node_transforms(bad, transforms) && transforms.size() == 0, "Invalid hierarchy left usable bind matrices");
        require(!assets::validate_imported_scene(bad), "Malformed rig accepted");
        require(assets::bind_node_transforms(*roundtrip, transforms) && transforms.size() == roundtrip->nodes.size(),
            "Bind output could not be reused after a failed hierarchy");
        bytes[8] = std::byte{3};
        require(!assets::decode_imported_scene(bytes), "Old scene version accepted");
        std::cout << name << ": " << source->skins[0].joints.size() << " bones, " << maximum << " influences, head=" << head_y << ", hand=" << hand_y << '\n';
    }
    for (const gameplay::SliceCharacter character : {gameplay::SliceCharacter::archangel, gameplay::SliceCharacter::shadow}) {
        gameplay::VerticalSliceSimulation simulation{{.opponent_ai_enabled = false, .original_factory = true}};
        const gameplay::SlicePlayerSelection selection{.character = character,
            .ability = character == gameplay::SliceCharacter::archangel ? gameplay::SliceAbility::diamond_skin : gameplay::SliceAbility::invisibility};
        require(gameplay::valid_slice_selection(selection), "Original character selection invalid");
        require(!gameplay::valid_slice_selection({.character = character, .ability = gameplay::SliceAbility::bite}), "Foreign original ability enabled");
        gameplay::SliceSnapshot snapshot = simulation.snapshot();
        snapshot.player.character = character;
        snapshot.player.ability = selection.ability;
        snapshot.player.secondary_ability = gameplay::secondary_ability(selection);
        const std::expected<gameplay::SliceSnapshot, std::string> decoded = gameplay::decode_slice_snapshot(gameplay::encode_slice_snapshot(snapshot, 0, 1));
        require(decoded && decoded->player.character == character, "Character identity lost on wire");
    }
    require(gameplay::valid_slice_selection({.character = gameplay::SliceCharacter::berserker, .ability = gameplay::SliceAbility::none}),
        "Legacy compatibility slot lost");
    const render::Transform attached = render::attach_transform({.position = {3, 2, 1}, .rotation = {0, 1, 0, 0}, .scale = {2, 2, 2}}, {.position = {1, 0, 2}});
    require(std::abs(attached.position.x - 1) < 1e-5F && std::abs(attached.position.z + 3) < 1e-5F, "Attachment failed under rotation/scale");
    std::cout << "Original character restoration checks passed\n";
    return 0;
}
