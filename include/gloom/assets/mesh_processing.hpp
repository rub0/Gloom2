#pragma once

#include <gloom/assets/gltf_importer.hpp>

#include <cstddef>
#include <expected>
#include <string>

namespace gloom::assets {

struct MeshProcessingMetrics {
    std::size_t source_vertices{0};
    std::size_t optimized_vertices{0};
    std::size_t source_indices{0};
    std::size_t lod_indices{0};
};

struct MeshProcessingSettings {
    bool source_normals{true};
    bool source_texture_coordinates{true};
};

// Offline-only processing: MikkTSpace tangent splitting, vertex/index
// optimization, bounds and simplified LOD index streams.
[[nodiscard]] std::expected<MeshProcessingMetrics, std::string>
process_imported_primitive(ImportedPrimitive& primitive,
                           MeshProcessingSettings settings = {});

} // namespace gloom::assets
